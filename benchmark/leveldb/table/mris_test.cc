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
#include <set>
#include <string.h>
#include "leveldb/slice.h"
#include "leveldb/env.h"
#include "table/mris.h"
#include "util/testharness.h"
#include "util/testutil.h"
#include "util/random.h"

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

static int sequence = 0;

// The maximum size of object used in tests
static const size_t LEN = 256 << 10;
static const size_t HALF_LEN = LEN / 2;

namespace leveldb { namespace mris {

TEST(LargeBlockHandle, BlockHandleTest) {
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
  ASSERT_FALSE(new_block.initialized());
  ASSERT_TRUE(new_block.empty());

  Slice input(store);
  ASSERT_OK(new_block.DecodeFrom(&input));
  ASSERT_EQ(large_block, new_block);
}

TEST(ValueDelegate, ValueDelegateTest) {
  offset = 120;
  size = 160;
  std::string vdstr;
  EncodeTo(&vdstr);

  ValueDelegate vd;
  Slice input(vdstr);
  ASSERT_OK(vd.DecodeFrom(&input));
  ASSERT_EQ(vd.offset, offset);
  ASSERT_EQ(vd.size, size);
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
    Random rnd((long)time(NULL));
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

// Base class of tests
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

    Slice BuildInternalKey(const Slice &key, char *buf) {
      char* p = EncodeVarint32(buf, key.size() + 8);
      size_t keylen = (p - buf) + key.size() + 8;
      memcpy(p, key.data(), key.size());
      p += key.size();
      EncodeFixed64(p, (0xbeef << 8) | kTypeValue);
      return Slice(buf, keylen);
    }

    void NewKVPair(Slice *key, Slice *value) {
      assert(key && value);
      *key = rgen.Generate(64);
      *value = rgen.Generate(rand.Uniform(LEN));
    }

    // generate N random objects and insert them into store
    // @N: number of objects generated
    // @sources: generated random objects
    // @results: results returned by the insersion
    // @writer: object writer who writes object into object store
    // @reader: object reader who reades object from object store
    void TestWrite(size_t N, std::vector<Slice> *sources,
                   std::vector<ValueDelegate> *results,
                   ObjectWriter *writer, ObjectReader *reader) {
      char buf[LEN];
      for (size_t i = 0; i < N; ++i) {
        size_t size = rand.Uniform(LEN);
        Slice in = rgen.Generate(size);
        sources->push_back(in);

        uint64_t offset = 0;
        ASSERT_OK(writer->Write(in, &offset));
        results->push_back(ValueDelegate(offset, size));

        // read new inserted value out and check
        Slice out;
        ASSERT_OK(reader->Read(offset, size, &out, buf));
        ASSERT_EQ(out.size(), size);
        ASSERT_EQ(0, memcmp(in.data(), out.data(), size));
      }
    }

    // read M random objects from store and compare them with origins
    // @M: number of objects generated
    // @sources: generated random objects
    // @results: results returned by the insersion
    // @reader: object reader who reades object from object store
    void TestRead(size_t M, const std::vector<Slice> &sources,
                  const std::vector<ValueDelegate> &results, 
                  ObjectReader *reader) {
      char buf[LEN];
      size_t N = sources.size();
      for (size_t i = 0; i < M; ++i) {
        size_t j = rand.Uniform(N);
        Slice in = sources[j];
        Slice out;
        ValueDelegate vd = results[j];
        ASSERT_OK(reader->Read(vd.offset, vd.size, &out, buf));
        ASSERT_EQ(out.size(), in.size());
        ASSERT_EQ(0, memcmp(out.data(), in.data(), out.size()));
      }
    }
};

TEST(MrisTest, MrisDBTest) {
  DB* db;
  Options opt;
  opt.create_if_missing = true;
  Status status = leveldb::DB::Open(opt, "mris-test-db-orig", &db);
  ASSERT_OK(status);
  const size_t Test_size = 1000;

  std::vector<std::pair<Slice, Slice> > kvs;
  std::set<const char *> key_offsets;
  WriteOptions wopt;
  for (size_t i = 0; i < Test_size; ++i) {
    Slice key, value;

    do {
      NewKVPair(&key, &value);
    } while (key_offsets.find(key.data()) != key_offsets.end());
    key_offsets.insert(key.data());

    kvs.push_back(std::pair<Slice, Slice>(key, value));
    ASSERT_OK(db->Put(wopt, key, value));
  }

  ReadOptions ropt;
  for (size_t i = 0; i < Test_size; ++i) {
    Slice key, value; 
    std::string result;
    key = kvs[i].first;
    value = kvs[i].second;
    ASSERT_OK(db->Get(ropt, key, &result));

    if (value.compare(Slice(result)) != 0) {
      std::cerr << i << " key: "; 
      for (size_t j = 0; j < key.size(); ++j)
        std::cerr << *(key.data() + j);
      std::cerr << std::endl;
      std::cerr << "value length: " << value.size();
      for (size_t j = 0; j < 64; ++j)
        std::cerr << *(value.data() + j);
      std::cerr << std::endl;
      std::cerr << "result length: " << result.size();
      for (size_t j = 0; j < 64; ++j)
        std::cerr << *(result.c_str() + j);
      std::cerr << std::endl;
    }
    ASSERT_EQ(0, value.compare(Slice(result)));
  }

  delete db;
}

TEST(MrisTest, LargeSpaceTestIO) {
  Options opt;
  LargeSpace* space = LargeSpace::GetSpace(dbname, &opt);

  ASSERT_TRUE(space);

  std::vector<Slice> keys;
  std::vector<Slice> values;
  std::vector<std::string> mris_values;
  std::vector<ValueDelegate> delegates;
  const size_t Test_size = 2000;
  char key_buf[1024];

  for (size_t i = 0; i < Test_size; ++i) {
    Slice key, value;
    NewKVPair(&key, &value);
    keys.push_back(key);
    values.push_back(value);

    Slice ikey = BuildInternalKey(key, key_buf);

    std::string mris_key, mris_value;
    ASSERT_OK(space->Deposit(ikey, value, &mris_key, &mris_value));
    mris_values.push_back(mris_value);
  }

  for (size_t i = 0; i < Test_size; ++i) {
    Slice value = values[i];
    std::string mris_value = mris_values[i];
    ASSERT_OK(space->Retrieve(&mris_value, 0));

    ASSERT_EQ(0, memcmp(value.data(), mris_value.data(), value.size()));
  }

}

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

