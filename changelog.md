# Changelog

## v0.1.5
- Removed iOS target (Geode's iOS SDK doesn't support std::filesystem yet)
- Fixed post-restore crash when tapping objects (guarded canPasteState against null)
- Rewrote README and about.md
- Fixed compiler errors: removed unhookable canPasteState, use updateButtons guard instead

## v0.1.4
- Added iOS support (reverted in v0.1.5)
- Fixed crash related to BetterEdit compatibility during snapshot comparison
- Optimized change detection to skip unnecessary file reads

## v0.1.3
- Fixed crash when tapping objects after restoring
- Fixed freeze on editor exit after restore
- Fixed snapshots saving when nothing changed

## v0.1.2
- Fixed crash when restoring while a touch event was still processing

## v0.1.1
- Fixed mod index submission

## v0.1.0
- Initial release
- Snapshot on Save, Save and Play, Save and Exit
- Timeline popup in editor pause menu
- Navigate with arrows, restore in one tap
- Delete single or all snapshots
- Configurable max snapshot count
- Optional autosave snapshotting
- Timestamp display toggle
