/*
 * ===========================================================================
 *
 *       Filename:  mris.cc
 *         Author:  Ming Chen, brianchenming@gmail.com
 *        Created:  10/21/2012 10:01:39 PM
 *
 *    Description:  
 *
 *       Revision:  none
 *
 * ===========================================================================
 */

#include "db/filename.h"
#include "db/mris.h"
#include "leveldb/env.h"
#include "util/crc32c.h"
#include "util/coding.h"

namespace leveldb { namespace mris {

// copied from filename.cc
static std::string MakeFileName(const std::string& name, uint64_t number,
                                const char* suffix) {
  char buf[100];
  snprintf(buf, sizeof(buf), "/%06llu.%s",
           static_cast<unsigned long long>(number),
           suffix);
  return name + buf;
}

static std::string LargeBlockFileName(const std::string& dbname, 
																			uint64_t number) {
	return MakeFileName(dbname, number, "lbf");
}

static std::string LargeMetaFileName(const std::string& dbname, 
																		 uint64_t number) {
	return MakeFileName(dbname, number, "lmf");
}

std::string LargeHeadFileName(const std::string& dbname) {
	return dbname + "/LARGEHEAD";
}

uint64_t LoadFixedUint64(uint64_t offset, SequentialFile* file) {
	uint64_t result;
	Slice buffer;
	char scrach[8+1];
	Status s = file->Read(offset, 8, &buffer, scrach);
	assert(s.ok());
	return DecodeFixed64(scrach);
}

uint32_t LoadFixedUint32(uint64_t offset, SequentialFile* file) {
	uint32_t result;
	Slice buffer;
	char scrach[4+1];
	Status s = file->Read(offset, 4, &buffer, scrach);
	assert(s.ok());
	return DecodeFixed32(scrach);
}

void BlockFileHandle::EncodeTo(std::string* dst) const {
	assert(size_ > 0);
	PutLengthPrefixedSlice(dst, name_);
	PutVarint64(offset_);
	PutVarint64(size_);
}

Status BlockFileHandle::DecodeFrom(Slice* input) {
	if (GetLengthPrefixedSlice(input, &name_) &&
			GetVarint64(input, &offset_) &&
			GetVarint64(input, &size_)) {
		return Status::OK();
	} else {
    return Status::Corruption("bad block handle");
	}
}

LargeSpace::LargeSpace(const Options *opt, const std::string& db_name) 
		: env_(opt->env), 
			db_options_(opt),
			mris_options_(db_name) {
	if (env_->FileExists(LargeHeadFileName(db_name))) {
		if (db_options_->error_if_exists) {
      return Status::InvalidArgument(
          dbname_, "exists (error_if_exists is true)");
		}
	} else {
    if (db_options_.create_if_missing) {
		}
	}
}

Status LargeSpace::BuildReaders(Slice* input, size_t nblock) {
	blocks_.clear();
	Status s;
	for (size_t i = 0; i < nblock; ++i) {
		BlockFileReader block(env_);
		s = block.DecodeFrom(input);
		if (!s.ok())
			return s;
		blocks_.push_back(block);
	}
	return Status::OK();
}

// meta_size is the size without the length prefix and crc suffix
// meta_file format:
// [number-of-blocks] a.k.a. nblock
// [number-of-bytes-of-block-metadata] a.k.a. meta_size
// [block-metadata]
// [crc]
Status LargeSpace::Load(const char *meta_name, uint64_t meta_size, uint64_t nblock) {
	// check sanity of parameters
	assert(nblock > 0 && meta_size > 8);
	SequentialFile* file;
	Status s = env_->NewSequentialFile(meta_name, &file);
	if (!s.ok()) {
		MaybeIgnoreError(&s);
		return s;
	}

	// read nblock and meta_size from meta file and check
	// for simplicity, we assert the status to be ok
	assert(LoadFixedUint64(0, file) == nblock);
	assert(LoadFixedUint64(8, file) == meta_size);

	char *scrach = new char[meta_size+1];
	Slice buffer;
	s = file->Read(offset, 16, &buffer, scrach);
	assert(s.ok());

	uint32_t crc1 = crc32c::Value(scrach, meta_size);
	uint32_t crc2 = LoadFixedUint32(meta_size + 8, file);
	if (crc1 == crc2) {
		s = BuildBlocks(&buffer, nblock);
	} else {
		s = Status::Corruption("crc error");
	}

	delete[] scrach;
	return s;
}

Status LargeSpace::NewWriter(const std::string &name) {
	if (writer_) {
		Status s = writer_.Close();
		if (!s.ok())
			return s;
		delete writer_;
		writer_ = NULL;
	}
	std::string name = LargeBlockFileName(mris_options_.dbname, blocks_.size());
	writer_ = new BlockFileWriter(env_, DataSize(), name);
	return Status::OK();
}

Status LargeSpace::Write(const Slice& slice, uint64_t& offset) {
	offset = writer_->offset();
	Status s = writer_->Write(slice);
	if (!s.ok()) 
		return s;

	if (writer_->size() > mris_options_.kSplitThreshold) {
	}
	return s;
}

Status LargeSpace::UpdateHead() {

}

}}

// vim: set shiftwidth=2 tabstop=2:
