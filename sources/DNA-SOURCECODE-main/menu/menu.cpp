#include "menu.hpp"
#include "config/config.hpp"
#include "fonts/menu_font.hpp"
#include "fonts/icon.h"
#include "../hooks/hooks.hpp"
#include "../features/scripts/scripts.hpp"
#include "../features/misc/misc.hpp"
#include "../features/movement/movement.hpp"
#include "../features/skins/skins.hpp"
#include "../features/panorama/scaleform.hpp"
#include "../includes/imgui/imgui_internal.h"
#include "../includes/imgui/imgui_impl_dx9.h"
#include "../includes/imgui/imgui_impl_win32.h"
#include "../imgui/freetype/include/freetype/freetype.h"
#include <Windows.h>

#include "../includes/imgui/imgui.h"
#include "../includes/imgui/byte_array.h"
#include "../includes/imgui/image_array.h"
#include "../includes/imgui/nav_elements.h"
#include "../includes/imgui/etc_elements.h"
#include "../includes/imgui/imgui_impl_win32.h"
#include <imgui_subfeature.h>
#include <d3d9.h>
#include <tchar.h>


#include <stdexcept>
#include <misc/freetype/imgui_freetype.h>
#include "../features/movement/moveutil.h"

static const char* WeatherTypes[] = { "rain","ash","heavy rain","snow" };
static const char* edgebug_lock_types[] = { "classic lock", "static lock", "dynamic lock", "predicted lock" };
static const char* mov_toggle_type[] = { "always on", "on hotkey", "toggle hotkey" };
static const char* eb_priority[3] = { "crouching", "standing", "dynamic" };
static const char* ljnulling[4] = { "-w", "-s", "-a", "-d" };
static const char* ad_key[3] = { "ej", "mj", "lj" };
static const char* fb_angles[] = { "right", "backwards", "left" };
static const char* tabs[] = { "indicators","positions" };
static const char* indicators[8] = { "eb", "jb", "lj", "mj", "ps", "ej", "lb", "fm" };
const char* font_flags[] = { "no hinting","no autohint","light hinting","mono hinting","bold","italic","no antialiasing","load color","bitmap","dropshadow","outline" };
const char* fnt_tab[] = { "main indicator font", "sub indicator font", "spec font", "name font", "health font", "player weapon font", "dropped weapon font", "screen logs font", "watermark" };
static const char* hitboxes[] = { "head","neck","chest","pelvis" };
static const char* removals[5] = { "r_3dsky", "mat_postprocess", "cl_shadows", "mat_disable_bloom", "panorama_disable_blur" };
static const char* materials[] = { "regular", "flat", "crystal", "pearlescent", "reverse pearlescent", "fog", "damascus", "model" };
static const char* chams_overlay_types[] = { "glow", "outline", "metallic", "snow" };
static const char* flags[6] = { "bot", "armor", "money", "scoped", "flashed", "defusing" };
static const char* weapon_type[2] = { "text", "icon" };
static const char* outline_type[2] = { "outter", "inner" };
const char* data_center_list_names[] = { "australia", "austria", "brazil", "chile", "dubai", "france", "germany", "hong kong", "india (chennai)", "india (mumbai)", "japan", "luxembourg", "netherlands", "peru", "philipines", "poland", "singapore", "south africa", "spain", "sweden", "uk", "usa (atlanta)", "usa (seattle)", "usa (chicago)", "usa (los angeles)", "usa (moses lake)", "usa (oklahoma)", "usa (seattle)", "usa (washington dc)" };
std::string data_center_list[] = { "syd", "vie", "gru", "scl", "dxb", "par", "fra", "hkg",
   "maa", "bom", "tyo", "lux", "ams", "limc", "man", "waw", "sgp", "jnb",
   "mad", "sto", "lhr", "atl", "eat", "ord", "lax", "mwh", "okc", "sea", "iad" };
ImGuiColorEditFlags no_alpha = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar;
ImGuiColorEditFlags w_alpha = ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar;

ImFont* medium;
ImFont* bold;
ImFont* tab_icons;
ImFont* tab_title;
ImFont* tab_title_icon;
ImFont* subtab_title;
ImFont* combo_arrow;

///Gearcog is icon E
std::array<float, 3> convert_color(int hex_value) {
	std::array<float, 3> rgb_color;
	rgb_color[2] = ((hex_value >> 16) & 0xFF) / 255.0;
	rgb_color[1] = ((hex_value >> 8) & 0xFF) / 255.0;
	rgb_color[0] = ((hex_value) & 0xFF) / 255.0;
	return rgb_color;
}

bool isEnter_Pressed()
{
	static bool keyWasDown = false;
	if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
		keyWasDown = true;
	}
	else if (keyWasDown) {
		keyWasDown = false;
		return true;
	}
	return false;
}

bool SubFeature(const char* id, const std::vector<GearSettingEntry>& entries)
{
	ImGui::PushID(id);
	ImGui::InvisibleButton("##cog_button", ImVec2(30, 30));
	if (ImGui::IsItemVisible()) {
		auto* draw = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetItemRectMin();
		pos.x += 8;
		ImGui::PushFont(fonts::dna_font);
		draw->AddText(ImVec2(pos.x + 1, pos.y + 1), ImColor(1.f, 1.f, 1.f, 0.6f), "M");
		draw->AddText(pos, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.6f), "M");
		ImGui::PopFont();
	}

	std::string popup_id = std::string("##settings_popup_") + id;
	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		ImGui::OpenPopup((std::string("settings_popup_") + id).c_str());

	bool changed = false;
	ImGui::SetNextWindowSize(ImVec2(450, 385), ImGuiCond_Appearing);

	if (ImGui::BeginPopup((std::string("settings_popup_") + id).c_str())) {
		// Get popup window position and size
		ImVec2 popup_pos = ImGui::GetWindowPos();
		ImVec2 popup_size = ImGui::GetWindowSize();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 rect_min = popup_pos;
		ImVec2 rect_max = ImVec2(popup_pos.x + popup_size.x, popup_pos.y + popup_size.y);

		draw_list->PushClipRectFullScreen();

		// Multi-pass blur effect (matching etc elements) - extended to full width
		for (int i = 0; i < 4; i++) {
			float offset = (float)i * 0.5f;
			float blur_alpha = 0.5f / (i + 1);
			draw_list->AddRectFilled(
				ImVec2(rect_min.x - offset, rect_min.y - offset),
				ImVec2(rect_max.x + offset, rect_max.y + offset),
				ImColor(0.02f, 0.02f, 0.02f, blur_alpha * 0.5f),
				4.0f + offset
			);
		}

		// Main background (matching etc elements)
		draw_list->AddRectFilled(rect_min, rect_max, ImColor(0.1f, 0.1f, 0.1f, 0.15f), 4.0f);
		draw_list->AddRect(rect_min, rect_max, ImColor(0.1f, 0.1f, 0.1f, 0.5f), 4.0f);

		// ===== TOP GRADIENT BAR (kept from original but blurred) =====
		const float grad_width = popup_size.x;
		const float grad_height = 3.f;
		const float center_x = popup_pos.x + popup_size.x * 0.5f;
		const float grad_top_y = popup_pos.y + 1.f;
		const float grad_bottom_y = grad_top_y + grad_height;
		const int slices = 150;

		for (int i = 0; i < slices; i++) {
			float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
			float slice_x1 = slice_x0 + (grad_width / slices);

			float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
			float alpha = 1.f - dist;
			if (alpha < 0.f) alpha = 0.f;

			// Reduced alpha for blurred effect, matching color scheme
			ImColor col = ImColor(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				0.35f * alpha  // Reduced from 0.75f for blurred look
			);

			draw_list->AddRectFilled(
				ImVec2(slice_x0, grad_top_y),
				ImVec2(slice_x1, grad_bottom_y),
				col,
				4.0f, 0  // Increased rounding to match main rect
			);
		}

		// Separator line below gradient (matching etc elements style)
		draw_list->AddLine(
			ImVec2(rect_min.x + 1, rect_min.y + 32),
			ImVec2(rect_max.x - 1, rect_min.y + 32),
			ImColor(0.02f, 0.02f, 0.02f, 0.3f)
		);

		// Add spacing
		ImGui::Dummy(ImVec2(0, 12));

		// ===== SETTINGS CONTENT =====
		for (const auto& entry : entries) {
			std::visit([&](auto* ptr) {
				using T = std::decay_t<decltype(*ptr)>;
				if constexpr (std::is_same_v<T, bool>) {
					// Style checkbox with accent color
					ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(
						menu::menu_accent[0],
						menu::menu_accent[1],
						menu::menu_accent[2],
						1.f
					));
					ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(
						menu::menu_accent[0] * 0.3f,
						menu::menu_accent[1] * 0.3f,
						menu::menu_accent[2] * 0.3f,
						0.4f
					));
					changed |= ImGui::Checkbox(entry.label.c_str(), ptr);
					ImGui::PopStyleColor(2);
				}
				else if constexpr (std::is_same_v<T, float>) {
					ImGui::Spacing();
					ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(
						menu::menu_accent[0],
						menu::menu_accent[1],
						menu::menu_accent[2],
						1.f
					));
					ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(
						menu::menu_accent[0] * 1.2f,
						menu::menu_accent[1] * 1.2f,
						menu::menu_accent[2] * 1.2f,
						1.f
					));
					changed |= ImGui::SliderFloat(entry.label.c_str(), ptr, entry.float_min, entry.float_max, "%.1f");
					ImGui::PopStyleColor(2);
				}
				else if constexpr (std::is_same_v<T, int>) {
					ImGui::Spacing();
					ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(
						menu::menu_accent[0],
						menu::menu_accent[1],
						menu::menu_accent[2],
						1.f
					));
					ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(
						menu::menu_accent[0] * 1.2f,
						menu::menu_accent[1] * 1.2f,
						menu::menu_accent[2] * 1.2f,
						1.f
					));
					changed |= ImGui::SliderInt(entry.label.c_str(), ptr, entry.int_min, entry.int_max, "%d");
					ImGui::PopStyleColor(2);
				}
				}, entry.value);
		}

		ImGui::EndPopup();
	}

	ImGui::PopID();
	return changed;
}


static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParam,
	LPARAM longParam
);

// window process
LRESULT CALLBACK WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParam,
	LPARAM longParam
);


bool gui::SetupWindowClass(const char* windowClassName) noexcept
{
	// populate window class
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.hIcon = NULL;
	windowClass.hCursor = NULL;
	windowClass.hbrBackground = NULL;
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = NULL;

	// register
	if (!RegisterClassEx(&windowClass))
		return false;

	return true;
}

void gui::DestroyWindowClass() noexcept
{
	UnregisterClass(
		windowClass.lpszClassName,
		windowClass.hInstance
	);
}

