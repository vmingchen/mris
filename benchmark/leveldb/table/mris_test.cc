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
#include "table/mris.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace leveldb { namespace mris {
TEST(LargeBlockHandle, basic) {
	std::string name = "block000001.lbf";
	LargeBlockHandle large_block(200, 299, name);

	EXPECT_TRUE(large_block.initialized());
	EXPECT_TRUE(large_block.contains(200, 299));
	EXPECT_TRUE(large_block.contains(210, 289));
	EXPECT_TRUE(large_block.contains(300, 0));
	EXPECT_TRUE(large_block.contains(499, 0));
	EXPECT_TRUE(large_block.contains(499, 1));

	EXPECT_FALSE(large_block.contains(0, 300));
	EXPECT_FALSE(large_block.contains(0, 500));
	EXPECT_FALSE(large_block.contains(200, 300));
	EXPECT_FALSE(large_block.contains(499, 2));
	EXPECT_FALSE(large_block.contains(500, 1));
}
} }

int main(int argc, char** argv) {
  return leveldb::test::RunAllTests();
}
