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
  // offset of real value in the 
  size_t offset;
  // size of real value
  size_t size;
  ValueDelegate(size_t off, size_t sz) : offset(off), size(sz) {}
};

struct BlockFileHandle : public BlockHandle {
	std::string name_;
	Env* env_;
	RandomAccessFile *file_;

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
	uint64_t end() const { return offset_ + size_; }
};


class SpaceManager {
private:
  WritableFile *file_;
  const MrisOptions *options_;
	std::vector<BlockFileHandle> files_;
	BlockFileHandle* getBlock(uint64_t offset) {
		// binary search
	}
public:
  SpaceManager(const MrisOptions *opt, const char *meta_name) {

	}
	Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
		BlockFileHandle* block = getBlock(offset);
		assert(offset >= block->offset() && (offset + n) <= block->end());
		return block->(offset - block->offset(), n, result, scratch);
	}
};

} }

#endif

// vim: set shiftwidth=2 tabstop=2:
