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
#define STORAGE_LEVELDB_INCLUDE_MRIS_H_

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include "db/dbformat.h"
#include "db/filename.h"
#include "leveldb/db.h"
#include "leveldb/env.h"

#define MRIS

namespace leveldb { namespace mris {

uint64_t LoadFixedUint64(uint64_t offset, SequentialFile* file);

class MrisAppendReadFile;

Status NewMrisAppendReadFile(const std::string&, MrisAppendReadFile**);

class MrisAppendReadFile : public WritableFile, public RandomAccessFile {
private:
  std::string filename_;
  int fd_;
  size_t size_;

public:
  MrisAppendReadFile(const std::string& fname, int fd)
  		: filename_(fname), fd_(fd), size_(0) { }

  virtual ~MrisAppendReadFile() { 
  	assert(close(fd_) == 0);
  }

  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const {
    if (offset + n > size_) {
      return Status::IOError(filename_, "[mris] out of file bound");
    }
    Status s;
    ssize_t r = pread(fd_, scratch, n, static_cast<off_t>(offset));
    *result = Slice(scratch, (r < 0) ? 0 : r);
    if (r < 0) {
      // An error: return a non-ok status
      s = Status::IOError(filename_, strerror(errno));
    }
    return s;
  }

  virtual Status Append(const Slice& data) {
  	Status s;
  	const char* buf = data.data();
  	size_t len = data.size();

  	if (ftruncate(fd_, size_ + len) != 0) {
      s = Status::IOError(filename_, strerror(errno));
  	}

  	off_t offset = static_cast<off_t>(size_);
  	while (s.ok() && len > 0) {
  		ssize_t n = pwrite(fd_, buf, len, offset);
  		if (n < 0) {
  			s = Status::IOError(filename_, strerror(errno));
  			break;
  		}
  		offset += n;
  		buf += n;
  		len -= n;
  	}

  	if (s.ok()) {
  		size_ = static_cast<size_t>(offset);
  	}

  	return s;
  }

  virtual Status Close() {
  	return Status::OK();
  }

  virtual Status Sync() {
  	Status s;
  	if (fdatasync(fd_) < 0) {
  		s = Status::IOError(filename_, strerror(errno));
  	}
  	return s;
  }

  virtual Status Flush() {
  	return Status::OK();
  }
};

struct MrisOptions {
  // the threshold of taking record as large
  uint32_t kSizeThreshold;

  // the threshold of splitting file, default to 64MB
  uint32_t kSplitThreshold;

  // once cealed, some of the options become immutable
  bool cealed;

  MrisOptions() 
  		: kSizeThreshold(128 << 10),
  			kSplitThreshold(64 << 20),
  			cealed(false) { }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);
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
  friend bool operator==(const LargeBlockHandle& a, const LargeBlockHandle& b);
  friend bool operator!=(const LargeBlockHandle& a, const LargeBlockHandle& b);
  friend std::ostream& operator<<(std::ostream& oss, const LargeBlockHandle& b);

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

inline bool operator==(const LargeBlockHandle& a, const LargeBlockHandle& b) {
  return a.offset_ == b.offset_ 
      && a.size_ == b.size_
      && a.name_ == b.name_;
}

inline bool operator!=(const LargeBlockHandle& a, const LargeBlockHandle& b) {
  return !(a == b);
}

inline std::ostream& operator<<(std::ostream& oss, const LargeBlockHandle& b) {
  oss << "name: " << b.name_ 
    << ", offset: " << b.offset_ 
    << ", size: " << b.size_ << std::endl;
}

class LargeBlockReader : public LargeBlockHandle {
private:
  Env* env_;
  RandomAccessFile *file_;

public:
  LargeBlockReader(Env* env) : env_(env), file_(NULL) {}
  LargeBlockReader(Env* env, const LargeBlockHandle* handle)
  		: LargeBlockHandle(handle),
  		  env_(env), 
  			file_(NULL) {}

