#include "misc.hpp"
#include "../../menu/menu.hpp"
#include "../../includes/imgui/imgui.h"
#include "../../includes/imgui/imgui_internal.h"
#include "../../features/visuals/visuals.hpp"
#include "../../sdk/sdk.hpp"

void features::misc::draw() {
	const auto stay_time = 4.f;
	const auto fade_time = 0.6f;

	for (auto i = 0; i < features::misc::notify_list.size(); i++) {
		if (features::misc::notify_list.empty())
			continue;

		notify_t notify = features::misc::notify_list[i];

		float elapsed_time = interfaces::globals->cur_time - (notify.time + stay_time);
		int fade_alpha = static_cast<int>(255.f - (elapsed_time / fade_time) * 255.f);
		fade_alpha = std::clamp(fade_alpha, 0, 255);

		if (fade_alpha <= 0) {
			features::misc::notify_list.erase(features::misc::notify_list.begin() + i);
			i--;
			continue;
		}

		ImGui::PushFont(fonts::logs_font_flag);
		ImGui::GetForegroundDrawList()->AddText(ImVec2(5 + 1, 5 + (i * 15) + 1), ImColor(0, 0, 0, fade_alpha), notify.text.c_str());
		ImGui::GetForegroundDrawList()->AddText(ImVec2(5, 5 + (i * 15)), ImColor(notify._color.r, notify._color.g, notify._color.b, fade_alpha), notify.text.c_str());
		ImGui::PopFont();
	}
}

void features::misc::notify(std::string text, color _color) {
	if (text.empty()) {
		return;
	}

	features::misc::notify_list.push_back(notify_t(text, color(_color.r, _color.g, _color.b, _color.a)));
}

