#include "../hooks.hpp"
#include "../../menu/config/config.hpp"
#include "../../features/misc/misc.hpp"
#include "../../features/visuals/visuals.hpp"
#include "../../features/movement/prediction/prediction.hpp"

void __fastcall sdk::hooks::level_init_pre_entity::level_init_pre_entity(void* self, void* edx, const char* map) {

	ofunc(self, edx, map);
}

void __fastcall sdk::hooks::level_shutdown::level_shutdown(void* self, void* edx) {
	ofunc(self, edx);
	//features::weather::reset_weather(false);
}