#pragma once
// Extracted from DNA-SOURCECODE-main/features/movement/moveutil.h
// Contains the superior EdgebugAssist implementation

#include "movement.hpp"
#include "../../menu/menu.hpp"
#include "../../menu/config/config.hpp"
#include "../movement/prediction/prediction.hpp"

// Minimal command recording struct (used by EdgebugAssist)
struct NewCmd3 {
	vec3_t viewangle;
	float forwardmove;
	float sideMove;
	int buttons;
	vec3_t origin;
};
inline NewCmd3 NewCmd_assist[128];

// Minimal UtilityAssist providing movement fix/correction
// (needed by EdgebugAssist::handle_edgebug)
class UtilityAssist {
public:
	void handle_fix(c_usercmd* cmd, vec3_t& angle) {
		if (!g::local)
			return;

		vec3_t move, dir;
		vec3_t move_angle;
		float delta, len;

		if (!(g::local->flags() & fl_onground) && cmd->view_angles.z != 0 && cmd->buttons & in_attack)
			cmd->side_move = 0;

		move = { cmd->forward_move, cmd->side_move, 0 };

		len = move.normalize_movement();

		if (!len)
			return;

		math::vectorangles(move, move_angle);

		delta = (cmd->view_angles.y - angle.y);

		move_angle.y += delta;

		math::angle_vectors(move_angle, &dir);

		dir *= len;

		if (g::local->move_type() == movetype_ladder) {
			if (cmd->view_angles.x >= 45 && angle.x < 45 && std::abs(delta) <= 65)
				dir.x = -dir.x;

			cmd->forward_move = dir.x;
			cmd->side_move = dir.y;

			if (cmd->forward_move > 200)
				cmd->buttons |= in_forward;

			else if (cmd->forward_move < -200)
				cmd->buttons |= in_back;

			if (cmd->side_move > 200)
				cmd->buttons |= in_moveright;

			else if (cmd->side_move < -200)
				cmd->buttons |= in_moveleft;
		}
		else {
			if (cmd->view_angles.x < -90 || cmd->view_angles.x > 90)
				dir.x = -dir.x;

			cmd->forward_move = dir.x;
			cmd->side_move = dir.y;
		}

		cmd->forward_move = std::clamp(cmd->forward_move, -450.f, 450.f);
		cmd->side_move = std::clamp(cmd->side_move, -450.f, 450.f);
		cmd->up_move = std::clamp(cmd->up_move, -320.f, 320.f);

		if (g::local->move_type() != movetype_ladder)
			cmd->buttons &= ~(in_forward | in_back | in_moveright | in_moveleft);
	}
};
inline UtilityAssist UtilityAssistant;

// Superior EdgebugAssist implementation from DNA-SOURCECODE-main
// Replaces the inferior lobotomy_eb detection/execution
class EdgebugAssist {
public:

	std::vector<vec3_t> ebpos;
	vec3_t backup_origin = vec3_t(0, 0, 0);
	vec3_t backup_velocity = vec3_t(0, 0, 0);
	vec3_t origin = vec3_t(0, 0, 0);
	vec3_t angles = vec3_t(0, 0, 0);
	bool should_predict = false;
	bool should_edge_bug = false;
	bool detected_normal_edge_bug = false;
	bool crouched = false;
	bool is_double_eb = false;
	int detecttick = 0;
	int edgebugtick = 0;
	short mousedx;
	bool strafing = false;
	float yawdelta = 0.f;
	float yaw = 0.f;
	float forwardmove, upmove, sidemove = 0.f;
	float startingyaw = 0.f;
	int eblength = 0;
	int ticks_left = 0;
	int did_edgebug = 0;
	int did_edgebug_strafe = 0;
	int buttons = 0;
	int second_eb_tick = 0;
	float highest_ground = 0.f;
	float saved_velocity_z = 0.f;
	float saved_origin_z = 0.f;
	vec3_t velocity_backup;
	vec3_t old_viewangles;
	int flags_backup;

	enum ducktype_t {
		standing,
		crouching
	};
	ducktype_t ducktype;

public:

	bool check_edge_bug(c_usercmd* cmd, bool& brk) {
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

		static convar* Sv_gravity = interfaces::console->get_convar(("sv_gravity"));
		auto sv_gravity = Sv_gravity->get_float();
		float fTickInterval = interfaces::globals->interval_per_tick;
		float fTickRate = (fTickInterval > 0) ? (1.0f / fTickInterval) : 0.0f;
		float gravityvelo = ((sv_gravity / 2) / fTickRate) * -1.f;

		float z_vel_pred = round(predicted_velocity.z);
		float l2d_vel_pred = round(g::local->velocity().length_2d());

		//there is no point in edgebugging while having 0 velocity
		if (z_vel_pred >= 0.f || (predicted_flags & fl_onground) || l2d_vel_pred == 0.f) {
			brk = true;
			return false;
		}
		else if (unpredicted_velocity.z < 0.f && predicted_velocity.z > unpredicted_velocity.z && predicted_velocity.z < 0.f) {
			//we dont need to calc edgebugs which we cant hit by player origin
			if (unpredicted_origin.z < predicted_origin.z) {
				return false;
			}

			int z_vel = predicted_velocity.z;

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

private:
	void handle_pre_edgebug(c_usercmd* cmd) {
		prediction_backup::origin = g::local->origin();
		prediction_backup::velocity = g::local->velocity();
		prediction_backup::flags = g::local->flags();

		if (this->should_edge_bug && this->crouched && this->ticks_left) {
			g::local->velocity() = this->backup_velocity;
			cmd->buttons |= in_duck;
		}

		if (this->should_edge_bug && this->ticks_left && this->strafing) {
			cmd->forward_move = this->forwardmove;
			cmd->side_move = this->sidemove;
		}
	}

	void handle_edgebug(c_usercmd* cmd) {
		if (!c::movement::edge_bug || !menu::checkkey(c::movement::edge_bug_key, c::movement::edge_bug_key_s) || !g::local) {
			this->detecttick = 0;
			this->edgebugtick = 0;
			this->ticks_left = 0;
			this->should_edge_bug = false;
			features::movement::should_edge_bug = false;
			return;
		}

		const int move_type = g::local->move_type();
		if ((prediction_backup::flags & fl_onground) || move_type != movetype_walk || move_type == movetype_noclip || move_type == movetype_observer || move_type == movetype_ladder) {
			this->detecttick = 0;
			this->edgebugtick = 0;
			this->ticks_left = 0;
			this->should_edge_bug = false;
			features::movement::should_edge_bug = false;
			return;
		}

		features::movement::detect_data.ticks_left = this->ticks_left;

		vec3_t originalangles = cmd->view_angles;
		float originalfmove = cmd->forward_move;
		float originalsmove = cmd->side_move;
		float originalupmove = cmd->up_move;
		vec3_t originalvelocity = g::local->velocity();

		if (!this->ticks_left) {
			convar* get_yaw = interfaces::console->get_convar("m_yaw");
			convar* get_sens = interfaces::console->get_convar("sensitivity");
			const float m_yaw = get_yaw->get_float();
			const float sensitivity = get_sens->get_float();
			float yawdelta = std::clamp(cmd->mouse_dx * m_yaw * sensitivity, -30.f, 30.f);
			int ticklimit = time_to_ticks(c::movehelp::edgebug_scan);
			int pred_rounds = (c::movehelp::advanced_edgebug && yawdelta != 0.f) ? 4 : 2;
			this->startingyaw = originalangles.y;
			this->highest_ground = 0.f;
			std::vector<vec3_t> positions;

			vec3_t originalpos = prediction_backup::origin;
			vec3_t originalvel = prediction_backup::velocity;
			int originalflags = prediction_backup::flags;
			int cmd_pred = interfaces::prediction->split->commands_predicted - 1;

			ebpos.clear();
			ebpos.push_back(g::local->origin());

			const float max_radias = m_pi * 2.f;
			const float step = max_radias / 16.f;

			const float velocity_threshold = 0.f;
			const float ground_distance_threshold = 2.f;
			int consecutive_ground_checks = 0;
			bool potential_double_eb = false;
			int first_eb_tick = 0;

			for (int round = 0; round < pred_rounds; round++) {
				if (round > 1) {
					float max_delta = yawdelta;
					int divisor = c::movehelp::advanced_edgebug ? c::movement::advanced_radius : 2;
					for (yawdelta = max_delta / divisor; abs(yawdelta) <= abs(max_delta); yawdelta += max_delta / divisor) {

						c_usercmd predictcmd = *cmd;
						if (round == 2) {
							this->crouched = true;
							predictcmd.buttons |= in_duck;
							ducktype = crouching;
							this->strafing = true;
							predictcmd.view_angles.y = originalangles.y;
							predictcmd.forward_move = originalfmove;
							predictcmd.side_move = predictcmd.view_angles.y > 0 ? originalsmove * 450.f : originalsmove * -450.f;
							predictcmd.up_move = originalupmove;
						}
						else if (round == 3) {
							this->crouched = false;
							predictcmd.buttons &= ~in_duck;
							ducktype = standing;
							this->strafing = true;
							predictcmd.view_angles.y = originalangles.y;
							predictcmd.forward_move = originalfmove;
							predictcmd.side_move = originalsmove * 450.f;
							predictcmd.up_move = originalupmove;
						}

						prediction::restore_ent_to_predicted_frame(cmd_pred);

						for (int t = 1; t <= ticklimit; t++) {
							if (round > 1)
								predictcmd.view_angles.y = math::normalize_yaw(originalangles.y + (yawdelta * t));

							if (c::movement::advanced_radius == 0 || c::movement::edge_bug_angle_limit == 0)
								break;

							if (abs(predictcmd.view_angles.y - this->startingyaw) > c::movement::enable_angle_limit ? c::movement::edge_bug_angle_limit : 1)
								break;

							prediction::begin(&predictcmd);
							prediction::end();

							vec3_t current_vel = g::local->velocity();
							vec3_t current_pos = g::local->origin();

							if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold || prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
								g::local->move_type() == movetype_ladder)
								break;

							ebpos.push_back(current_pos);

							bool br = false;

							if (this->check_edge_bug(&predictcmd, br)) {
								this->ticks_left = t;
								this->eblength = t;
								this->edgebugtick = interfaces::globals->tick_count + t;
								this->detecttick = interfaces::globals->tick_count;
								this->forwardmove = predictcmd.forward_move;
								this->sidemove = predictcmd.side_move;
								this->upmove = predictcmd.up_move;
								this->yawdelta = yawdelta;
								this->yaw = predictcmd.view_angles.y;
								this->backup_velocity = originalvel;
								this->backup_velocity.z = 0;
								this->mousedx = cmd->mouse_dx;
								break;
							}

							ebpos.push_back(current_pos);

							if (br)
								break;
						}
					}
				}
				else {
					c_usercmd predictcmd = *cmd;
					if (round == 0) {
						this->crouched = true;
						predictcmd.buttons |= in_duck;
						ducktype = crouching;
						this->strafing = false;
						predictcmd.forward_move = 0.f;
						predictcmd.side_move = 0.f;
						predictcmd.up_move = 0.f;
					}
					else if (round == 1) {
						this->crouched = false;
						predictcmd.buttons &= ~in_duck;
						ducktype = standing;
						this->strafing = false;
						predictcmd.forward_move = 0.f;
						predictcmd.side_move = 0.f;
						predictcmd.up_move = 0.f;
					}

					prediction::restore_ent_to_predicted_frame(cmd_pred);

					for (int t = 1; t <= ticklimit; ++t) {

						prediction::begin(&predictcmd);
						prediction::end();

						vec3_t current_vel = g::local->velocity();
						vec3_t current_pos = g::local->origin();

						if (originalvelocity.z > 0.f || originalvelocity.length_2d() < velocity_threshold || prediction_backup::velocity.z > 0.f || prediction_backup::velocity.length_2d() < velocity_threshold ||
							g::local->move_type() == movetype_ladder)
							break;

						ebpos.push_back(current_pos);

						bool br = false;

						if (this->check_edge_bug(&predictcmd, br)) {
							this->ticks_left = t;
							this->eblength = t;
							this->edgebugtick = interfaces::globals->tick_count + t;
							this->detecttick = interfaces::globals->tick_count;
							this->forwardmove = predictcmd.forward_move;
							this->sidemove = predictcmd.side_move;
							this->upmove = predictcmd.up_move;
							this->yawdelta = yawdelta;
							this->backup_velocity = originalvel;
							this->backup_velocity.z = 0;
							this->mousedx = cmd->mouse_dx;
							break;
						}

						ebpos.push_back(current_pos);

						if (br)
							break;
					}
				}

				if (this->ticks_left)
					break;
			}
		}

		if (this->ticks_left) {
			if (this->ticks_left == 1) {
				if (c::movement::edge_bug_detection_printf) {
					interfaces::chat_element->chatprintf("#dna#_print_edgebugged");
				}
				detected_normal_edge_bug = true;
			}

			this->should_edge_bug = true;
			features::movement::should_edge_bug = true;

			if (this->crouched) {
				cmd->buttons |= in_bullrush;
				cmd->buttons |= in_duck;
			}
			else {
				cmd->buttons &= ~in_duck;
			}

			if (this->strafing) {
				cmd->forward_move = this->forwardmove;
				cmd->side_move = this->sidemove;
				cmd->mouse_dx = this->mousedx;
				vec3_t ang = vec3_t(originalangles.x + 18.f, math::normalize_yaw(startingyaw + (yawdelta * (eblength - (ticks_left - 1)))), 0);
				cmd->view_angles.y = ang.y;
				UtilityAssistant.handle_fix(cmd, ang);
			}
			else {
				cmd->forward_move = 0.f;
				cmd->side_move = 0.f;
				UtilityAssistant.handle_fix(cmd, originalangles);
			}

			this->ticks_left--;
		}
		else {
			should_edge_bug = false;
			features::movement::should_edge_bug = false;
			detected_normal_edge_bug = false;
			features::movement::detected_normal_edge_bug = false;
			this->is_double_eb = false;
			this->second_eb_tick = 0;
		}
	}

	void handle_visualize() {
		if (!g::local || !g::local->is_alive())
			return;

		if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
			return;

		static float alpha = 0.0f;

		float target_alpha = (ticks_left > 0) ? 1.0f : 0.0f;
		float lerp_speed = 8.0f;

		float frame_time = interfaces::globals->frame_time;
		alpha = alpha + (target_alpha - alpha) * lerp_speed * frame_time;

		if (c::movement::visualize_edge_bug && alpha > 0.0f) {

			for (int i = 0; i < (int)ebpos.size() - 1; i++) {
				vec3_t cur, next;
				if (interfaces::debug_overlay->world_to_screen(ebpos[i], cur) &&
					interfaces::debug_overlay->world_to_screen(ebpos[i + 1], next)) {

					im_render.drawline(
						cur.x, cur.y, next.x, next.y,
						color_t(
							c::movement::visualize_edge_bug_clr[0],
							c::movement::visualize_edge_bug_clr[1],
							c::movement::visualize_edge_bug_clr[2],
							alpha
						)
					);
				}
			}

			if (!ebpos.empty()) {
				vec3_t endpoints[4];
				if (interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x - 16.f, ebpos.back().y - 16.f, ebpos.back().z }, endpoints[0]) &&
					interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x - 16.f, ebpos.back().y + 16.f, ebpos.back().z }, endpoints[1]) &&
					interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x + 16.f, ebpos.back().y - 16.f, ebpos.back().z }, endpoints[2]) &&
					interfaces::debug_overlay->world_to_screen(vec3_t{ ebpos.back().x + 16.f, ebpos.back().y + 16.f, ebpos.back().z }, endpoints[3]))
				{
					im_render.drawline(endpoints[0].x, endpoints[0].y, endpoints[1].x, endpoints[1].y,
						color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
					im_render.drawline(endpoints[1].x, endpoints[1].y, endpoints[3].x, endpoints[3].y,
						color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
					im_render.drawline(endpoints[3].x, endpoints[3].y, endpoints[2].x, endpoints[2].y,
						color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
					im_render.drawline(endpoints[2].x, endpoints[2].y, endpoints[0].x, endpoints[0].y,
						color_t(c::movement::visualize_edge_bug_clr[0], c::movement::visualize_edge_bug_clr[1], c::movement::visualize_edge_bug_clr[2], alpha));
				}
			}
		}
	}

public:

	void pre(c_usercmd* cmd) {
		this->handle_pre_edgebug(cmd);
	}

	void run(c_usercmd* cmd) {
		this->handle_edgebug(cmd);
	}

	void draw() {
		this->handle_visualize();
	}
};
inline EdgebugAssist EdgebugAssistant;
