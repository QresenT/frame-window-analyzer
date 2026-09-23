#include "Analyzer.hpp"
#include "Exporter.hpp"
#include "ui/ResultOverlay.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>

using namespace geode::prelude;

namespace fwa {
    namespace {
        // resetLevelFromStart emits stale destroyPlayer callbacks through tick
        // 22 on GD 2.2081. Keep input/death handling quiet until that cleanup
        // wave has completed.
        constexpr std::int64_t RESET_SETTLE_TICKS = 32;
    }

    Analyzer& Analyzer::get() {
        static Analyzer instance;
        return instance;
    }

    Macro const& Analyzer::macro() const { return m_recorder.macro(); }
    std::vector<AnalysisResult> const& Analyzer::results() const { return m_results; }
    Mode Analyzer::mode() const { return m_mode; }
    Settings const& Analyzer::settings() const { return m_settings; }
    std::size_t Analyzer::selectedInput() const { return m_selected; }
    std::string Analyzer::statusText() const { return m_status; }
    bool Analyzer::isInjecting() const { return m_injecting; }

    void Analyzer::setSelectedInput(std::size_t index) {
        auto const& macro = m_recorder.macro();
        if (!macro.empty()) m_selected = std::min(index, macro.size() - 1);
    }

    void Analyzer::setSettings(Settings settings) {
        settings.left = std::clamp(settings.left, 0, 120);
        settings.right = std::clamp(settings.right, 0, 120);
        settings.validationTicks = std::clamp(settings.validationTicks, 1, 7200);
        settings.validationEvents = std::clamp(settings.validationEvents, 0, 20);
        m_settings = settings;
    }

    void Analyzer::notify(std::string const& text) const {
        Notification::create(text)->show();
    }

    void Analyzer::startRecording(PlayLayer* layer) {
        if (!layer) return;
        m_mode = Mode::Idle;
        m_recorder.stop();
        m_results.clear();
        m_selected = 0;
        m_layer = layer;
        log::info("Analyzer recorder started; restarting level at TPS {}", TPS);
        layer->resetLevelFromStart();
        if (layer->m_level) {
            m_levelName = layer->m_level->m_levelName.c_str();
            m_levelID = static_cast<int>(layer->m_level->m_levelID);
        }
        m_recorder.begin();
        m_mode = Mode::Recording;
        m_recordingArmTick = static_cast<std::int64_t>(layer->m_tickIndex) + RESET_SETTLE_TICKS;
        m_deathIgnoreUntilTick = m_recordingArmTick;
        m_status = fmt::format("Recording: {}", m_levelName);
        log::debug("Recorder armed at tick {}; ignoring UI release/reset cleanup before then", m_recordingArmTick);
    }

    void Analyzer::stopRecording() {
        if (m_mode != Mode::Recording) return;
        m_mode = Mode::Idle;
        m_recorder.stop();
        m_status = fmt::format("Recorded {} inputs", m_recorder.macro().size());
        log::info("Recorder stopped with {} input events", m_recorder.macro().size());
        notify(m_status);
    }

    void Analyzer::startPlayback(PlayLayer* layer) {
        auto const& macro = m_recorder.macro();
        if (!layer || macro.empty()) {
            notify("No macro recorded");
            return;
        }
        m_layer = layer;
        m_mode = Mode::Playback;
        m_status = "Playback";
        m_playback.load(macro);
        log::info("Playback started with {} events", macro.size());
        layer->resetLevelFromStart();
    }

    bool Analyzer::startVisualization(PlayLayer* layer) {
        auto const& macro = m_recorder.macro();
        if (!layer || macro.empty() || m_results.empty()) {
            notify("Analyze at least one input first");
            return false;
        }
        m_layer = layer;
        m_mode = Mode::Visualizing;
        m_visualCursor = 0;
        m_status = fmt::format("Visualizing {} analyzed input{}", m_results.size(), m_results.size() == 1 ? "" : "s");
        log::info("Result visualization started with {} results", m_results.size());
        layer->resetLevelFromStart();
        return true;
    }

    bool Analyzer::validateSettings() const {
        auto const& macro = m_recorder.macro();
        return !macro.empty() && m_selected < macro.size() && m_settings.left >= 0 &&
            m_settings.right >= 0 && m_settings.validationTicks > 0;
    }

