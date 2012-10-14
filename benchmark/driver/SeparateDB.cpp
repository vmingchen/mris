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
#include	<iostream>
#include	"leveldb/db.h"
#include	"leveldb/env.h"

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
	leveldb::Status put(const leveldb::WriteOptions &options,
			const leveldb::Slice &key, 
			const leveldb::Slice &value) {
		size_t len = value.size();
		size_t i;
		for (i = 0; i < ceil_.size() && len > ceil_[i]; ++i)
			;
		assert(i < ceil_.size());
		return stores_[i]->Put(options, key, value);
	}
	leveldb::Status get(const leveldb::ReadOptions &options,
			const leveldb::Slice &key,
			std::string *value) {
		leveldb::Status s;
		for (size_t i = 0; i < store_.size(); ++i) {
			s = store_[i]->Get(options, key, value);
			if (s.ok()) break;
		}
		return s;
	}
};

MrisDB *newMrisDB() {
	MrisDB *mdb = new MrisDB();
	mdb->addStore("/home/mchen/mrisdb/small", 256);
	mdb->addStore("/home/mchen/mrisdb/large", INT_MAX);
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
	leveldb::DB *db;
	leveldb::Options options;

	options.create_if_missing = true;
	options.write_buffer_size = 32 * 1024 * 1024 * 8;
	options.compression = leveldb::kNoCompression;

	leveldb::Status status = leveldb::DB::Open(options, "/home/mchen/mrisdb", &db);
	if (status.ok()) {
		leveldb::Slice key1 = "apple";
		leveldb::Slice val1 = "fruit";
		db->Put(leveldb::WriteOptions(), key1, val1);
		leveldb::Slice key2 = "carrot";
		leveldb::Slice val2 = "vegetable";
		db->Put(leveldb::WriteOptions(), key2, val2);
	}

	delete db;

	status = leveldb::DB::Open(options, "/home/mchen/mrisdb", &db);
	if (status.ok()) {
		std::string val;
		status = db->Get(leveldb::ReadOptions(), leveldb::Slice("carrot"), &val);
		if (status.ok()) 
			std::cout << "carrot: " << val << std::endl;
	}

	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