void features::misc::spectators_list() {
	if (!c::misc::spectators_list || !interfaces::engine->is_in_game())
		return;

	static ImVec2 list_pos = ImVec2(c::visuals::speclist_x, c::visuals::speclist_y); // draggable position
	static bool is_dragging = false;
	static ImVec2 drag_offset;

	int screen_w, screen_h;
	interfaces::engine->get_screen_size(screen_w, screen_h);

	auto mouse_pos = get_mouse_position();
	bool is_press = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

	// Prepare spectator lines
	std::vector<std::string> spectator_lines;
	for (int i = 0; i < interfaces::globals->max_clients; i++) {
		auto observer = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(i));
		if (!observer || observer->is_alive() || observer->dormant())
			continue;

		auto target = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity_handle(observer->observer_target()));
		if (!target || !target->is_alive())
			continue;

		if (!c::misc::spectatorlist_show_target && target != g::local)
			continue;

		player_info_t observer_info, target_info;
		interfaces::engine->get_player_info(observer->index(), &observer_info);
		interfaces::engine->get_player_info(target->index(), &target_info);

		char line[128];
		if (c::misc::spectatorlist_show_target)
			sprintf_s(line, "%s > %s", observer_info.name, target_info.name);
		else
			sprintf_s(line, "%s", observer_info.name);

		spectator_lines.push_back(line);
	}

	static float alpha = 0.f;
	const float fade_speed = 255.f * (1.f / 10.f) * interfaces::globals->frame_time;
	alpha += (spectator_lines.empty() ? -1.f : 1.f) * fade_speed;
	alpha = std::clamp(alpha, 0.f, 1.f);
	if (alpha <= 0.f) return;

	const float font_size = 15.f;
	const ImVec2 padding = ImVec2(7.f, 7.f);
	const float line_spacing = 3.f;

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImGui::PushFont(fonts::watermark_font);

	// Compute max width for body and title to match
	float max_width = im_render.measure_text("Spectators", fonts::watermark_font, font_size).x;
	for (const auto& line : spectator_lines)
		max_width = std::max<float>(max_width, im_render.measure_text(line.c_str(), fonts::watermark_font, font_size).x);

	float spectators_height = spectator_lines.empty()
		? 0.f
		: spectator_lines.size() * (font_size + line_spacing) - line_spacing + padding.y * 2.f;

	// Title & body rectangles
	ImVec2 title_tl = list_pos;
	ImVec2 title_br = ImVec2(list_pos.x + max_width + padding.x * 2.f, list_pos.y + font_size + padding.y * 2.f);
	ImVec2 body_tl = ImVec2(title_tl.x, title_br.y);
	ImVec2 body_br = ImVec2(title_br.x, body_tl.y + spectators_height);

	// Handle dragging over title
	bool hovering_title = mouse_pos.x >= title_tl.x && mouse_pos.x <= title_br.x &&
		mouse_pos.y >= title_tl.y && mouse_pos.y <= title_br.y;

	if (hovering_title && is_press && !is_dragging) {
		drag_offset = ImVec2(mouse_pos.x - list_pos.x, mouse_pos.y - list_pos.y);
		is_dragging = true;
	}
	else if (!is_press) {
		is_dragging = false;
	}

	if (is_dragging) {
		list_pos.x = mouse_pos.x - drag_offset.x;
		list_pos.y = mouse_pos.y - drag_offset.y;

		// Clamp inside screen
		list_pos.x = std::clamp(list_pos.x, 0.f, screen_w - (title_br.x - title_tl.x));
		list_pos.y = std::clamp(list_pos.y, 0.f, screen_h - (body_br.y - title_tl.y));
	}

	// === Draw boxes ===
	draw_list->AddRectFilled(title_tl, title_br, ImColor(0.07f, 0.07f, 0.07f, 1.f), 4.5f, ImDrawFlags_RoundCornersTop);
	draw_list->AddRectFilled(body_tl, body_br, ImColor(0.07f, 0.07f, 0.07f, 0.8f), 4.5f, ImDrawFlags_RoundCornersBottom);
	draw_list->AddRect(title_tl, body_br, ImColor(0.1f, 0.1f, 0.1f, 0.7f), 5.5f);

	// Title gradient
	const float grad_height = 3.f;
	const int slices = 60;
	const float grad_width = title_br.x - title_tl.x;
	const float center_x = title_tl.x + grad_width / 2.f;
	for (int i = 0; i < slices; ++i) {
		float slice_x0 = title_tl.x + (grad_width / slices) * i;
		float slice_x1 = slice_x0 + (grad_width / slices);
		float a = 1.f - fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width / 2.f);
		draw_list->AddRectFilled(ImVec2(slice_x0, title_tl.y), ImVec2(slice_x1, title_tl.y + grad_height),
			ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * a));
	}

	// === Draw title ===
	draw_list->AddText(ImVec2(title_tl.x + padding.x, title_tl.y + padding.y), ImColor(0.8f, 0.8f, 0.8f, 1.f), "Spectators");

	// === Draw spectator lines ===
	for (size_t i = 0; i < spectator_lines.size(); ++i) {
		ImVec2 line_pos(body_tl.x + padding.x, body_tl.y + i * (font_size + line_spacing) + padding.y);
		draw_list->AddText(fonts::second_spec_font, font_size, line_pos, ImColor(0.8f, 0.8f, 0.8f, alpha), spectator_lines[i].c_str());
	}

	ImGui::PopFont();
}


void temp() {
	if (!c::misc::spectators_list)
		return;

	if (c::misc::spectatorlist_type != 0)
		return;

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	const float font_size = 18.f;
	const ImVec2 padding = ImVec2(7.f, 7.f);
	const ImVec2 margin = ImVec2(3.f, 3.f);
	const float line_spacing = 3.f;

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	ImGui::PushFont(fonts::watermark_font);

	// Top-left of box
	ImVec2 box_top_left(
		margin.x * 2.f + 250.f,
		padding.y + margin.y + 40.f
	);

	// Force fixed 250x250 size
	ImVec2 box_bottom_right(
		box_top_left.x + 250.f,
		box_top_left.y + 250.f
	);

	// === TITLE BOX (full opacity, fixed height 30) ===
	float title_height = 30.f;
	ImVec2 title_bottom_right(box_bottom_right.x, box_top_left.y + title_height);

	draw_list->AddRectFilled(
		box_top_left, title_bottom_right,
		ImColor(0.07f, 0.07f, 0.07f, 1.f),
		5.5f, ImDrawFlags_RoundCornersTop
	);

	// === BLOOD GRADIENT (inside title box) ===
	{
		const float grad_width = (title_bottom_right.x - box_top_left.x);
		const float grad_height = 3.f;
		const float grad_top_y = box_top_left.y;
		const float grad_bottom_y = grad_top_y + grad_height;
		const float center_x = box_top_left.x + grad_width * 0.5f;
		const int slices = 60;

		for (int i = 0; i < slices; ++i) {
			float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
			float slice_x1 = slice_x0 + (grad_width / slices);
			float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
			float a = 1.f - dist;
			if (a < 0.f) a = 0.f;
			ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * a);

			draw_list->AddRectFilled(
				ImVec2(slice_x0, grad_top_y),
				ImVec2(slice_x1, grad_bottom_y),
				col,
				0.f
			);
		}
	}

	// === BODY BOX (rest of 250x250 area) ===
	ImVec2 body_top_left = ImVec2(box_top_left.x, title_bottom_right.y);
	ImVec2 body_bottom_right = ImVec2(box_bottom_right.x, box_bottom_right.y);

	draw_list->AddRectFilled(
		body_top_left, body_bottom_right,
		ImColor(0.07f, 0.07f, 0.07f, 0.8f),
		5.5f, ImDrawFlags_RoundCornersBottom
	);

	// Border
	draw_list->AddRect(box_top_left, box_bottom_right, ImColor(0.1f, 0.1f, 0.1f, 0.7f), 5.5f);

	// === Title text ===
	draw_list->AddText(
		ImVec2(box_top_left.x + padding.x - 1, box_top_left.y + padding.y - 1),
		ImColor(0.f, 0.f, 0.f, 1.f),
		"Spectators"
	);
	draw_list->AddText(
		ImVec2(box_top_left.x + padding.x, box_top_left.y + padding.y),
		ImColor(0.95f, 0.95f, 0.95f, 1.f),
		"Spectators"
	);

	ImGui::PopFont();
}

vec3_t origin;
float pressdelay = 0.f;
int currentcheckpoint = 0;
std::deque<std::pair<vec3_t, vec3_t>> checkpoints;
void features::misc::kz_practice_logic(c_usercmd* cmd) {
	auto local_player = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));
	if (!g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || !c::misc::practice) {
		checkpoints.clear();
		currentcheckpoint = 0;
		pressdelay = 0;
		return;
	}

	float j = interfaces::console->get_convar("sv_cheats")->get_float();
	if (j == 1) {
		if (g::local->is_alive() && pressdelay < interfaces::globals->realtime) {
			std::stringstream ss;
			if (GetAsyncKeyState(c::misc::savepos) && (g::local->flags() & fl_onground)) {
				checkpoints.push_front({ vec3_t(cmd->view_angles.x, cmd->view_angles.y, 0.f), local_player->origin() });
				currentcheckpoint = 0;
				if (checkpoints.size() > 99)
					checkpoints.pop_back();
				pressdelay = interfaces::globals->realtime + 0.2;
				for (int i = 1; i <= 1; ++i) {


				}
			}
			else if (GetAsyncKeyState(c::misc::savepos) && !(g::local->flags() & fl_onground)) {
				pressdelay = interfaces::globals->realtime + 0.2;
				for (int i = 1; i == 1; ++i) {

				}
			}

			if (!checkpoints.empty()) {
				if (menu::checkkey(c::misc::loadpos, c::misc::loadpos_s)) {
					auto& cur = checkpoints.at(currentcheckpoint);
					std::string cmd = ("setpos ") + std::to_string(cur.second.x) + (" ") + std::to_string(cur.second.y) + (" ") + std::to_string(cur.second.z) + (";");
					interfaces::engine->execute_cmd(cmd.c_str());
					cmd = ("setang ") + std::to_string(cur.first.x) + (" ") + std::to_string(cur.first.y) + (" ") + std::to_string(cur.first.z) + (";");
					interfaces::engine->execute_cmd(cmd.c_str());
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i <= 1; ++i) {

					}
				}
				if (menu::checkkey(c::misc::nextkey, c::misc::nextkey_s)) {
					if (currentcheckpoint > 0)
						currentcheckpoint--;
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i <= 1; ++i) {

					}
				}
				if (menu::checkkey(c::misc::prevkey, c::misc::prevkey_s)) {
					if (currentcheckpoint + 1 < checkpoints.size())
						currentcheckpoint++;
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i <= 1; ++i) {

					}
				}
				if (menu::checkkey(c::misc::undokey, c::misc::undokey_s)) {
					checkpoints.erase(checkpoints.begin() + currentcheckpoint);
					if (currentcheckpoint >= checkpoints.size())
						currentcheckpoint = max(0, checkpoints.size() - 1);
					pressdelay = interfaces::globals->realtime + 0.2;
					for (int i = 1; i <= 1; ++i) {

					}
				}
			}
		}
	}
}

void features::misc::kz_practice() {
	if (!c::misc::practice_window)
		return;

}

void features::misc::reveal_server_ranks(c_usercmd* cmd) {
	if (!c::misc::misc_reveal_ranks || !interfaces::engine->is_in_game())
		return;

	if (cmd->buttons & in_score)
		interfaces::client->dispatch_user_message(50, 0, 0, nullptr);
}

void features::misc::thirdperson() {

}

void features::misc::reset_thirdperson() {

}

//void features::misc::view_model() {
//	if (c::misc::view_model) {
//		if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
//			return;
//
//		if (!g::local || !g::local->is_alive())
//			return;
//
//		const auto view_model = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity_handle(g::local->view_model()));
//
//		if (!view_model)
//			return;
//
//		vec3_t forward{}, right{}, up{};
//		math::angle_vectors_alternative(view_model->abs_angles(), &forward, &right, &up);
//
//		view_model->set_abs_origin(view_model->abs_origin() + forward * c::misc::view_model_z + up * c::misc::view_model_y + right * c::misc::view_model_x);
//	}
//
//	//
//
//	static auto default_scale = interfaces::console->get_convar("cl_wpn_sway_scale")->get_float();
//	static auto* swayscale = interfaces::console->get_convar("cl_wpn_sway_scale");
//
//	swayscale->set_value(c::misc::swayscale ? c::misc::swayscale_value : default_scale);
//}

void features::misc::view_model() {
	if (c::misc::view_model) {
		if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
			return;

		if (!g::local || !g::local->is_alive())
			return;

		const auto view_model = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity_handle(g::local->view_model()));

		if (!view_model)
			return;

		vec3_t forward{}, right{}, up{};
		math::angle_vectors_alternative(view_model->abs_angles(), &forward, &right, &up);

		view_model->set_abs_origin(view_model->abs_origin() + forward * c::misc::view_model_z + up * c::misc::view_model_y + right * c::misc::view_model_x);
	}

	// Optional: force weapon offsets
	/*interfaces::console->get_convar("viewmodel_offset_x")->set_value(-2.0f);
	interfaces::console->get_convar("viewmodel_offset_y")->set_value(-2.0f);
	interfaces::console->get_convar("viewmodel_offset_z")->set_value(-2.0f);*/
}

void features::misc::door_spam(c_usercmd* cmd) {
	if (!c::misc::door_spam || !menu::checkkey(c::misc::door_spam_key, c::misc::door_spam_key_s))
		return;

	static bool spam = false;
	static float time = 0;
	if (interfaces::globals->cur_time > time) {

		cmd->buttons &= in_use;

		time = interfaces::globals->cur_time + 0.2f;
	}
	if (fabs(time - interfaces::globals->cur_time) > 0.3f)
		time = interfaces::globals->cur_time;
}

