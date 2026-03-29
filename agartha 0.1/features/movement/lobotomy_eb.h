#pragma once
#include "movement.hpp"
#include "moveutil.h"

// lobotomy_eb compatibility interface using the superior DNA EdgebugAssist implementation
namespace lobotomy_eb {
    // Inline state variables synced from EdgebugAssistant after each tick
    inline bool EdgeBug_Founded;
    inline std::vector<vec3_t> lbebpos;

    void PrePredictionEdgeBug(c_usercmd* cmd);
    bool EdgeBugCheck(c_usercmd* cmd);
    void ReStorePrediction();
    void EdgeBugPostPredict(c_usercmd* cmd);
    void edgebug_detect(c_usercmd* cmd);
    void edgebug_lock(float& x, float& y);
    void frame_stage(int stage);
}