bool gui::SetupWindow(const char* windowName) noexcept
{
	// create temp window
	window = CreateWindow(
		windowClass.lpszClassName,
		windowName,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		100,
		100,
		0,
		0,
		windowClass.hInstance,
		0
	);

	if (!window)
		return false;
	return true;
}

void gui::DestroyWindow() noexcept
{
	if (window)
		DestroyWindow(window);
}

bool gui::SetupDirectX() noexcept
{
	const auto handle = GetModuleHandle("d3d9.dll");

	if (!handle)
		return false;

	using CreateFn = LPDIRECT3D9(__stdcall*)(UINT);

	const auto create = reinterpret_cast<CreateFn>(GetProcAddress(
		handle,
		"Direct3DCreate9"
	));

	if (!create)
		return false;

	d3d9 = create(D3D_SDK_VERSION);

	if (!d3d9)
		return false;

	D3DPRESENT_PARAMETERS params = { };
	params.BackBufferWidth = 0;
	params.BackBufferHeight = 0;
	params.BackBufferFormat = D3DFMT_UNKNOWN;
	params.BackBufferCount = 0;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality = NULL;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.Windowed = 1;
	params.EnableAutoDepthStencil = 0;
	params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	params.Flags = NULL;
	params.FullScreen_RefreshRateInHz = 0;
	params.PresentationInterval = 0;

	if (d3d9->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_NULLREF,
		window,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
		&params,
		&device
	) < 0) return false;

	return true;
}

void gui::DestroyDirectX() noexcept
{
	if (device)
	{
		device->Release();
		device = NULL;
	}

	if (d3d9)
	{
		d3d9->Release();
		d3d9 = NULL;
	}
}

void gui::Setup()
{
	if (!SetupWindowClass("hackClass001"))
		throw std::runtime_error("Failed to create window class.");

	if (!SetupWindow("Hack Window"))
		throw std::runtime_error("Failed to create window.");

	if (!SetupDirectX())
		throw std::runtime_error("Failed to create device.");

	DestroyWindow();
	DestroyWindowClass();
}
void gui::SetupMenu(LPDIRECT3DDEVICE9 device) noexcept
{
	static std::once_flag init_flag;
	std::call_once(init_flag, [&]()
		{
			auto params = D3DDEVICE_CREATION_PARAMETERS{};
			device->GetCreationParameters(&params);

			window = params.hFocusWindow;

			originalWindowProcess = reinterpret_cast<WNDPROC>(
				SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess))
				);

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavNoCaptureKeyboard;

			ImGui_ImplWin32_Init(window);
			ImGui_ImplDX9_Init(device);
			ImGui::StyleColorsDark();

			ImFontConfig font_config;

			static const ImWchar ranges[] =
			{
				0x0020, 0x00FF, // Basic Latin + Latin Supplement
				0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
				0x2DE0, 0x2DFF, // Cyrillic Extended-A
				0xA640, 0xA69F, // Cyrillic Extended-B
				0xE000, 0xE226, // icons
				0x800, 0x400,
				0,
			};

			font_config.GlyphRanges = ranges;

			fonts::menu_font_thin = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			fonts::menu_font_bold = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12.0f, &font_config, ranges);
			medium = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			bold = io.Fonts->AddFontFromMemoryTTF(tahomabd, sizeof(tahomabd), 15.0f, &font_config, ranges);
			fonts::clarity_watermark = io.Fonts->AddFontFromMemoryTTF(PTRootUIBold, sizeof(PTRootUIBold), 15.0f, &font_config, ranges);

			tab_icons = io.Fonts->AddFontFromMemoryTTF(clarityfont, sizeof(clarityfont), 19.0f, &font_config, ranges);
			fonts::menu_icons = io.Fonts->AddFontFromMemoryTTF(menu_icon, sizeof(menu_icon), 19.0f, &font_config, ranges);
			fonts::sub_feature_icon = io.Fonts->AddFontFromMemoryTTF(menu_icon, sizeof(menu_icon), 19.0f, &font_config, ranges);

			fonts::logo = io.Fonts->AddFontFromMemoryTTF(DNA, sizeof(DNA), 75.0f, &font_config, ranges);
			fonts::logo_watermark = io.Fonts->AddFontFromMemoryTTF(DNA, sizeof(DNA), 50.0f, &font_config, ranges);

			fonts::dna_font = io.Fonts->AddFontFromMemoryTTF(DNA, sizeof(DNA), 18.0f, &font_config, ranges);
			fonts::dna_icon_rain = io.Fonts->AddFontFromMemoryTTF(DNA, sizeof(DNA), 5.0f, &font_config, ranges);

			fonts::watermark_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);

			tab_title = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			tab_title_icon = io.Fonts->AddFontFromMemoryTTF(clarityfont, sizeof(clarityfont), 18.0f, &font_config, ranges);
			fonts::watermark_icons = io.Fonts->AddFontFromMemoryTTF(clarityfont, sizeof(clarityfont), 50.0f, &font_config, ranges);
			fonts::menutab_icons = io.Fonts->AddFontFromMemoryTTF(clarityfont, sizeof(clarityfont), 29.0f, &font_config, ranges);

			//fonts::damage_indi_font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdanab.ttf", 24.0f, &font_config, ranges);
			fonts::main_spec_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			fonts::second_spec_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			fonts::spec_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			fonts::logs_font_flag = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12.0f, &font_config, ranges);
			fonts::esp_damage_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 24.0f, &font_config, ranges);
			fonts::esp_name_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12, &font_config, ranges);
			fonts::esp_hp_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12, &font_config, ranges);
			fonts::esp_wpn_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12, &font_config, ranges);
			fonts::esp_dropped = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12, &font_config, ranges);
			fonts::esp_misc = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 12, &font_config, ranges);
			fonts::esp_wpn_icon = io.Fonts->AddFontFromMemoryCompressedTTF(icon_font, icon_font_size, 12.f, &font_config, ranges);
			fonts::key_strokes_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			subtab_title = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.0f, &font_config, ranges);
			combo_arrow = io.Fonts->AddFontFromMemoryTTF(combo, sizeof(combo), 9.0f, &font_config, ranges);

			ImVec4 clear_color = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
			ImFontConfig cfg_menu;
			ImFontConfig cfg_screen;
			ImFontConfig indifont_flag;
			ImFontConfig sub_indifont_flag;

			fonts::indicator_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), c::fonts::indi_size, &indifont_flag, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
			fonts::indicator_font_tahoma = io.Fonts->AddFontFromMemoryTTF(tahomabd, sizeof(tahomabd), c::fonts::indi_size, &indifont_flag, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
			fonts::sub_indicator_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), c::fonts::sub_indi_size, &sub_indifont_flag, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
			fonts::points_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 15.f, &sub_indifont_flag, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
			fonts::points_big_font = io.Fonts->AddFontFromMemoryTTF(Montserrat_thin, sizeof(Montserrat_thin), 23.f, &sub_indifont_flag, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

			setup = true;
		});
}

void gui::Destroy() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SetWindowLongPtr(
		window,
		GWLP_WNDPROC,
		reinterpret_cast<LONG_PTR>(originalWindowProcess)
	);

	DestroyDirectX();
}

enum heads {
	ragebot, antiaim, visuals, movement, misc, players, settings, skins, configs, scripts, tools
};

enum sub_heads {
	general, accuracy, exploits, _general, advanced, surfaces, glow, chams
};

struct particle_t {
	ImVec2 pos;
	ImVec2 vel;
	float size;
	float alpha;        // final displayed alpha
	float base_alpha;   // max alpha
	float fade_time;    // internal timer
	float fade_speed;   // how fast it fades
};

static std::vector<particle_t> particles;
static bool particles_initialized = false;

inline void to_json(nlohmann::json& j, const changed_entry& entry) {
	j = nlohmann::json{
		{"name", entry.name},
		{"value", entry.value}
	};
}

inline void from_json(const nlohmann::json& j, changed_entry& entry) {
	j.at("name").get_to(entry.name);
	j.at("value").get_to(entry.value);
}

void save_convars(int config_index) {
	if (config_index < 0 || config_index >= c::configs.size())
		return;

	std::string config_name = c::configs[config_index];
	std::string path = "C:/dna/convars/" + config_name + "_convars.json";

	nlohmann::json j;
	j["convars"] = c::changed_convars;

	std::ofstream file(path);
	if (file.is_open()) {
		file << j.dump(4);
		file.close();
	}
}

// Load convars from file
void load_convars(int config_index) {
	if (config_index < 0 || config_index >= c::configs.size())
		return;

	std::string config_name = c::configs[config_index];
	std::string path = "C:/dna/convars/" + config_name + "_convars.json";

	std::ifstream file(path);
	if (!file.is_open())
		return;

	try {
		nlohmann::json j;
		file >> j;
		file.close();

		// Clear current changed convars
		c::changed_convars.clear();

		// Load from JSON
		if (j.contains("convars")) {
			// Manual deserialization to avoid ADL issues
			c::changed_convars.clear();
			for (auto& item : j["convars"]) {
				changed_entry entry;
				from_json(item, entry);
				c::changed_convars.push_back(entry);
			}

			// Apply all loaded convars
			convar* head = **reinterpret_cast<convar***>(reinterpret_cast<std::uintptr_t>(interfaces::console) + 0x34);
			if (head) {
				for (auto& entry : c::changed_convars) {
					// Find the convar in the list
					for (convar* c = head; c != nullptr; c = c->next) {
						if (c && c->name && _stricmp(c->name, entry.name.c_str()) == 0) {
							// Detect if value is numeric
							bool is_number = true;
							for (const char* ch = entry.value.c_str(); *ch; ch++) {
								if ((*ch < '0' || *ch > '9') && *ch != '.' && *ch != '-') {
									is_number = false;
									break;
								}
							}

							// Apply the value
							if (is_number) {
								float f = atof(entry.value.c_str());
								c->set_value(f);
							}
							else {
								c->set_value(entry.value.c_str());
							}
							break;
						}
					}
				}
			}
		}
	}
	catch (...) {
		// Handle JSON parsing errors
	}
}

