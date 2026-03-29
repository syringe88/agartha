#include "lobotomy_eb.h"
#include "../../menu/menu.hpp"
#include "../../menu/config/config.hpp"
#include "../movement/prediction/prediction.hpp"

// Improved edgebug implementation using DNA-SOURCECODE-main EdgebugAssist
// Replaces the inferior lobotomy_eb implementation with superior prediction-based detection

// Internal state for notification, lock and frame_stage features
static vec3_t BackupQAngle{ };
static int sec_LastType = 0;
static vec3_t aim{ };
static int edgebug_prediction_ticks = 0;
static int edgebug_prediction_timestamp = 0;
static int edgebug_mouse_offset = 0;

void lobotomy_eb::PrePredictionEdgeBug(c_usercmd* cmd)
{
    if (!g::local || !g::local->is_alive())
        return;

    // Save the original view angles for frame_stage smooth angle transitions
    BackupQAngle = cmd->view_angles;

    // Delegate backup/restore logic to the superior DNA EdgebugAssist
    EdgebugAssistant.pre(cmd);
}

bool lobotomy_eb::EdgeBugCheck(c_usercmd* cmd)
{
    if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
        return false;

    // Delegate to EdgebugAssist's accurate prediction-based check
    bool brk = false;
    return EdgebugAssistant.check_edge_bug(cmd, brk);
}

void lobotomy_eb::ReStorePrediction()
{
    prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
}

void lobotomy_eb::EdgeBugPostPredict(c_usercmd* cmd)
{
    if (!c::movement::edge_bug || !menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s) || !g::local) {
        EdgeBug_Founded = false;
        return;
    }
    if (!g::local->is_alive())
        return;
    if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip) {
        EdgeBug_Founded = false;
        return;
    }

    // Run the superior DNA edgebug detection and execution
    EdgebugAssistant.run(cmd);

    // Sync state for notification, lock, and frame_stage features
    EdgeBug_Founded = EdgebugAssistant.should_edge_bug;
    lbebpos = EdgebugAssistant.ebpos;

    if (EdgeBug_Founded) {
        // Map DNA state to lobotomy-compatible tracking vars
        edgebug_prediction_ticks = EdgebugAssistant.eblength;
        edgebug_prediction_timestamp = EdgebugAssistant.detecttick;
        edgebug_mouse_offset = std::abs(EdgebugAssistant.mousedx);
        // Determine type: strafing edgebugs get type 2+, non-strafing get type 1
        sec_LastType = EdgebugAssistant.strafing ? 2 : 1;
        aim = cmd->view_angles;
    }
    else {
        sec_LastType = 0;
    }
}

struct DATAFORDETECT {
    vec3_t velocity;
    bool onground;
};

std::deque<DATAFORDETECT> VelocitiesForDetection;

void lobotomy_eb::edgebug_detect(c_usercmd* cmd)
{
    if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
        return;

    if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip)
        return;

    if (const auto fg = g::local->flags(); fg == fl_inwater || fg == fl_waterjump)
        return;

    if (!c::movement::edge_bug || !menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s) || c::movement::edgebug_type == 0)
        return;

    // Notify when the edgebug tick is reached (detection_tick + total_length == current_tick)
    if (EdgeBug_Founded && edgebug_prediction_timestamp + edgebug_prediction_ticks == interfaces::globals->tick_count)
    {
        if (c::movement::edge_bug_detection_printf) {
            interfaces::chat_element->chatprintf("#delusional#_print_edgebugged");
        }
        switch (c::movement::edge_bug_detection_sound) {
        case 0: break;
        case 1: interfaces::surface->play_sound("buttons\\arena_switch_press_02.wav"); break;
        case 2: interfaces::surface->play_sound("buttons\\button22.wav"); break;
        case 3: interfaces::surface->play_sound("survival\\money_collect_01.wav"); break;
        case 4: interfaces::surface->play_sound("Ui\\beep07.wav"); break;
        }
    }
}

void lobotomy_eb::edgebug_lock(float& x, float& y)
{
    if (!g::local || !g::local->is_alive() || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
        return;

    if (const auto fg = g::local->flags(); fg == fl_inwater || fg == fl_waterjump)
        return;

    if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip)
        return;

    if (EdgeBug_Founded) {
        if (x != 0.0) {
            float v3 = (float)(edgebug_prediction_timestamp + edgebug_prediction_ticks - interfaces::globals->tick_count) / x;
            float v4 = 100.0 / (atan2f((float)edgebug_prediction_ticks, v3) + 100.0 + (float)(c::movement::EdgeBugMouseLock / 10 * edgebug_mouse_offset));
            if (!isnan(v4))
                x = x * v4;
        }
    }
}

void lobotomy_eb::frame_stage(int stage)
{
    if (stage == frame_start) {
        static vec3_t aim_to = aim;

        if (EdgeBug_Founded) {
            // Apply smooth angle transition for strafing edgebug types
            if (!c::movement::SiletEdgeBug && sec_LastType > 1 && !c::movement::AutoStrafeEdgeBug) {
                const float smoothingFactor = 0.1f;
                const float maxDeltaAngle = 10.f;

                auto normalizeDelta = [](float delta) -> float {
                    while (delta > 180.f)
                        delta -= 360.f;
                    while (delta < -180.f)
                        delta += 360.f;
                    return delta;
                    };

                float deltaPitch = normalizeDelta(aim.x - aim_to.x);
                if (fabs(deltaPitch) > maxDeltaAngle)
                    deltaPitch = (deltaPitch > 0 ? maxDeltaAngle : -maxDeltaAngle);
                aim_to.x += deltaPitch * smoothingFactor;

                float deltaYaw = normalizeDelta(aim.y - aim_to.y);
                if (fabs(deltaYaw) > maxDeltaAngle)
                    deltaYaw = (deltaYaw > 0 ? maxDeltaAngle : -maxDeltaAngle);
                aim_to.y += deltaYaw * smoothingFactor;

                float deltaRoll = normalizeDelta(aim.z - aim_to.z);
                if (fabs(deltaRoll) > maxDeltaAngle)
                    deltaRoll = (deltaRoll > 0 ? maxDeltaAngle : -maxDeltaAngle);
                aim_to.z += deltaRoll * smoothingFactor;

                aim_to.normalize();
                interfaces::engine->set_view_angles(aim_to);
            }
        }
        else {
            aim_to = BackupQAngle;
        }
    }
}
