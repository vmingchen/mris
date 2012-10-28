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

class MrisTest {
  public:
    std::string dbname;
    Env* env;
    MrisTest() : dbname("mris-test-db"), 
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
  ASSERT_OK(NewMrisAppendReadFile(filename, &mris_file));
  ASSERT_TRUE(mris_file);
  ASSERT_OK(mris_file->Sync());
  ASSERT_TRUE(env->FileExists(filename));

  char inbuf[1024];
  Slice input;
  // read from empty file should be IOError
  ASSERT_TRUE(mris_file->Read(0, 10, &input, inbuf).IsIOError());

  // append some data
  char *first_half = inbuf;
  memset(first_half, 'a', 512);
  Slice first(first_half, 512);
  ASSERT_OK(mris_file->Append(first));

  char *second_half = inbuf + 512;
  memset(second_half, 'b', 512);
  Slice second(second_half, 512);
  ASSERT_OK(mris_file->Append(second));

  char outbuf[1024];
  Slice result;
  ASSERT_OK(mris_file->Read(0, 1024, &result, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, outbuf, 1024));

  ASSERT_OK(mris_file->Read(512 - 5, 10, &result, outbuf));
  ASSERT_EQ(0, strncmp("aaaaabbbbb", outbuf, 10));

  delete mris_file;
}

TEST(MrisTest, LargeBlockBuilderTest) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);

  ASSERT_OK(builder->Sync());
  // because of lazy init
  ASSERT_FALSE(env->FileExists(filename));

  char inbuf[1024];
  Slice result;
  ASSERT_TRUE(builder->Read(0, 512, &result, inbuf).IsIOError());

  char *first_half = inbuf;
  memset(first_half, '0', 512);
  Slice first(first_half, 512);
  ASSERT_OK(builder->Write(first));
  ASSERT_EQ(512, builder->end());
  // file should exits now
  ASSERT_OK(builder->Sync());
  ASSERT_TRUE(env->FileExists(filename));

  char outbuf[1024];
  ASSERT_OK(builder->Read(0, 512, &result, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, outbuf, 512));

  char *second_half = inbuf + 512;
  memset(second_half, '1', 512);
  Slice second(second_half, 512);
  ASSERT_OK(builder->Write(second));
  ASSERT_OK(builder->Sync());

  ASSERT_OK(builder->Read(0, 1024, &result, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, outbuf, 1024));

  ASSERT_OK(builder->Read(512 - 5, 10, &result, outbuf));
  ASSERT_EQ(0, strncmp("0000011111", outbuf, 10));

  delete builder;
}

TEST(MrisTest, LargeBlockReaderTest) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);

  char inbuf[1024];
  memset(inbuf, 'X', 1024);
  Slice input(inbuf, 1024);
  ASSERT_OK(builder->Write(input));
  ASSERT_EQ(1024, builder->end());
  ASSERT_OK(builder->Sync());

  LargeBlockReader* reader = new LargeBlockReader(env, builder);

  char outbuf[1024];
  Slice result;
  Random rand(383);
  for (int i = 0; i < 20; ++i) {
    uint32_t off = rand.Uniform(1024);
    uint32_t size = 1 + rand.Uniform(1024-off);
    ASSERT_OK(reader->Read(off, size, &result, outbuf));
    ASSERT_EQ(size, result.size());
    ASSERT_EQ(0, memcmp(result.data(), inbuf + off, size));
  }
  ASSERT_TRUE(reader->Read(1, 1024, &result, outbuf).IsIOError());
  ASSERT_OK(reader->Read(1, 0, &result, outbuf));

  delete reader;
  delete builder;
}

} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
