#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <chrono>
#include <thread>

#include "sdk/sdk.hpp"
//#include "utils/xor.hpp"
#include "menu/menu.hpp"
#include "features/scripts/scripts.hpp"
#include "features/events/events.hpp"
#include "hooks/hooks.hpp"
#include "menu/config/config.hpp"
#include "features/misc/misc.hpp"
#include "sdk/netvars/netvars.hpp"
#include "features/skins/skins.hpp"
#include "features/movement/movement.hpp"
#include "includes/discord/discord_rpc.h"

static DWORD WINAPI on_attach(void* instance) {
    [&]() {	
        interfaces::initialize();
        sdk::hooks::init();
        c::create_directory();
        cvar::init();
        route = std::make_unique<savingroute>("records");
        mplayer.sessionManager.reset();
        mplayer.session.reset();
        mplayer.thumbnail.reset();
        c_discord::get().update();
        interfaces::console->console_color_printf({ 242, 242, 242, 255 }, ("[dna] "));
        interfaces::console->console_printf(std::string("Movement Extention. Version [Beta]").append(" \n").c_str());
        }();
        FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 1);
        return 0;
}

static DWORD WINAPI GetNowPlayingInfoAndSaveAlbumArt(void* instance)
{
    while (true) {

        if (!c::misc::show_spotify_currently_playing) {
            Sleep(100);
            continue;
        }
        mplayer.Update(interfaces::device);
        if (oldtitle != strtitle || oldartist != strartist || !albumArtTexture) {
            albumArtTexture = mplayer.thumb;
            oldtitle = strtitle;
            oldartist = strartist;
        }
        Sleep(10);
        strartist = mplayer.Artist;
        Sleep(10);
        strtitle = mplayer.Title;
        Sleep(10);
        albumArtTexture = mplayer.thumb;
        Sleep(100);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread1 = CreateThread(
            nullptr,
            0,
            on_attach,
            hModule,
            0,
            nullptr
        );

        HANDLE hThread2 = CreateThread(
            nullptr,
            0,
            GetNowPlayingInfoAndSaveAlbumArt,
            hModule,
            0,
            nullptr
        );

        // Close both handles safely
        if (hThread1)
            CloseHandle(hThread1);

        if (hThread2)
            CloseHandle(hThread2);
    }

    return TRUE;
}