  TestWrite(100, &sources, &values, builder, builder);

  TestRead(256, sources, values, builder);

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

  TestWrite(100, &sources, &values, builder, builder);
  ASSERT_OK(builder->Sync());

  LargeBlockReader* reader = new LargeBlockReader(env, builder);

  TestRead(256, sources, values, reader);

  delete reader;
  delete builder;
}

TEST(MrisTest, LargeSpaceTestBasic) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  uint64_t offset1;
  std::string message = "hello";
  Slice input(message);
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Write(message, &offset1));
  ASSERT_OK(space->Close());

  delete space;

  // load the large space again
  space = new LargeSpace(&opt, dbname);
  char buf[1024];
  Slice result;
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Read(offset1, message.size(), &result, buf));
  ASSERT_EQ(0, memcmp(result.data(), message.c_str(), result.size()));

  // write more and read
  message = "world";
  uint64_t offset2;
  ASSERT_OK(space->Write(message, &offset2));
  Slice result2;
  ASSERT_OK(space->Read(offset2, message.size(), &result2, buf));
  ASSERT_EQ(0, memcmp(result2.data(), message.c_str(), result2.size()));

  delete space;
}

TEST(MrisTest, LargeSpaceTestFull) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  std::vector<Slice> sources;
  std::vector<ValueDelegate> values;

  ASSERT_OK(space->Open());

  TestWrite(5000, &sources, &values, space, space);

  TestRead(1000, sources, values, space);

  ASSERT_OK(space->Close());
  delete space;

  // reopen and test
  space = new LargeSpace(&opt, dbname);
  ASSERT_OK(space->Open());
  TestRead(1000, sources, values, space);

  // write more and test
  TestWrite(2000, &sources, &values, space, space);
  TestRead(1000, sources, values, space);

  ASSERT_OK(space->Close());
  delete space;
}  

TEST(MrisTest, LargeSpaceTestRandom) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  std::vector<Slice> sources;
  std::vector<ValueDelegate> values;
  const int MAXN = 100;

  ASSERT_OK(space->Open());

  for (size_t i = 0; i < 100; ++i) {
    int N = rand.Uniform(MAXN);
    TestWrite(N, &sources, &values, space, space);
    // we randomly close and reopen the space in the middle
    if ((N & 0x7) == 0x7) {
      ASSERT_OK(space->Close());
      delete space;
      space = new LargeSpace(&opt, dbname);
      ASSERT_OK(space->Open());
    }
    int M = rand.Uniform(MAXN);
    TestRead(M, sources, values, space);
  }

  ASSERT_OK(space->Close());
  delete space;
}

TEST(MrisTest, LargeSpaceTestInterface) {
  Options opt;
  LargeSpace* space = LargeSpace::GetSpace(dbname, &opt);

  ASSERT_TRUE(space);

  char buf[128];
  std::string keystr = "largekey";
  char* p = EncodeVarint32(buf, keystr.size() + 8);
  size_t keylen = (p - buf) + keystr.size() + 8;
  memcpy(p, keystr.data(), keystr.size());
  p += keystr.size();
  EncodeFixed64(p, (0xbeef << 8) | kTypeValue);
  Slice key(buf, keylen);

  Slice value = rgen.Generate(LEN);
  std::string mris_key, mris_value;
  ASSERT_OK(space->Deposit(key, value, &mris_key, &mris_value));

  ASSERT_OK(space->Retrieve(&mris_value, 0));
  ASSERT_EQ(0, memcmp(value.data(), mris_value.data(), value.size()));
}


} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
