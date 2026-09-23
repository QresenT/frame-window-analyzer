# 0.1.12

- Corrected GD 2.2081 `m_tickIndex` units: two raw tick-index units equal one 240 Hz physics frame.
- Analysis now tests only the reference tick parity in steps of two, groups windows in steps of two, and reports proper 240 Hz frame and millisecond values.
- Search Left and Search Right now use 240 Hz frames and default to 5 on each side.
- Removed the odd half-tick native timestamp injection path; standard 240 TPS analysis uses deterministic direct playback again.
- Cleared the in-progress result buffer at the start of every analysis so repeated runs cannot retain prior valid ticks.

# 0.1.11

- Fixed Analyze consuming the entire future macro at tick 2. GD's native button queue is per-update rather than a persistent future-event scheduler.
- Analysis inputs are now queued just in time before the update whose tick range contains each event, preserving the macro while allowing odd-tick timestamp splitting.

# 0.1.10

- Fixed native queued analysis inputs being discarded because their timestamps were incorrectly level-relative instead of based on GD's absolute input clock.
- Analysis now uses GD's `queueButton` path and temporarily enables between-step timestamp handling so odd 240 TPS ticks are consumed, restoring the prior setting afterward.

# 0.1.9

- Split full-macro analysis into `All Clicks`, `All Releases`, and `All Inputs` so timing windows can be measured separately for presses, releases, or both.
- Full-analysis progress now reports the filtered target number, original macro input index, and PRESS/RELEASE action.

# 0.1.8

- Fixed odd 240 TPS candidate ticks being silently skipped when `processCommands` advanced by two steps.
- Analysis now pre-schedules the macro in GD 2.2081's native timestamp button queue, allowing `update` to split at odd half-ticks as the game does for timestamped input.

# 0.1.7

- Partial validation is now the default; `Until End` remains optional.
- Replaced `Next Clicks` with `Next Inputs` (default 3), counting each subsequent PRESS or RELEASE and replaying it at the recorded tick plus a short settling tail. A nonzero input horizon takes precedence over the tick horizon.

# 0.1.6

- Added `Until End` validation (enabled by default): only the selected event tick moves, every other PRESS/RELEASE stays at its recorded tick, and the candidate succeeds only on actual level completion.
- Tick and `Next Clicks` horizons remain available as an optional faster partial-validation mode.

# 0.1.5

- Added a `Next Clicks` input horizon (default 1) so candidates replay and survive through a subsequent PRESS/RELEASE segment before succeeding.
- Tick validation remains a minimum horizon; the next same-player/button PRESS defines the boundary before a later obstacle.

# 0.1.4

- Playback, visualization, and analysis now ignore the delayed reset-cleanup death wave before tick 32.
- Retry recording ignores cleanup RELEASE events during the same reset settling window.
- Fixed CSV `progress,valid_ticks` serialization, which previously emitted the numeric value of a C++ multi-character literal.

# 0.1.3

- Recording no longer treats delayed `destroyPlayer` callbacks from level restart as a real attempt death.
- Recording attempt boundaries are now handled by `resetLevel`: a restarted attempt discards the previous attempt's inputs and continues recording.

# 0.1.2

- Added 240 TPS PRESS/RELEASE recording for P1/P2 and all three player buttons.
- Added deterministic Stage A macro playback from level start.
- Added SAFE exhaustive selected/all-input local window analysis with tick horizon validation.
- Added disjoint-window and primary-window result modeling.
- Added progress HUD, cancellation, and CSV/JSON export.
- Added color-coded in-level result replay with a recent-input history overlay.
- Export now opens the mod-specific result folder in Windows Explorer.
- Fixed Record immediately stopping when the pre-recording restart destroyed the previous player.
- Results are now stored in separate level-ID and level-name folders.
- Fixed PauseLayer remaining visible and its Record-button release leaking into gameplay input.
- Recording now arms after reset with a short guard against UI-release and reset-cleanup events.