void features::misc::fix_mouse_delta(c_usercmd* cmd) {
	if (!c::misc::mouse_fix)
		return;

	static vec3_t delta_viewangles{ };
	vec3_t delta = cmd->view_angles - delta_viewangles;
	delta.clamp();

	static convar* sensitivity = interfaces::console->get_convar("sensitivity");

	if (!sensitivity)
		return;

	if (delta.x != 0.f) {
		static convar* m_pitch = interfaces::console->get_convar("m_pitch");

		if (!m_pitch)
			return;

		int final_dy = static_cast<int>((delta.x / m_pitch->get_float()) / sensitivity->get_float());
		if (final_dy <= 32767) {
			if (final_dy >= -32768) {
				if (final_dy >= 1 || final_dy < 0) {
					if (final_dy <= -1 || final_dy > 0)
						final_dy = final_dy;
					else
						final_dy = -1;
				}
				else {
					final_dy = 1;
				}
			}
			else {
				final_dy = 32768;
			}
		}
		else {
			final_dy = 32767;
		}

		cmd->mouse_dy = static_cast<short>(final_dy);
	}

	if (delta.y != 0.f) {
		static convar* m_yaw = interfaces::console->get_convar("m_yaw");

		if (!m_yaw)
			return;

		int final_dx = static_cast<int>((delta.y / m_yaw->get_float()) / sensitivity->get_float());
		if (final_dx <= 32767) {
			if (final_dx >= -32768) {
				if (final_dx >= 1 || final_dx < 0) {
					if (final_dx <= -1 || final_dx > 0)
						final_dx = final_dx;
					else
						final_dx = -1;
				}
				else {
					final_dx = 1;
				}
			}
			else {
				final_dx = 32768;
			}
		}
		else {
			final_dx = 32767;
		}

		cmd->mouse_dx = static_cast<short>(final_dx);
	}

	delta_viewangles = cmd->view_angles;
}

void features::misc::radaringame(player_t* p) {


}

void apply_clan_tag(const char* tag, const char* name) {
	using fn = int(__fastcall*)(const char*, const char*);
	static auto apply_clan_tag_fn = reinterpret_cast<fn>(find_pattern("engine.dll", "53 56 57 8B DA 8B F9 FF 15"));
	apply_clan_tag_fn(tag, name);
};

void features::misc::clantag_spammer() {
	if (!c::misc::misc_clantag_spammer || !interfaces::engine->is_in_game())
		return;

	static int last_index = -1;
	static int last_tick = 0;

	const int tickinterval = 48; // >= 10 tickrate sync

	int cur_tick = interfaces::globals->tick_count;

	// Only advance when enough ticks passed
	if (cur_tick - last_tick < tickinterval)
		return;

	last_tick = cur_tick;

	static int index = 0;
	index++;

	// DNA Software animation frames
	static const std::vector<std::string> dna_frames = {
		"", "", "",
		"D", "DN", "DNA", "DNA S", "DNA Si", "DNA Sig", "DNA Sigm",
		"DNA Sigma", "DNA Sigma G", "DNA Sigma Ga", "DNA Sigma Gam",
		"DNA Sigma Game", "DNA Sigma Gamer",
		"DNA Sigma Game", "DNA Sigma Gam", "DNA Sigma Ga", "DNA Sigma G",
		"DNA Sigma", "DNA Sigm", "DNA Sig", "DNA Si", "DNA S", "DNA",
		"DN", "D", "", "", ""
	};

	// === CUSTOM TEXT ===
	std::string custom_text = c::misc::misc_clantag_text;
	bool use_custom = !custom_text.empty();

	if (use_custom) {
		int len = custom_text.length();
		if (len == 0)
			return;

		int rot_index = index % len;

		std::string rotated = custom_text;
		std::rotate(
			rotated.begin(),
			rotated.begin() + (len - rot_index),
			rotated.end()
		);

		if (last_index != index) {
			apply_clan_tag(rotated.c_str(), custom_text.c_str());
			last_index = index;
		}
		return;
	}

	// === DNA ANIMATION ===
	int frame = index % dna_frames.size();

	if (frame != last_index) {
		apply_clan_tag(dna_frames[frame].c_str(), "DNA Sigma Gamer");
		last_index = frame;
	}
}

inline int hitmarker_time = 0;
void features::misc::hitmarker::event(i_game_event* event) {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	if (!g::local || !g::local->is_alive())
		return;

	auto event_name = fnv::hash(event->get_name());
	auto attacker = interfaces::ent_list->get_client_entity(interfaces::engine->get_player_for_user_id(event->get_int("attacker")));

	if (!attacker || !event_name)
		return;

	switch (event_name) {
	case fnv::hash("player_hurt"):
		if (attacker == g::local) {
			hitmarker_time = 255;

			if (c::misc::misc_hitmarker_sound) {

				switch (c::misc::misc_hitmarker_sound_type) {
				case 0: interfaces::surface->play_sound("buttons\\arena_switch_press_02.wav"); break;
				case 1: interfaces::surface->play_sound("buttons\\button22.wav"); break;
				case 2: interfaces::surface->play_sound("survival\\money_collect_01.wav"); break;
				case 3: interfaces::surface->play_sound("Ui\\beep07.wav"); break;
				}
			}
		}
		break;
	}
}

