#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../sdk/structs/materials.hpp"
#include "../../features/visuals/visuals.hpp"

void __fastcall sdk::hooks::get_color_modulation::get_color_modulation(i_material* material, void* edx, float* r, float* g, float* b) {
	if (!material || material->is_error_material() || !g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game()) {
		ofunc(material, edx, r, g, b);
		return;
	}

	ofunc(material, edx, r, g, b);
	const char* group = material->get_texture_group_name();
	if (!group)
		return ofunc(material, edx, r, g, b);

	vec3_t css_tint = { 0.99f, 0.99f, 0.99f };

	bool should_modulate = false;
	float shade_strength = 1.0f;

	if (strstr(group, "World")) {
		shade_strength = 0.2f;
		should_modulate = true;
	}
	else if (strstr(group, "StaticProp")) {
		shade_strength = 0.3f;
		should_modulate = true;
	}
	else if (strstr(group, "Prop")) {
		shade_strength = 0.3f;
		should_modulate = true;
	}
	else if (strstr(group, "SkyBox")) {
		shade_strength = 0.35f;
		should_modulate = true;
	}
	else if (strstr(group, "Model") || strstr(group, "Generic")) {
		shade_strength = 0.45f;
		should_modulate = true;
	}

	if (!should_modulate)
		return ofunc(material, edx, r, g, b);

	/*if (strstr(group, "World")) {
		if (auto base_tex = material->find_var("$basetexture", nullptr, false)) {
			const char* mat_name = material->get_name();
			base_tex->set_string_value("plaster/plasterwall001a");
		}
	}*/

	const auto safe_set_float = [&](const char* name, float value) {
		if (auto var = material->find_var(name, nullptr, false))
			var->set_float_value(value);
		};

	const auto safe_set_vec = [&](const char* name, float x, float y, float z) {
		if (auto var = material->find_var(name, nullptr, false))
			var->set_vec_value(x, y, z);
		};

	if (c::visuals::custom_shaders) {

		safe_set_float("$phong", 1.0f);
		safe_set_float("$phongboost", 0.01f);
		safe_set_float("$phongexponent", 0.01f);
		safe_set_float("$envmapfresnel", 0.01f);
		safe_set_vec("$phongfresnelranges", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$envmapfresnelminmaxexp", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$envmaptint", 0.01f, 0.01f, 0.01f);
		safe_set_vec("$selfillumtint", 0.01f, 0.01f, 0.01f);

	}

	// Apply color tint & darkening
	if (c::visuals::world_modulate) {
		*r *= shade_strength * css_tint.x * c::visuals::world_color[0];
		*g *= shade_strength * css_tint.y * c::visuals::world_color[1];
		*b *= shade_strength * css_tint.z * c::visuals::world_color[2];
	}

	// apply changes for selected material or matching group
	for (auto& entry : changed_materials) {
		if ((_stricmp(entry.name.c_str(), material->get_name()) == 0)
			|| (entry.group[0] && strstr(group, entry.group.c_str()))) {
			vec3_t col = parse_rgb(entry.value.c_str());
			*r = col.x;
			*g = col.y;
			*b = col.z;
			break;
		}
	}
}


float __fastcall sdk::hooks::get_alpha_modulation::get_alpha_modulation(i_material* material, void* edx) {
	if (!g::local || !interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return ofunc(material, edx);

	if (!c::visuals::world_modulate)
		return ofunc(material, edx);

	if (!material || material->is_error_material())
		return ofunc(material, edx);

	float alpha = c::visuals::world_color[3];

	const auto group = material->get_texture_group_name();
	if (strstr(group, ("World textures"))
		|| strstr(group, ("StaticProp textures"))
		|| strstr(group, ("SkyBox textures")))
		return alpha;

	return ofunc(material, edx);
}