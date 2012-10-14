/*
 * ===========================================================================
 *
 *       Filename:  SeparateDB.cpp
 *         Author:  Ming Chen, brianchenming@gmail.com
 *        Created:  10/13/2012 03:23:43 AM
 *
 *    Description:  Create a Mris store by using different DBs
 *
 *       Revision:  none
 *
 * ===========================================================================
 */


#include	<stdlib.h>
#include	<assert.h>
#include	<limits.h>
#include	<stdio.h>
#include	"leveldb/db.h"
#include	"leveldb/env.h"
#include	"util/testutil.h"
#include	"util/random.h"
#include	"mrisdb/RequestGenerator.h"

const int SMALL_IMAGE_SIZE = 256;
const int LARGE_IMAGE_SIZE = 4096;
const int IMAGE_COUNT = 10;

namespace mris {

class MrisDB {
private:
	std::vector<int> ceils_;
	std::vector<leveldb::DB *> stores_;
	void clear() {
		while (!stores_.empty()) {
			leveldb::DB *db = stores_.back();
			stores_.pop_back();
			delete db;
		}
		ceils_.clear();
	}
public:
	MrisDB() {}
	~MrisDB() { clear(); }
	bool addStore(const char *name, int ceil) {
		leveldb::DB *db;
		leveldb::Options options;

		options.create_if_missing = true;
		options.write_buffer_size = 32 * 1024 * 1024 * 8;
		options.compression = leveldb::kNoCompression;

		leveldb::Status status = leveldb::DB::Open(options, name, &db);
		if (! status.ok()) return false;

		stores_.push_back(db);
		ceils_.push_back(ceil);

		return true;
	}
	leveldb::Status put(const leveldb::Slice &key,
			const leveldb::Slice &value) {
		return put(leveldb::WriteOptions(), key, value);
	}
	leveldb::Status put(const leveldb::WriteOptions &options,
			const leveldb::Slice &key, 
			const leveldb::Slice &value) {
		size_t len = value.size();
		size_t i;
		for (i = 0; i < ceils_.size() && len > ceils_[i]; ++i)
			;
		assert(i < ceils_.size());
		return stores_[i]->Put(options, key, value);
	}
	leveldb::Status get(const leveldb::Slice &key, std::string *value) {
		return get(leveldb::ReadOptions(), key, value);
	}
	leveldb::Status get(const leveldb::ReadOptions &options,
			const leveldb::Slice &key,
			std::string *value) {
		leveldb::Status s;
		for (size_t i = 0; i < stores_.size(); ++i) {
			s = stores_[i]->Get(options, key, value);
			if (s.ok()) break;
		}
		return s;
	}
};

}

mris::MrisDB *newMrisDB() {
	mris::MrisDB *mdb = new mris::MrisDB();
	if (! mdb->addStore("/home/mchen/mrisdb/small", 1024) 
			|| ! mdb->addStore("/home/mchen/mrisdb/large", INT_MAX))
		return NULL;
	return mdb;
}

/* 
 * ===  FUNCTION  ============================================================
 *         Name:  main
 *  Description:  
 * ===========================================================================
 */
	int
main ( int argc, char *argv[] )
{ 
	using namespace mris;
	MrisDB *mdb = newMrisDB();

	if (! mdb) {
		printf("Cannot create MrisDB.\n");
		return EXIT_FAILURE;
	}

	// Insert Images
	std::string lkey = "large-image-";
	std::string skey = "small-image-";
	std::string value;
	leveldb::Random rand(383);
	for (int i = 0; i < IMAGE_COUNT; ++i) {
		char ids[16];
		snprintf(ids, 16, "%06d", i);
		using leveldb::test::RandomString;
		mdb->put(skey + ids, RandomString(&rand, SMALL_IMAGE_SIZE, &value));
		mdb->put(lkey + ids, RandomString(&rand, LARGE_IMAGE_SIZE, &value));
	}

	const int REQUEST_COUNT = 100;
	RequestGenerator generator(Request(256, 13), Request(4096, 1));

	double start = leveldb::Env::Default()->NowMicros();
	int smallCount = 0;
	int largeCount = 0;

	for (int i = 0; i < REQUEST_COUNT; ++i) {
		int idx = (int)(drand48() * IMAGE_COUNT);
		char ids[16];
		snprintf(ids, 16, "%06d", idx);
		if (generator.generate() == SMALL_IMAGE_SIZE) {
			++smallCount;
			mdb->get(skey + ids, &value);
		} else {
			++largeCount;
			mdb->get(lkey + ids, &value);
		}
	}

	double finish = leveldb::Env::Default()->NowMicros();

	printf("%d requests served (%d small + %d large).\n", REQUEST_COUNT, 
			smallCount, largeCount);
	printf("%.2lf request/second\n", REQUEST_COUNT * 1e6 / (finish - start));

	delete mdb;

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
