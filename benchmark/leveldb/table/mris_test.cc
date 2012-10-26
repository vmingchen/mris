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
#include "leveldb/slice.h"
#include "leveldb/env.h"
#include "table/mris.h"
#include "util/testharness.h"
#include "util/testutil.h"

static Status bad = Status::NotFound("[mris] bad status");

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))
#define ASSERT_BAD(x) ASSERT_OK(((x).ok() ? bad : Status::OK()))

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

TEST(MrisAppendReadFile, basic) {
  Env* env = new PosixEnv();
  std::string filename = "block000002.lbf";
  MrisAppendReadFile *mris_file = NULL;

  // create an empty file
  ASSERT_OK(NewMrisAppendReadFile(filename, &mris_file));
  ASSERT_TRUE(mris_file);
  ASSERT_OK(mris_file->Sync());
  ASSERT_TRUE(env->FileExists(filename));

  char buf[1024];
  Slice result;
  // read from empty file should be IOError
  ASSERT_TRUE(mris_file->Read(0, 10, &result, buf).IsIOError());

  // append some data
  memset(buf, 'a', 512);
  Slice data(buf, 512);
  ASSERT_OK(mris_file->Append(data));

  delete env;
}

TEST(LargeBlockBuilder, basic) {
  Env* env = new PosixEnv();
  std::string name = "block000003.lbf";

  delete env;
}

TEST(LargeBlockReader, basic) {
  Env* env = new PosixEnv();
  std::string name = "block000002.lbf";
  LargeBlockReader reader(env, 1000, 1000, name);



  delete env;
}

} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}

// vim: set shiftwidth=2 tabstop=2 expandtab:
