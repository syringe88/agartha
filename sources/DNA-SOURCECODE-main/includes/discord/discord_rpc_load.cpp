#include "discord_rpc.h"
#include "../../menu/config/config.hpp"
#include "../../hooks/hooks.hpp"

void c_discord::initialize() {
    DiscordEventHandlers Handle;
    memset(&Handle, 0, sizeof(Handle));

    Discord_Initialize("1448003416826581175", &Handle, 1, NULL);
}

void c_discord::update() {
    static bool init = false;
    const bool run = true;
    if (run) {
        if (!init) {
            initialize();
            init = true;
        }
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        static auto elapsed = std::time(nullptr);
        const int uid = 1;
        std::string build = "beta";
        std::string UID_STATUS = { "Counter-Strike: Global Offensive" };
        /* if (g::local)
         UID_STATUS += "\n Playing On: " + std::string(interfaces::engine->get_level_name());*/

        std::string BUILD_STATUS = interfaces::engine->is_in_game() ? "Playing on: " + std::string(interfaces::engine->get_level_name()) : "";

        std::string largeImageText_version = { "Version: beta 0.5" };
        discordPresence.largeImageText = largeImageText_version.c_str();
        discordPresence.details = UID_STATUS.c_str();
        discordPresence.state = BUILD_STATUS.c_str();
        discordPresence.largeImageKey = "dna_logo";
        discordPresence.startTimestamp = elapsed;
        discordPresence.joinSecret = "";
        Discord_UpdatePresence(&discordPresence);
    }
    else {
        shutdown();
        init = false;
    }
}

void c_discord::shutdown() {
    Discord_ClearPresence();
    Discord_Shutdown();
}