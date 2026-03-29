#pragma once
#include "config/config.hpp"
#include "../utils/render/draw.hpp"
#pragma once
#include <d3d9.h>

namespace gui
{

	constexpr int WIDTH = 700;
	constexpr int HEIGHT = 700;

	// show menu?
	inline bool open = true;

	// is menu open?
	inline bool setup;

	// winapi related
	inline HWND window = nullptr;
	inline WNDCLASSEX windowClass = { };
	inline WNDPROC originalWindowProcess = nullptr;

	// dx stuff
	inline LPDIRECT3DDEVICE9 device = nullptr;
	inline LPDIRECT3D9 d3d9 = nullptr;

	bool SetupWindowClass(const char* windowClassName) noexcept;
	void DestroyWindowClass() noexcept;

	bool SetupWindow(const char* windowName) noexcept;
	void DestroyWindow() noexcept;

	bool SetupDirectX() noexcept;
	void DestroyDirectX() noexcept;

	// setup device
	void Setup();

	void SetupMenu(LPDIRECT3DDEVICE9 device) noexcept;
	void Destroy() noexcept;

	void Render() noexcept;
	void render_checkpoints() noexcept;
}
#define IMGUI_DEFINE_MATH_OPERATORS

class ctab
{
public:
	const char* szName = nullptr;
	void (*render_function)() = nullptr;
};

namespace menu {
	inline int menu_key = 0x2D;
	inline bool open = true;
	inline int main_tab = 0;
	inline bool initialized = false;
	inline bool unhook = true;
	inline int indicator_tab = 0;
	inline int indicators_tab = 0;
	inline int chams_tab = 0;
	inline int esp_tab = 0;
	inline int weapon_selection = 0;
	inline int skin_selection = 0;
	inline bool fonts_initialized;
	inline float menu_accent[3]{ 255 / 255.f, 165 / 255.f, 255 / 255.f };
	inline float menu_accent2[3]{ 255 / 255.f, 255 / 255.f, 255 / 255.f };
	inline float menu_accent3[3]{ 242 / 255.f, 242 / 255.f, 242 / 255.f };
	inline float fade_speed = 1.f / 0.25f;
	inline float alpha = 0.f;
	static const auto red = '\x07';
	inline auto color_p = red;
	inline static int m_tabs = 0;
	inline static int font_tab = 0;

	inline ImVec2 menu_pos = ImVec2();
	inline ImVec2 menu_size = ImVec2();

	void load_font_index();
	void render_tab(const char* szTabBar, const ctab* arrTabs, const std::size_t nTabsCount, int* nCurrentTab, const ImVec4& colActive, const ImVec4& colInactive, bool tabEnabled, ImGuiTabBarFlags flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_NoTooltip);
	bool iskeydown(int key);
	bool iskeyup(int key);
	bool checkkey(int key, int keystyle);
	inline std::vector<std::string> get_installed_fonts();
	inline static std::vector<std::string> fonts = get_installed_fonts();

	inline static const char* agents[] = {
		"Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman",
		"Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman",
		"Lieutenant Rex Krikey | SEAL Frogman",
		"Michael Syfers | FBI Sniper",
		"Operator | FBI SWAT",
		"Special Agent Ava | FBI",
		"Markus Delrow | FBI HRT",
		"Sous-Lieutenant Medic | Gendarmerie Nationale",
		"Chem-Haz Capitaine | Gendarmerie Nationale",
		"Chef d'Escadron Rouchard | Gendarmerie Nationale",
		"Aspirant | Gendarmerie Nationale",
		"Officer Jacques Beltram | Gendarmerie Nationale",
		"D Squadron Officer | NZSAS",
		"B Squadron Officer | SAS",
		"Seal Team 6 Soldier | NSWC SEAL",
		"Buckshot | NSWC SEAL",
		"Lt. Commander Ricksaw | NSWC SEAL",
		"'Blueberries' Buckshot | NSWC SEAL",
		"3rd Commando Company | KSK",
		"'Two Times' McCoy | TACP Cavalry",
		"''Two Times' McCoy | USAF TACP",
		"'Primeiro Tenente | Brazilian 1st Battalion",
		"'Cmdr. Mae 'Dead Cold' Jamison | SWAT",
		"'1st Lieutenant Farlow | SWAT",
		"'John 'Van Healen' Kask | SWAT",
		"'Bio-Haz Specialist | SWAT",
		"'Sergeant Bombson | SWAT",
		"'Chem-Haz Specialist | SWAT", //
		"'Lieutenant 'Tree Hugger' Farlow | SWAT",
		"Getaway Sally | The Professionals",
		"Number K | The Professionals",
		"Little Kev | The Professionals",
		"Safecracker Voltzmann | The Professionals",
		"dna Darryl The Strapped | The Professionals",
		"Sir dna Loudmouth Darryl | The Professionals",
		"Sir dna Darryl Royale | The Professionals",
		"Sir dna Skullhead Darryl | The Professionals",
		"Sir dna Silent Darryl | The Professionals",
		"Sir dna Miami Darryl | The Professionals",
		"Street Soldier | Phoenix",
		"Soldier | Phoenix",
		"Slingshot | Phoenix",
		"Enforcer | Phoenix",
		"Mr. Muhlik | Elite Crew",
		"Prof. Shahmat | Elite Crew",
		"Osiris | Elite Crew",
		"Ground Rebel | Elite Crew",
		"The Elite Mr. Muhlik | Elite Crew",
		"Trapper | Guerrilla Warfare",
		"Trapper Aggressor | Guerrilla Warfare",
		"Vypa Sista of the Revolution | Guerrilla Warfare",
		"Col. Mangos Dabisi | Guerrilla Warfare",
		"Arno The Overgrown | Guerrilla Warfare",
		"'Medium Rare' Crasswater | Guerrilla Warfare",
		"Crasswater The Forgotten | Guerrilla Warfare",
		"Elite Trapper Solman | Guerrilla Warfare",
		"'The Doctor' Romanov | Sabre",
		"Blackwolf | Sabre",
		"Maximus | Sabre",
		"Dragomir | Sabre",
		"Rezan The Ready | Sabre",
		"Rezan the Redshirt | Sabre",
		"Dragomir | Sabre Footsoldier",
	};
}