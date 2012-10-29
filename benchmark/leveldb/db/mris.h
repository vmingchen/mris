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
#include "db/dbformat.h"
#include "leveldb/db.h"
#include "leveldb/env.h"

#define MRIS

namespace leveldb { namespace mris {

uint64_t LoadFixedUint64(uint64_t offset, SequentialFile* file);

struct MrisOptions {
  // the threshold of taking record as large
  size_t kSizeThreshold;

  // size of block, which is the allocation unit for large files
  size_t kLargeBlockSize;

  // the threshold of splitting file, default to 64MB
  size_t kSplitThreshold;

  // once cealed, some of the options become immutable
  bool cealed;

  MrisOptions() 
			: kSizeThreshold(128 << 10),
				kLargeBlockSize(64 << 10),
				kSplitThreshold(64 << 20),
				cealed(false) { }
};

struct ValueDelegate {
  // offset of real value
  size_t offset;
  // size of real value
  size_t size;
  ValueDelegate(size_t off, size_t sz) : offset(off), size(sz) {}
};

class LargeBlockHandle {
protected:
	uint64_t offset_;
	uint64_t size_;
	std::string name_;

public:
	LargeBlockHandle() : offset_(0), size_(0) { }
	LargeBlockHandle(const LargeBlockHandle* handle)
			: offset_(handle->offset()),
				size_(handle->size()),
				name_(handle->name()) {}
	LargeBlockHandle(uint64_t off, uint64_t size, const std::string& name)
			: offset_(off), size_(size), name_(name) { }
  uint64_t offset() const { return offset_; }
  uint64_t size() const { return size_; }
	uint64_t end() const { return offset_ + size_; }
	const std::string& name() const { return name_; }
	bool contains(uint64_t offset, uint64_t n) const {
		return offset >= offset_ && (offset + n) <= end();
	}
	bool initialized() const { return name_.length() > 0; }
	bool empty() const { return size_ == 0; }
	void EncodeTo(std::string* dst) const;
	Status DecodeFrom(Slice* input);
};

class LargeBlockReader : public LargeBlockHandle {
private:
	Env* env_;
	RandomAccessFile *file_;

public:
	LargeBlockReader(Env* env) : file_(NULL), env_(env) {}
	LargeBlockReader(Env* env, const LargeBlockHandle* handle)
			: env_(env), 
				LargeBlockHandle(handle),
				file_(NULL) {}
	~LargeBlockReader() { if (file_) delete file_; }

	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		assert(initialized());
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

class LargeBlockWriter : public LargeBlockHandle {
private:
	Env* env_;
	WritableFile* file_;

public:
	LargeBlockWriter(Env* env) : file_(NULL), env_(env) {}
	LargeBlockWriter(Env* env, uint64_t off, const std::string& name) 
			: env_(env), 
			  LargeBlockHandle(off, 0, name),
				file_(NULL) {}
	~LargeBlockWriter() { 
		if (file_) {
			file_->Flush();
			file_->Close();
			delete file_;
		}
	}

	Status Write(const Slice& data) {
		assert(initialized());
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
	Status Flush() { return file_ ? file_->Flush() : Status::OK(); }
	Status Close() { return file_ ? file_->Close() : Status::OK(); }
};

const size_t kValueDelegateSize = sizeof(ValueDelegate);

class LargeSpace {
private:
	// meta_size is the size with the length prefix but not the crc suffix
	// meta_file format:
	// [number-of-reader-blocks] a.k.a. nblock
	// [number-of-writer-blocks] a.k.a. 1 or 0
	// [block-metadata]
	// [number-of-bytes-of-block-metadata] a.k.a. meta_size
	// [crc]
	struct LargeMeta {
		LargeSpace* space;
		LargeMeta(LargeSpace* sp) : space(sp) {}
		Status Load(const std::string& filename);
		Status Dump(const std::string& filename);
		Status DecodeFrom(Slice* input);
		void EncodeTo(std::string* dst) const;
	};

	Env* env_;
	const Options* db_options_;
  MrisOptions mris_options_;
	std::vector<LargeBlockReader*> blocks_;
	std::string dbname_;
	LargeMeta meta_;
	// sequence of current meta file. valid sequence starts from 1, and 0
	// means no meta file, which means the space is empty
	uint64_t meta_sequence_;

	// make sure it points to a ready writer or NULL
	LargeBlockWriter* writer_;

	// find the file block contains @offset
	LargeBlockReader* getBlockReader(uint64_t offset) {
		// binary search
		size_t first = 0;
		size_t count = blocks_.size();
		assert(count > 0);
		while (count > 1) {
			size_t step = count / 2;
			size_t mid = first + step;
			if (blocks_[mid]->offset() == offset) {
				return blocks_[mid];
			} else if (blocks_[mid]->offset() > offset) {
				count = step;
			} else {
				first = mid;
				count -= step;
			}
		}
		return blocks_[first];
	}

	Status NewWriter();

	// instanciate from disk files
	Status LoadLargeSpace();

	// save to disk files
	Status DumpLargeSpace();

	Status NewLargeSpace();

	Status SealLargeBlock();

public:
  LargeSpace(const Options *opt, const std::string& dbname);
	~LargeSpace();

	// Load metadata from file given by @meta_name
	Status Load(const char *meta_name, uint64_t meta_size, uint64_t nblock);

	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		LargeBlockReader* reader = getBlockReader(offset);
		assert(reader->contains(offset, n));
		return reader->Read(offset - reader->offset(), n, result, scratch);
	}

	Status Write(const Slice& slice, uint64_t& offset);

	// size of all data
	uint64_t DataSize() const {
		if (writer_) {
			return writer_->offset() + writer_->size();
		} else if (blocks_.size() > 0) {
			return blocks_.back()->end();
		} else {
			return 0;
		}
	}

	bool IsEmpty() const { return meta_sequence_ == 0; }
};

} }

#endif

// vim: set shiftwidth=2 tabstop=2:
