/*
 * ===========================================================================
 *
 *       Filename:  mris.h
 *         Author:  Ming Chen, v.mingchen@gmail.com
 *        Created:  10/21/2012 09:36:40 AM
 *
 *    Description:  
 *
 * ===========================================================================
 */

#ifndef STORAGE_LEVELDB_INCLUDE_MRIS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <vector>
#include <string>
#include "db/filename.h"
#include "leveldb/dbformat.h"
#include "leveldb/env.h"

#define MRIS

namespace leveldb { namespace mris {

uint64_t LoadFixedUint64(uint64_t offset, SequentialFile* file);

const size_t kValueDelegateSize = sizeof(ValueDelegate);

struct MrisOptions {
  // the threshold of taking record as large
  size_t kSizeThreshold;

  // size of block, which is the allocation unit for large files
  size_t kLargeBlockSize;

  // the threshold of splitting file, default to 64MB
  size_t kSplitThreshold;

  // once cealed, some of the options become immutable
  bool cealed;

	std::string dbname;

  MrisOptions(const char* name) 
	: kSizeThreshold(128 << 10),
	  kLargeBlockSize(64 << 10),
	  kSplitThreshold(64 << 20),
	  cealed(false),
		dbname(name) { }
};

struct ValueDelegate {
  // offset of real value
  size_t offset;
  // size of real value
  size_t size;
  ValueDelegate(size_t off, size_t sz) : offset(off), size(sz) {}
};

class BlockFileHandle {
private:
	uint64_t offset_;
	uint64_t size_;
	std::string name_;

public:
  uint64_t offset() const { return offset_; }
  uint64_t size() const { return size_; }
	uint64_t end() const { return offset_ + size_; }
	const std::string& name() const { return name_; }
	bool Contains(uint64_t offset, uint64_t n) const {
		return offset >= offset() && (offset + n) <= end();
	}
	bool Initialized() const { return name_.length() > 0; }
};

class BlockFileReader : BlockFileHandle {
private:
	Env* env_;
	RandomAccessFile *file_;

public:
	BlockFileReader(Env* env) : file_(NULL), env_(env) {}
	BlockFileReader(Env* env, uint64_t off, const std::string& name) 
			: offset_(off), size_(0), name_(name), env_(env), file_(NULL) {}
	~BlockFileReader() { if (file_) delete file_; }
	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		assert(Initialized());
		Status s;
		if (file_ == NULL) {
			s = env_->NewRandomAccessFile(name_, &file_);
			if (!s.ok()) {
				assert(file_ == NULL);
				return s;
			}
		}
		return file_->Read(offset, n, result, scratch);
	}
};

class BlockFileWriter : BlockFileHandle {
private:
	WritableFile* file_;

public:
	BlockFileWriter(Env* env) : file_(NULL), env_(env) {}
	~BlockFileWriter() { if (file_) delete file_; }
	Status Write(const Slice& data) {
		assert(Initialized());
		Status s;
		if (file_ == NULL) {
			s = env_->NewWritableFile(name_, &file_);
			if (!s.ok()) {
				assert(file_ == NULL);
				return s;
			}
		}
		s = file_->Append(data);
		if (s.ok()) 
			size_ += data.size();
		return s;
	}
	Status Close() { return file_ ? file_->Close() : Status::OK(); }
};

struct LargeMeta {
	uint64_t size;
	uint64_t nblock;

};

class LargeSpace {
private:
	Env* env_;
	const Options* db_options_;
  MrisOptions mris_options_;
	std::vector<BlockFileReader> blocks_;

	// make sure it points to a ready writer all the time
	BlockFileWriter *writer_;

	// find the file block contains @offset
	BlockFileReader* getBlockReader(uint64_t offset) {
		// binary search
		size_t first = 0;
		size_t count = file_.size();
		assert(count > 0);
		while (count > 1) {
			size_t step = count / 2;
			size_t mid = first + step;
			if (blocks_[mid].offset() == offset) {
				return &blocks_[mid];
			} else if (blocks_[mid].offset() > offset) {
				count = step;
			} else {
				first = mid;
				count -= step;
			}
		}
		return &blocks_[first];
	}

	BlockFileWriter* NewWriter(const std::string &name);

	Status NewLargeSpace();

public:
  LargeSpace(const Options *opt, const char *db_name);

	// Build block readers
	Status BuildReaders(Slice* input, size_t nblock);

	// Load metadata from file given by @meta_name
	Status Load(const char *meta_name, uint64_t meta_size, uint64_t nblock);

	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		BlockFileReader* reader = getBlockReader(offset);
		assert(block->Contains(offset, n));
		return block->Read(offset - block->offset(), n, result, scratch);
	}

	Status Write(const Slice& slice, uint64_t& offset);

	// size of all data
	uint64_t DataSize() const {
		return writer_->offset() + writer_->size();
	}

	Status UpdateHead();
};

} }

#endif

// vim: set shiftwidth=2 tabstop=2:
