#include "kvstore.h"

#include <algorithm>
#include <string>
#include <vector>

#include "Level.h"
#include "file_ops.h"
#include "utils.h"

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir), dir(dir) {
  config.loadConfig(lsmkv_constants::CONFIG_FILENAME);
  if (utils::dirExists(dir)) {
    std::vector<std::string> paths;
    utils::scanDir(dir, paths);
    for (const auto &path : paths) {
      std::string levelPath = dir + "/" + path;
      if (!file_ops::isDirectory(levelPath) || path.substr(0, 6) != "level-") {
        throw std::runtime_error("Unexpected file in kvstore directory");
      }
      auto level_id = std::stoul(path.substr(6));
      if (level_id >= levels.size()) {
        for (auto i = levels.size(); i <= level_id; i++) {
          levels.push_back(
              Level(i, config.getLayerSize(i), config.getLayerType(i)));
        }
      }
      std::vector<std::string> level_filenames;
      utils::scanDir(levelPath, level_filenames);
      for (const auto &sstFilename : level_filenames) {
        std::string file_path = levelPath + "/" + sstFilename;
        if (!file_ops::isFile(file_path) ||
            sstFilename.substr(sstFilename.length() - 4) != ".sst") {
          throw std::runtime_error(
              "Unexpected file in kvstore level directory");
        }
        // auto sst_id=std::stoi(sstFilename.substr(0,sstFilename.length()-4));
        auto timestamp = levels[level_id].addExistingSSTable(file_path);
        this->maxTimeStamp = std::max(this->maxTimeStamp, timestamp);
      }
    }
  } else {
    utils::mkdir(dir.c_str());
    // levels are left empty.
  }
}

void KVStore::saveMemTable() {
  if(memTable.itemCnt()==0)return;
  maxTimeStamp++;
  if (!utils::dirExists(dir + "/level-0")) {
    utils::mkdir((dir + "/level-0").c_str());
    levels.emplace_back(0, config.getLayerSize(0), config.getLayerType(0));
  }
  std::string filename =
      dir + "/level-0/" + std::to_string(maxTimeStamp) + ".sst";
  auto sstable = memTable.toSSTable(maxTimeStamp);
  sstable.toFile(filename);
  sstable.freeData();
  levels[0].addNewSSTable(std::move(sstable));
  memTable.clear();
  // TODO:compaction
}

KVStore::~KVStore() { saveMemTable(); }

void KVStore::compact(){

}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
  bool memTablePutSuccessful = memTable.put(key, s);
  if (!memTablePutSuccessful) {
    saveMemTable();
    memTable.clear();
    memTable.put(key, s);
  }
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
  auto value = memTable.get(key);
  if (value == "~DELETED~") return "";
  if (value != "") return value;
  for (size_t levelId = 0; levelId < levels.size(); levelId++) {
    std::string value;
    if (levels[levelId].find(key, value)) {
      if (value == "~DELETED~") return "";
      return value;
    }
  }
  return "";
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
  if (this->get(key) == "") return false;
  this->put(key, "~DELETED~");
  return true;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
  memTable.clear();
  levels.clear();
  maxTimeStamp=0;
  std::vector<std::string> paths;
  utils::scanDir(dir, paths);
  for (const auto &path : paths) {
    std::string levelPath = dir + "/" + path;
    if (!file_ops::isDirectory(levelPath) || path.substr(0, 6) != "level-") {
      throw std::runtime_error("Unexpected file in kvstore directory");
    }
    std::vector<std::string> level_filenames;
    utils::scanDir(levelPath, level_filenames);
    for (const auto &sstFilename : level_filenames) {
      std::string file_path = levelPath + "/" + sstFilename;
      if (!file_ops::isFile(file_path) ||
          sstFilename.substr(sstFilename.length() - 4) != ".sst") {
        throw std::runtime_error("Unexpected file in kvstore level directory");
      }
      utils::rmfile(file_path.c_str());
    }
    utils::rmdir(levelPath.c_str());
  }
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2,
                   std::list<std::pair<uint64_t, std::string> > &list) {}
