#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <iostream>

#include "MemTable.h"
#include "SSTable.h"
#include "utils.h"

enum class LevelType { TIERING, LEVELING };

inline void insertWithHint(std::multimap<uint64_t, SSTable> &multimap,uint64_t key,SSTable &value){
  if(multimap.count(key)==0){
    multimap.emplace(key,value);
    return;
  }
  auto i1 = multimap.equal_range(key);
  for(auto i2 = i1.first; i2 != i1.second; ++i2){
    if(i2->second.minKey>value.minKey){
      multimap.emplace_hint(i2,key,value);
      return;
    }
  }
  multimap.emplace(key,value);
}

class Level {
 private:
  int id;
  LevelType type;
  int maxSize;
  std::multimap<uint64_t, SSTable> sstables;

 public:
  Level(int id, int maxSize, LevelType type)
      : id(id), type(type), maxSize(maxSize){};
  uint64_t addExistingSSTable(const std::string &filename) {
    SSTable sstable(0);
    sstable.filename = filename;
    sstable.loadCache();
    uint64_t timestamp = sstable.timestamp;
    insertWithHint(sstables,timestamp,sstable);//sstables.emplace(timestamp, sstable);
    // if (!emplaceRet.second)
    //   throw std::runtime_error("Duplicated SSTable timestamp");
    return timestamp;
  }
  void addNewSSTable(SSTable &&sstable) {
    uint64_t timestamp = sstable.timestamp;
    insertWithHint(sstables,timestamp,sstable);// sstables.emplace(timestamp, sstable);
    // if (!emplaceRet.second)
    //   throw std::runtime_error("Duplicated SSTable timestamp");
  }
  bool find(uint64_t key, std::string &value, bool useCache = true,
            bool useBloomFilter = true) {
    if (!useCache && useBloomFilter)
      throw std::runtime_error("Illegal finding method");
    if (this->type == LevelType::TIERING) {
      std::string candidateValue;
      uint64_t candidateTimestamp;
      bool found = false;
      for (auto &pair : sstables) {
        auto &sstable = pair.second;
        if (sstable.find(key, candidateValue, useCache, useBloomFilter)) {
          if (!found || candidateTimestamp < sstable.timestamp) {
            value = candidateValue;
            candidateTimestamp = sstable.timestamp;
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

  void compactInto(Level &nextLevel, uint64_t &maxFileNo,
                   const std::string &dir) {
    std::vector<std::multimap<uint64_t, SSTable>::iterator> thisLevelSelected,
        nextLevelSelected;
    uint64_t thisLevelSelectedMinKey = this->sstables.begin()->second.minKey,
             thisLevelSelectedMaxKey = this->sstables.begin()->second.maxKey;
    uint64_t selectedMaxTimestamp=0;
    if (this->type == LevelType::TIERING) {
      for (auto it = this->sstables.begin(); it != this->sstables.end(); it++) {
        thisLevelSelected.push_back(it);
        it->second.loadData();
        thisLevelSelectedMinKey =
            std::min(thisLevelSelectedMinKey, it->second.minKey);
        thisLevelSelectedMaxKey =
            std::max(thisLevelSelectedMaxKey, it->second.maxKey);
        selectedMaxTimestamp=std::max(it->first,selectedMaxTimestamp);
      }
    } else {
      // LEVELLING
      int i = 0;
      for (auto it = this->sstables.begin();
           it != this->sstables.end() &&
           i < (int)this->sstables.size() - this->maxSize;
           it++, i++) {
        thisLevelSelected.push_back(it);
        it->second.loadData();
        thisLevelSelectedMinKey =
            std::min(thisLevelSelectedMinKey, it->second.minKey);
        thisLevelSelectedMaxKey =
            std::max(thisLevelSelectedMaxKey, it->second.maxKey);
        selectedMaxTimestamp=std::max(it->first,selectedMaxTimestamp);
      }
    }
    if (nextLevel.type == LevelType::LEVELING) {
      for (auto it = nextLevel.sstables.begin(); it != nextLevel.sstables.end();
           it++) {
        uint64_t minKey = it->second.minKey, maxKey = it->second.maxKey;
        if (minKey <= thisLevelSelectedMaxKey &&
            thisLevelSelectedMinKey <= maxKey) {
          nextLevelSelected.push_back(it);
          it->second.loadData();
        }
      }
    }
    // if next level is tiering, nothing is selected.
    uint64_t *thisLevelSelectedPos = new uint64_t[thisLevelSelected.size()](),
             *nextLevelSelectedPos = new uint64_t[nextLevelSelected.size()]();
    MemTable tmpMemTable;
    struct candidate_t {
      bool thisLevel;
      uint64_t tableIndex;
      uint64_t posInTable;
    };

    // if(maxFileNo==1983){
    //   std::cout<<"a";
    // }

    while (1) {
      // if(maxFileNo==1983){
      //   std::cout<<"a";
      // }
      uint64_t minKey = UINT64_MAX, minKeyTimestamp = 0;
      std::vector<candidate_t> candidates;
      candidate_t candidateToMerge;
      bool found = false;
      for (uint64_t i = 0; i < thisLevelSelected.size(); i++) {
        if (thisLevelSelectedPos[i] >= thisLevelSelected[i]->second.count)
          continue;
        uint64_t candidateKey =
            thisLevelSelected[i]->second.mapping[thisLevelSelectedPos[i]].first;
        // if(candidateKey==17836){
        //   std::cout<<"a";
        // }
        candidate_t candidate({true, i, thisLevelSelectedPos[i]});
        if (!found || candidateKey < minKey) {
          minKey = candidateKey;
          found = true;
          candidates.clear();
          candidates.push_back(candidate);
          candidateToMerge = candidate;
          minKeyTimestamp = thisLevelSelected[i]->first;
        } else if (candidateKey == minKey) {
          candidates.push_back(candidate);
          if (thisLevelSelected[i]->first > minKeyTimestamp) {
            candidateToMerge = candidate;
            minKeyTimestamp = thisLevelSelected[i]->first;
          }
        }
      }
      for (uint64_t i = 0; i < nextLevelSelected.size(); i++) {
        if (nextLevelSelectedPos[i] >= nextLevelSelected[i]->second.count)
          continue;
        uint64_t candidateKey =
            nextLevelSelected[i]->second.mapping[nextLevelSelectedPos[i]].first;
        // if(candidateKey==17836){
        //   std::cout<<"a";
        // }
        candidate_t candidate = {false, i, nextLevelSelectedPos[i]};
        if (!found || candidateKey < minKey) {
          minKey = candidateKey;
          found = true;
          candidates.clear();
          candidates.push_back(candidate);
          candidateToMerge = candidate;
          minKeyTimestamp = nextLevelSelected[i]->first;
        } else if (candidateKey == minKey) {
          candidates.push_back(candidate);
          if (nextLevelSelected[i]->first > minKeyTimestamp) {
            candidateToMerge = candidate;
            minKeyTimestamp = nextLevelSelected[i]->first;
          }
        }
      }
      if (!found) break;
      std::string value;
      auto &selectedTablesToMerge =
          (candidateToMerge.thisLevel ? thisLevelSelected : nextLevelSelected);
      auto &selectedTableToMerge =
          selectedTablesToMerge[candidateToMerge.tableIndex]->second;
      uint64_t &valuePosToMerge =
          selectedTableToMerge.mapping[candidateToMerge.posInTable].second;
      uint64_t pos = valuePosToMerge;
      std::string valueToMerge;
      while (selectedTableToMerge.data[pos] != '\0') {
        valueToMerge += selectedTableToMerge.data[pos];
        pos++;
      }
      if (!tmpMemTable.put(minKey, valueToMerge)) {
        ++maxFileNo;
        SSTable table = tmpMemTable.toSSTable(selectedMaxTimestamp);
        table.toFile(dir + "/level-" + std::to_string(nextLevel.id) + "/" +
                     std::to_string(maxFileNo) + ".sst");
        table.freeData();
        nextLevel.addNewSSTable(std::move(table));
        tmpMemTable.clear();
        tmpMemTable.put(minKey, valueToMerge);
      }
      for (auto &candidate : candidates) {
        auto &posArray =
            (candidate.thisLevel ? thisLevelSelectedPos : nextLevelSelectedPos);
        posArray[candidate.tableIndex]++;
      }
      // SSTable
      // &minKeyTable=(candidateToBeMerged.thisLevel?(thisLevelSelected[candida]):(nextLevel));

      // tmpMemTable.put(minKey,)
    }
    delete[] thisLevelSelectedPos;
    delete[] nextLevelSelectedPos;
    if (tmpMemTable.itemCnt() != 0) {
      ++maxFileNo;
      SSTable table = tmpMemTable.toSSTable(selectedMaxTimestamp);
      std::string filename = dir + "/level-" + std::to_string(nextLevel.id) +
                             "/" + std::to_string(maxFileNo) + ".sst";
      table.toFile(filename);
      table.freeData();
      nextLevel.addNewSSTable(std::move(table));
    }
    for (auto &it : thisLevelSelected) {
      utils::rmfile(it->second.filename.c_str());
      it->second.freeData();
      this->sstables.erase(it);
    }
    for (auto &it : nextLevelSelected) {
      utils::rmfile(it->second.filename.c_str());
      it->second.freeData();
      nextLevel.sstables.erase(it);
    }
  }
  bool overSized() { return (int)this->sstables.size() > this->maxSize; }
};
