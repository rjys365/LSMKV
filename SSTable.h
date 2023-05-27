#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "BloomFilter.h"
#include "file_ops.h"
#include "kvstore_exceptions.h"
struct SSTable {
  std::string filename = "";
  static constexpr ssize_t HEADER_SIZE = 32;
  uint64_t dataSize = 0;
  uint64_t timestamp, count, minKey, maxKey;
  BloomFilter bloomFilter;
  std::vector<std::pair<uint64_t, uint64_t>> mapping;
  uint8_t *data;
  explicit SSTable(uint64_t dataSize) : dataSize(dataSize) {
    if (dataSize != 0)
      data = new uint8_t[dataSize]();
    else
      data = nullptr;
  }
  SSTable(const SSTable &right)
      : filename(right.filename),
        dataSize(right.dataSize),
        timestamp(right.timestamp),
        count(right.count),
        minKey(right.minKey),
        maxKey(right.maxKey),
        bloomFilter(right.bloomFilter),
        mapping(right.mapping) {
    if (dataSize != 0) {
      data = new uint8_t[dataSize]();
      memcpy(data, right.data, dataSize);
    } else
      data = nullptr;
  }
  SSTable(SSTable &&right)
      : filename(std::move(right.filename)),
        dataSize(right.dataSize),
        timestamp(right.timestamp),
        count(right.count),
        minKey(right.minKey),
        maxKey(right.maxKey),
        bloomFilter(std::move(right.bloomFilter)),
        mapping(std::move(right.mapping)),
        data(right.data) {
    right.dataSize = 0;
    right.data = nullptr;
  }
  ~SSTable() { delete[] data; }

  void toFile(const std::string &filename) {
    uint64_t byteCnt = 0;
    this->filename = filename;
    if (data == nullptr) return;
    std::fstream out;
    out.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    out.seekp(0);
    out.write(reinterpret_cast<const char *>(&timestamp), sizeof(uint64_t));
    byteCnt += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&count), sizeof(uint64_t));
    byteCnt += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&minKey), sizeof(uint64_t));
    byteCnt += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(&maxKey), sizeof(uint64_t));
    byteCnt += sizeof(uint64_t);
    out.write(reinterpret_cast<const char *>(bloomFilter.getDataPtr()),
              BloomFilter::SIZE * sizeof(bool));
    byteCnt += BloomFilter::SIZE * sizeof(bool);
    for (const auto &pair : mapping) {
      out.write(reinterpret_cast<const char *>(&pair.first), sizeof(uint64_t));
      out.write(reinterpret_cast<const char *>(&pair.second), sizeof(uint64_t));
      byteCnt += sizeof(uint64_t) + sizeof(uint64_t);
    }
    out.write(reinterpret_cast<const char *>(data), dataSize * sizeof(char));
    byteCnt += dataSize * sizeof(char);
    out.flush();
    out.close();
  }

  void freeData() {
    delete[] data;
    data = nullptr;
    dataSize = 0;
  }

  void loadData() {
    // should be called after cache is loaded
    if (filename == "") throw new EmptyFileNameException();
    std::fstream in;
    in.open(filename, std::ios::in | std::ios::binary);
    in.seekg(HEADER_SIZE + 10240ul +
             count * (sizeof(uint64_t) + sizeof(uint64_t)));
    auto fileSize = file_ops::getFileSize(filename);
    if (fileSize < 0) throw std::runtime_error("can't get file size");
    dataSize = fileSize - HEADER_SIZE - 10240ul -
               count * (sizeof(uint64_t) + sizeof(uint64_t));
    if (dataSize <= 0) throw std::runtime_error("data size is not positive");
    if (data != nullptr) delete[] data;
    data = new uint8_t[dataSize]();
    in.read(reinterpret_cast<char *>(data), dataSize);
    in.close();
  }

  static inline uint64_t calcPos(uint64_t keyNo) {
    // keyNo is 0-indexed
    constexpr uint64_t minKeyPos =
        4ul * sizeof(uint64_t) + BloomFilter::SIZE * sizeof(bool);
    return minKeyPos + keyNo * (sizeof(uint64_t) + sizeof(uint64_t));
  }

  void loadCache() {
    if (filename == "") throw new EmptyFileNameException();
    std::fstream in;
    in.open(filename, std::ios::in | std::ios::binary);
    in.seekg(0);
    in.read(reinterpret_cast<char *>(&this->timestamp), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&this->count), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&this->minKey), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&this->maxKey), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(this->bloomFilter.getDataPtr()),
            BloomFilter::SIZE * sizeof(bool));
    this->mapping.clear();
    for (uint64_t i = 0; i < this->count; i++) {
      uint64_t key, pos;
      in.read(reinterpret_cast<char *>(&key), sizeof(uint64_t));
      in.read(reinterpret_cast<char *>(&pos), sizeof(uint64_t));
      this->mapping.push_back(std::make_pair(key, pos));
    }
    in.close();
  }

  bool findWithoutCache(uint64_t key, std::string &result) {
    // try to find the value only knowing the filename.
    if (filename == "") throw new EmptyFileNameException();
    std::fstream in;
    in.open(filename, std::ios::in | std::ios::binary);
    uint64_t count, minKey, maxKey;
    in.seekg(sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&count), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&minKey), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&maxKey), sizeof(uint64_t));
    result = "";
    if (key < minKey || key > maxKey) {
      return false;
    }
    uint64_t leftNo = 0, rightNo = count - 1;  // binary search, [l,r]
    while (leftNo <= rightNo) {
      uint64_t midNo = (leftNo + rightNo) / 2;
      in.seekg(calcPos(midNo));
      uint64_t midKey;
      in.read(reinterpret_cast<char *>(&midKey), sizeof(uint64_t));
      if (midKey == key) {
        uint64_t valPos;
        in.read(reinterpret_cast<char *>(&valPos), sizeof(uint64_t));
        in.seekg(valPos);
        // result="";
        char currentChar;
        while (1) {
          in.read(&currentChar, sizeof(char));
          if (currentChar == '\0') break;
          result += currentChar;
        }
        return true;
      } else if (midKey < key) {
        leftNo = midNo + 1;
      } else {
        rightNo = midNo - 1;
      }
    }
    in.close();
    return false;
  }

  bool findWithCache(uint64_t key, std::string &result, bool useBloomFilter) {
    if (filename == "") throw new EmptyFileNameException();
    if (useBloomFilter && !bloomFilter.get(key)) return false;
    result = "";
    if (key < minKey || key > maxKey) {
      return false;
    }
    uint64_t leftNo = 0, rightNo = count - 1;  // binary search, [l,r]
    while (leftNo <= rightNo) {
      uint64_t midNo = (leftNo + rightNo) / 2;
      uint64_t midKey = mapping[midNo].first;
      if (midKey == key) {
        uint64_t valPos = mapping[midNo].second;
        // result="";
        char currentChar;
        std::fstream in;
        in.open(filename, std::ios::in | std::ios::binary);
        in.seekg(32ul + BloomFilter::SIZE +
                 count * (sizeof(uint64_t) + sizeof(uint64_t)) + valPos);
        while (1) {
          in.read(&currentChar, sizeof(char));
          if (currentChar == '\0') break;
          result += currentChar;
        }
        in.close();
        return true;
      } else if (midKey < key) {
        leftNo = midNo + 1;
      } else {
        rightNo = midNo - 1;
      }
    }
    return false;
  }

  bool find(uint64_t key, std::string &result, bool useCache,
            bool useBloomFilter) {
    if (useCache) {
      return findWithCache(key, result, useBloomFilter);
    } else {
      return findWithoutCache(key, result);
    }
  }
};