const char* hitgroup_name(int hitgroup) {
	switch (hitgroup) {
	case hitgroup_head:
		return "head";
	case hitgroup_leftleg:
		return "left leg";
	case hitgroup_rightleg:
		return "right leg ";
	case hitgroup_stomach:
		return "stomach";
	case hitgroup_leftarm:
		return "left arm";
	case hitgroup_rightarm:
		return "right arm";
	default:
		return "body";
	}
}

void features::misc::hitmarker::draw() {
	if (!c::misc::misc_hitmarker)
		return;

	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	if (!g::local || !g::local->is_alive())
		return;

	int width, height;
	interfaces::engine->get_screen_size(width, height);

	if (hitmarker_time > 0) {
		float alpha = hitmarker_time;

		surface::line((width / 2) + 6, (height / 2) + 6, (width / 2) + 3, (height / 2) + 3, color::white(alpha));
		surface::line((width / 2) - 6, (height / 2) + 6, (width / 2) - 3, (height / 2) + 3, color::white(alpha));
		surface::line((width / 2) + 6, (height / 2) - 6, (width / 2) + 3, (height / 2) - 3, color::white(alpha));
		surface::line((width / 2) - 6, (height / 2) - 6, (width / 2) - 3, (height / 2) - 3, color::white(alpha));

		hitmarker_time -= 2;
	}
}

void features::misc::kill_say(i_game_event* event) {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	if (!g::local || !g::local->is_alive())
		return;

	auto event_name = fnv::hash(event->get_name());
	auto attacker = interfaces::ent_list->get_client_entity(interfaces::engine->get_player_for_user_id(event->get_int("attacker")));
	auto entity = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_player_for_user_id(event->get_int("userid"))));

	if (!attacker || !entity || !event_name)
		return;

	switch (event_name) {
	case fnv::hash("player_death"):
		if (attacker == g::local && entity != g::local) {

			if (c::misc::misc_kill_msg)
				interfaces::engine->execute_cmd((std::string("say ").append(c::misc::misc_kill_message)).c_str());

			if (c::misc::misc_hitmarker_screen_effect)
				g::local->health_boost_time() = interfaces::globals->cur_time + 0.4f;
		}
		break;
	}
}

void features::misc::force_crosshair() {
	const static auto weapon_debug_spread_show = interfaces::console->get_convar(xs("weapon_debug_spread_show"));

	const auto should_draw_crosshair = c::misc::force_crosshair && g::local &&
		g::local->is_alive() &&
		!g::local->is_scoped();

	weapon_debug_spread_show->set_value(should_draw_crosshair ? 3 : 0);
}

void features::misc::vote_revealer(i_game_event* event) {
	/*
	if (!c::misc::vote_revealer || !interfaces::engine->is_in_game())
		return;

	int vote = event->get_int("vote_option");
	int id = event->get_int("entityid");
	player_info_t player;
	interfaces::engine->get_player_info(id, &player);
	interfaces::chat_element->chatprintf(0, 0, " \x07 %s\x08 voted %s\x08.\n", player.name, vote == 0 ? "\x07yes" : "\x02no");
	*/
}

