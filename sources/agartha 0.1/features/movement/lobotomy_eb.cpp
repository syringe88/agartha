#include "lobotomy_eb.h"
#include "../../menu/menu.hpp"
#include "../../menu/config/config.hpp"
#include "../movement/prediction/prediction.hpp"

// DNA-SOURCECODE-main superior edgebug implementation
// Replaces the original Lobotomy V2 edgebug logic

// Internal state (mirrors DNA's EdgebugAssist class members)
static vec3_t dna_backup_origin;
static vec3_t dna_backup_velocity;
static int dna_flags_backup;

static bool dna_crouched = false;
static bool dna_strafing = false;
static int dna_ticks_left = 0;
static int dna_eblength = 0;
static int dna_edgebugtick = 0;
static int dna_detecttick = 0;
static float dna_forwardmove = 0.f;
static float dna_sidemove = 0.f;
static float dna_upmove = 0.f;
static float dna_yawdelta = 0.f;
static float dna_yaw = 0.f;
static float dna_startingyaw = 0.f;
static short dna_mousedx = 0;
static bool dna_detected_normal_eb = false;

enum dna_ducktype_t { dna_standing, dna_crouching };
static dna_ducktype_t dna_ducktype = dna_standing;

// Adjust movement vectors when view angle changes (DNA's handle_fix equivalent)
static void dna_correct_movement(c_usercmd* cmd, vec3_t wish_angle) {
    if (!g::local)
        return;

    vec3_t move, dir, move_angle;
    float delta, len;

    if (!(g::local->flags() & fl_onground) && cmd->view_angles.z != 0 && cmd->buttons & in_attack)
        cmd->side_move = 0;

    move = { cmd->forward_move, cmd->side_move, 0 };
    len = move.normalize_movement();
    if (!len)
        return;

    math::vectorangles(move, move_angle);
    delta = cmd->view_angles.y - wish_angle.y;
    move_angle.y += delta;

    math::angle_vectors(move_angle, &dir);
    dir *= len;

    cmd->forward_move = std::clamp(dir.x, -450.f, 450.f);
    cmd->side_move = std::clamp(dir.y, -450.f, 450.f);
}

// DNA's superior edge bug detection check
static bool dna_check_edge_bug(c_usercmd* cmd, bool& brk) {
    if (!g::local)
        return false;

    if (const auto mt = g::local->move_type(); mt == movetype_ladder || mt == movetype_noclip) {
        brk = true;
        return false;
    }

    vec3_t unpredicted_velocity = prediction_backup::velocity;
    vec3_t predicted_velocity = g::local->velocity();
    vec3_t unpredicted_origin = prediction_backup::origin;
    vec3_t predicted_origin = g::local->origin();
    int predicted_flags = g::local->flags();

    static convar* sv_gravity_cvar = interfaces::console->get_convar("sv_gravity");
    float sv_gravity = sv_gravity_cvar->get_float();
    float fTickInterval = interfaces::globals->interval_per_tick;
    float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
    float gravityvelo = ((sv_gravity / 2) / fTickRate) * -1.f;

    float z_vel_pred = round(predicted_velocity.z);
    float l2d_vel_pred = round(g::local->velocity().length_2d());

    // No point edgebugging with 0 velocity or while on ground
    if (z_vel_pred >= 0.f || (predicted_flags & fl_onground) || l2d_vel_pred == 0.f) {
        brk = true;
        return false;
    }
    else if (unpredicted_velocity.z < 0.f && predicted_velocity.z > unpredicted_velocity.z && predicted_velocity.z < 0.f) {
        // Skip if we can't reach the edge by our current origin
        if (unpredicted_origin.z < predicted_origin.z)
            return false;

        int z_vel = (int)predicted_velocity.z;

        prediction::begin(cmd);
        prediction::end();

        float rounded_vel = round(-sv_gravity * interfaces::globals->interval_per_tick) + z_vel;
        float unpredicted_vel = round(g::local->velocity().z);

        if (rounded_vel == unpredicted_vel || (unpredicted_vel == 0.f && (g::local->flags() & fl_onground))) {
            return true;
        }
        else {
            brk = true;
            return false;
        }
    }
    else if (unpredicted_velocity.z < gravityvelo && round(g::local->velocity().z) == round(gravityvelo) && g::local->move_type() != movetype_ladder) {
        return true;
    }

    return false;
}

void lobotomy_eb::PrePredictionEdgeBug(c_usercmd* cmd) {
    if (!g::local || !g::local->is_alive())
        return;

    // Save state before prediction (DNA's handle_pre_edgebug)
    prediction_backup::origin = g::local->origin();
    prediction_backup::velocity = g::local->velocity();
    prediction_backup::flags = g::local->flags();

    if (dna_ticks_left && dna_crouched) {
        g::local->velocity() = dna_backup_velocity;
        cmd->buttons |= in_duck;
    }

    if (dna_ticks_left && dna_strafing) {
        cmd->forward_move = dna_forwardmove;
        cmd->side_move = dna_sidemove;
    }
}

