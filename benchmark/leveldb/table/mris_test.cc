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

static const size_t LEN = 10240;
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
  Slice result;
  ASSERT_OK(mris_file->Read(0, LEN, &result, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, outbuf, LEN));

  ASSERT_OK(mris_file->Read(HALF_LEN - 5, 10, &result, outbuf));
  ASSERT_EQ(0, strncmp("aaaaabbbbb", outbuf, 10));

  delete mris_file;
}

TEST(MrisTest, LargeBlockBuilderTest) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);

  ASSERT_OK(builder->Sync());
  // because of lazy init
  ASSERT_FALSE(env->FileExists(filename));

  char inbuf[LEN];
  Slice result;
  ASSERT_TRUE(builder->Read(0, HALF_LEN, &result, inbuf).IsIOError());

  char *first_half = inbuf;
  uint64_t offset;
  memset(first_half, '0', HALF_LEN);
  Slice first(first_half, HALF_LEN);
  ASSERT_OK(builder->Write(first, &offset));

  char outbuf[LEN];
  ASSERT_OK(builder->Read(offset, HALF_LEN, &result, outbuf));
  ASSERT_EQ(0, memcmp(inbuf, outbuf, HALF_LEN));

  //ASSERT_EQ(HALF_LEN, builder->end());
  //// file should exits now
  //ASSERT_OK(builder->Sync());
  //ASSERT_TRUE(env->FileExists(filename));

  //char outbuf[LEN];
  //ASSERT_OK(builder->Read(0, HALF_LEN, &result, outbuf));
  //ASSERT_EQ(0, memcmp(inbuf, outbuf, HALF_LEN));

  //char *second_half = inbuf + HALF_LEN;
  //memset(second_half, '1', HALF_LEN);
  //Slice second(second_half, HALF_LEN);
  //ASSERT_OK(builder->Write(second));
  //ASSERT_OK(builder->Sync());

  //ASSERT_OK(builder->Read(0, LEN, &result, outbuf));
  //ASSERT_EQ(0, memcmp(inbuf, outbuf, LEN));

  //ASSERT_OK(builder->Read(HALF_LEN - 5, 10, &result, outbuf));
  //ASSERT_EQ(0, strncmp("0000011111", outbuf, 10));

  //delete builder;
}

TEST(MrisTest, LargeBlockReaderTest) {
  std::string filename = NewBlockFileName();
  LargeBlockBuilder* builder = new LargeBlockBuilder(env, 0, filename);

  //char inbuf[LEN]; // use un-initialized as random data
  //Slice input(inbuf, LEN);
  //ASSERT_OK(builder->Write(input));
  //ASSERT_EQ(LEN, builder->end());
  //ASSERT_OK(builder->Sync());

  //LargeBlockReader* reader = new LargeBlockReader(env, builder);

  //char outbuf[LEN];
  //Slice result;
  //Random rand(383);
  //for (int i = 0; i < 100; ++i) {
    //uint32_t off = rand.Uniform(LEN);
    //uint32_t size = 1 + rand.Uniform(LEN - off);
    //ASSERT_OK(reader->Read(off, size, &result, outbuf));
    //ASSERT_EQ(size, result.size());
    //ASSERT_EQ(0, memcmp(result.data(), inbuf + off, size));
  //}
  //ASSERT_TRUE(reader->Read(1, LEN, &result, outbuf).IsIOError());
  //ASSERT_OK(reader->Read(1, 0, &result, outbuf));

  delete reader;
  delete builder;
}

TEST(MrisTest, LargeSpaceTest) {
  Options opt;
  LargeSpace* space = new LargeSpace(&opt, dbname);

  uint64_t offset;
  std::string message = "hello, world";
  Slice input(message);
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Write(input, offset));
  ASSERT_OK(space->Close());

  delete space;

  // load the large space again
  space = new LargeSpace(&opt, dbname);
  char buf[1024];
  Slice result;
  ASSERT_OK(space->Open());
  ASSERT_OK(space->Read(offset, message.length(), &result, buf));
  ASSERT_EQ(0, memcmp(buf, message.c_str(), message.length()));
}

} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
