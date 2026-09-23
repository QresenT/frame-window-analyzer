#pragma once

#include "Macro.hpp"

class GJBaseGameLayer;
class PlayLayer;

namespace fwa {
    class Recorder {
    public:
        void begin();
        void stop();
        bool active() const;
        bool capture(PlayLayer* layer, bool down, int button, bool isPlayer1);
        Macro const& macro() const;

    private:
        Macro m_macro;
        bool m_active = false;
    };
}
