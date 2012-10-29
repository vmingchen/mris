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

#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "db/filename.h"
#include "leveldb/db.h"
#include "leveldb/env.h"
#include "util/crc32c.h"
#include "util/coding.h"
#include "table/mris.h"

namespace leveldb { namespace mris {

static const char* EMPTY_LARGESPACE = "EMPTY_LARGESPACE\n";

// copied from filename.cc
static std::string MakeFileName(const std::string& name, uint64_t number,
                                const char* suffix) {
  char buf[256];
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

static std::string LargeHeadFileName(const std::string& dbname) {
  return dbname + "/LARGEHEAD";
}

Status NewMrisAppendReadFile(const std::string& fname, 
  													 MrisAppendReadFile** result) {
  *result = NULL;
  // create a file that we will append and read
  int fd = open(fname.c_str(), O_RDWR | O_EXCL | O_CREAT, 0644);
  if (fd == -1) {
  	return Status::IOError(fname, strerror(errno));
  }

  *result = new MrisAppendReadFile(fname, fd);
  return Status::OK();
}

uint64_t LoadFixedUint64(uint64_t offset, RandomAccessFile* file) {
  uint64_t result;
  Slice buffer;
  char scrach[8+1];
  Status s = file->Read(offset, 8, &buffer, scrach);
  assert(s.ok());
  return DecodeFixed64(scrach);
}

uint32_t LoadFixedUint32(uint64_t offset, RandomAccessFile* file) {
  uint32_t result;
  Slice buffer;
  char scrach[4+1];
  Status s = file->Read(offset, 4, &buffer, scrach);
  assert(s.ok());
  return DecodeFixed32(scrach);
}

// ======================= MrisOptions Begin ================================
void MrisOptions::EncodeTo(std::string* dst) const {
  PutVarint32(dst, kSizeThreshold);
  PutVarint32(dst, kSplitThreshold);
}

Status MrisOptions::DecodeFrom(Slice* input) {
  if (GetVarint32(input, &kSizeThreshold) && 
      GetVarint32(input, &kSplitThreshold)) {
  	return Status::OK();
  } else {
    return Status::Corruption("[mris] bad option data");
  }
}

// ======================= LargeBlockHandle Begin ===========================

void LargeBlockHandle::EncodeTo(std::string* dst) const {
  //assert(size_ > 0); It is Okay to write a blank block
  PutLengthPrefixedSlice(dst, name_);
  PutVarint64(dst, offset_);
  PutVarint64(dst, size_);
}

Status LargeBlockHandle::DecodeFrom(Slice* input) {
  Slice name;
  if (GetLengthPrefixedSlice(input, &name) &&
  		GetVarint64(input, &offset_) &&
  		GetVarint64(input, &size_)) {
  	name_.assign(name.data(), name.size());
  	return Status::OK();
  } else {
    return Status::Corruption("[mris] bad block handle");
  }
}

// ========================== LargeMeta Begin ==============================

Status LargeSpace::LargeMeta::Load(const std::string& fname) {
  std::string metadata;
  Status s = ReadFileToString(space->env_, fname, &metadata);
  if (!s.ok()) {
  	return s;
  }
  Slice buffer(metadata);

  return DecodeFrom(&buffer);
}

Status LargeSpace::LargeMeta::DecodeFrom(Slice* input) {
  // pointer to block metadata
  const char *block_raw = input->data();
  uint64_t block_length = static_cast<uint64_t>(input->size());

  Status s;
  s = space->mris_options_.DecodeFrom(input);
  if (!s.ok()) {
    return s;
  }

  // read nblock from meta file
  // for simplicity, we assert the status to be ok
  uint64_t nblock;
  if (! GetVarint64(input, &nblock)) {
  	return Status::Corruption("[mris] error in large meta head");
  }

  // read if there is any writer block
  uint32_t nwblock;
  if (! GetVarint32(input, &nwblock) || nwblock > 1) {
  	return Status::Corruption("[mris] corrupted nwblock");
  }

  // decode block information and build space.blocks_
  for (uint64_t i = 0; i < nblock; ++i) {
  	LargeBlockReader *block = new LargeBlockReader(space->env_);
  	s = block->DecodeFrom(input);
  	if (!s.ok())
  		return s;
  	space->blocks_.push_back(block);
  }

  // decode writer block and build space->builder_
  if (nwblock > 0) {
  	assert(space->builder_ == NULL);
  	space->builder_ = new LargeBlockBuilder(space->env_);
  	s = space->builder_->DecodeFrom(input);
  	if (!s.ok())
  		return s;
  }

  // check meta_size
  uint64_t meta_size = 0;
  block_length -= input->size();
  if (! GetVarint64(input, &meta_size) || 
  		block_length != meta_size) {
  	return Status::Corruption("error in large meta size");
  }

  // check crc
  uint32_t crc1 = crc32c::Value(block_raw, block_length);
  uint32_t crc2;
  if (!GetVarint32(input, &crc2) || crc1 != crc2) {
  	return Status::Corruption("crc error");
  }

  return Status::OK();
}

Status LargeSpace::LargeMeta::Dump(const std::string& fname) {
  std::string metadata;
  EncodeTo(&metadata);
  Status s = WriteStringToFile(space->env_,
  		Slice(metadata), fname);
  return s;
}

void LargeSpace::LargeMeta::EncodeTo(std::string* dst) const {
  // save the begining of the metadata
  size_t block_offset = dst->length();

  space->mris_options_.EncodeTo(dst);

  uint64_t nblock = space->blocks_.size();
  PutVarint64(dst, nblock);
  PutVarint32(dst, space->builder_ ? 1 : 0);
  
  for (uint64_t i = 0; i < nblock; ++i) {
  	space->blocks_[i]->EncodeTo(dst);
  }

  if (space->builder_) {
  	space->builder_->EncodeTo(dst);
  }

  uint64_t block_length = static_cast<uint64_t>(dst->length() - block_offset);
  const char *block_raw = dst->c_str() + block_offset;
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
  		meta_sequence_(0),
      closed_(false),
  		builder_(NULL) { 
  // empty
}

LargeSpace::~LargeSpace() {
  Status s;
  if (!closed_) {
    s = Close();
    assert(s.ok());
  }
  if (builder_) {
  	assert(s.ok());
  	delete builder_;
  	builder_ = NULL;
  }
  for (size_t i = 0; i < blocks_.size(); ++i) {
  	delete blocks_[i];
  }
}

Status LargeSpace::LoadLargeSpace() {
  // Read "LARGEHEAD" file, which contains the sequence of current meta file
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
  meta_sequence_ = static_cast<uint64_t>(atoll(head.c_str()));
  return meta_.Load(LargeMetaFileName(dbname_, meta_sequence_));
}

Status LargeSpace::DumpLargeSpace() {
  std::string metaname = LargeMetaFileName(dbname_, meta_sequence_);
  Status s = meta_.Dump(metaname);
  if (!s.ok()) {
  	return s;
  }

  // write meta sequence number
  std::ostringstream oss;
  oss << ++meta_sequence_ << std::endl;

  return WriteStringToFile(env_, oss.str(), LargeHeadFileName(dbname_));
}

Status LargeSpace::NewLargeSpace() {
  // Write empty "LARGEHEAD" file
  meta_sequence_ = 0;
  return WriteStringToFile(env_, EMPTY_LARGESPACE, 
  		LargeHeadFileName(dbname_));
}

Status LargeSpace::Open() {
  Status s;
  if (env_->FileExists(LargeHeadFileName(dbname_))) {
    s = LoadLargeSpace();
  } else {
    s = NewLargeSpace();
  }
  return s;
}

Status LargeSpace::Close() {
  Status s;
  if (builder_) {
    s = builder_->Sync();
    if (! s.ok()) {
      return s;
    }
  }
  s = DumpLargeSpace();
  if (s.ok()) {
    closed_ = true;
  }
  return s;
}

Status LargeSpace::NewBuilder() {
  assert(builder_ == NULL);

  std::string name = LargeBlockFileName(dbname_, blocks_.size());
  builder_ = new LargeBlockBuilder(env_, DataSize(), name);
  if (!builder_) {
  	return Status::IOError("[mris] cannot create writer");
  }

  // add reader for the new block
  LargeBlockReader *reader = new LargeBlockReader(env_, builder_);
  blocks_.push_back(reader);
  
  // once a new block is created, it is dumped onto disk immediately
  return DumpLargeSpace();
}

// TODO: consider concurrent issues
Status LargeSpace::SealLargeBlock() {
  assert(builder_);

  Status s = builder_->Sync();
  if (!s.ok()) {
  	return s;
  }

  // a new writer will be created lazily
  delete builder_;
  builder_ = NULL;

  return Status::OK();
}

Status LargeSpace::Write(const Slice& slice, uint64_t& offset) {
  // TODO: mutex_.AssertHeld() ? 
  // lazy init: create a new writer if there is no current writer
  Status s;
  if (builder_ == NULL) {
  	s = NewBuilder();
  	if (!s.ok()) {
  		return s;
  	}
  }

  offset = builder_->offset();
  s = builder_->Write(slice);
  if (!s.ok()) 
  	return s;

  if (builder_->size() > mris_options_.kSplitThreshold) {
  	SealLargeBlock();
  }
  return s;
}

}}

// vim: set shiftwidth=2 tabstop=2 expandtab:
