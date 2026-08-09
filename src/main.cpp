#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

using namespace geode::prelude;

// ─── Hook 1: Intercept Save ────────────────────────────────────────────────

class $modify(GitDashEditorLayer, LevelEditorLayer) {
    void saveLevel() {
        // Call original first — let GD write its normal save
        LevelEditorLayer::saveLevel();

        // ── Snapshot logic goes here ──
        auto levelString = m_level->m_levelString;

        log::info("[GitDash] saveLevel() fired!");
        log::info("[GitDash] Level ID: {}", m_level->m_levelID.value());
        log::info("[GitDash] Level string length: {} chars", levelString.size());

        // TODO: compress levelString and write to:
        //   Mod::get()->getSaveDir() / "snapshots" / "<levelID>" / "<timestamp>.snap"
    }

    void autoSave() {
        // Intercept autosave separately so we can handle it differently
        LevelEditorLayer::autoSave();
        log::info("[GitDash] autoSave() fired — snapshot skipped for now.");
    }
};

// ─── Hook 2: Inject Timeline UI into Editor Pause Menu ─────────────────────

class $modify(GitDashPauseLayer, EditorPauseLayer) {
    void customSetup() {
        EditorPauseLayer::customSetup();

        // Placeholder: just log that the pause menu opened
        // TODO: add CCSlider + "Restore" button here
        log::info("[GitDash] EditorPauseLayer opened — Timeline UI will go here.");
    }
};
