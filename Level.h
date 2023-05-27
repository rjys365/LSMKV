#pragma once

#include <set>
#include <algorithm>

#include "SSTable.h"

enum class LevelType { TIERING, LEVELING };

class Level {
 private:
  int id;
  LevelType type;
  int maxSize;
  std::map<uint64_t, SSTable> sstables;

 public:
  Level(int id, int maxSize, LevelType type)
      : id(id), type(type), maxSize(maxSize){};
  uint64_t addExistingSSTable(const std::string &filename) {
    SSTable sstable(0);
    sstable.filename = filename;
    sstable.loadCache();
    uint64_t timestamp = sstable.timestamp;
    auto emplaceRet = sstables.emplace(timestamp, sstable);
    if (!emplaceRet.second)
      throw std::runtime_error("Duplicated SSTable timestamp");
    return timestamp;
  }
  void addNewSSTable(SSTable &&sstable) {
    uint64_t timestamp = sstable.timestamp;
    auto emplaceRet = sstables.emplace(timestamp, sstable);
    if (!emplaceRet.second)
      throw std::runtime_error("Duplicated SSTable timestamp");
  }
  bool find(uint64_t key, std::string &value, bool useCache = true,
            bool useBloomFilter = true) {
    if (!useCache && useBloomFilter)
      throw std::runtime_error("illegal finding method");
    if (this->type == LevelType::TIERING) {
      std::string candidateValue;
      uint64_t candidateTimeStamp;
      bool found = false;
      for (auto &pair : sstables) {
        auto &sstable = pair.second;
        if (sstable.find(key, candidateValue, useCache, useBloomFilter)) {
          if (!found || candidateTimeStamp < sstable.timestamp) {
            value = candidateValue;
            candidateTimeStamp = sstable.timestamp;
            found = true;
          }
        }
      }
      // if(found)value=candidateValue;
      return found;
    } else {
      // LEVELING
      for (auto &pair : sstables) {
        auto &sstable = pair.second;
        if (sstable.find(key, value, useCache, useBloomFilter)) {
          return true;
        }
      }
      return false;
    }
  }
  void compactInto(Level &nextLevel) {
    std::vector<std::map<uint64_t, SSTable>::iterator> thisLevelSelected;
    if (this->type == LevelType::TIERING) {
      for (auto it = this->sstables.begin(); it != this->sstables.end(); it++) {
        thisLevelSelected.push_back(it);
      }
    } else {
      // LEVELLING
      int i = 0;
      for (auto it = this->sstables.begin();
           it != this->sstables.end() &&
           i < this->sstables.size() - this->maxSize;
           it++) {
        thisLevelSelected.push_back(it);
      }
    }
  }
};
