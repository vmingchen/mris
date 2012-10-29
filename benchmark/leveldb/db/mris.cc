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

#include <string.h>
#include "db/filename.h"
#include "db/mris.h"
#include "leveldb/env.h"
#include "util/crc32c.h"
#include "util/coding.h"

namespace leveldb { namespace mris {

static const char* EMPTY_LARGESPACE = "EMPTY_LARGESPACE";

// copied from filename.cc
static std::string MakeFileName(const std::string& name, uint64_t number,
                                const char* suffix) {
  char buf[100];
  snprintf(buf, sizeof(buf), "/%08llu.%s",
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

void LargeBlockHandle::EncodeTo(std::string* dst) const {
	assert(size_ > 0);
	PutLengthPrefixedSlice(dst, name_);
	PutVarint64(offset_);
	PutVarint64(size_);
}

Status LargeBlockHandle::DecodeFrom(Slice* input) {
	if (GetLengthPrefixedSlice(input, &name_) &&
			GetVarint64(input, &offset_) &&
			GetVarint64(input, &size_)) {
		return Status::OK();
	} else {
    return Status::Corruption("bad block handle");
	}
}


// ========================== LargeMeta Begin ==============================

Status LargeMeta::Load(const std::string& fname) {
	std::string metadata;
	Status s = ReadFileToString(space->env_, fname, &metadata);
	if (!s.ok()) {
		MaybeIgnoreError(&s);
		return s;
	}
	Slice buffer(metadata);

	return DecodeFrom(&buffer);
}

Status LargeMeta::DecodeFrom(Slice* input) {
	// read nblock and meta_size from meta file and check
	// for simplicity, we assert the status to be ok
	if (! GetVarint64(input, &nblock)) {
		return Status::Corruption("error in large meta head");
	}

	// pointer to block metadata
	const char *block_raw = input->data();
	uint64_t *block_length = input->size();

	// decode block information and build space.blocks_
	for (uint64_t i = 0; i < nblock; ++i) {
		LargeBlockReader *block = new LargeBlockReader(space->env_);
		s = block->DecodeFrom(input);
		if (!s.ok())
			return s;
		space.blocks_.push_back(block);
	}

	block_length -= input->size();
	if (! GetLengthPrefixedSlice(input, &meta_size) || 
			block_length != meta_size) {
		return Status::Corruption("error in large meta size");
	}

	// check crc
	uint32_t crc1 = crc32c::Value(block_raw, block_length);
	uint32_t crc2;
	s = GetVarint32(input, &crc2);
	if (!s.ok( || crc1 != crc2)) {
		return Status::Corruption("crc error");
	}

	return s;
}

Status LargeMeta::Dump(const std::string& fname) {
	std::string metadata;
	EncodeTo(&metadata);
	Status s = WriteStringToFileSync(space->env_,
			Slice(metadata), fname);
	return s;
}

void LargeMeta::EncodeTo(std::string* dst) const {
	assert(nblock == static_cast<uint64_t>(space->blocks_.size()));

	PutVarint64(dst, nblock);
	size_t block_offset = dst.length();
	
	for (uint64_t i = 0; i < nblock; ++i) {
		space->blocks_[i]->EncodeTo(dst);
	}

	uint64_t block_length = static_cast<uint64_t>(dst.length() - block_offset);
	const char *block_raw = dst.c_str() + block_offset;
	PutVarint64(dst, block_length);

	uint32_t crc = crc32c::Value(block_raw, block_length);
	PutVarint32(dst, crc);
}

// ========================== LargeSpace Begin ==============================

LargeSpace::LargeSpace(const Options *opt, const std::string& dbname) 
		: env_(opt->env), 
			db_options_(opt),
			dbname_(dbname),
			meta_(this),
			writer_(NULL) {
	if (env_->FileExists(LargeHeadFileName(dbname))) {
		OpenLargeSpace(dbname);
	} else {
		NewLargeSpace(dbname);
	}
}

~LargeSpace::LargeSpace() {
	for (size_t i = 0; i < blocks_.size(); ++i) {
		delete blocks_[i];
	}
}

Status LargeSpace::OpenLargeSpace() {
  // Read "LARGEHEAD" file, which contains the name of the current meta file
  std::string head;
  Status s = ReadFileToString(env_, LargeHeadFileName(dbname_), &head);
  if (!s.ok()) {
    return s;
  }
  if (head.empty() || head[head.size()-1] != '\n') {
    return Status::Corruption("LARGEHEAD file does not end with newline");
  }

	// Check if the large space is empty
	if (strcmp(head.c_str(), EMPTY_LARGESPACE) == 0) {
		meta_sequence_ = 0;
		return Status::OK();
	}

	// Load block information from file given by head
	meta_sequence_ = static_cast<uint64_t>(atoll(head));
	return meta_.Load(LargeBlockFileName(dbname_, meta_sequence_));
}

Status LargeSpace::NewLargeSpace() {
	// Write empty "LARGEHEAD" file
	meta_sequence_ = 0;
	return WriteStringToFileSync(env_, EMPTY_LARGESPACE, 
			LargeHeadFileName(dbname_));
}

LargeBlockWriter* LargeSpace::NewWriter() {
	if (writer_) {
		Status s = writer_.Close();
		if (!s.ok())
			return s;
		delete writer_;
		writer_ = NULL;
	}

	std::string name = LargeBlockFileName(dbname_, blocks_.size());
	writer_ = new LargeBlockWriter(env_, offset, name);
	if (!writer_) {
		return Status::IOError("[mris] cannot create writer");
	}
}

// TODO: consider concurrent issues
Status LargeSpace::SealLargeBlock() {
	// skip empty block
	if (writer_->empty()) {
		return Status::OK();
	}

	Status s = writer_->Flush();
	if (!s.ok()) {
		return s;
	}

	LargeBlockReader *reader = new LargeBlockReader(env_, writer_);
	blocks_.push_back(reader);
	meta_.nblock++;
	
	// writer will be re-created lazily
	writer_ = NULL;

	return Status::OK();
}

Status LargeSpace::Write(const Slice& slice, uint64_t& offset) {
	// create a new writer if there is no current writer
	Status s;
	if (writer_ == NULL) {
		s = NewWriter(DataSize());
		if (!s.ok()) {
			return s;
		}
	}

	offset = writer_->offset();
	Status s = writer_->Write(slice);
	if (!s.ok()) 
		return s;

	if (writer_->size() > mris_options_.kSplitThreshold) {
		SealLargeBlock();
	}
	return s;
}

Status LargeSpace::UpdateHead() {

}

}}

// vim: set shiftwidth=2 tabstop=2:
