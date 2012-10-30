/*
 * ===========================================================================
 *
 *       Filename:  mris_test.cc
 *         Author:  Ming Chen, brianchenming@gmail.com
 *        Created:  10/30/2012 09:25:43 PM
 *
 *    Description:  Mris tests
 *
 *       Revision:  none
 *
 * ===========================================================================
 */

#include <string>
#include <vector>
#include <string.h>
#include "leveldb/slice.h"
#include "leveldb/env.h"
#include "table/mris.h"
#include "util/testharness.h"
#include "util/testutil.h"
#include "util/random.h"

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

static int sequence = 0;

static const size_t LEN = 102400;
static const size_t HALF_LEN = LEN / 2;

namespace leveldb { namespace mris {

TEST(LargeBlockHandle, basic) {
  std::string name = "block000001.lbf";
  LargeBlockHandle large_block(200, 299, name);

  ASSERT_TRUE(large_block.initialized());
  ASSERT_FALSE(large_block.empty());
  ASSERT_EQ(499, large_block.end());

  ASSERT_TRUE(large_block.contains(200, 299));
  ASSERT_TRUE(large_block.contains(210, 289));
  ASSERT_TRUE(large_block.contains(300, 0));
  ASSERT_TRUE(large_block.contains(499, 0));

  ASSERT_FALSE(large_block.contains(0, 300));
  ASSERT_FALSE(large_block.contains(0, 500));
  ASSERT_FALSE(large_block.contains(200, 300));
  ASSERT_FALSE(large_block.contains(499, 1));
  ASSERT_FALSE(large_block.contains(500, 1));

  std::string store;
  large_block.EncodeTo(&store);

  LargeBlockHandle new_block;
  ASSERT_TRUE(! new_block.initialized());
  ASSERT_TRUE(new_block.empty());

  Slice input(store);
  ASSERT_OK(new_block.DecodeFrom(&input));
  ASSERT_EQ(large_block, new_block);
}

// Copied from db_bench.cc
// Helper for quickly generating random data.
class RandomGenerator {
 private:
  std::string data_;
  int pos_;

 public:
  RandomGenerator() {
    // We use a limited amount of data over and over again and ensure
    // that it is larger than the compression window (32KB), and also
    // large enough to serve all typical value sizes we want to write.
    Random rnd(301);
    std::string piece;
    while (data_.size() < 2*LEN) {
      // Add a short fragment that is as compressible as specified
      // by FLAGS_compression_ratio.
      test::CompressibleString(&rnd, 0.5, 100, &piece);
      data_.append(piece);
    }
    pos_ = 0;
  }

  Slice Generate(int len) {
    if (pos_ + len > data_.size()) {
      pos_ = 0;
      assert(len < data_.size());
    }
    pos_ += len;
    return Slice(data_.data() + pos_ - len, len);
  }
};

class MrisTest {
  public:
    Random rand;
    RandomGenerator rgen;
    std::string dbname;
    Env* env;
    MrisTest() : rand(383), dbname("mris-test-db"), 
                 env(Env::Default()) {
      // new a blank test dir, empty it if already exist
      if (env->FileExists(dbname)) {
        //std::cerr << "dir '" << dbname << "' already exits" << std::endl;
        std::vector<std::string> children;
        assert(env->GetChildren(dbname, &children).ok());
        for (size_t i = 0; i < children.size(); ++i) {
          if (children[i][0] == '.') {
            continue;
          }
          std::string cname = dbname + "/" + children[i];
          assert((env->DeleteFile(cname)).ok());
        }
        //std::cerr << "dir '" << dbname << "' emptied" << std::endl;
      } else {
        assert(env->CreateDir(dbname).ok());
        //std::cerr << "dir '" << dbname << "' created" << std::endl;
      }
    }
    virtual ~MrisTest() {}
    std::string NewBlockFileName() {
      char buf[256];
      snprintf(buf, sizeof(buf), "/%08llu.lbf",
               static_cast<unsigned long long>(sequence));
      ++sequence;
      return dbname + buf;
    }
};

TEST(MrisTest, MrisAppendReadFileTest) {
  std::string filename = NewBlockFileName();
  MrisAppendReadFile *mris_file = NULL;

  // create an empty file
  ASSERT_OK(MrisAppendReadFile::New(filename, &mris_file));
  ASSERT_TRUE(mris_file);
  ASSERT_OK(mris_file->Sync());
  ASSERT_TRUE(env->FileExists(filename));

  char inbuf[LEN];
  Slice input;
  // read from empty file should be IOError
  ASSERT_TRUE(mris_file->Read(0, 10, &input, inbuf).IsIOError());

  // append some data
  char *first_half = inbuf;
  memset(first_half, 'a', HALF_LEN);
  Slice first(first_half, HALF_LEN);
  ASSERT_OK(mris_file->Append(first));

  char *second_half = inbuf + HALF_LEN;
  memset(second_half, 'b', HALF_LEN);
  Slice second(second_half, HALF_LEN);
  ASSERT_OK(mris_file->Append(second));

  char outbuf[LEN];
  Slice output;
  ASSERT_OK(mris_file->Read(HALF_LEN - 5, 10, &output, outbuf));
  ASSERT_EQ(0, strncmp("aaaaabbbbb", output.data(), 10));

  ASSERT_OK(mris_file->Read(0, LEN, &output, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, output.data(), LEN));

  delete mris_file;
}

// write a random value, then read it, and compare
TEST(MrisTest, BuilderTestBasic) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);

  ASSERT_OK(builder->Sync());
  // because of lazy init
  ASSERT_FALSE(env->FileExists(filename));

  char outbuf[LEN];
  Slice result;
  ASSERT_TRUE(builder->Read(0, HALF_LEN, &result, outbuf).IsIOError());

  uint64_t offset;
  Slice source = rgen.Generate(LEN);
  ASSERT_OK(builder->Write(source, &offset));
  ASSERT_EQ(source.size(), LEN);
  ASSERT_TRUE(offset >= 0);

  ASSERT_OK(builder->Read(offset, LEN, &result, outbuf));
  ASSERT_EQ(result.size(), LEN);
  ASSERT_EQ(0, memcmp(source.data(), result.data(), LEN));
}

// write 100 random value, then randomly read some of them, and compare
TEST(MrisTest, BuilderTestFull) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);
  std::vector<Slice> sources;
  std::vector<ValueDelegate> values;
  char buf[LEN+1];
  const size_t N = 100;

  for (size_t i = 0; i < N; ++i) {
    size_t size = rand.Uniform(LEN);
    Slice in = rgen.Generate(size);
    sources.push_back(in);
    ASSERT_EQ(size, in.size());

    uint64_t offset = 0;
    ASSERT_OK(builder->Write(in, &offset));

    Slice out;
    ASSERT_OK(builder->Read(offset, size, &out, buf));
    ASSERT_EQ(out.size(), size);
    ASSERT_EQ(0, memcmp(in.data(), out.data(), size));
    values.push_back(ValueDelegate(offset, size));
  }

  for (size_t i = 0; i < 256; ++i) {
    size_t j = rand.Uniform(N);
    Slice in = sources[j];

    Slice out;
    ValueDelegate vd = values[j];
    ASSERT_OK(builder->Read(vd.offset, vd.size, &out, buf));
    ASSERT_EQ(out.size(), in.size());
    ASSERT_EQ(0, memcmp(out.data(), in.data(), out.size()));
  }

  delete builder;
}