void gui::Render() noexcept
{
	if (!features::visuals::has_initalized)
		return;

	static heads tab{ ragebot };
	static sub_heads subtab{ general };

	const char* tab_name = tab == ragebot ? "Ragebot" : tab == antiaim ? "Anti-aim" : tab == visuals ? "Visuals" : tab == settings ? "Settings" : tab == skins ? "Skins" : tab == configs ? "Configs" : tab == scripts ? "Scripts" : 0;
	const char* tab_icon = tab == ragebot ? "B" : tab == antiaim ? "C" : tab == visuals ? "D" : tab == settings ? "E" : tab == skins ? "F" : tab == configs ? "G" : tab == scripts ? "H" : 0;

	static bool boolean, boolean_1 = false;
	static int sliderscalar, combo = 0;

	const char* combo_items[3] = { "Value", "Value 1", "Value 2" };

	static int config_index = -1;
	static char buffer[10];

	// Auto-select top config if available and none selected yet
	if (config_index == -1 && !c::configs.empty()) {
		config_index = 0;
	}

	static char search_knife_skin[256];
	static ImGuiTextFilter filter1;
	static ImGuiTextFilter filter2;

	static ImGuiTextFilter filter;
	static char new_value[64] = "";
	static int selected_index = -1;

	static std::vector<convar*> convar_list;
	static bool initialized = false;

	ImGui::SetNextWindowSize({ 998, 868 });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

	ImGui::Begin("", &menu::open, ImGuiWindowFlags_NoDecoration); {
		auto draw = ImGui::GetWindowDrawList();
		auto pos = ImGui::GetWindowPos();
		auto size = ImGui::GetWindowSize();
		ImGuiStyle style = ImGui::GetStyle();

		// layout constants
		const float title_h = 112.0f;
		const float title_round = 4.5f;
		const float body_round = 4.5f;
		const float border_round = 5.5f;

		// === base faint background (subtle) ===
		// Draw blurred background
		ImVec2 blur_min = pos;
		ImVec2 blur_max = ImVec2(pos.x + size.x, pos.y + size.y);

		// Multi-pass blur effect (draw multiple offset rects with decreasing alpha)
		for (int i = 0; i < 4; i++) {
			float offset = (float)i * 0.5f;
			float blur_alpha = 0.5f / (i + 1);
			draw->AddRectFilled(
				ImVec2(blur_min.x - offset, blur_min.y - offset),
				ImVec2(blur_max.x + offset, blur_max.y + offset),
				ImColor(0.02f, 0.02f, 0.02f, blur_alpha * 0.5f),
				style.WindowRounding + offset
			);
		}

		// Main background
		draw->AddRectFilled(
			pos,
			ImVec2(pos.x + size.x, pos.y + size.y),
			ImColor(0.02f, 0.02f, 0.02f, 0.5f),
			style.WindowRounding
		);

		// === PARTICLE INITIALIZATION ===
		if (!particles_initialized) {
			particles_initialized = true;

			const int count = 75;
			particles.reserve(count);

			for (int i = 0; i < count; i++) {
				particle_t p;
				p.pos = ImVec2(
					pos.x + (rand() % (int)size.x),
					pos.y + (rand() % (int)size.y)
				);

				float dx = ((rand() % 200) - 100) * 0.003f;
				float dy = ((rand() % 200) - 100) * 0.003f;
				p.vel = ImVec2(dx, dy);

				p.size = (rand() % 3) + 1;

				// store original alpha as max alpha
				p.base_alpha = ((rand() % 40) / 255.f);

				// random starting fade phase so they don't sync
				p.fade_time = (rand() % 1000) * 0.01f;

				// fade speed (lower = slower, elegant)
				p.fade_speed = 0.6f + ((rand() % 40) / 100.f); // 0.6–1.0

				particles.push_back(p);
			}
		}
		// === MOVING + FADING PARTICLE SYSTEM ===
		{
			float dt = ImGui::GetIO().DeltaTime;

			for (auto& p : particles) {

				// update motion
				p.pos.x += p.vel.x * dt * 120.0f;
				p.pos.y += p.vel.y * dt * 120.0f;

				// wrap
				if (p.pos.x < pos.x) p.pos.x = pos.x + size.x;
				if (p.pos.x > pos.x + size.x) p.pos.x = pos.x;
				if (p.pos.y < pos.y) p.pos.y = pos.y + size.y;
				if (p.pos.y > pos.y + size.y) p.pos.y = pos.y;

				// update fade timer
				p.fade_time += dt * p.fade_speed;

				// sinusoidal fade (0 → 1 → 0 → 1…)
				float fade = (sinf(p.fade_time) * 0.5f) + 0.5f;

				// final draw alpha
				p.alpha = p.base_alpha * fade;

				// draw particle
				draw->AddCircleFilled(
					p.pos,
					p.size,
					ImColor(
						menu::menu_accent[0],
						menu::menu_accent[1],
						menu::menu_accent[2],
						p.alpha
					)
				);
			}
		}

		//Here's the animated version with vertical and horizontal lines pulsing at different rates:
			//cpp// === ANIMATED SLEEK GRID (Spotify-style with pulsing alpha) ===
		{
			const float cell = 42.0f;             // grid cell size (tweak 32–48)
			const float thickness = 1.0f;         // line thickness
			const float baseAlpha = 0.006f;       // base faintness (0.01–0.03)

			// Get time for animation
			float time = ImGui::GetIO().DeltaTime;

			// Different animation rates for vertical and horizontal
			float verticalPulse = 0.5f + 0.5f * sinf(time * 0.7f);  // slower pulse
			float horizontalPulse = 0.5f + 0.5f * sinf(time * 0.9f); // faster pulse

			// Calculate animated alphas
			float verticalAlpha = baseAlpha * (1.0f + verticalPulse * 2.0f);
			float horizontalAlpha = baseAlpha * (1.0f + horizontalPulse * 2.0f);

			ImColor vertLine = ImColor(1.f, 1.f, 1.f, verticalAlpha);
			ImColor horzLine = ImColor(1.f, 1.f, 1.f, horizontalAlpha);

			float x0 = pos.x;
			float y0 = pos.y;
			float x1 = pos.x + size.x;
			float y1 = pos.y + size.y;

			// vertical lines (slower pulse)
			for (float x = x0; x <= x1; x += cell) {
				draw->AddLine(ImVec2(x, y0), ImVec2(x, y1), vertLine, thickness);
			}

			// horizontal lines (faster pulse)
			for (float y = y0; y <= y1; y += cell) {
				draw->AddLine(ImVec2(x0, y), ImVec2(x1, y), horzLine, thickness);
			}

			// optional: animated vertical highlight
			float highlightAlpha = baseAlpha * 1.2f * (1.0f + verticalPulse * 1.5f);
			draw->AddLine(
				ImVec2(x0, y0),
				ImVec2(x0, y1),
				ImColor(1.f, 1.f, 1.f, highlightAlpha),
				1.0f
			);
		}

		// === TITLE BOX (solid) ===
		ImVec2 title_tl = pos;
		ImVec2 title_br = ImVec2(pos.x + size.x, pos.y + title_h);

		// === TOP "BLOOD" GRADIENT STRIP ===
		{
			const float grad_width = title_br.x - title_tl.x;
			const float grad_height = 3.0f;
			const float grad_top_y = title_tl.y;
			const float grad_bottom_y = grad_top_y + grad_height;
			const float center_x = title_tl.x + grad_width * 0.5f;
			const int slices = 80;

			for (int i = 0; i < slices; ++i) {
				float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
				float slice_x1 = slice_x0 + (grad_width / slices);
				float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
				float a = 1.f - dist;
				if (a < 0.f) a = 0.f;

				ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * a);
				draw->AddRectFilled(
					ImVec2(slice_x0, grad_top_y),
					ImVec2(slice_x1, grad_bottom_y),
					col, 0.0f
				);
			}
		}

		// === vertical separator (tabs) ===
		draw->AddLine(ImVec2(pos.x + 210, pos.y + 2), ImVec2(pos.x + 210, pos.y + size.y - 2),
			ImColor(1.0f, 1.0f, 1.0f, 0.03f));

		// === logo / big letter on the top-left ===
		draw->AddText(fonts::logo, 75.0f, ImVec2(pos.x + 49, pos.y + 14),
			ImColor(255.f, 255.f, 255.f, 0.2f), "A");

		draw->AddText(fonts::logo, 75.0f, ImVec2(pos.x + 51, pos.y + 16),
			ImColor(255.f, 255.f, 255.f, 0.2f), "A");

		draw->AddText(fonts::logo, 75.0f, ImVec2(pos.x + 50, pos.y + 15),
			ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 1.f), "A");

		draw->AddRect(pos + ImVec2(1, 1), pos + size - ImVec2(1, 1), ImColor(1.0f, 1.0f, 1.0f, 0.03f), style.WindowRounding);

		// === NEW TAB SYSTEM WITH EXPANDABLE SUBTABS ===
		// Add this to your gui::Render() function, replacing the existing tab system

		// === NEVERLOSE-STYLE TAB SYSTEM ===
		ImGui::SetCursorPos({ 8, 112 });
		ImGui::BeginGroup(); {

			// COMBAT CATEGORY
			elements::category_title("COMBAT");

			if (elements::icon_tab("J", "aimbot", tab == ragebot && subtab == general)) {
				tab = ragebot;
				subtab = general;
			}

			if (elements::icon_tab("R", "backtrack", tab == ragebot && subtab == accuracy)) {
				tab = ragebot;
				subtab = accuracy;
			}

			ImGui::Dummy(ImVec2(0, 4)); // Spacing between categories

			// VISUALS CATEGORY
			elements::category_title("VISUALS");

			if (elements::icon_tab("K", "esp", tab == visuals && subtab == general)) {
				tab = visuals;
				subtab = general;
			}

			if (elements::icon_tab("L", "screen", tab == visuals && subtab == accuracy)) {
				tab = visuals;
				subtab = accuracy;
			}

			if (elements::icon_tab("C", "world", tab == visuals && subtab == exploits)) {
				tab = visuals;
				subtab = exploits;
			}

			ImGui::Dummy(ImVec2(0, 4));

			// MOVEMENT CATEGORY
			elements::category_title("MOVEMENT");

			if (elements::icon_tab("D", "general", tab == settings && subtab == general)) {
				tab = settings;
				subtab = general;
			}

			if (elements::icon_tab("S", "recorder", tab == settings && subtab == accuracy)) {
				tab = settings;
				subtab = accuracy;
			}

			if (elements::icon_tab("U", "studio", tab == settings && subtab == exploits)) {
				tab = settings;
				subtab = exploits;
			}

			ImGui::Dummy(ImVec2(0, 4));

			// MISC CATEGORY
			elements::category_title("MISC");

			if (elements::icon_tab("E", "misc", tab == configs && subtab == general)) {
				tab = configs;
				subtab = general;
			}

			ImGui::Dummy(ImVec2(0, 4));

			// TOOLS CATEGORY
			elements::category_title("TOOLS");

			if (elements::icon_tab("H", "practice", tab == tools && subtab == general)) {
				tab = tools;
				subtab = general;
			}

			if (elements::icon_tab("X", "cvars", tab == tools && subtab == accuracy)) {
				tab = tools;
				subtab = accuracy;
			}

			ImGui::Dummy(ImVec2(0, 4));

			// SKINS CATEGORY
			elements::category_title("SKINS");

			if (elements::icon_tab("G", "knives", tab == skins && subtab == general)) {
				tab = skins;
				subtab = general;
			}

			if (elements::icon_tab("T", "gloves", tab == skins && subtab == accuracy)) {
				tab = skins;
				subtab = accuracy;
			}

			if (elements::icon_tab("P", "agents", tab == skins && subtab == exploits)) {
				tab = skins;
				subtab = exploits;
			}

			ImGui::Dummy(ImVec2(0, 4));

			// CONFIG CATEGORY
			elements::category_title("CONFIG");

			if (elements::icon_tab("I", "configs", tab == scripts && subtab == general)) {
				tab = scripts;
				subtab = general;
			}

			if (elements::icon_tab("Q", "servers", tab == scripts && subtab == accuracy)) {
				tab = scripts;
				subtab = accuracy;
			}

			if (elements::icon_tab("V", "beta", tab == scripts && subtab == exploits)) {
				tab = scripts;
				subtab = exploits;
			}

		} ImGui::EndGroup();

		// === CONTENT AREA ===
		switch (tab) {

		case ragebot:
			switch (subtab) {
			case general:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("aimbot", ImVec2(374, 504)); {

					ImGui::Checkbox("aimbot", &c::aimbot::aimbot);
					ImGui::SameLine();
					SubFeature("aimbot_settings", {
					{ "silent aim",  &c::aimbot::aimbot_silent },
					{ "field of view",        &c::aimbot::aimbot_fov, 0, 180 },
					{ "autowall min dmg",        &c::aimbot::aimbot_autowall_dmg, 1, 100 },
						});
					ImGui::SameLine();
					ImGui::Keybind(("##aimbot key"), &c::aimbot::aimbot_key, &c::aimbot::aimbot_key_s);

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("target", ImVec2(374, 444)); {

					ImGui::Combo("hitbox", &c::aimbot::aimbot_hitbox, "head\0neck\0chest\0pelvis", IM_ARRAYSIZE("head\0neck\0chest\0pelvis"));

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("", ImVec2(374, 308)); {

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("", ImVec2(374, 368)); {



				}
				e_elements::end_child();
				break;

			case accuracy:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("backtrack", ImVec2(374, 504)); {

					ImGui::Checkbox("backtrack", &c::backtrack::backtrack);
					ImGui::SameLine();
					SubFeature("backtrack_settings", {
						{ "extended backtrack", &c::backtrack::fake_latency },
						{ "backtrack distance", &c::backtrack::fake, 0.f, 1.f }
						});

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("", ImVec2(374, 444)); {


				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("", ImVec2(374, 308)); {



				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("", ImVec2(374, 368)); {



				}
				e_elements::end_child();
				break;
			}
			break;

			// Add other tab cases here (visuals, settings, configs, etc.)
		case visuals:
			switch (subtab) {
			case general:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("main", ImVec2(374, 504)); {

					ImGui::Checkbox(("enable esp"), &c::visuals::enable_visuals);
					ImGui::SameLine();
					SubFeature("esp_settings", {
						{ "fade animation", &c::visuals::fade_animation },
						{ "enemy box", &c::visuals::playerbox },
						{ "enemy name", &c::visuals::playername },
						{ "enemy health", &c::visuals::healthesp },
						{ "enemy weapon", &c::visuals::playerweapon },
						{ "enemy skeleton", &c::visuals::skeleton },
						{ "healthbar custom color", &c::visuals::override_bar },
						{ "healthbar gradient", &c::visuals::gradient_bar }
						});

					if (c::visuals::enable_visuals) {
						ImGui::Text("enemy box");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##player box color"), c::visuals::playerbox_color, no_alpha);
						static const char* outline_type[2] = { "outter", "inner" };
						ImGui::Text("enemy name");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##Name Color"), c::visuals::playername_color, no_alpha);

						ImGui::Text("enemy health");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##hp color"), c::visuals::health_color, no_alpha);

						ImGui::Text("enemy weapon");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##player wpn color"), c::visuals::player_weapon_color, no_alpha);
						static const char* weapon_type[2] = { "text", "icon" };
						if (c::visuals::playerweapon) {
							///ImGui::Combo(("##wpntype"), weapon_type, c::visuals::player_weapon_type, );
						}

						ImGui::Text("enemy skeleton");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##skeleton clr"), c::visuals::skeletonesp_clr, no_alpha);

						if (c::visuals::healthesp_s == 0) {
							ImGui::Text("healthbar custom");
							ImGui::SameLine();
							ImGui::ColorEdit4(("##bar color"), c::visuals::health_bar, no_alpha);
							if (c::visuals::override_bar) {

								ImGui::Text("healthbar gradient");
								ImGui::SameLine();
								ImGui::ColorEdit4(("##bar gradient color"), c::visuals::health_bar_gradient, no_alpha);
							}
						}

						if (c::visuals::playerbox) {
							ImGui::Combo(("box type"), &c::visuals::boxtype, "filled\0corner");
							if (c::visuals::boxtype == 1) {
								ImGui::Text("corner length");
								ImGui::SliderFloat("##crnerlenght", &c::visuals::corner_lenght, 1.55f, 20.0f, " %.2f");

							}
						}

						if (c::visuals::healthesp) {
							ImGui::Combo(("health type"), &c::visuals::healthesp_s, "bar\0under player\0only text");
						}
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("chams", ImVec2(374, 444)); {

					static const char* materials[] = { "regular", "flat", "glow", "chrome", "plastic", "glass", "crystal", "animated", "bubble", "metallic" };
					static const char* chams_overlay_types[] = { "regular", "flat", "glow", "chrome", "plastic", "glass", "crystal", "animated", "bubble", "metallic" };
					ImGui::Checkbox(("enable chams"), &c::visuals::enable_chams);
					ImGui::SameLine();
					SubFeature("chams_settings", {
						{ "visible chams", &c::chams::visible_chams },
						{ "invisible chams", &c::chams::invisible_chams },
						{ "visible wireframe", &c::chams::visible_wireframe },
						{ "invisible wireframe", &c::chams::invisible_wireframe },
						{ "remove original model", &c::visuals::remove_original_model },
						{ "cham lights", &c::visuals::cham_lights },
						{ "overlay chams", &c::chams::visible_chams_ov },
						{ "overlay wireframe", &c::chams::visible_wireframe_ov },
						});
					ImGui::Text("visible chams");
					ImGui::SameLine();
					ImGui::ColorEdit4(("##visible clr"), c::chams::visible_chams_clr, w_alpha);
					ImGui::Text("invisible chams");
					ImGui::SameLine();
					ImGui::ColorEdit4(("##invisible clr"), c::chams::invisible_chams_clr, w_alpha);
					ImGui::Text("overlay chams");
					ImGui::SameLine();
					ImGui::ColorEdit4(("##ovcolor0"), c::chams::visible_chams_clr_ov, w_alpha);

					ImGui::Combo(("target material"), &c::chams::cham_type, materials, IM_ARRAYSIZE(materials));
					ImGui::Combo(("overlay material"), &c::chams::cham_type_overlay, chams_overlay_types, IM_ARRAYSIZE(chams_overlay_types));

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("glow", ImVec2(374, 308)); {

					ImGui::Checkbox(("enable player glow"), &c::visuals::glow);
					ImGui::SameLine();
					if (c::visuals::glow) {
						ImGui::ColorEdit4(("##glowivs"), c::visuals::glow_visible_clr, w_alpha);
						ImGui::SameLine();
						ImGui::ColorEdit4(("##glowinv"), c::visuals::glow_invisible_clr, w_alpha);
						ImGui::Text("glow style");
						ImGui::Combo((""), &c::visuals::glow_style, "default\0rim\0outline\0pulse\0");
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("backtrack", ImVec2(374, 368)); {

					static const char* materials[] = { "regular", "flat", "glow", "chrome", "plastic", "glass", "crystal", "animated", "bubble", "metallic" };
					ImGui::Checkbox(("enable backtrack chams"), &c::visuals::enable_backtrack_chams);
					ImGui::SameLine();
					SubFeature("backtrack_chams_settings", {
						{ "backtrack visible chams", &c::chams::backtrack_chams },
						{ "backtrack invisible chams", &c::chams::backtrack_chams_invisible },
						{ "bactrack cham lights", &c::visuals::backtrack_cham_lights },
						{ "backtrack gradient", &c::chams::backtrack_chams_gradient },
						{ "backtrack dot", &c::visuals::backtrack_dot },
						});
					ImGui::Text("backtrack dot");
					ImGui::SameLine();
					ImGui::ColorEdit4("backtrack dot", c::visuals::backtrack_dot_clr, w_alpha);

					ImGui::Text("backtrack chams");
					ImGui::SameLine();
					ImGui::ColorEdit4(("##playerbt1"), c::chams::backtrack_chams_clr2, w_alpha);
					ImGui::Text("backtrack gradient");
					ImGui::SameLine();
					ImGui::ColorEdit4(("##playerbt2"), c::chams::backtrack_chams_clr1, w_alpha);

					ImGui::Combo(("backtrack material"), &c::chams::cham_type_bt, materials, IM_ARRAYSIZE(materials));


				}
				e_elements::end_child();
				break;

			case accuracy:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("displays", ImVec2(374, 504)); {

					ImGui::Text(("menu accent"));
					ImGui::SameLine();
					ImGui::ColorEdit4(("##main_theme"), menu::menu_accent, no_alpha);
					ImGui::Checkbox("watermark", &c::movement::billware_wm);
					ImGui::Combo("watermark type", &c::movement::watermark_type, "dna\0clarity\0");
					ImGui::Checkbox("spotify display", &c::misc::show_spotify_currently_playing);
					ImGui::Checkbox("style points system", &c::movement::points_system);
					ImGui::SameLine();
					SubFeature("stylepoints_settings", {
					  { "show background",      &c::visuals::style_points_background },
					  { "show combos",      &c::visuals::style_points_combo },
						});
					ImGui::Checkbox("spectators list", &c::misc::spectators_list);
					ImGui::Checkbox("iwebz crosshair", &c::misc::sniper_crosshair);
					ImGui::Checkbox("clantag", &c::misc::misc_clantag_spammer);
					ImGui::Checkbox(("display damage"), &c::visuals::damage_indicator);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##damage IndiClr"), c::visuals::damage_indi_clr, no_alpha);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##damage IndiClrshadow"), c::visuals::damage_indi_clr_shadow, no_alpha);
					ImGui::Checkbox("edgebug chat print", &c::movement::edge_bug_detection_printf);
					ImGui::Checkbox("jumpbug chat print", &c::movement::jump_bug_detection_printf);
					ImGui::Checkbox("pixelsurf chat print", &c::movement::pixel_surf_detection_printf);

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("indicators", ImVec2(374, 444)); {

					ImGui::Checkbox(("velocity indicator"), &c::movement::velocity_indicator);
					if (c::movement::velocity_indicator_type == 0) {
						ImGui::SameLine();
						SubFeature("velocity_classic_settings", {
							{ "show pre",      &c::movement::velocity_indicator_show_pre },
							{ "disable takeoff on ground",      &c::movement::velocity_indicator_disable_ong_show_pre },
							{ "velocity fade",      &c::movement::velocity_indicator_custom_color },
							});
					}
					else if (c::movement::velocity_indicator_type == 1) {
						ImGui::SameLine();
						SubFeature("velocity_anim_settings", {
							{ "show ghost",      &c::movement::velocity_indicator_show_ghost },
							});
					}
					ImGui::Combo("velocity type", &c::movement::velocity_indicator_type, "classic\0animation\0");
					if (c::movement::velocity_indicator_type == 0) {
						ImGui::Text("velocity colors");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color"), c::movement::velocity_indicator_custom_clr, w_alpha);
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color2"), c::movement::velocity_indicator_fade_clr3, w_alpha);
					}
					else if (c::movement::velocity_indicator_type == 1) {
						ImGui::Text("velocity colors");
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color"), c::movement::velocity_indicator_custom_clr, w_alpha);
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color2"), c::movement::velocity_indicator_fade_clr3, w_alpha);
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color3"), c::movement::shadow_indicator_custom_clr, w_alpha);
						ImGui::SameLine();
						ImGui::ColorEdit4(("##static color4"), c::movement::shadow_indicator_custom_clr2, w_alpha);
					}


					ImGui::Checkbox(("stamina indicator"), &c::movement::stamina_indicator);
					ImGui::SameLine();
					SubFeature("stamina_settings", {
						{ "show pre",      &c::movement::stamina_indicator_show_pre },
						{ "disable takeoff on ground",      &c::movement::stamina_indicator_disable_ong_show_pre },
						{ "stamina animation",      &c::movement::stamina_indicator_fade }
						});
					ImGui::SameLine();
					ImGui::ColorEdit4(("##stamina indicator color"), c::movement::stamina_indicator_clr, w_alpha);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##stamina indicator intr color"), c::movement::stamina_fade_clr, w_alpha);

					ImGui::Checkbox(("movement indicators"), &c::movehelp::indicatorEnable);
					ImGui::SameLine();
					SubFeature("movementindi_settings", {
						{ "edgebug indicator",      &c::movehelp::edgebug_indicator },
						{ "jumpbug indicator",      &c::movehelp::jumpbug_indicator },
						{ "edgejump indicator",      &c::movehelp::edgejump_indicator },
						{ "longjump indicator",      &c::movehelp::longjump_indicator },
						{ "minijump indicator",      &c::movehelp::minijump_indicator },
						{ "fireman indicator",      &c::movehelp::fireman_indicator },
						{ "pixelsurf indicator",      &c::movehelp::pixelsurf_indicator },
						{ "ast indicator",      &c::movehelp::ast_indicator },
						{ "fade speed",      &c::movement::indicators_fading_speed, 0.f, 5.f },

						});
					//ImGui::Checkbox(("headbang indicator"), &c::movehelp::headbang_indicator);
					//ImGui::Checkbox(("airstuck indicator"), &c::movement::airstuck_indicator);

					ImGui::Checkbox(("velocity graph"), &c::movement::velocity_graph);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##graphcolor"), c::movement::velocity_graph_color, no_alpha);
					if (c::movement::velocity_graph) {
						ImGui::Checkbox(("show landed velocity"), &c::movement::velocity_graph_show_landed_speed);
						ImGui::Checkbox(("show edgebugs"), &c::movement::velocity_graph_show_edge_bug);
						ImGui::Checkbox(("show jumpbugs"), &c::movement::velocity_graph_show_jump_bug);
						ImGui::Checkbox(("show pixelsurfs"), &c::movement::velocity_graph_show_pixel_surf);

					}

					ImGui::Checkbox(("stamina graph"), &c::movement::stamina_graph);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##stamcolrgraph"), c::movement::stamina_graph_color, no_alpha);

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("view", ImVec2(374, 308)); {

					ImGui::Checkbox("camera fix", &c::visuals::camera_fix_enabled);
					ImGui::SameLine();
					SubFeature("camera_fix_settings", {
						{ "camera smoothing", &c::visuals::camrea_enable_smoothing },
						{ "camera angle fix", &c::visuals::camera_angle_fix },
						{ "camera teleport fix", &c::visuals::camera_teleport_fix },
						});

					static const char* blur_mode[] = { "low", "medium", "high" };
					ImGui::Checkbox("motion blur", &c::visuals::mbenabled);
					ImGui::SameLine();
					SubFeature("motionblur_settings", {
						{ "strength", &c::visuals::mbstrength, 0.f, 10.f },
						{ "rotation intensity", &c::visuals::mbrotationIntensity, 0.f, 8.f },
						{ "falling intensity", &c::visuals::mbfallingIntensity, 0.f, 8.f },
						{ "falling min", &c::visuals::mbfallingMin, 0.f, 50.f },
						{ "falling max", &c::visuals::mbfallingMax, 0.f, 50.f },
						});

					if (c::visuals::mbenabled) {
						ImGui::Combo(("video adapter type"), &c::visuals::mb_video_adapter, "AMD\0NVIDIA\0INTEL\0");
					}

					ImGui::Checkbox("custom aspect ratio", &c::misc::aspect_ratio);
					ImGui::SameLine();
					SubFeature("aspectratio_settings", {
						{ "aspect ratio", &c::misc::aspect_ratio_amount, 0.f, 2.f }
						});

					ImGui::Checkbox(("custom fov"), &c::misc::enable_fov);
					ImGui::SameLine();
					SubFeature("customfov_settings", {
						{ "field of view", &c::misc::field_of_view, 0.f, 60.f }
						});

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("view model", ImVec2(374, 368)); {

					ImGui::Checkbox(("custom view model"), &c::misc::view_model);
					ImGui::SameLine();
					SubFeature("viewmodel_settings", {
						{ "x", &c::misc::view_model_x, -10.f, 10.f },
						{ "y", &c::misc::view_model_y, -10.f, 10.f },
						{ "z", &c::misc::view_model_z, -10.f, 10.f },
						});

				}
				e_elements::end_child();
				break;

			case exploits:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("sky", ImVec2(374, 504)); {

					ImGui::Checkbox("custom sun", &c::visuals::custom_sun);
					if (c::visuals::custom_sun)
					{
						ImGui::Text(("sun direction x"));
						ImGui::SliderInt(("##sunx"), &c::visuals::custom_sun_x, 0, 500);
						ImGui::Text((""));
						ImGui::Text(("sun direction y"));
						ImGui::SliderInt(("##suny"), &c::visuals::custom_sun_y, 0, 500);
						ImGui::Text((""));
						ImGui::Text(("sun shadow distance"));
						ImGui::SliderInt(("##sundist"), &c::visuals::custom_sun_dist, 0, 1000);
					}
					ImGui::Text(("skybox changer"));
					ImGui::Combo((""), &c::visuals::skybox, "none\0tibet\0baggage\0italy\0aztec\0vertigo\0daylight\0daylight-2\0clouds\0cloulds-2\0gray\0clear\0canals\0cobblestone\0assault\0cloudsdark\0night\0nigh2\0nightflat\0dusty\0rainy");

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("weather", ImVec2(374, 444)); {

					ImGui::Checkbox("custom fog", &c::visuals::fog);
					if (c::visuals::fog) {
						ImGui::SameLine();
						ImGui::ColorEdit4(("##fog color1"), c::visuals::fog_color, w_alpha);
						ImGui::Text(("fog distance"));
						ImGui::SliderInt(("##fog distance"), &c::visuals::fog_distance, 0, 2500);
						ImGui::Text((""));
						ImGui::Text(("fog density"));
						ImGui::SliderInt(("##fog density"), &c::visuals::fog_density, 0, 100);
						ImGui::Text((""));
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("world", ImVec2(374, 308)); {

					ImGui::Checkbox("world modulate", &c::visuals::world_modulate);
					ImGui::SameLine();
					ImGui::ColorEdit4(("##wrldmodulate"), c::visuals::world_color, no_alpha);

					ImGui::Checkbox("shader modulate", &c::visuals::custom_shaders);

					ImGui::Checkbox("dark mode", &c::visuals::enable_darkmode);
					if (c::visuals::enable_darkmode)
						ImGui::SliderFloat(("##darkmode amount"), &c::visuals::darkmode_val, 0.f, 1.f);

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("removals", ImVec2(374, 368)); {

					ImGui::Checkbox(("removals"), &c::visuals::enable_removals);
					ImGui::SameLine();
					SubFeature("removal_settings", {
					  { "remove smoke",  &c::visuals::nosmoke },
					  { "remove flash",  &c::visuals::change_flashalpha },
					  { "remove swayscale",  &c::misc::swayscale },
					  { "remove shadows",  &c::visuals::remove_shadows },
					  { "remove post processing",  &c::visuals::remove_post_processing },
					  { "remove panorama blur",  &c::visuals::remove_panorama_blur },
						});

				}
				e_elements::end_child();
				break;
			}
			break;

		case settings:
			switch (subtab) {
			case general:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("general", ImVec2(374, 504)); {

					ImGui::Checkbox("auto bunnyhop", &c::movement::bhop);
					ImGui::SameLine();
					SubFeature("bhop_settings", {
					  { "perfect bunnyhop",  &c::movement::fast_bhop },
					  { "fast duck",  &c::movement::fastduck },
					  { "fast stop",  &c::movement::fast_stop },
						});

					ImGui::Checkbox("auto edgejump", &c::movement::edgejump);
					ImGui::SameLine();
					SubFeature("edgejump_settings", {
					  { "edgejump 252 pre",  &c::movement::ej_252pre },
					  { "edgejump adaptive nulls",  &c::movement::ej_adaptive_nulls },
					  { "edgejump on ladder",  &c::movement::edge_jump_on_ladder },
						});
					ImGui::SameLine();
					ImGui::Keybind(("edgejump key"), &c::movement::edgejump_key, &c::movement::edgejump_key_s);

					ImGui::Checkbox("auto longjump", &c::movement::long_jump_on_edge);
					ImGui::SameLine();
					SubFeature("longjump_settings", {
						{ "longjump 252 pre",  &c::movement::lj_252pre },
						{ "longjump adaptive nulls",  &c::movement::lj_adaptive_nulls },
					  { "longjump on ladder",  &c::movement::ladderjump_lj },
					  { "unduck after longjump",  &c::movement::unduck_after_lj }
						});
					ImGui::SameLine();
					ImGui::Keybind(("ljkey"), &c::movement::long_jump_key, &c::movement::long_jump_key_s);

					ImGui::Checkbox("auto minijump", &c::movement::mini_jump);
					ImGui::SameLine();
					SubFeature("minijump_settings", {
						{ "minijump 252 pre",  &c::movement::mj_252pre },
						{ "minijump adaptive nulls",  &c::movement::mj_adaptive_nulls },
					  { "minijump on ladder",  &c::movement::ladderjump_mj },
					  { "unduck after minijump",  &c::movement::unduck_after_mj }
						});
					ImGui::SameLine();
					ImGui::Keybind(("minijump key"), &c::movement::mini_jump_key, &c::movement::mini_jump_key_s);

					ImGui::Checkbox("auto fireman", &c::movement::fireman);
					ImGui::SameLine();
					SubFeature("fireman_settings", {

						});
					ImGui::SameLine();
					ImGui::Keybind(("fireman key"), &c::movement::fireman_key, &c::movement::fireman_key_s);

					ImGui::Checkbox("auto staminahop", &c::movement::delay_hop);
					ImGui::SameLine();
					SubFeature("staminahop_settings", {
						{ "ticks to wait",        &c::movement::dh_tick, 1, 8 },
						});
					ImGui::SameLine();
					ImGui::Keybind(("staminahop key"), &c::movement::delay_hop_key, &c::movement::delay_hop_key_s);

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("advanced", ImVec2(374, 444)); {

					ImGui::Checkbox(("auto jumpbug"), &c::movement::jump_bug);
					ImGui::SameLine();
					SubFeature("jumpbug_settings", {
						{ "scan time",        &c::movement::jumpbug_scan, 0.0f, 0.4f },
						{ "angle limit",        &c::movement::jumpbug_angle, 0.0f, 1.f },
						});
					ImGui::SameLine();
					ImGui::Keybind(("##jump bug key"), &c::movement::jump_bug_key, &c::movement::jump_bug_key_s);

					ImGui::Checkbox(("auto edgebug"), &c::movement::edge_bug);
					ImGui::SameLine();
					SubFeature("edgebug_settings", {
						{ "edgebug lock mode",  &c::movehelp::edgebug_lock },
						{ "edgebug advanced mode",  &c::movehelp::advanced_edgebug },
						{ "edgebug strafe custom angle", &c::movement::enable_angle_limit },
						{ "edgebug visualization",  &c::movement::visualize_edge_bug },
						{ "scan time",        &c::movehelp::edgebug_scan, 0.0f, 1.0f },
						{ "advanced range",      &c::movement::advanced_radius, 0, 10 },
						{ "lock amount",      &c::movement::edge_bug_lock_amount, 0.f, 100.f },
						{ "angle limit",      &c::movement::edgebug_angle_limit, 0.f, 45.f },
						});
					ImGui::SameLine();
					ImGui::Keybind(("edgebug key"), &c::movement::edge_bug_key, &c::movement::edge_bug_key_s);

					ImGui::Checkbox(("auto pixelsurf"), &c::movement::pixel_surf);
					ImGui::SameLine();
					SubFeature("pixelsurf_settings", {
						{ "pixelsurf lock mode",  &c::movehelp::pixelsurf_lock },
						 { "scan time",        &c::movement::ps_ticks, 0, 0.4f },
						 { "lock amount",      &c::movement::ps_lock_amount, 0.f, 100.f },
						});

					ImGui::Checkbox(("auto bounce"), &c::movement::auto_bounce);
					ImGui::SameLine();
					SubFeature("bounce_settings", {
						//{ "scan time",        &c::movement::ps_ticks, 0, 0.4f },
						});


					ImGui::Checkbox(("auto headbang"), &c::movement::auto_headbang);
					ImGui::SameLine();
					SubFeature("headbang_settings", {
						//{ "scan time",        &c::movement::ps_ticks, 0, 0.4f },
						});

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("strafes", ImVec2(374, 308)); {

					ImGui::Checkbox("nulls", &c::movement::null_strafing);
					ImGui::Checkbox("strafe helper", &c::movement::enable_strafe_type);
					if (c::movement::enable_strafe_type) {
						ImGui::Combo(("strafe types"), &c::movement::strafe_type, "perfect\0frame perfect\0legit autostrafe\0silent autostrafe\0robotic");
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("utility", ImVec2(374, 368)); {

					ImGui::Checkbox("mouse fix", &c::misc::mouse_fix);
					ImGui::SameLine();
					SubFeature("mouse_fix_settings", {
						//{ "freelook ",  &c::movehelp::free_angle }
						});

					ImGui::Checkbox("pixelsurf fix", &c::movement::pixel_surf_fix);
					ImGui::SameLine();
					SubFeature("pixelsurf_fix_settings", {
						//{ "freelook ",  &c::movehelp::free_angle }
						});

					ImGui::Checkbox("auto align", &c::movement::auto_align);
					ImGui::SameLine();
					SubFeature("alignment_settings", {
						{ "freelook ",  &c::movehelp::free_angle }
						});

					ImGui::Checkbox(("headbang align"), &c::movement::marker_align);
					ImGui::SameLine();
					SubFeature("headbang_align_settings", {
						 { "simulate ticks",        &c::movement::alignment_rage, 0.f, 0.4f, },
						});

					ImGui::Checkbox(("avoid collisions"), &c::movement::auto_duck_collision);
					ImGui::SameLine();
					SubFeature("avoidcollision_settings", {

						});

				}
				e_elements::end_child();
				break;

			case accuracy:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("general", ImVec2(374, 504)); {

					ImGui::Checkbox(("enable recorder##mr"), &c::misc::movement_rec);
					ImGui::Checkbox(("show recording line"), &c::misc::movement_rec_show_line);
					ImGui::Checkbox(("original playback viewangles"), &c::misc::movement_rec_lockva);
					ImGui::Text("smoothing");
					ImGui::SliderFloat(("##smoothing"), &c::misc::movement_rec_smoothing, 1.f, 100.f, ("%.3f"));
					//ImGui::Text("angle type");
					//ImGui::Combo(("##cltype"), &c::misc::movement_rec_angletype, "default\0sideways\0backwards");

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("records", ImVec2(374, 444)); {

					recorder->draw();

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("", ImVec2(374, 308)); {



				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("keybinds", ImVec2(374, 368)); {

					ImGui::Text(("start recording"));
					ImGui::Keybind(("start rec"), &c::misc::movement_rec_keystartrecord, &c::misc::movement_rec_keystartrecord_s);

					ImGui::Text(("start playback"));
					ImGui::Keybind(("start playback"), &c::misc::movement_rec_keystartplayback, &c::misc::movement_rec_keystartplayback_s);

					ImGui::Text(("stop recording"));
					ImGui::Keybind(("stop rec"), &c::misc::movement_rec_keystoprecord, &c::misc::movement_rec_keystoprecord_s);

					ImGui::Text(("stop playback"));
					ImGui::Keybind(("stop playback"), &c::misc::movement_rec_keystopplayback, &c::misc::movement_rec_keystopplayback_s);

					ImGui::Text(("clear recording"));
					ImGui::Keybind(("clear recording"), &c::misc::movement_rec_keyclearrecord, &c::misc::movement_rec_keyclearrecord_s);

				}
				e_elements::end_child();
				break;

			case exploits:
				ImGui::SetCursorPos({ 226, 16 });
				e_elements::begin_child("general", ImVec2(374, 504)); {

					ImGui::Checkbox(("route studio"), &c::movement::pscalc_enable);
					ImGui::SameLine();
					SubFeature("routestudio_settings", {
						 { "enable scan for pixelsurf points",        &c::assist::assist_enable_pixelsurf_points },
						 { "enable scan for bounce points",        &c::assist::assist_enable_bounce_points },
						 { "enable scan for headbang points",        &c::assist::assist_enable_headbang_points },
						 { "enable scan for floor points",        &c::assist::assist_enable_floor_points },
						 { "enable scan for ceiling points",        &c::assist::assist_enable_ceiling_points },
						});

					ImGui::Checkbox(("route studio draw"), &c::movement::pscalc_enable_draw);

					ImGui::Checkbox(("pixelsurf jump"), &c::assist::pixelsurf_assist);
					ImGui::SameLine();
					SubFeature("pixelsurfassist_settings", {
						 { "pixelsurf assist display",        &c::assist::assist_render },
						 { "simple display",        &c::assist::simple_display },
						 { "pixelsurf assist stamina hops",        &c::assist::assist_broke_hop },
						 { "assist scan time",        &c::assist::pixelsurf_assist_ticks, 0.f, 1.f, },
						 { "maximum allowed stamina amount",        &c::assist::assist_stamina_value, 0.f, 100.f, },
						 //{ "maximum allowed points",        &c::assist::pixelsurf_max_points, 1, 6, },
						});
					ImGui::SameLine();
					ImGui::Keybind(("pixelsurf assist key"), &c::assist::pixelsurf_assist_key, &c::assist::pixelsurf_assist_key_s);

					ImGui::Checkbox(("chain markers"), &c::movement::chain_assist);

					ImGui::Text(("draw point"));
					ImGui::Keybind(("draw point"), &c::movement::pscalc_drawpoint_key, &c::movement::pscalc_drawpoint_key_s);
					ImGui::Text(("calculate point"));
					ImGui::Keybind(("calculate point"), &c::assist::ps_calc_route_key, &c::assist::ps_calc_route_key_s);
					ImGui::Text(("clear point"));
					ImGui::Keybind(("clear point"), &c::movement::ps_calc_selection_clear_key, &c::movement::ps_calc_selection_clear_key_s);
					ImGui::Text(("clear all points"));
					ImGui::Keybind(("clear all points"), &c::movement::ps_calc_selection_clear_all_key, &c::movement::ps_calc_selection_clear_all_key_s);

					if (ImGui::Button("load", ImVec2(-1, 20)))
					{
						RouteStudio.load_markers();

						if (interfaces::engine->is_in_game())
						{
							if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#dna#_print_marker_loaded");
						}
					}

					if (ImGui::Button("save", ImVec2(-1, 20)))
					{
						RouteStudio.save_markers();

						if (interfaces::engine->is_in_game())
						{
							if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#dna#_print_marker_saved");
						}
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 16 });
				e_elements::begin_child("marker editor", ImVec2(374, 444)); {

					if (RouteStudio.route_markers.empty()) {
						ImGui::Text("No markers to edit.");
					}
					else {
						for (int i = 0; i < (int)RouteStudio.route_markers.size(); ++i) {
							auto& marker = RouteStudio.route_markers[i];

							ImGui::PushID(i);
							ImGui::Separator();

							ImGui::Text("marker %d: %s", i, marker.name.c_str());

							// Marker type selection
							const char* types[] = { "floor","ceiling","bounce","pixelsurf","headbang" };
							int type_idx = static_cast<int>(marker.type);
							if (ImGui::Combo("type", &type_idx, types, IM_ARRAYSIZE(types))) {
								marker.type = static_cast<RouteAssist::jumptype_t>(type_idx);
								marker.name = types[type_idx];
							}

							// Name edit
							char name_buf[64];
							strncpy(name_buf, marker.name.c_str(), sizeof(name_buf));
							if (ImGui::InputText("name", name_buf, sizeof(name_buf))) {
								marker.name = name_buf;
							}

							// Velocity edit
							ImGui::InputFloat3("velocity", &marker.velocity.x);

							// Delete button
							if (ImGui::Button("delete marker")) {
								RouteStudio.route_markers.erase(RouteStudio.route_markers.begin() + i);
								if (RouteStudio.current_marker_index >= i)
									RouteStudio.current_marker_index--;
								ImGui::PopID();
								break; // early break because vector size changed
							}

							ImGui::PopID();
						}
					}

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 226, 532 });
				e_elements::begin_child("", ImVec2(374, 308)); {

				}
				e_elements::end_child();

				ImGui::SetCursorPos({ 610, 472 });
				e_elements::begin_child("", ImVec2(374, 368)); {



				}
				e_elements::end_child();
				break;
			}
			break;

			case tools:
				switch (subtab) {
				case general:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("general", ImVec2(374, 504)); {

						ImGui::Checkbox("practice", &c::misc::practice);

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("offline", ImVec2(374, 444)); {

						if (ImGui::Button("start practice server")) {
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

						if (ImGui::Button("awp & p250")) {
							interfaces::engine->execute_cmd("give weapon_awp");
							interfaces::engine->execute_cmd("give weapon_p250");
							interfaces::engine->execute_cmd("give weapon_smokegrenade");
						}

						if (ImGui::Button("awp & deagle")) {
							interfaces::engine->execute_cmd("give weapon_awp");
							interfaces::engine->execute_cmd("give weapon_deagle");
							interfaces::engine->execute_cmd("give weapon_smokegrenade");
						}

						if (ImGui::Button("ak47 & p250")) {
							interfaces::engine->execute_cmd("give weapon_ak47");
							interfaces::engine->execute_cmd("give weapon_p250");
							interfaces::engine->execute_cmd("give weapon_smokegrenade");
						}

						if (ImGui::Button("ak47 & deagle")) {
							interfaces::engine->execute_cmd("give weapon_ak47");
							interfaces::engine->execute_cmd("give weapon_deagle");
							interfaces::engine->execute_cmd("give weapon_smokegrenade");
						}

						if (ImGui::Button("inf ammo")) {
							interfaces::engine->execute_cmd("sv_infinite_ammo 1");
						}

						if (ImGui::Button("max cash")) {
							interfaces::engine->execute_cmd("impulse 101");
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("display", ImVec2(374, 308)); {

						ImGui::Checkbox("practice window", &c::misc::practice_window);

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("keybinds", ImVec2(374, 368)); {

						ImGui::Text(("save position"));
						ImGui::Keybind(("save pos"), &c::misc::savepos, &c::misc::savepos_s);
						ImGui::Text(("load position"));
						ImGui::Keybind(("load pos"), &c::misc::loadpos, &c::misc::loadpos_s);
						ImGui::Text(("next position"));
						ImGui::Keybind(("next pos"), &c::misc::nextkey, &c::misc::nextkey_s);
						ImGui::Text(("previous position"));
						ImGui::Keybind(("prev pos"), &c::misc::prevkey, &c::misc::prevkey_s);
						ImGui::Text(("undo position"));
						ImGui::Keybind(("undo pos"), &c::misc::undokey, &c::misc::undokey_s);

					}
					e_elements::end_child();
					break;

				case accuracy:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("convar editor", ImVec2(374, 504)); {

						ImGui::Text("convar list");

						// --- Build list of convars only once ---
						if (!initialized)
						{
							initialized = true;
							//convar* head = interfaces::console->get_convar("sv_cheats");
							convar* head = **reinterpret_cast<convar***>(reinterpret_cast<std::uintptr_t>(interfaces::console) + 0x34);
							if (head)
							{
								for (convar* c = head; c != nullptr; c = c->next)
								{
									if (c && c->name)
										convar_list.push_back(c);
								}
							}
						}

						// --- Draw list ---
						if (ImGui::ListBoxHeader("##convar_list", ImVec2(200, 200)))
						{
							for (int i = 0; i < convar_list.size(); i++)
							{
								convar* cvar = convar_list[i];
								if (!cvar || !cvar->name || !cvar->string)
									continue; // skip invalid or no value

								if (cvar->name[0] == '\0' || std::all_of(cvar->name, cvar->name + strlen(cvar->name), isspace))
									continue;

								if (strncmp(cvar->name, "npc_", 4) == 0 || strncmp(cvar->name, "ai", 2) == 0 || strncmp(cvar->name, "budda", 5) == 0)
									continue;

								if (filter.PassFilter(cvar->name))
								{
									bool selected = (selected_index == i);
									ImGui::PushID(i);

									if (ImGui::Selectable(cvar->name, selected))
									{
										selected_index = i;
										const char* val_to_copy = (cvar->string && *cvar->string)
											? cvar->string
											: (cvar->default_value ? cvar->default_value : "");
										strncpy(new_value, val_to_copy, sizeof(new_value));
										new_value[sizeof(new_value) - 1] = '\0';
									}

									ImGui::PopID();
									if (selected)
										ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::ListBoxFooter();
						}

						ImGui::Text("search:");
						filter.Draw("##filter_convar");

						ImGui::Separator();

						// --- Selected ConVar details ---
						if (selected_index != -1 && selected_index < convar_list.size())
						{
							convar* selected = convar_list[selected_index];
							if (!selected) return;

							ImGui::Text("name: %s", selected->name);
							ImGui::Text("default: %s", selected->default_value ? selected->default_value : "none");
							ImGui::Text("current: %s", selected->string ? selected->string : "none");

							ImGui::InputText("new value", new_value, IM_ARRAYSIZE(new_value));

							// Detect numeric types
							bool is_number = true;
							for (const char* c = new_value; *c; c++)
							{
								if ((*c < '0' || *c > '9') && *c != '.' && *c != '-')
								{
									is_number = false;
									break;
								}
							}

							if (ImGui::Button("apply"))
							{
								if (is_number)
								{
									float f = atof(new_value);
									selected->set_value(f);
								}
								else
								{
									selected->set_value(new_value);
								}

								// record changed cvar
								bool exists = false;
								for (auto& entry : c::changed_convars)
								{
									if (_stricmp(entry.name.c_str(), selected->name) == 0)
									{
										entry.value = new_value;
										exists = true;
										break;
									}
								}
								if (!exists)
									c::changed_convars.push_back({ selected->name, new_value });
							}
							ImGui::SameLine();

							// --- New Remove ConVar button ---
							if (ImGui::Button("remove"))
							{
								// Reset convar to default or empty string
								const char* reset_value = (selected->default_value && *selected->default_value)
									? selected->default_value
									: "";
								selected->set_value(reset_value);

								// Remove from changed_convars list
								c::changed_convars.erase(
									std::remove_if(c::changed_convars.begin(), c::changed_convars.end(),
										[&](const auto& entry) { return _stricmp(entry.name.c_str(), selected->name) == 0; }),
									c::changed_convars.end()
								);

								// Reset selection index safely
								selected_index = -1;
								memset(new_value, 0, sizeof(new_value));

								selected->set_value(reset_value);

								// update change list to reflect removal
								for (auto it = c::changed_convars.begin(); it != c::changed_convars.end(); ++it)
								{
									if (_stricmp(it->name.c_str(), selected->name) == 0)
									{
										if (*reset_value == '\0')
											c::changed_convars.erase(it); // fully removed
										else
											it->value = reset_value;    // reset to default
										break;
									}
								}
							}
						}

						// --- Save changed list to global for the second child ---
						static auto* changed_ptr = &c::changed_convars;

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("edited convars", ImVec2(374, 444)); {

						if (c::changed_convars.empty())
						{
							ImGui::TextDisabled("none yet");
						}
						else
						{
							if (ImGui::BeginTable("changed_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
							{
								ImGui::TableSetupColumn("convar");
								ImGui::TableSetupColumn("new value");
								ImGui::TableHeadersRow();

								for (auto& entry : c::changed_convars)
								{
									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									ImGui::TextUnformatted(entry.name.c_str());
									ImGui::TableSetColumnIndex(1);
									ImGui::TextUnformatted(entry.value.c_str());
								}
								ImGui::EndTable();
							}
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;

				case exploits:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("", ImVec2(374, 504)); {

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("", ImVec2(374, 444)); {



					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;
				}
				break;

			case configs:
				switch (subtab) {
				case general:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("", ImVec2(374, 504)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("", ImVec2(374, 444)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;

				case accuracy:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("", ImVec2(374, 504)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("", ImVec2(374, 444)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {



					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;

				case exploits:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("", ImVec2(374, 504)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("", ImVec2(374, 444)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {



					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;
				}
				break;

			case skins:
				switch (subtab) {
				case general:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("models", ImVec2(374, 504)); {

						ImGui::Checkbox(("enable knife models"), &c::skins::knife_changer_enable);
						if (c::skins::knife_changer_enable) {
							ImGui::Combo(("knife models"), &c::skins::knife_changer_model, "default\0bayonet\0m9\0karambit\0bowie\0butterfly\0falchion\0flip\0gut\0huntsman\0shaddow-daggers\0navaja\0stiletto\0talon\0ursus\0default ct\0default t\0skeleton\0");
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("skins", ImVec2(374, 444)); {

						ImGui::Text("knife skin");
						if (ImGui::ListBoxHeader("##knifeskin", ImVec2(-1, 200)))
						{
							for (int i = 0; i < features::skins::parser_skins.size(); i++)
							{
								bool is_selected = (c::skins::knife_changer_vector_paint_kit == i);
								const std::string& name = features::skins::parser_skins[i].name;

								// Filter names
								if (filter1.PassFilter(name.c_str()))
								{
									ImGui::PushID(i); // Use a unique ID based on the index
									if (ImGui::Selectable(name.c_str(), is_selected))
									{
										c::skins::knife_changer_vector_paint_kit = i;
										c::skins::knife_changer_paint_kit = features::skins::parser_skins[i].id;
									}
									ImGui::PopID();

									if (is_selected)
										ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::ListBoxFooter();
							ImGui::Text("search skin");
							filter1.Draw("##filter_skin");
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("refresh", ImVec2(374, 308)); {

						if (ImGui::Button(("update skins"))) {
							if (interfaces::engine->is_in_game() && g::local) {
								features::skins::forcing_update = true;
								menu::fonts_initialized = true;
							}
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("wear", ImVec2(374, 368)); {

						ImGui::Combo(("knife wear"), &c::skins::knife_changer_wear, "factory-new\0minimal-wear\0field-tested\0well-worn\0battle-scarred");

					}
					e_elements::end_child();
					break;

				case accuracy:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("models", ImVec2(374, 504)); {

						ImGui::Checkbox(("enable gloves model"), &c::skins::gloves_endable);
						if (c::skins::gloves_endable) {
							ImGui::Combo("gloves models", &c::skins::gloves_model, "default\0blood\0sport\0slick\0leather\0moto\0speci\0hydra");
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("skins", ImVec2(374, 444)); {

						ImGui::Text(("glove skins"));
						ImGui::ListBoxHeader("##glovesskin", ImVec2(-1, 200)); {
							std::string name = ("elpepe");
							for (int i = 0; i < features::skins::parser_gloves.size(); i++) {
								bool is_selected = (c::skins::gloves_skin == i);

								std::string name = features::skins::parser_gloves[i].name;

								if (filter2.PassFilter(name.c_str())) {

									if (name == features::skins::parser_gloves[i].name)
										ImGui::PushID(std::hash<std::string>{}(features::skins::parser_gloves[i].name)* i);

									if (ImGui::Selectable(features::skins::parser_gloves[i].name.c_str(), &is_selected)) {
										c::skins::gloves_skin = i;
										c::skins::gloves_skin_id = features::skins::parser_gloves[c::skins::gloves_skin].id;
									}

									if (name == features::skins::parser_gloves[i].name)
										ImGui::PopID();

									name = features::skins::parser_gloves[i].name;
								}

								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::ListBoxFooter();
						}
						ImGui::Text(("search skin"));
						filter2.Draw("##filter_skin_gloves");

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("refresh", ImVec2(374, 308)); {

						if (ImGui::Button(("update skins"))) {
							if (interfaces::engine->is_in_game() && g::local) {
								features::skins::forcing_update = true;
								menu::fonts_initialized = true;
							}
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("wear", ImVec2(374, 368)); {

						ImGui::Combo("gloves wear", &c::skins::gloves_wear, "factory-new\0minimal-wear\0field-tested\0well-worn\0battle-scarred");

					}
					e_elements::end_child();
					break;

				case exploits:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("models", ImVec2(374, 504)); {

						ImGui::Checkbox(("enable agents models"), &c::skins::agent_changer);

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("counter terrorist", ImVec2(374, 444)); {

						ImGui::Combo(("counter terrorist"), &c::skins::agent_ct, menu::agents, IM_ARRAYSIZE(menu::agents));

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("terrorist", ImVec2(374, 308)); {

						ImGui::Combo(("terrorist"), &c::skins::agent_t, menu::agents, IM_ARRAYSIZE(menu::agents));

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;
				}
				break;

			case scripts:
				switch (subtab) {
				case general:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("load", ImVec2(374, 504)); {

						ImGui::SetCursorPosX(15);
						if (ImGui::ListBoxHeader(("##configs"), ImVec2(-1, 200))) {
							const std::vector<std::string>& configs_vector = c::configs;
							for (std::size_t i = 0u; i < configs_vector.size(); ++i) {
								std::string const& config_name = configs_vector[i].data();
								if (ImGui::Selectable(config_name.c_str(), i == config_index)) {
									config_index = i;
								}
							}
							ImGui::ListBoxFooter();
						}
						ImGui::Text((""));

						ImGui::SetNextItemWidth(145);
						if (ImGui::Button("Load", ImVec2(-1, 20)))
						{
							c::load(config_index);
							load_convars(config_index);

							if (interfaces::engine->is_in_game())
							{
								if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#dna#_print_loaded");
								//features::skins::forcing_update = true;
							}
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("save", ImVec2(-1, 444)); {

						ImGui::SetCursorPosX(15);
						if (ImGui::ListBoxHeader(("##configs"), ImVec2(-1, 200))) {
							const std::vector<std::string>& configs_vector = c::configs;
							for (std::size_t i = 0u; i < configs_vector.size(); ++i) {
								std::string const& config_name = configs_vector[i].data();
								if (ImGui::Selectable(config_name.c_str(), i == config_index)) {
									config_index = i;
								}
							}
							ImGui::ListBoxFooter();
						}
						ImGui::Text((""));

						// Save Config Button
						ImGui::SetNextItemWidth(145);
						if (ImGui::Button("Save", ImVec2(-1, 20)))
						{
							if (c::configs.empty())
								ImGui::OpenPopup("No Configs To Save");
							else
								ImGui::OpenPopup("Config Save Popup");
						}

						// No config popup
						if (ImGui::BeginPopup("No Configs To Save"))
						{
							ImGui::Text("no configs are available to save.");
							if (ImGui::Button("OK", ImVec2(120, 0)))
								ImGui::CloseCurrentPopup();
							ImGui::EndPopup();
						}

						// Save Confirmation Popup
						if (ImGui::BeginPopup("Config Save Popup"))
						{
							ImGui::Text("Are you sure you want to save the selected config?..");
							static const char* choices[] = { "  yes", "  no" };

							for (int i = 0; i < IM_ARRAYSIZE(choices); i++)
							{
								if (ImGui::Selectable(choices[i]))
								{
									if (i == 0) // Save Confirmation
									{
										c::save(config_index);
										save_convars(config_index);

										if (interfaces::engine->is_in_game())
										{
											if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#billware_print_saved");
										}
									}
								}
							}
							ImGui::EndPopup();
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("refresh", ImVec2(374, 308)); {

						if (ImGui::Button("refresh", ImVec2(-1, 20)))
						{
							c::update_configs();

							if (interfaces::engine->is_in_game())
							{
								if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#dna#_print_refreshed");
							}
						}

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("create", ImVec2(374, 368)); {

						ImGui::SetCursorPosX(15);
						if (ImGui::ListBoxHeader(("##configs"), ImVec2(-1, 200))) {
							const std::vector<std::string>& configs_vector = c::configs;
							for (std::size_t i = 0u; i < configs_vector.size(); ++i) {
								std::string const& config_name = configs_vector[i].data();
								if (ImGui::Selectable(config_name.c_str(), i == config_index)) {
									config_index = i;
								}
							}
							ImGui::ListBoxFooter();
						}
						ImGui::Text((""));

						// Config Creation and Management
						ImGui::NextColumn();
						{
							ImGui::SetNextItemWidth(-1);
							if (ImGui::InputTextWithHint("", "create new?..", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
							{
								c::create_file(buffer);
								memset(buffer, 0, sizeof(buffer));

								if (interfaces::engine->is_in_game())
								{
									if (c::movehelp::push_notice) interfaces::chat_element->chatprintf("#dna#_print_created");
								}
							}

							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("Press enter to create a new config");
							}
						}

					}
					e_elements::end_child();
					break;

				case accuracy:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("servers", ImVec2(374, 504)); {

						if (ImGui::Button("aletheria server"))
							interfaces::engine->execute_cmd("connect aletheria.club:27015");

						if (ImGui::Button("y6 server"))
							interfaces::engine->execute_cmd("connect 62.122.213.57:27015");

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("", ImVec2(374, 444)); {


					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {



					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;

				case exploits:
					ImGui::SetCursorPos({ 226, 16 });
					e_elements::begin_child("general", ImVec2(374, 504)); {
						
						ImGui::Checkbox(("auto airstuck"), &c::movement::airstuck_enabled);
						ImGui::SameLine();
						SubFeature("airstuck_settings", {

							});
						ImGui::SameLine();
						ImGui::Keybind(("airstuck key"), &c::movement::airstuck_key, &c::movement::airstuck_key_s);

						ImGui::Checkbox(("jetpack"), &c::movement::jetpack_to_surf);
						ImGui::SameLine();
						SubFeature("jetpack_settings", {

							});
						ImGui::SameLine();
						ImGui::Keybind(("jetpack key"), &c::movement::jetpack_to_surf_key, &c::movement::jetpack_to_surf_key_s);

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 16 });
					e_elements::begin_child("misc", ImVec2(374, 444)); {

						ImGui::Checkbox(("airstuck indicator"), &c::movehelp::air_indicator);

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 226, 532 });
					e_elements::begin_child("", ImVec2(374, 308)); {

					}
					e_elements::end_child();

					ImGui::SetCursorPos({ 610, 472 });
					e_elements::begin_child("", ImVec2(374, 368)); {



					}
					e_elements::end_child();
					break;
				}
				break;

			}
		}
	ImGui::End();
	ImGui::PopStyleVar();
}

LRESULT CALLBACK WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParam,
	LPARAM longParam
)
{
	// toggle menu
	if (GetAsyncKeyState(VK_INSERT) & 1)
		gui::open = !gui::open;

	return CallWindowProc(
		gui::originalWindowProcess,
		window,
		message,
		wideParam,
		longParam
	);
}


void gui::render_checkpoints() noexcept
{
	if (!features::visuals::has_initalized)
		return;

	if (!c::misc::practice_window)
		return;

	ImGui::SetNextWindowSize({ 450, 350}, ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

	if (ImGui::Begin("Checkpoints", &menu::open, ImGuiWindowFlags_NoDecoration))
	{
		auto draw = ImGui::GetWindowDrawList();
		auto pos = ImGui::GetWindowPos();
		auto size = ImGui::GetWindowSize();
		ImGuiStyle style = ImGui::GetStyle();

		// layout constants
		const float title_h = 35.0f;
		const float title_round = 4.5f;
		const float body_round = 4.5f;
		const float border_round = 5.5f;

		// === base faint background (subtle) ===
		draw->AddRectFilled(
			pos,
			ImVec2(pos.x + size.x, pos.y + size.y),
			ImColor(0.03f, 0.03f, 0.03f, 0.08f),
			border_round
		);

		// === TITLE BOX (solid) ===
		ImVec2 title_tl = pos;
		ImVec2 title_br = ImVec2(pos.x + size.x, pos.y + title_h);

		// === TOP "BLOOD" GRADIENT STRIP ===
		{
			const float grad_width = title_br.x - title_tl.x;
			const float grad_height = 3.0f;
			const float grad_top_y = title_tl.y;
			const float grad_bottom_y = grad_top_y + grad_height;
			const float center_x = title_tl.x + grad_width * 0.5f;
			const int slices = 60;

			for (int i = 0; i < slices; ++i) {
				float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
				float slice_x1 = slice_x0 + (grad_width / slices);
				float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
				float a = 1.f - dist;
				if (a < 0.f) a = 0.f;

				ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * a);
				draw->AddRectFilled(
					ImVec2(slice_x0, grad_top_y),
					ImVec2(slice_x1, grad_bottom_y),
					col, 0.0f
				);
			}
		}

		// === Title text ===
		draw->AddText(ImGui::GetFont(), 18.0f, ImVec2(pos.x + 10, pos.y + 8),
			ImColor(0.8f, 0.8f, 0.8f, 1.0f), "Checkpoints");

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 35); // push down under the title
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15); // push down under the title

		ImGui::Text("Save Position:");
		ImGui::Keybind("##save_pos", &c::misc::savepos, &c::misc::savepos_s);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15); // push down under the title

		ImGui::Text("Load Position:");
		ImGui::Keybind("##load_pos", &c::misc::loadpos, &c::misc::loadpos_s);

	}
	ImGui::End();
	ImGui::PopStyleVar();
}
