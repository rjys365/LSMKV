#pragma once

#include <string>
#include <vector>

#include "Config.h"
#include "MemTable.h"
#include "kvstore_api.h"

class KVStore : public KVStoreAPI {
  // You can add your implementation here
 private:
  std::string dir;
  MemTable memTable;
  uint64_t maxFileNo = 0;
  uint64_t maxTimeStamp = 0;
  Config config;
  void saveMemTable();
  std::vector<Level> levels;
  // void init();
  void compact();

 public:
  static const uint64_t MAX_FILE_SIZE = 2ul * 1024ul * 1024ul;

  KVStore(const std::string &dir);

  ~KVStore();

  void put(uint64_t key, const std::string &s) override;

  void putDiag(uint64_t key, const std::string &s, bool &compact);

  std::string get(uint64_t key) override;

  std::string getDiag(uint64_t key, bool useCache, bool useBloomfilter);

  bool del(uint64_t key) override;

  void reset() override;

  void scan(uint64_t key1, uint64_t key2,
            std::list<std::pair<uint64_t, std::string> > &list) override;
};