TEST(MrisTest, ReaderTestBasic) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);
  uint64_t offset = 0;
  std::string message = "hello";
  Slice input(message);

  ASSERT_OK(builder->Write(input, &offset));
  ASSERT_OK(builder->Sync());

  LargeBlockReader* reader = new LargeBlockReader(env, builder);
  Slice output;
  char buf[10];
  ASSERT_OK(reader->Read(offset, input.size(), &output, buf));
  ASSERT_EQ(input.size(), output.size());
  ASSERT_EQ(0, memcmp(input.data(), output.data(), input.size()));
}

TEST(MrisTest, ReaderTestFull) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);
  std::vector<Slice> sources;
  std::vector<ValueDelegate> values;
  char buf[LEN+1];
  const size_t N = 100;

  for (size_t i = 0; i < N; ++i) {
    size_t size = rand.Uniform(LEN);
    Slice in = rgen.Generate(size);
    ASSERT_EQ(size, in.size());
    sources.push_back(in);

    uint64_t offset = 0;
    ASSERT_OK(builder->Write(in, &offset));
    values.push_back(ValueDelegate(offset, size));
  }

  ASSERT_OK(builder->Sync());

  LargeBlockReader* reader = new LargeBlockReader(env, builder);

  Slice result;
  ValueDelegate del = values[0];
  ASSERT_OK(reader->Read(del.offset, del.size, &result, buf));
  ASSERT_EQ(result.size(), del.size);
  ASSERT_EQ(0, memcmp(sources[0].data(), result.data(), result.size()));

  for (size_t i = 0; i < 256; ++i) {
    size_t j = rand.Uniform(N);
    Slice in = sources[j];
    ValueDelegate vd = values[j];
    Slice out;
    ASSERT_OK(reader->Read(vd.offset, vd.size, &out, buf));
    ASSERT_EQ(out.size(), in.size());
    ASSERT_EQ(0, memcmp(out.data(), in.data(), out.size()));
  }

  delete reader;
  delete builder;
}

TEST(MrisTest, LargeSpaceTestBasic) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  uint64_t offset;
  std::string message = "hello";
  Slice input(message);
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Write(input, &offset));
  ASSERT_OK(space->Close());

  delete space;

  // load the large space again
  space = new LargeSpace(&opt, dbname);
  char buf[1024];
  Slice result;
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Read(offset, message.length(), &result, buf));
  ASSERT_EQ(0, memcmp(result.data(), input.data(), result.size()));

  delete space;
}

TEST(MrisTest, LargeSpaceTestFull) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  std::vector<Slice> sources;
  std::vector<ValueDelegate> values;
  char buf[LEN+1];
  const size_t N = 5000;

  ASSERT_OK(space->Open());

  for (size_t i = 0; i < N; ++i) {
    size_t size = rand.Uniform(LEN);
    Slice in = rgen.Generate(size);
    sources.push_back(in);

    uint64_t offset = 0;
    ASSERT_OK(space->Write(in, &offset));
    values.push_back(ValueDelegate(offset, size));
  }

  for (size_t i = 0; i < 256; ++i) {
    size_t j = rand.Uniform(N);
    Slice in = sources[j];

    Slice out;
    ValueDelegate vd = values[j];
    ASSERT_OK(space->Read(vd.offset, vd.size, &out, buf));
    ASSERT_EQ(out.size(), in.size());
    ASSERT_EQ(0, memcmp(out.data(), in.data(), out.size()));
  }

  ASSERT_OK(space->Close());
  delete space;

  // reopen
  space = new LargeSpace(&opt, dbname);
  ASSERT_OK(space->Open());

  for (size_t i = 0; i < 256; ++i) {
    size_t j = rand.Uniform(N);
    Slice in = sources[j];

    Slice out;
    ValueDelegate vd = values[j];
    ASSERT_OK(space->Read(vd.offset, vd.size, &out, buf));
    ASSERT_EQ(out.size(), in.size());
    ASSERT_EQ(0, memcmp(out.data(), in.data(), out.size()));
  }

  ASSERT_OK(space->Close());
  delete space;
}

} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
