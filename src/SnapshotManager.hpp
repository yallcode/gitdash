#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <ctime>
#include <filesystem>
#include <unordered_map>

using namespace geode::prelude;
namespace fs = std::filesystem;

struct Snapshot {
    std::time_t timestamp = 0;
    std::string filename;
    std::string label;
    size_t      dataSize = 0;

    std::string relativeTimeString() const;
    std::string timeString() const;
};

class SnapshotManager {
public:
    static SnapshotManager& get();
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;

    bool takeSnapshot(int levelID, const std::string& levelString);
    std::vector<Snapshot> getSnapshots(int levelID);
    std::string loadSnapshot(int levelID, const Snapshot& snap);
    bool deleteSnapshot(int levelID, const Snapshot& snap);
    void deleteAllSnapshots(int levelID);
    size_t snapshotCount(int levelID);

private:
    SnapshotManager() = default;

    fs::path getLevelDir(int levelID);
    fs::path getIndexPath(int levelID);
    bool saveIndex(int levelID, const std::vector<Snapshot>& snapshots);
    std::vector<Snapshot> loadIndex(int levelID);
    void pruneIfNeeded(int levelID);

    std::unordered_map<int, std::vector<Snapshot>> m_cache;
};