    void Analyzer::beginRun(PlayLayer* layer) {
        m_layer = layer;
        m_mode = Mode::Analyzing;
        m_results.clear();
        m_currentResult = {};
        m_candidateOffset = -m_settings.left * RAW_TICKS_PER_FRAME;
        m_testsComplete = 0;
        m_pendingRestart = false;
        prepareCandidate();
        log::info("Analyzer started: input {}, reference {}, range -{}..+{}, validation {}",
            m_currentTarget, m_recorder.macro().events()[m_currentTarget].tick, m_settings.left,
            m_settings.right, m_settings.runUntilEnd
                ? "until level end"
                : (m_settings.validationEvents > 0
                    ? fmt::format("{} next inputs", m_settings.validationEvents)
                    : fmt::format("{} frames", m_settings.validationTicks)));
        layer->resetLevelFromStart();
    }

    bool Analyzer::analyzeSelected(PlayLayer* layer) {
        if (!layer || !validateSettings()) {
            notify("Record a macro and check analysis settings");
            return false;
        }
        m_targets = {m_selected};
        m_targetCursor = 0;
        m_currentTarget = m_targets.front();
        beginRun(layer);
        return true;
    }

    bool Analyzer::analyzeAll(PlayLayer* layer, InputFilter filter) {
        if (!layer || !validateSettings()) {
            notify("Record a macro and check analysis settings");
            return false;
        }
        m_targets.clear();
        auto const& events = m_recorder.macro().events();
        for (std::size_t index = 0; index < events.size(); ++index) {
            auto const matches = filter == InputFilter::Both ||
                (filter == InputFilter::Presses && events[index].pressed) ||
                (filter == InputFilter::Releases && !events[index].pressed);
            if (matches) m_targets.push_back(index);
        }
        if (m_targets.empty()) {
            notify(filter == InputFilter::Presses ? "No click inputs recorded" : "No release inputs recorded");
            return false;
        }
        m_targetCursor = 0;
        m_currentTarget = m_targets.front();
        beginRun(layer);
        return true;
    }

    void Analyzer::cancel(PlayLayer* layer) {
        if (m_mode == Mode::Idle) return;
        log::info("Analyzer cancelled in mode {}", static_cast<int>(m_mode));
        m_mode = Mode::Idle;
        m_pendingRestart = false;
        m_status = "Cancelled";
        if (layer) layer->resetLevelFromStart();
        notify("Frame Window Analyzer cancelled");
    }

    void Analyzer::onLayerInit(PlayLayer* layer) {
        m_layer = layer;
        m_mode = Mode::Idle;
        m_pendingRestart = false;
        m_status = "Idle";
    }

    void Analyzer::onLayerExit(PlayLayer* layer) {
        if (m_layer != layer) return;
        if (m_mode != Mode::Idle) log::warn("Level exited during analyzer operation; cleaning state");
        m_layer = nullptr;
        m_mode = Mode::Idle;
        m_pendingRestart = false;
        m_status = "Level exited";
    }

    void Analyzer::onReset(PlayLayer* layer) {
        if (layer != m_layer) return;
        auto const resetTick = static_cast<std::int64_t>(layer->m_tickIndex);
        m_deathIgnoreUntilTick = resetTick + RESET_SETTLE_TICKS;
        m_playback.reset();
        if (m_mode == Mode::Recording) {
            auto const discarded = m_recorder.macro().size();
            m_recorder.begin();
            m_selected = 0;
            m_recordingArmTick = m_deathIgnoreUntilTick;
            m_status = fmt::format("Recording: {}", m_levelName);
            if (discarded > 0) {
                log::info("Attempt restarted; discarded {} recorded input events", discarded);
                notify("Attempt restarted: previous inputs discarded");
            }
            return;
        }
        if (m_mode == Mode::Visualizing) {
            m_visualCursor = 0;
            if (auto overlay = typeinfo_cast<ResultOverlay*>(layer->getChildByTag(RESULT_OVERLAY_TAG))) {
                overlay->clearHistory();
            }
        }
        if (m_mode == Mode::Analyzing) {
            prepareCandidate();
            auto tick = m_recorder.macro().events()[m_currentTarget].tick + m_candidateOffset;
            m_status = fmt::format("Target {}/{} (#{} {})  Test {}/{}  Tick {} -> {}",
                m_targetCursor + 1, m_targets.size(), m_currentTarget,
                actionName(m_recorder.macro().events()[m_currentTarget].pressed), m_testsComplete + 1,
                m_settings.left + m_settings.right + 1, tick, m_candidateEndTick);
            log::debug("Restart; testing candidate tick {} through validation tick {}", tick, m_candidateEndTick);
            if (tick < 0) finishCandidate(false, "negative tick");
        }
    }

