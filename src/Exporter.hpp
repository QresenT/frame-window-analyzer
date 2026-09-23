#pragma once

#include "AnalysisResult.hpp"
#include <Geode/Geode.hpp>

namespace fwa {
    class Exporter {
    public:
        static std::filesystem::path levelDirectory(std::string_view levelName, int levelID);
        static geode::Result<std::filesystem::path> csv(
            std::vector<AnalysisResult> const& results, std::string_view levelName, int levelID
        );
        static geode::Result<std::filesystem::path> json(
            std::vector<AnalysisResult> const& results, std::string_view levelName, int levelID
        );
    };
}
