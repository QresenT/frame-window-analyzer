#include "Recorder.hpp"
#include <Geode/Geode.hpp>

namespace fwa {
    void Recorder::begin() {
        m_macro.clear();
        m_active = true;
    }

    void Recorder::stop() {
        m_active = false;
    }

    bool Recorder::active() const {
        return m_active;
    }

    Macro const& Recorder::macro() const {
        return m_macro;
    }

    bool Recorder::capture(PlayLayer* layer, bool down, int button, bool isPlayer1) {
        if (!m_active || !layer || button < static_cast<int>(PlayerButton::Jump) ||
            button > static_cast<int>(PlayerButton::Right)) return false;
        InputEvent event {
            layer->m_tickIndex,
            static_cast<std::uint8_t>(isPlayer1 ? 1 : 2),
            static_cast<PlayerButton>(button),
            down,
            layer->getCurrentPercent(),
        };
        if (!m_macro.empty() && event.tick < m_macro.events().back().tick) {
            m_active = false;
            return false;
        }
        m_macro.add(event);
        return true;
    }
}
