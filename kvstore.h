#pragma once

#include "kvstore_api.h"
#include "MemTable.h"
#include "Config.h"
#include <vector>
#include <string>

class KVStore : public KVStoreAPI {
	// You can add your implementation here
private:
	std::string dir;
	MemTable memTable;
	uint64_t maxTimeStamp=0;
	Config config;
	void saveMemTable();
	std::vector<Level> levels;
	// void init();
	void compact();
public:
	static const uint64_t MAX_FILE_SIZE=2ul*1024ul*1024ul;

	KVStore(const std::string &dir);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

	void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string> > &list) override;
};
