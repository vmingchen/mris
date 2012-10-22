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
#include "leveldb/dbformat.h"
#include "leveldb/env.h"
#include "table/format.h"

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

  MrisOptions() 
	: kSizeThreshold(128 << 10),
	  kLargeBlockSize(64 << 10),
	  kSplitThreshold(64 << 20),
	  cealed(false) {
  }
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
	Env* env_;
	RandomAccessFile *file_;

public:
	BlockFileHandle(const std::string& name, Env* env) 
			: name_(name_), file_(NULL), env_(env) {}

	~BlockFileHandle() {
		if (file_) delete file_;
	}
	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		Status s;
		if (file_ == NULL) {
			s = Env->NewRandomAccessFile(name_, &file_);
			if (!s.ok()) {
				assert(file_ == NULL);
				return s;
			}
		}
		return file_->Read(offset, n, result, scratch);
	}
  uint64_t offset() const { return offset_; }
  uint64_t size() const { return size_; }
	uint64_t end() const { return offset_ + size_; }
	bool Contains(uint64_t offset, uint64_t n) const {
		return offset >= offset() && (offset + n) <= end();
	}

};

class SpaceManager {
private:
  WritableFile *file_;
	Env* env_;
  const MrisOptions *options_;
	std::vector<BlockFileHandle> blocks_;

	// find the file block contains @offset
	BlockFileHandle* getBlock(uint64_t offset) {
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

public:
  SpaceManager(const Options *opt, const char *meta_name) : env_(opt->env) {}

	// Build blocks
	Status BuildBlocks(Slice* input);

	// Load metadata from file given by @meta_name
	Status Load(const char *meta_name, uint64_t meta_size, uint64_t nblock);

	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		BlockFileHandle* block = getBlock(offset);
		assert(block->Contains(offset, n));
		return block->Read(offset - block->offset(), n, result, scratch);
	}
};

} }

#endif

// vim: set shiftwidth=2 tabstop=2:
