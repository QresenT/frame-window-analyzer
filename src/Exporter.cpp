#include "Exporter.hpp"
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>
#include <sstream>

using namespace geode::prelude;

namespace fwa {
    std::filesystem::path Exporter::levelDirectory(std::string_view levelName, int levelID) {
        std::string safeName = levelName.empty() ? "Unnamed Level" : std::string(levelName);
        for (auto& ch : safeName) {
            if (static_cast<unsigned char>(ch) < 32 || ch == '<' || ch == '>' || ch == ':' ||
                ch == '"' || ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*') {
                ch = '_';
            }
        }
        while (!safeName.empty() && (safeName.back() == ' ' || safeName.back() == '.')) safeName.pop_back();
        if (safeName.empty()) safeName = "Unnamed Level";
        auto folderName = fmt::format("{} - {}", levelID, safeName);
#ifdef GEODE_IS_WINDOWS
        auto component = std::filesystem::path(utils::string::utf8ToWide(folderName));
#else
        auto component = std::filesystem::path(folderName);
#endif
        return Mod::get()->getSaveDir() / "results" / component;
    }

    Result<std::filesystem::path> Exporter::csv(
        std::vector<AnalysisResult> const& results, std::string_view levelName, int levelID
    ) {
        if (results.empty()) return Err("No analysis results to export");
        auto dir = levelDirectory(levelName, levelID);
        GEODE_UNWRAP(utils::file::createDirectoryAll(dir));
        auto path = dir / "frame-windows.csv";
        std::ostringstream out;
        out << "input_index,player,button,type,reference_tick,first_valid,last_valid,window_frames,window_ms,progress,valid_ticks\n";
        for (auto const& result : results) {
            auto first = result.validTicks.empty() ? -1 : result.validTicks.front();
            auto last = result.validTicks.empty() ? -1 : result.validTicks.back();
            auto frames = result.primaryWindow ? result.primaryWindow->frames() : 0;
            out << result.inputIndex << ',' << static_cast<int>(result.input.player) << ','
                << buttonName(result.input.button) << ',' << actionName(result.input.pressed) << ','
                << result.input.tick << ',' << first << ',' << last << ',' << frames << ','
                << fmt::format("{:.4f}", result.primaryMilliseconds()) << ','
                << fmt::format("{:.2f}", result.input.progress) << ",\"";
            for (std::size_t i = 0; i < result.validTicks.size(); ++i) {
                if (i) out << ' ';
                out << result.validTicks[i];
            }
            out << "\"\n";
        }
        GEODE_UNWRAP(utils::file::writeStringSafe(path, out.str()));
        log::info("Exported CSV to {}", path);
        return Ok(path);
    }

    Result<std::filesystem::path> Exporter::json(
        std::vector<AnalysisResult> const& results, std::string_view levelName, int levelID
    ) {
        if (results.empty()) return Err("No analysis results to export");
        auto dir = levelDirectory(levelName, levelID);
        GEODE_UNWRAP(utils::file::createDirectoryAll(dir));
        auto path = dir / "frame-windows.json";
        auto root = matjson::Value::object();
        root["tps"] = TPS;
        root["level_name"] = levelName;
        root["level_id"] = levelID;
        auto rows = matjson::Value::array();
        for (auto const& result : results) {
            auto row = matjson::Value::object();
            row["input_index"] = result.inputIndex;
            row["player"] = result.input.player;
            row["button"] = buttonName(result.input.button);
            row["pressed"] = result.input.pressed;
            row["reference_tick"] = result.input.tick;
            row["progress"] = result.input.progress;
            auto ticks = matjson::Value::array();
            for (auto tick : result.validTicks) ticks.push(tick);
            row["valid_ticks"] = ticks;
            auto windows = matjson::Value::array();
            for (auto const& window : result.windows) {
                auto item = matjson::Value::object();
                item["first"] = window.first;
                item["last"] = window.last;
                item["frames"] = window.frames();
                windows.push(item);
            }
            row["windows"] = windows;
            rows.push(row);
        }
        root["results"] = rows;
        GEODE_UNWRAP(utils::file::writeStringSafe(path, root.dump()));
        log::info("Exported JSON to {}", path);
        return Ok(path);
    }
}