bool lobotomy_eb::EdgeBugCheck(c_usercmd* cmd) {
    bool brk = false;
    return dna_check_edge_bug(cmd, brk);
}

void lobotomy_eb::ReStorePrediction() {
    prediction::restore_ent_to_predicted_frame(interfaces::prediction->split->commands_predicted - 1);
}

void lobotomy_eb::EdgeBugPostPredict(c_usercmd* cmd) {
    // DNA's superior handle_edgebug logic
    if (!c::movement::edge_bug || !menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s) || !g::local) {
        dna_detecttick = 0;
        dna_edgebugtick = 0;
        dna_ticks_left = 0;
        EdgeBug_Founded = false;
        features::movement::should_edge_bug = false;
        return;
    }

    const int move_type = g::local->move_type();
    if ((prediction_backup::flags & fl_onground) || move_type != movetype_walk ||
        move_type == movetype_noclip || move_type == movetype_observer || move_type == movetype_ladder) {
        dna_detecttick = 0;
        dna_edgebugtick = 0;
        dna_ticks_left = 0;
        EdgeBug_Founded = false;
        features::movement::should_edge_bug = false;
        return;
    }

    vec3_t originalangles = cmd->view_angles;
    float originalfmove = cmd->forward_move;
    float originalsmove = cmd->side_move;
    float originalupmove = cmd->up_move;
    vec3_t originalvelocity = g::local->velocity();

    if (!dna_ticks_left) {
        convar* get_yaw = interfaces::console->get_convar("m_yaw");
        convar* get_sens = interfaces::console->get_convar("sensitivity");
        float m_yaw = get_yaw->get_float();
        float sensitivity = get_sens->get_float();
        float ydelta = std::clamp(cmd->mouse_dx * m_yaw * sensitivity, -30.f, 30.f);

        int ticklimit = time_to_ticks(c::movehelp::edgebug_scan);
        int pred_rounds = (c::movehelp::advanced_edgebug && ydelta != 0.f) ? 4 : 2;

        dna_startingyaw = originalangles.y;
        dna_yawdelta = 0.f;

        vec3_t originalvel = prediction_backup::velocity;
        int cmd_pred = interfaces::prediction->split->commands_predicted - 1;

        lbebpos.clear();
        lbebpos.push_back(g::local->origin());

        const float velocity_threshold = 0.f;

        for (int round = 0; round < pred_rounds; round++) {
            if (round > 1) {
                // Advanced strafe-based search (DNA's rounds 2 and 3)
                float max_delta = ydelta;
                int divisor = (c::movehelp::advanced_edgebug && c::movement::advanced_radius > 0) ? c::movement::advanced_radius : 2;

                for (float cur_delta = max_delta / divisor; abs(cur_delta) <= abs(max_delta); cur_delta += max_delta / divisor) {
                    c_usercmd predictcmd = *cmd;

                    if (round == 2) {
                        dna_crouched = true;
                        predictcmd.buttons |= in_duck;
                        dna_ducktype = dna_crouching;
                        dna_strafing = true;
                        predictcmd.view_angles.y = originalangles.y;
                        predictcmd.forward_move = originalfmove;
                        predictcmd.side_move = predictcmd.view_angles.y > 0 ? originalsmove * 450.f : originalsmove * -450.f;
                        predictcmd.up_move = originalupmove;
                    }
                    else if (round == 3) {
                        dna_crouched = false;
                        predictcmd.buttons &= ~in_duck;
                        dna_ducktype = dna_standing;
                        dna_strafing = true;
                        predictcmd.view_angles.y = originalangles.y;
                        predictcmd.forward_move = originalfmove;
                        predictcmd.side_move = originalsmove * 450.f;
                        predictcmd.up_move = originalupmove;
                    }

                    prediction::restore_ent_to_predicted_frame(cmd_pred);

                    for (int t = 1; t <= ticklimit; t++) {
                        predictcmd.view_angles.y = math::normalize_yaw(originalangles.y + (cur_delta * t));

                        if (c::movement::advanced_radius == 0 || c::movement::edge_bug_angle_limit == 0.f)
                            break;

                        if (c::movement::enable_angle_limit && abs(predictcmd.view_angles.y - dna_startingyaw) > c::movement::edge_bug_angle_limit)
                            break;

                        prediction::begin(&predictcmd);
                        prediction::end();

                        if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold ||
                            prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
                            g::local->move_type() == movetype_ladder)
                            break;

                        lbebpos.push_back(g::local->origin());

                        bool br = false;
                        if (dna_check_edge_bug(&predictcmd, br)) {
                            dna_ticks_left = t;
                            dna_eblength = t;
                            dna_edgebugtick = interfaces::globals->tick_count + t;
                            dna_detecttick = interfaces::globals->tick_count;
                            dna_forwardmove = predictcmd.forward_move;
                            dna_sidemove = predictcmd.side_move;
                            dna_upmove = predictcmd.up_move;
                            dna_yawdelta = cur_delta;
                            dna_yaw = predictcmd.view_angles.y;
                            dna_backup_velocity = originalvel;
                            dna_backup_velocity.z = 0;
                            dna_mousedx = cmd->mouse_dx;
                            break;
                        }

                        if (br)
                            break;
                    }

                    if (dna_ticks_left)
                        break;
                }
            }
            else {
                // Base search: ducked (round 0) and standing (round 1)
                c_usercmd predictcmd = *cmd;

                if (round == 0) {
                    dna_crouched = true;
                    predictcmd.buttons |= in_duck;
                    dna_ducktype = dna_crouching;
                    dna_strafing = false;
                    predictcmd.forward_move = 0.f;
                    predictcmd.side_move = 0.f;
                    predictcmd.up_move = 0.f;
                }
                else if (round == 1) {
                    dna_crouched = false;
                    predictcmd.buttons &= ~in_duck;
                    dna_ducktype = dna_standing;
                    dna_strafing = false;
                    predictcmd.forward_move = 0.f;
                    predictcmd.side_move = 0.f;
                    predictcmd.up_move = 0.f;
                }

                prediction::restore_ent_to_predicted_frame(cmd_pred);

                for (int t = 1; t <= ticklimit; ++t) {
                    prediction::begin(&predictcmd);
                    prediction::end();

                    if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold ||
                        prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
                        g::local->move_type() == movetype_ladder)
                        break;

                    lbebpos.push_back(g::local->origin());

                    bool br = false;
                    if (dna_check_edge_bug(&predictcmd, br)) {
                        dna_ticks_left = t;
                        dna_eblength = t;
                        dna_edgebugtick = interfaces::globals->tick_count + t;
                        dna_detecttick = interfaces::globals->tick_count;
                        dna_forwardmove = predictcmd.forward_move;
                        dna_sidemove = predictcmd.side_move;
                        dna_upmove = predictcmd.up_move;
                        dna_yawdelta = ydelta;
                        dna_backup_velocity = originalvel;
                        dna_backup_velocity.z = 0;
                        dna_mousedx = cmd->mouse_dx;
                        break;
                    }

                    if (br)
                        break;
                }
            }

            if (dna_ticks_left)
                break;
        }
    }

    if (dna_ticks_left) {
        if (dna_ticks_left == 1) {
            if (c::movement::edge_bug_detection_printf)
                interfaces::chat_element->chatprintf("#agartha#_print_edgebugged");
            dna_detected_normal_eb = true;
        }

        EdgeBug_Founded = true;
        features::movement::should_edge_bug = true;

        if (dna_crouched) {
            cmd->buttons |= in_bullrush;
            cmd->buttons |= in_duck;
        }
        else {
            cmd->buttons &= ~in_duck;
        }

        if (dna_strafing) {
            cmd->forward_move = dna_forwardmove;
            cmd->side_move = dna_sidemove;
            cmd->mouse_dx = dna_mousedx;
            vec3_t ang = vec3_t(
                originalangles.x + 18.f,
                math::normalize_yaw(dna_startingyaw + (dna_yawdelta * (dna_eblength - (dna_ticks_left - 1)))),
                0.f
            );
            cmd->view_angles.y = ang.y;
            dna_correct_movement(cmd, ang);
        }
        else {
            cmd->forward_move = 0.f;
            cmd->side_move = 0.f;
            dna_correct_movement(cmd, originalangles);
        }

        dna_ticks_left--;
    }
    else {
        EdgeBug_Founded = false;
        features::movement::should_edge_bug = false;
        dna_detected_normal_eb = false;
    }
}

void lobotomy_eb::edgebug_detect(c_usercmd* cmd) {
    // Detection is handled inside EdgeBugPostPredict (DNA architecture)
    // This stub maintains API compatibility with hooks
}

void lobotomy_eb::edgebug_lock(float& x, float& y) {
    // DNA implementation does not use a separate mouse lock step
    // Mouse delta is managed directly in EdgeBugPostPredict via cmd->mouse_dx
}

void lobotomy_eb::frame_stage(int stage) {
    // DNA implementation does not require per-frame-stage processing
}