void features::misc::sniper_crosshair() {
	if (!c::misc::sniper_crosshair)
		return;

	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	if (!g::local || !g::local->is_alive())
		return;

	const auto weapon = g::local->active_weapon();
	if (!weapon)
		return;

	const auto data = interfaces::weapon_system->get_weapon_data(weapon->item_definition_index());
	if (!data || data->m_iWeaponType != WEAPONTYPE_SNIPER_RIFLE)
		return;

	if (g::local->is_scoped())
		return;

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	int cx = w / 2;
	int cy = h / 2;

	const int length = 9;
	const int gap = 8;
	const int thickness = 1;
	const int dot_size = 3;
	const int dot_outline = 4;

	const color_t white(255, 255, 255);
	const color_t black(0, 0, 0);
	const color_t red(255, 0, 0);

	// --- Outline ---
	im_render.drawrectfilled(cx - gap - length - 1, cy - thickness - 1, length + 2, thickness + 2, black); // Left
	im_render.drawrectfilled(cx + gap - 1, cy - thickness - 1, length + 2, thickness + 2, black); // Right
	im_render.drawrectfilled(cx - thickness, cy - gap - length - 1, thickness + 2, length + 2, black); // Top
	im_render.drawrectfilled(cx - thickness, cy + gap - 1, thickness + 2, length + 2, black); // Bottom

	// --- Lines ---
	im_render.drawrectfilled(cx - gap - length, cy - thickness, length, thickness, white); // Left
	im_render.drawrectfilled(cx + gap, cy - thickness, length, thickness, white); // Right
	im_render.drawrectfilled(cx - thickness + 1, cy - gap - length, thickness, length, white); // Top
	im_render.drawrectfilled(cx - thickness + 1, cy + gap, thickness, length, white); // Bottom

	// --- Center dot ---
	im_render.drawrectfilled(cx - 1, cy - 1, dot_size, dot_size, red);
}

struct HitMarker {
	vec3_t position; // 3D world position
	int damage; // Damage dealt
	float opacity; // Current opacity
	float y_offset; // Vertical offset
	std::chrono::steady_clock::time_point creation_time; // Time of creation
};

// Global vector to store active hit markers
std::vector<HitMarker> hit_markers;

void features::misc::hit_info(i_game_event* event) {
	if (!interfaces::engine->is_in_game())
		return;

	if (!event)
		return;

	auto local_player = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_local_player()));
	if (!local_player)
		return;

	auto victim_id = event->get_int("userid");
	auto attacker_id = event->get_int("attacker");

	if (interfaces::engine->get_player_for_user_id(attacker_id) != interfaces::engine->get_local_player())
		return;

	auto victim = reinterpret_cast<player_t*>(interfaces::ent_list->get_client_entity(interfaces::engine->get_player_for_user_id(victim_id)));
	if (!victim)
		return;

	// Create a new hit marker
	HitMarker marker;
	marker.position = victim->origin() + vec3_t(0, 0, 72); // Offset above the player's head
	marker.damage = event->get_int("dmg_health");
	marker.opacity = 1.0f; // Fully visible
	marker.y_offset = 0.0f; // No vertical offset initially
	marker.creation_time = std::chrono::steady_clock::now();

	// Add the hit marker to the list
	hit_markers.push_back(marker);
}

void features::visuals::draw_hit_markers() {
	if (!c::visuals::damage_indicator || !g::local)
		return;

	auto current_time = std::chrono::steady_clock::now();

	// Constants
	const float animation_duration = 0.75f; // Total lifetime of hitmarker
	const float max_height = 30.0f;         // Maximum vertical offset
	const float fade_peak = 0.6f;           // Start fading out after this % of lifetime

	for (auto it = hit_markers.begin(); it != hit_markers.end();) {
		float elapsed = std::chrono::duration<float>(current_time - it->creation_time).count();

		// Early-out if expired
		if (elapsed > animation_duration) {
			it = hit_markers.erase(it);
			continue;
		}

		// Calculate normalized lifetime progress (0.0 - 1.0)
		float t = elapsed / animation_duration;

		// Eased upward movement (quadratic ease-out)
		it->y_offset = max_height * (1.0f - (1.0f - t) * (1.0f - t));

		// Fade out earlier
		if (t > fade_peak)
			it->opacity = 1.0f - ((t - fade_peak) / (1.0f - fade_peak));
		else
			it->opacity = 1.0f;

		// World to screen
		vec3_t screen_pos;
		if (interfaces::debug_overlay->world_to_screen(it->position + vec3_t(0, 0, it->y_offset), screen_pos)) {

			im_render.text(
				screen_pos.x + 1, screen_pos.y + 1,
				24.f, fonts::esp_damage_font,
				std::to_string(it->damage).c_str(),
				true,
				color_t(
					static_cast<int>(c::visuals::damage_indi_clr_shadow[0] * 255),
					static_cast<int>(c::visuals::damage_indi_clr_shadow[1] * 255),
					static_cast<int>(c::visuals::damage_indi_clr_shadow[2] * 255),
					static_cast<int>(it->opacity * 255)
				),
				false
			);

			im_render.text(
				screen_pos.x, screen_pos.y,
				24.f, fonts::esp_damage_font,
				std::to_string(it->damage).c_str(),
				true,
				color_t(
					static_cast<int>(c::visuals::damage_indi_clr[0] * 255),
					static_cast<int>(c::visuals::damage_indi_clr[1] * 255),
					static_cast<int>(c::visuals::damage_indi_clr[2] * 255),
					static_cast<int>(it->opacity * 255)
				),
				false
			);
		}

		++it;
	}
}

