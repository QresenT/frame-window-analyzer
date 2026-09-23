#pragma once

#include "AnalysisResult.hpp"
#include "Playback.hpp"
#include "Recorder.hpp"
#include <string>

class GJBaseGameLayer;
class PlayLayer;
class PlayerObject;
class GameObject;

namespace fwa {
    enum class Mode {
        Idle,
        Recording,
        Playback,
        Visualizing,
        Analyzing,
    };

    enum class InputFilter {
        Presses,
        Releases,
        Both,
    };

    struct Settings {
        int left = 5;
        int right = 5;
        int validationTicks = 240;
        int validationEvents = 3;
        bool runUntilEnd = false;
    };

    class Analyzer {
    public:
        static Analyzer& get();

        Macro const& macro() const;
        std::vector<AnalysisResult> const& results() const;
        Mode mode() const;
        Settings const& settings() const;
        std::size_t selectedInput() const;
        std::string statusText() const;
        bool isInjecting() const;

        void setSelectedInput(std::size_t index);
        void setSettings(Settings settings);
        void startRecording(PlayLayer* layer);
        void stopRecording();
        void startPlayback(PlayLayer* layer);
        bool startVisualization(PlayLayer* layer);
        bool analyzeSelected(PlayLayer* layer);
        bool analyzeAll(PlayLayer* layer, InputFilter filter);
        void cancel(PlayLayer* layer);
        geode::Result<std::filesystem::path> exportCSV() const;
        geode::Result<std::filesystem::path> exportJSON() const;
        std::filesystem::path exportDirectory() const;

        void onLayerInit(PlayLayer* layer);
        void onLayerExit(PlayLayer* layer);
        void onReset(PlayLayer* layer);
        void onButton(GJBaseGameLayer* layer, bool down, int button, bool isPlayer1);
        void beforeProcessCommands(GJBaseGameLayer* layer);
        void afterProcessCommands(GJBaseGameLayer* layer);
        void afterUpdate(PlayLayer* layer);
        void onDeath(PlayLayer* layer, PlayerObject* player, GameObject* object);
        bool onLevelComplete(PlayLayer* layer);

    private:
        Recorder m_recorder;
        Playback m_playback;
        std::vector<AnalysisResult> m_results;
        Settings m_settings;
        Mode m_mode = Mode::Idle;
        PlayLayer* m_layer = nullptr;
        std::size_t m_selected = 0;
        std::size_t m_visualCursor = 0;
        std::size_t m_currentTarget = 0;
        std::size_t m_targetCursor = 0;
        std::vector<std::size_t> m_targets;
        int m_candidateOffset = 0;
        int m_testsComplete = 0;
        std::int64_t m_recordingArmTick = 0;
        std::int64_t m_deathIgnoreUntilTick = 0;
        std::int64_t m_candidateEndTick = 0;
        bool m_injecting = false;
        bool m_pendingRestart = false;
        AnalysisResult m_currentResult;
        std::string m_status = "Idle";
        std::string m_levelName = "Unnamed Level";
        int m_levelID = 0;

        bool validateSettings() const;
        void beginRun(PlayLayer* layer);
        void prepareCandidate();
        std::int64_t calculateCandidateEndTick() const;
        void finishCandidate(bool success, char const* reason);
        void advanceCandidate();
        void finishTarget();
        void finishAnalysis();
        void inject(InputEvent const& event);
        void notify(std::string const& text) const;
    };
}
