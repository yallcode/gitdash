#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace geode::prelude;
namespace fs = std::filesystem;

// ─── Snapshot helpers ─────────────────────────────────────────────────────────

std::string Snapshot::relativeTimeString() const {
    auto now  = std::time(nullptr);
    auto diff = static_cast<long long>(now - timestamp);
    if (diff < 60)    return std::to_string(diff) + "s ago";
    if (diff < 3600)  return std::to_string(diff / 60) + " min ago";
    if (diff < 86400) return std::to_string(diff / 3600) + "h ago";
    return std::to_string(diff / 86400) + "d ago";
}

std::string Snapshot::timeString() const {
    std::tm* t = std::localtime(&timestamp);
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << t->tm_hour << ":"
       << std::setw(2) << t->tm_min  << ":"
       << std::setw(2) << t->tm_sec;
    return ss.str();
}

// ─── Singleton ────────────────────────────────────────────────────────────────

SnapshotManager& SnapshotManager::get() {
    static SnapshotManager instance;
    return instance;
}

// ─── Paths ────────────────────────────────────────────────────────────────────

fs::path SnapshotManager::getLevelDir(int levelID) {
    auto dir = Mod::get()->getSaveDir() / "snapshots" / std::to_string(levelID);
    if (!fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
    }
    return dir;
}

fs::path SnapshotManager::getIndexPath(int levelID) {
    return getLevelDir(levelID) / "index.txt";
}

// ─── Index I/O (pipe-delimited text, no JSON or zlib needed) ─────────────────

std::vector<Snapshot> SnapshotManager::loadIndex(int levelID) {
    auto path = getIndexPath(levelID);
    std::vector<Snapshot> result;
    if (!fs::exists(path)) return result;

    std::ifstream f(path);
    if (!f.is_open()) return result;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string tsStr, filename, sizeStr, label;
        if (!std::getline(ss, tsStr,    '|')) continue;
        if (!std::getline(ss, filename, '|')) continue;
        if (!std::getline(ss, sizeStr,  '|')) continue;
        std::getline(ss, label, '|');

        Snapshot s;
        try {
            s.timestamp = static_cast<std::time_t>(std::stoll(tsStr));
            s.dataSize  = static_cast<size_t>(std::stoull(sizeStr));
        } catch (...) { continue; }
        s.filename = filename;
        s.label    = label;
        result.push_back(s);
    }

    std::sort(result.begin(), result.end(), [](const Snapshot& a, const Snapshot& b) {
        return a.timestamp > b.timestamp;
    });
    return result;
}

bool SnapshotManager::saveIndex(int levelID, const std::vector<Snapshot>& snapshots) {
    std::ofstream f(getIndexPath(levelID));
    if (!f.is_open()) return false;
    for (const auto& s : snapshots) {
        f << s.timestamp << "|" << s.filename << "|" << s.dataSize << "|" << s.label << "\n";
    }
    return true;
}

// ─── Core API ─────────────────────────────────────────────────────────────────
// GD level strings are already base64+deflate compressed by the game,
// so we store them as-is without additional compression.

bool SnapshotManager::takeSnapshot(int levelID, const std::string& levelString) {
    if (levelString.empty()) return false;

    auto dir      = getLevelDir(levelID);
    auto ts       = std::time(nullptr);
    auto filename = std::to_string(ts) + ".snap";

    std::ofstream f(dir / filename, std::ios::binary);
    if (!f.is_open()) { log::error("[GitDash] Cannot write snap file"); return false; }
    f.write(levelString.data(), static_cast<std::streamsize>(levelString.size()));
    f.close();

    Snapshot snap;
    snap.timestamp = ts;
    snap.filename  = filename;
    snap.dataSize  = levelString.size();

    auto& cached = m_cache[levelID];
    if (cached.empty()) cached = loadIndex(levelID);
    cached.insert(cached.begin(), snap);
    saveIndex(levelID, cached);
    pruneIfNeeded(levelID);

    log::info("[GitDash] Snapshot #{} saved ({} bytes)", cached.size(), levelString.size());
    return true;
}

std::vector<Snapshot> SnapshotManager::getSnapshots(int levelID) {
    auto it = m_cache.find(levelID);
    if (it != m_cache.end()) return it->second;
    auto loaded = loadIndex(levelID);
    m_cache[levelID] = loaded;
    return loaded;
}

std::string SnapshotManager::loadSnapshot(int levelID, const Snapshot& snap) {
    std::ifstream f(getLevelDir(levelID) / snap.filename, std::ios::binary);
    if (!f.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

bool SnapshotManager::deleteSnapshot(int levelID, const Snapshot& snap) {
    std::error_code ec;
    fs::remove(getLevelDir(levelID) / snap.filename, ec);
    auto& cached = m_cache[levelID];
    cached.erase(std::remove_if(cached.begin(), cached.end(),
        [&](const Snapshot& s){ return s.filename == snap.filename; }), cached.end());
    saveIndex(levelID, cached);
    return !ec;
}

void SnapshotManager::deleteAllSnapshots(int levelID) {
    std::error_code ec;
    fs::remove_all(getLevelDir(levelID), ec);
    m_cache.erase(levelID);
}

size_t SnapshotManager::snapshotCount(int levelID) {
    return getSnapshots(levelID).size();
}

void SnapshotManager::pruneIfNeeded(int levelID) {
    int maxSnaps = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-snapshots"));
    auto& cached = m_cache[levelID];
    while (static_cast<int>(cached.size()) > maxSnaps) {
        std::error_code ec;
        fs::remove(getLevelDir(levelID) / cached.back().filename, ec);
        cached.pop_back();
    }
    saveIndex(levelID, cached);
}