void features::misc::handle_spotify() {
	static HWND spotify_hwnd = nullptr;
	static float last_hwnd_time = 0.f;
	if ((!spotify_hwnd || spotify_hwnd == INVALID_HANDLE_VALUE) && last_hwnd_time < interfaces::globals->realtime + 2.f) {
		for (HWND hwnd = GetTopWindow(0); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
			last_hwnd_time = interfaces::globals->realtime;

			if (!(IsWindowVisible)(hwnd))
				continue;

			int length = (GetWindowTextLengthW)(hwnd);
			if (length == 0)
				continue;

			WCHAR filename[300];
			DWORD pid;
			(GetWindowThreadProcessId)(hwnd, &pid);

			const auto spotify_handle = (OpenProcess)(PROCESS_QUERY_INFORMATION, FALSE, pid);
			(K32GetModuleFileNameExW)(spotify_handle, nullptr, filename, 300);

			std::wstring sane_filename{ filename };

			(CloseHandle)(spotify_handle);

			if (sane_filename.find((L"Spotify.exe")) != std::string::npos)
				spotify_hwnd = hwnd;
		}
	}
	else if (spotify_hwnd && spotify_hwnd != INVALID_HANDLE_VALUE) {
		WCHAR title[300];

		if (!(GetWindowTextW)(spotify_hwnd, title, 300)) {
			spotify_hwnd = nullptr;
		}
		else {
			std::wstring sane_title{ title };
			std::string song(sane_title.begin(), sane_title.end());
			if (sane_title.find((L"-")) != std::string::npos) {
				features::visuals::current_spotify_song = song;
			}
			else if (sane_title.find((L"Advertisment")) != std::string::npos) {
				features::visuals::current_spotify_song = (" advertisment");
			}
			else if (sane_title.find((L"Spotify")) != std::string::npos) {
				features::visuals::current_spotify_song = (" stopped / not playing");
			}
			else {
				features::visuals::current_spotify_song = (" advertisment");
			}
		}
	}

	static std::string old_song = features::visuals::current_spotify_song;
	if (old_song != features::visuals::current_spotify_song) {
		if (c::misc::show_spotify_currently_playing) {
			if (features::visuals::current_spotify_song.find(" stopped / not playing") != std::string::npos)
				interfaces::chat_element->chatprintf("#dna#_spotify_paused");
			else if (features::visuals::current_spotify_song.find(" advertisment") != std::string::npos)
				interfaces::chat_element->chatprintf("#dna#_spotify_advertisement");
			else
				interfaces::chat_element->chatprintf("#dna#_spotify_switch");
		}

		old_song = features::visuals::current_spotify_song;
	}
}

void features::misc::setup_offline() {
	if (!g::local || !g::local->is_alive())
		return;

	interfaces::engine->execute_cmd("sv_lan 1");
	interfaces::engine->execute_cmd("sv_cheats 1");
	interfaces::engine->execute_cmd("bot_stop 1");
	interfaces::engine->execute_cmd("mp_freezetime 0");
	interfaces::engine->execute_cmd("sv_autobunnyhopping 1");
	interfaces::engine->execute_cmd("mp_roundtime 60");
	interfaces::engine->execute_cmd("mp_buytime 600000");
	interfaces::engine->execute_cmd("mp_roundtime_defuse 60");
	interfaces::engine->execute_cmd("mp_roundtime_hostage 60");
	interfaces::engine->execute_cmd("mp_roundbombtime 60");
	interfaces::engine->execute_cmd("sv_falldamage_scale 0");
	interfaces::engine->execute_cmd("mp_restartgame 1");
}