    void Analyzer::onButton(GJBaseGameLayer* layer, bool down, int button, bool isPlayer1) {
        if (m_injecting || m_mode != Mode::Recording || layer != m_layer) return;
        if (static_cast<std::int64_t>(layer->m_tickIndex) < m_recordingArmTick) {
            log::debug("Ignored pre-arm input at tick {}", layer->m_tickIndex);
            return;
        }
        auto playLayer = static_cast<PlayLayer*>(layer);
        if (!m_recorder.capture(playLayer, down, button, isPlayer1)) return;
        auto const& macro = m_recorder.macro();
        auto const& event = macro.events().back();
        m_selected = macro.size() - 1;
        m_status = fmt::format("Recorded {} inputs", macro.size());
        log::debug("Input #{} P{} {} {} @ tick {}", macro.size() - 1, event.player,
            buttonName(event.button), actionName(event.pressed), event.tick);
    }

    void Analyzer::inject(InputEvent const& event) {
        if (!m_layer) return;
        m_injecting = true;
        m_layer->handleButton(event.pressed, static_cast<int>(event.button), event.player == 1);
        m_injecting = false;
    }

    void Analyzer::beforeProcessCommands(GJBaseGameLayer* layer) {
        if (layer != m_layer || (m_mode != Mode::Playback && m_mode != Mode::Visualizing && m_mode != Mode::Analyzing)) return;
        auto tick = static_cast<std::int64_t>(layer->m_tickIndex);
        if (m_mode != Mode::Visualizing) {
            for (auto const& event : m_playback.takeAt(tick)) inject(event);
            return;
        }

        auto const& events = m_recorder.macro().events();
        auto overlay = typeinfo_cast<ResultOverlay*>(m_layer->getChildByTag(RESULT_OVERLAY_TAG));
        while (m_visualCursor < events.size() && events[m_visualCursor].tick <= tick) {
            if (events[m_visualCursor].tick == tick) {
                auto const index = m_visualCursor;
                auto const& event = events[index];
                inject(event);
                AnalysisResult const* found = nullptr;
                for (auto const& result : m_results) {
                    if (result.inputIndex == index) {
                        found = &result;
                        break;
                    }
                }
                if (overlay) overlay->showInput(index, event, found);
            }
            ++m_visualCursor;
        }
    }

    void Analyzer::afterProcessCommands(GJBaseGameLayer* layer) {
        if (layer != m_layer) return;
        auto tick = static_cast<std::int64_t>(layer->m_tickIndex);
        auto const& macro = m_recorder.macro();
        if ((m_mode == Mode::Playback || m_mode == Mode::Visualizing) && !macro.empty() &&
            tick > macro.events().back().tick + RAW_TPS) {
            auto visualizing = m_mode == Mode::Visualizing;
            m_mode = Mode::Idle;
            m_status = visualizing ? "Result visualization complete" : "Playback complete";
            notify(m_status);
        } else if (m_mode == Mode::Analyzing) {
            if (!m_settings.runUntilEnd && tick >= m_candidateEndTick) {
                finishCandidate(true, "tick/input horizon reached");
            }
        }
    }

    void Analyzer::afterUpdate(PlayLayer* layer) {
        if (layer != m_layer || !m_pendingRestart || m_mode != Mode::Analyzing) return;
        m_pendingRestart = false;
        advanceCandidate();
    }

    void Analyzer::onDeath(PlayLayer* layer, PlayerObject*, GameObject*) {
        if (layer != m_layer) return;
        if (m_mode == Mode::Recording) {
            // A restart can emit delayed destroy callbacks from the previous
            // player. Use resetLevel as the recording attempt boundary instead.
            log::debug("Ignored destroyPlayer while recording at tick {}; waiting for reset boundary", layer->m_tickIndex);
        } else if (static_cast<std::int64_t>(layer->m_tickIndex) < m_deathIgnoreUntilTick) {
            log::debug("Ignored reset-cleanup death in mode {} at tick {} (armed at {})",
                static_cast<int>(m_mode), layer->m_tickIndex, m_deathIgnoreUntilTick);
        } else if (m_mode == Mode::Analyzing) finishCandidate(false, "death");
        else if (m_mode == Mode::Playback || m_mode == Mode::Visualizing) {
            auto visualizing = m_mode == Mode::Visualizing;
            m_mode = Mode::Idle;
            m_status = fmt::format("{} failed @ tick {}", visualizing ? "Visualization" : "Playback", layer->m_tickIndex);
            log::info("{}", m_status);
        }
    }

