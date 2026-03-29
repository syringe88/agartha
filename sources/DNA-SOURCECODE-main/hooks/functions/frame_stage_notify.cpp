#include "../hooks.hpp"
#include "../../features/misc/misc.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/movement/prediction/prediction.hpp"
#include "../../features/skins/skins.hpp"
#include "../../features/movement/movement.hpp"
#include "../../features/movement/moveutil.h"

void __stdcall sdk::hooks::frame_stage_notify::frame_stage_notify( int stage ) {
	i_net_channel* net_channel = interfaces::client_state->net_channel;

	if (stage == frame_render_start) {
        features::visuals::nosmoke();
        features::visuals::flashalpha();
        features::visuals::skybox_changer();
        features::visuals::custom_sun();
		backtrack.setup_records();
	}
	else if (stage == frame_render_end) {
        if (c::visuals::enable_darkmode) {
            static auto mat_force_tonemap_scale = interfaces::console->get_convar("mat_force_tonemap_scale");
            mat_force_tonemap_scale->get_float();
            mat_force_tonemap_scale->set_value(c::visuals::darkmode_val);
        }
        else {
            static auto mat_force_tonemap_scale = interfaces::console->get_convar("mat_force_tonemap_scale");
            mat_force_tonemap_scale->get_float();
            mat_force_tonemap_scale->set_value(0);
        }
        features::visuals::fog();
	}
    else if (stage == frame_net_update_postdataupdate_start) {
        features::skins::agent_changer();
        features::skins::knife_changer();
        features::skins::gloves_changer();
        features::skins::full_update();
    }
	else if (stage == frame_net_update_end) {
        
	}
	else if (stage == frame_start) {
		if (EdgebugAssistant.ticks_left && EdgebugAssistant.strafing) {
			vec3_t edgebugva = vec3_t{ features::movement::first_viewangles.x, EdgebugAssistant.startingyaw , features::movement::first_viewangles.z };

			float to_eb_time = ticks_to_time(EdgebugAssistant.edgebugtick) - ticks_to_time(EdgebugAssistant.detecttick);
			float from_detect_time = interfaces::globals->cur_time - ticks_to_time(EdgebugAssistant.detecttick);

			float addedyaw = math::normalize_yaw(EdgebugAssistant.yawdelta * (EdgebugAssistant.eblength * (from_detect_time / to_eb_time)));
			edgebugva.y += addedyaw;
			interfaces::engine->set_view_angles(edgebugva);
			UtilityAssistant.handle_fix(g::cmd, edgebugva);
		}
	}

	if (!interfaces::engine->is_in_game()) {
		fakeping.clear_sequence();
		return ofunc(interfaces::client, stage);
	}

	ofunc( interfaces::client, stage );
}