  LargeBlockReader(Env* env, uint64_t off, 
                   uint64_t size, const std::string& name)
      : LargeBlockHandle(off, size, name),
        env_(env_),
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

class LargeBlockBuilder : public LargeBlockHandle {
private:
  Env* env_;
  MrisAppendReadFile* file_;

public:
  LargeBlockBuilder(Env* env) : file_(NULL), env_(env) {}
  LargeBlockBuilder(Env* env, uint64_t off, const std::string& name) 
  		: env_(env), 
  		  LargeBlockHandle(off, 0, name),
  			file_(NULL) {}
  ~LargeBlockBuilder() { 
  	if (file_) {
  		file_->Sync();
  		file_->Close();
  		delete file_;
  	}
  }

  Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
  	Status s;
  	if (! file_) {
  		s = Status::IOError("[mris] out of file bound", name_);
  	} else {
  		s = file_->Read(offset, n, result, scratch);
  	}
  	return s;
  }

  Status Write(const Slice& data) {
  	assert(initialized());
  	Status s;
  	if (file_ == NULL) {
  		s = NewMrisAppendReadFile(name_, &file_);
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

  Status Sync() { 
  	return file_ ? file_->Sync() : Status::OK();
  }
};

const size_t kValueDelegateSize = sizeof(ValueDelegate);

class LargeSpace {
private:
  // meta_size is the size with the length prefix but not the crc suffix
  // meta_file format:
  // [large-size-threshold] MrisOptions.kSizeThreshold
  // [file-split-threshold] MrisOptions.kSplitThreshold
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
  // const Options* db_options_;
  const Options* db_options_;
  MrisOptions mris_options_;
  std::vector<LargeBlockReader*> blocks_;
  std::string dbname_;
  LargeMeta meta_;
  // sequence of current meta file. valid sequence starts from 1, and 0
  // means no meta file, which means the space is empty
  uint64_t meta_sequence_;
  bool closed_;

  // make sure it points to a ready writer or NULL
  LargeBlockBuilder* builder_;

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

  Status NewBuilder();

  // instanciate from disk files
  Status LoadLargeSpace();

  // save to disk files
  Status DumpLargeSpace();

  Status NewLargeSpace();

  Status SealLargeBlock();

public:
  LargeSpace(const Options *opt, const std::string& dbname);
  ~LargeSpace();

  Status Open();
  Status Close();

  // We make sure no value expands more than one LargeBlock
  Status Read(uint64_t offset, uint64_t n, Slice* result, char *scratch) {
  	Status s;
  	if (offset > DataSize()) {
  		s = Status::IOError("[mris] invalid offset");
  	} else if (builder_ && offset > builder_->offset()) {
  		if (builder_->contains(offset, n)) {
  			s = builder_->Read(offset, n, result, scratch);
  		} else {
  			s = Status::IOError("[mris] out-of-bound read");
  		}
  	} else {
  		LargeBlockReader* reader = getBlockReader(offset);
  		if (reader->contains(offset, n)) {
  			s = reader->Read(offset - reader->offset(), n, result, scratch);
  		} else {
  			s = Status::IOError("[mris] out-of-bound read");
  		}
  	}
  	return s;
  }

  Status Write(const Slice& slice, uint64_t& offset);

  // size of all data
  uint64_t DataSize() const {
  	if (builder_) {
  		return builder_->offset() + builder_->size();
  	} else if (blocks_.size() > 0) {
  		return blocks_.back()->end();
  	} else {
  		return 0;
  	}
  }

  void SetLargeThreshold(uint32_t size) {
    mris_options_.kSizeThreshold = size;
  }
  uint32_t GetLargeThreshold() const {
    return mris_options_.kSizeThreshold;
  }
  void SetSplitThreshold(uint32_t thres) {
    mris_options_.kSplitThreshold = thres;
  }
  uint32_t GetSplitThreshold() const {
    return mris_options_.kSplitThreshold;
  }

  bool IsEmpty() const { return meta_sequence_ == 0; }
};

} }

#endif

// vim: set shiftwidth=2 tabstop=2 expandtab:
