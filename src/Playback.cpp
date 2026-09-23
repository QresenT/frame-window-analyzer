#include "Playback.hpp"
#include <algorithm>

namespace fwa {
    void Playback::load(Macro const& macro, std::optional<std::size_t> movedInput, int offset) {
        m_schedule.clear();
        m_schedule.reserve(macro.size());
        for (std::size_t i = 0; i < macro.size(); ++i) {
            auto event = macro.events()[i];
            if (movedInput && i == *movedInput) event.tick += offset;
            m_schedule.emplace_back(i, event);
        }
        std::stable_sort(m_schedule.begin(), m_schedule.end(), [](auto const& a, auto const& b) {
            if (a.second.tick != b.second.tick) return a.second.tick < b.second.tick;
            return a.first < b.first;
        });
        reset();
    }

    void Playback::reset() {
        m_cursor = 0;
    }

    std::vector<InputEvent> Playback::takeAt(std::int64_t tick) {
        std::vector<InputEvent> events;
        while (m_cursor < m_schedule.size() && m_schedule[m_cursor].second.tick <= tick) {
            if (m_schedule[m_cursor].second.tick == tick) events.push_back(m_schedule[m_cursor].second);
            ++m_cursor;
        }
        return events;
    }

    bool Playback::exhausted() const {
        return m_cursor >= m_schedule.size();
    }
}
