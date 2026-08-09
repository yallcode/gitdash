#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/EditorUI.hpp>

#include "SnapshotManager.hpp"
#include "TimelinePopup.hpp"
#include "GitDashState.hpp"

using namespace geode::prelude;

// Definition lives here — shared via GitDashState.hpp
bool s_restorePending = false;

// ── Guard EditorUI::updateButtons against post-restore null state ─────────────

class $modify(GitDashEditorUI, EditorUI) {
    void updateButtons() {
        if (s_restorePending) {
            s_restorePending = false;
            return;
        }
        EditorUI::updateButtons();
    }
};

// ── EditorPauseLayer hooks ────────────────────────────────────────────────────

class $modify(GitDashPauseLayer, EditorPauseLayer) {

    void onSaveAndPlay(CCObject* sender) {
        EditorPauseLayer::onSaveAndPlay(sender);
        takeSnapshotIfChanged();
    }

    void onSaveAndExit(CCObject* sender) {
        EditorPauseLayer::onSaveAndExit(sender);
        takeSnapshotIfChanged();
    }

    void onSave(CCObject* sender) {
        EditorPauseLayer::onSave(sender);
        takeSnapshotIfChanged();
    }

    void customSetup() {
        EditorPauseLayer::customSetup();

        auto winSize = CCDirector::get()->getWinSize();
        auto spr = ButtonSprite::create(
            "Timeline", "bigFont.fnt", "GJ_button_04.png", 0.5f
        );
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(GitDashPauseLayer::onOpenTimeline)
        );
        auto menu = CCMenu::create(btn, nullptr);
        menu->setPosition({ winSize.width - 55.f, winSize.height - 25.f });
        menu->setZOrder(10);
        this->addChild(menu, 10);
    }

    void onOpenTimeline(CCObject*) {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;
        auto popup = TimelinePopup::create(editorLayer);
        if (popup) CCDirector::get()->getRunningScene()->addChild(popup, 999);
    }

    void takeSnapshotIfChanged() {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;

        int  levelID     = editorLayer->m_level->m_levelID.value();
        auto levelString = editorLayer->m_level->m_levelString;

        if (levelString.empty()) return;

        auto existing = SnapshotManager::get().getSnapshots(levelID);
        if (!existing.empty()) {
            if (existing.front().dataSize == levelString.size()) {
                auto lastData = SnapshotManager::get().loadSnapshot(levelID, existing.front());
                if (lastData == levelString) {
                    log::debug("[GitDash] Level unchanged — skipping snapshot.");
                    return;
                }
            }
        }

        bool ok = SnapshotManager::get().takeSnapshot(levelID, levelString);
        if (ok) {
            size_t count = SnapshotManager::get().snapshotCount(levelID);
            Notification::create(
                fmt::format("GitDash: Snapshot #{} saved", count),
                NotificationIcon::Success,
                1.5f
            )->show();
        }
    }
};
