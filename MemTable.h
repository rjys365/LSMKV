#pragma once
#include <cstdint>
#include <cstring>
// #include "kvstore.h"
#include "SSTable.h"
#include "SkipList.h"
#include "constants.h"

// class KVStore;

class MemTable {
 private:
  SkipList<uint64_t, std::string> skiplist;
  size_t currentDataSize = 0;

 public:
  std::string get(uint64_t key) {
    std::string *ptr = skiplist.at(key);
    if (ptr == nullptr) return "";  // TODO: really do this here?
    return *ptr;
  }
  bool put(uint64_t key, const std::string &value) {
    std::string *foundString = skiplist.at(key);
    if (foundString == nullptr) {
      size_t newDataSize = currentDataSize + value.length() + 1;
      if (newDataSize + 32ul + 10240ul +
              (skiplist.size()) * (sizeof(uint64_t) + sizeof(size_t)) >
          lsmkv_constants::MAX_FILE_SIZE)
        return false;
      skiplist.insert(key, value);
      currentDataSize = newDataSize;
    } else {
      bool willExceed = false;
      size_t length = value.length(), newLength = foundString->length();
      if (newLength > length) {
        if (newLength - length + currentDataSize + 32ul + 10240ul +
                (skiplist.size()) * (sizeof(uint64_t) + sizeof(size_t)) >
            lsmkv_constants::MAX_FILE_SIZE)
          willExceed = true;
      }
      if (willExceed) return false;
      *foundString = value;
      currentDataSize = currentDataSize + newLength - length;
    }
    return true;
  }
  void clear() {
    skiplist.clear();
    currentDataSize = 0;
  }
  // allocate and make an SSTable
  // the caller is responsible to delete it after using
  SSTable toSSTable(uint64_t timestamp) {
    SSTable ret(currentDataSize);
    ret.count = skiplist.size();
    ret.timestamp = timestamp;
    auto it = skiplist.begin(), endIt = skiplist.end();
    uint64_t dataPos = 0;
    for (; it != endIt; it++) {
      const uint64_t &key = it.getKey();
      const std::string &value = it.getValue();
      ret.mapping.push_back(std::make_pair(key, dataPos));
      memcpy(ret.data + dataPos, value.c_str(), value.length() + 1ul);
      dataPos += (value.length() + 1ul);
      ret.bloomFilter.set(key);
    }
    ret.minKey = ret.mapping[0].first;
    ret.maxKey = ret.mapping[ret.mapping.size() - 1].first;
    return ret;
  }
  int itemCnt() { return skiplist.size(); }
};
