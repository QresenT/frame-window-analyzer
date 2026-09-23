#include "Macro.hpp"

namespace fwa {
    void Macro::clear() {
        m_events.clear();
    }

    void Macro::add(InputEvent event) {
        m_events.push_back(event);
    }

    std::vector<InputEvent> const& Macro::events() const {
        return m_events;
    }

    bool Macro::empty() const {
        return m_events.empty();
    }

    std::size_t Macro::size() const {
        return m_events.size();
    }

    char const* buttonName(PlayerButton button) {
        switch (button) {
            case PlayerButton::Jump: return "JUMP";
            case PlayerButton::Left: return "LEFT";
            case PlayerButton::Right: return "RIGHT";
        }
        return "UNKNOWN";
    }

    char const* actionName(bool pressed) {
        return pressed ? "PRESS" : "RELEASE";
    }
}