    bool Analyzer::onLevelComplete(PlayLayer* layer) {
        if (layer != m_layer) return false;
        if (m_mode == Mode::Recording) {
            stopRecording();
            return false;
        }
        if (m_mode == Mode::Visualizing) {
            m_mode = Mode::Idle;
            m_status = "Result visualization complete";
            return false;
        }
        if (m_mode != Mode::Analyzing) return false;
        finishCandidate(true, "level complete");
        return true;
    }

    void Analyzer::prepareCandidate() {
        auto const& macro = m_recorder.macro();
        if (m_mode != Mode::Analyzing || m_currentTarget >= macro.size()) return;
        auto const& events = macro.events();
        m_playback.load(macro, m_currentTarget, m_candidateOffset);
        m_currentResult.inputIndex = m_currentTarget;
        m_currentResult.input = events[m_currentTarget];
        m_candidateEndTick = calculateCandidateEndTick();
    }

    std::int64_t Analyzer::calculateCandidateEndTick() const {
        auto const& events = m_recorder.macro().events();
        auto const& target = events[m_currentTarget];
        auto const candidate = target.tick + m_candidateOffset;
        if (m_settings.validationEvents <= 0) {
            return candidate + m_settings.validationTicks * RAW_TICKS_PER_FRAME;
        }

        // PRESS and RELEASE each count as one subsequent input. Replaying a
        // small fixed number avoids an arbitrary full-level horizon while still
        // proving that the changed target flows into the following actions.
        auto const remaining = events.size() - (m_currentTarget + 1);
        auto const count = std::min<std::size_t>(m_settings.validationEvents, remaining);
        if (count > 0) {
            auto const& lastRequired = events[m_currentTarget + count];
            return lastRequired.tick + RESET_SETTLE_TICKS;
        }
        return candidate + m_settings.validationTicks * RAW_TICKS_PER_FRAME;
    }

    void Analyzer::finishCandidate(bool success, char const* reason) {
        if (m_mode != Mode::Analyzing || m_pendingRestart) return;
        auto tick = m_recorder.macro().events()[m_currentTarget].tick + m_candidateOffset;
        if (success) m_currentResult.validTicks.push_back(tick);
        ++m_testsComplete;
        log::debug("Candidate {} -> {} ({})", tick, success ? "SUCCESS" : "FAIL", reason);
        m_pendingRestart = true;
    }

    void Analyzer::advanceCandidate() {
        if (m_candidateOffset < m_settings.right * RAW_TICKS_PER_FRAME) {
            m_candidateOffset += RAW_TICKS_PER_FRAME;
            m_layer->resetLevelFromStart();
            return;
        }
        finishTarget();
    }

    void Analyzer::finishTarget() {
        m_currentResult.finalize();
        m_results.push_back(m_currentResult);
        log::info("Result input #{}: {} valid ticks, {} windows", m_currentTarget,
            m_currentResult.validTicks.size(), m_currentResult.windows.size());
        ++m_targetCursor;
        if (m_targetCursor >= m_targets.size()) {
            finishAnalysis();
            return;
        }
        m_currentTarget = m_targets[m_targetCursor];
        m_candidateOffset = -m_settings.left * RAW_TICKS_PER_FRAME;
        m_testsComplete = 0;
        m_currentResult = {};
        m_layer->resetLevelFromStart();
    }

    void Analyzer::finishAnalysis() {
        m_mode = Mode::Idle;
        m_pendingRestart = false;
        m_status = fmt::format("Analysis complete: {} input{}", m_results.size(), m_results.size() == 1 ? "" : "s");
        log::info("{}", m_status);
        if (m_layer) m_layer->pauseGame(false);
        notify(m_status);
    }

    Result<std::filesystem::path> Analyzer::exportCSV() const {
        return Exporter::csv(m_results, m_levelName, m_levelID);
    }

    Result<std::filesystem::path> Analyzer::exportJSON() const {
        return Exporter::json(m_results, m_levelName, m_levelID);
    }

    std::filesystem::path Analyzer::exportDirectory() const {
        return Exporter::levelDirectory(m_levelName, m_levelID);
    }
}
