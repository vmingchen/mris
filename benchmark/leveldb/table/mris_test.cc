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

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

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
    static int sequence;
};

int MrisTest::sequence = 0;

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
  ASSERT_EQ(0, strcmp(inbuf, outbuf, 10));
}

TEST(MrisTest, LargeBlockBuilderTest) {
  std::string name = NewBlockFileName();
}

TEST(MrisTest, LargeBlockReaderTest) {
  std::string name = NewBlockFileName();
}

} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
