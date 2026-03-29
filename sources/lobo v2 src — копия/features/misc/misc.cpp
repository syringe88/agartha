#include "misc.h"
#include "../../game/sdk/includes/includes.h"
#include "../../globals/includes/includes.h"
#include "../../globals/logger/logger.h"
#include "../avatar_cache/avatar_cache.h"
#include "../entity_cache/entity_cache.h"
#include "../movement/movement.h"
#include "../instalisation fonts/fonts2.h"
void n_misc::impl_t::on_create_move_pre( )
{
	this->disable_post_processing( );

	this->remove_panorama_blur( );

	this->practice_window_think( );
}

void n_misc::impl_t::on_paint_traverse( )
{
	this->draw_spectating_local( );
	
	
#ifdef _DEBUG
	// TESTING FUNCTION
	[ & ]( const bool run ) {
		if ( !run || !GET_VARIABLE( g_variables.m_debugger_visual, bool ) )
			return;

		float offset = 0.f;

		static auto render_jb_debug = [ & ]( const std::string& text ) {
			g_render.m_draw_data.emplace_back( e_draw_type::draw_type_text, std::make_any< text_draw_object_t >(
																				g_render.m_fonts[ e_font_names::font_name_tahoma_bd_12 ],
																				c_vector_2d( g_ctx.m_width / 2, ( g_ctx.m_height / 1.5 ) + offset ),
																				text.c_str( ), ImColor( 1.f, 1.f, 1.f, 1.f ),
																				ImColor( 0.f, 0.f, 0.f, 1.f ), e_text_flags::text_flag_dropshadow ) );
			offset += 10;
		};

		render_jb_debug( std::string( "can_jb = " ).append( std::to_string( g_movement.m_jumpbug_data.m_can_jb ) ) );
		render_jb_debug( std::string( "height_diff = " ).append( std::to_string( g_movement.m_jumpbug_data.m_height_diff ) ) );
		render_jb_debug(
			std::string( "vertical_velocity_at_landing = " ).append( std::to_string( g_movement.m_jumpbug_data.m_vertical_velocity_at_landing ) ) );
		render_jb_debug( std::string( "abs_height_diff = " ).append( std::to_string( g_movement.m_jumpbug_data.m_abs_height_diff ) ) );
		render_jb_debug( std::string( "ticks_till_land = " ).append( std::to_string( g_movement.m_jumpbug_data.m_ticks_till_land ) ) );
	}( GET_VARIABLE( g_variables.m_jump_bug, bool ) && g_input.check_input( &GET_VARIABLE( g_variables.m_jump_bug_key, key_bind_t ) ) );
#endif
}

static std::chrono::steady_clock::time_point progressStartTime = std::chrono::steady_clock::now( );
static double calculatedPositionMs                             = 0.0; // ïîçèöèÿ â ìèëëèñåêóíäàõ
static std::string lastTitle                                   = "";  // äëÿ îòñëåæèâàíèÿ ñìåíû òðåêà

void UpdateCalculatedTrackPosition( Player& player )
{
	// Åñëè ïðîèçîøëà ñìåíà òðåêà, ñáðàñûâàåì ïîçèöèþ è âðåìÿ ñòàðòà
	if ( lastTitle != player.Title ) {
		calculatedPositionMs = 0.0;
		progressStartTime    = std::chrono::steady_clock::now( );
		lastTitle            = player.Title;
	}

	// Åñëè òðåê âîñïðîèçâîäèòñÿ, ïðèáàâëÿåì ïðîøåäøåå âðåìÿ
	if ( player.isPlaying ) {
		auto now     = std::chrono::steady_clock::now( );
		auto deltaMs = std::chrono::duration_cast< std::chrono::milliseconds >( now - progressStartTime ).count( );
		calculatedPositionMs += deltaMs;
		progressStartTime = now;
	}
}



// Ïðèìåð îòðèñîâêè ïëååðà ñ èñïîëüçîâàíèåì ImGui è íîâîãî ðàñ÷¸òà ïðîãðåññà
void RenderMediaPlayer( )
{
	// Îáíîâëÿåì ðàññ÷èòàííóþ ïîçèöèþ òðåêà
	UpdateCalculatedTrackPosition( player );

	// Âû÷èñëÿåì ïðîãðåññ, èñïîëüçóÿ äëèòåëüíîñòü òðåêà (TotalTime â ìñ)
	float progress = 0.0f;
	if ( player.TotalTime > 0 ) {
		progress = static_cast< float >( calculatedPositionMs ) / static_cast< float >( player.TotalTime );
		if ( progress > 1.0f )
			progress = 1.0f;
	}

	// Äîáàâëÿåì èíòåðïîëÿöèþ äëÿ ïëàâíîãî äâèæåíèÿ ïðîãðåññà
	static float smoothProgress = 0.0f;
	if ( progress < smoothProgress )
		smoothProgress = progress;
	smoothProgress += ( progress - smoothProgress ) * 0.1f; // êîýôôèöèåíò èíòåðïîëÿöèè (ìîæíî íàñòðîèòü)

	static ImVec2 sz{ };
	int x = g_ctx.m_width;
	int y = g_ctx.m_height;
	if ( GET_VARIABLE( g_variables.m_watermark, bool ) )
		ImGui::SetNextWindowPos( { x - sz.x - 10, 45 } );
	else
		ImGui::SetNextWindowPos( { x - sz.x - 10, 10 } );

	ImGui::Begin( "Media Player", nullptr,
	              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );
	ImGui::PushFont( fonts_for_gui::regular_11 );

	float padding    = 10.0f; // Îòñòóï ìåæäó òåêñòîì è èçîáðàæåíèåì
	float imageWidth = 30.0f; // Øèðèíà èçîáðàæåíèÿ

	// Ðàññ÷èòûâàåì ðàçìåðû òåêñòà
	auto sizex1       = ImGui::CalcTextSize( strartist.c_str( ) ).x;
	auto sizex2       = ImGui::CalcTextSize( strtitle.c_str( ) ).x;
	float windowWidth = ImGui::GetWindowSize( ).x;
	if ( albumArtTexture ) {
		ImGui::SetCursorPos( ImVec2( windowWidth - imageWidth - padding, 3 ) );
		ImGui::Image( albumArtTexture, ImVec2( imageWidth, imageWidth ) );
	}
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding, 20 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding, 20 } );

	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex1 - padding + 1, 19 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex1 - padding + 1, 19 } );
	ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.f ), strartist.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding, 5 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding, 5 } );

	ImGui::TextColored( ImVec4( 0.f, 0.f, 0.f, 1.f ), strtitle.c_str( ) );
	if ( albumArtTexture )
		ImGui::SetCursorPos( { windowWidth - imageWidth - padding - sizex2 - padding + 1, 4 } );
	else
		ImGui::SetCursorPos( { windowWidth - padding - sizex2 - padding + 1, 4 } );
	ImGui::TextColored( ImVec4( 1.f, 1.f, 1.f, 1.f ), strtitle.c_str( ) );

	if ( albumArtTexture ) {
		ImGui::PushItemWidth( 158 );
		ImGui::SetCursorPos( { sz.x - imageWidth - padding - 126, 40 } );
		ImGui::ProgressBar( smoothProgress, ImVec2( 0.0f, 2.0f ) );
		ImGui::PopItemWidth( );
	}
	ImGui::Spacing( );
	
	ImGui::SetWindowSize( { 400, 100 } );
	sz = ImGui::GetWindowSize( );

	ImGui::PopFont( );
	ImGui::End( );
}

// Watermark rendering function (labuba)
void RenderWatermark()
{
    const auto io     = ImGui::GetIO();
    static auto width = 0.f;
    ImGui::SetNextWindowSize(ImVec2(-1, 29.f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(g_ctx.m_width - width - 5.f, 5.f), ImGuiCond_Always);
    ImGui::Begin("hotwheels-watermark", 0,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);
    {
        const auto window    = ImGui::GetCurrentWindow();
        const auto draw_list = window->DrawList;
        const auto size      = window->Size;
        const auto position  = window->Pos;

        // Background with subtle blue-white glow
        draw_list->AddRectFilled(ImVec2(position.x, position.y), ImVec2(position.x + size.x, position.y + size.y),
            ImColor(20, 20, 25, 230), 10.0f);

        // Subtle blue-white glow behind text
        draw_list->AddRectFilled(ImVec2(position.x + 2.f, position.y + 2.f), ImVec2(position.x + size.x - 2.f, position.y + size.y - 2.f),
            ImColor(40, 110, 150, 20), 9.0f);

        // Rounded outline
        ImGui::PushClipRect(ImVec2(position.x + 1.f, position.y + 1.f), ImVec2(position.x + size.x - 1.f, position.y + size.y - 1.f), false);
        draw_list->AddRect(ImVec2(position.x + 1.f, position.y + 1.f), ImVec2(position.x + size.x - 1.f, position.y + size.y - 1.f),
            ImColor(40 / 255.f, 40 / 255.f, 45 / 255.f), 10.0f);
        ImGui::PopClipRect();

        // White text with custom font
        ImGui::PushFont(fonts_for_gui::bolder_11);

        // Center align text vertically
        float text_y_offset = (size.y - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPosY(text_y_offset);
        ImGui::SetCursorPosX(10.0f);

        // White text "labuba" - simple watermark with only the name
        ImGui::TextColored(ImColor(255, 255, 255, 255), "labuba");

        ImGui::PopFont();
        width = ImGui::GetWindowSize().x;
    }
    ImGui::End();
}


void n_misc::impl_t::on_end_scene( )
{
	if ( GET_VARIABLE( g_variables.trackdispay, bool ) )
		RenderMediaPlayer( );
	if (GET_VARIABLE(g_variables.m_watermark, bool))
		RenderWatermark();

	// functions that require player to be ingame
	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || g_ctx.m_local->get_observer_mode( ) == 1 /*DEATH_CAM*/ )
		return;

	// practice window
	[ & ]( const bool can_draw_practice_window ) {
		if ( !can_draw_practice_window )
			return;
		if ( !g_ctx.m_local->is_alive( ) )
			return;

		constexpr float window_width = 170.0f;
		constexpr float window_rounding = 7.0f;
		constexpr float window_padding = 0.0f;
		constexpr float title_top_margin = 5.0f;
		constexpr float first_bind_top = 25.0f;
		constexpr float bind_left_margin = 10.0f;
		constexpr float key_box_size = 20.0f;
		constexpr float key_box_rounding = 5.0f;
		constexpr float bind_row_height = 24.0f;
		constexpr float bind_row_spacing = 3.0f;
		constexpr float bottom_margin = 5.0f;
		constexpr ImU32 bg_color = IM_COL32(13,13,13,255);
		constexpr ImU32 key_box_border = IM_COL32(22,22,22,255);
		constexpr ImU32 key_text_color = IM_COL32(100,100,100,255);
		constexpr ImU32 label_color = IM_COL32(100,100,100,255);
		constexpr ImU32 title_color = IM_COL32(255,255,255,255);

		ImFont* key_font = fonts_for_gui::regular_11;
		float key_font_size = 10.f;
		float vertical_offset = -25.0f; // поднимаем бокс и текст чуть выше

		float window_height = first_bind_top + 2 * bind_row_height + bind_row_spacing + bottom_margin;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

		ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
		int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
		ImGui::Begin("Checkpoints", nullptr, flags);
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 win_pos = ImGui::GetWindowPos();

			// Заголовок по центру
			ImGui::PushFont(fonts_for_gui::regular_11);
			const char* title = "Checkpoints";
			ImVec2 title_size = ImGui::CalcTextSize(title);
			ImGui::SetCursorPosY(title_top_margin);
			ImGui::SetCursorPosX((window_width - title_size.x) * 0.5f);
			ImGui::TextColored(ImColor(title_color), title);
			ImGui::PopFont();

			// Массив подписей и биндов (только Save и Teleport)
			struct bind_row_t { const char* label; const char* key; };
			bind_row_t rows[2] = {
				{"Save", FILTERED_KEY_NAMES[GET_VARIABLE(g_variables.m_practice_cp_key, key_bind_t).m_key]},
				{"Teleport", FILTERED_KEY_NAMES[GET_VARIABLE(g_variables.m_practice_tp_key, key_bind_t).m_key]}
			};

			float y = first_bind_top;
			for (int i = 0; i < 2; ++i) {
				// Label
				ImGui::SetCursorPosY(y);
				ImGui::SetCursorPosX(bind_left_margin);
				ImGui::PushFont(fonts_for_gui::regular_11);
				ImGui::TextColored(ImColor(label_color), rows[i].label);
				ImGui::PopFont();

				// Key box — строго напротив текста, но чуть выше
				ImVec2 label_pos = ImGui::GetCursorScreenPos(); // Сохраняем ДО вывода текста!
				ImVec2 key_box_pos = ImVec2(win_pos.x + window_width - bind_left_margin - key_box_size, label_pos.y + vertical_offset);
				draw_list->AddRect(
					key_box_pos,
					ImVec2(key_box_pos.x + key_box_size, key_box_pos.y + key_box_size),
					key_box_border,
					3.0f, // радиус скругления
					ImDrawFlags_RoundCornersAll, // скруглять все углы
					2.0f
				);

				// Key text — по центру бокса
				std::string key_text = rows[i].key;
				std::transform(key_text.begin(), key_text.end(), key_text.begin(), ::toupper);
				ImVec2 key_text_size = key_font->CalcTextSizeA(key_font_size, FLT_MAX, 0.0f, key_text.c_str());
				ImVec2 key_text_pos = ImVec2(
					key_box_pos.x + (key_box_size - key_text_size.x) * 0.5f,
					key_box_pos.y + (key_box_size - key_text_size.y) * 0.2f + 2.5f // <-- вот это!
				);
				draw_list->AddText(key_font, key_font_size, key_text_pos, key_text_color, key_text.c_str());

				y += bind_row_height + bind_row_spacing;
			}
		}
		ImGui::End();
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(3);
	}( GET_VARIABLE( g_variables.m_practice_window, bool ) );

	[ & ]( bool can_draw_spectator_list, int type ) {
		if ( type == 0 && can_draw_spectator_list )
			draw_spectator_list( );
	}( GET_VARIABLE( g_variables.m_spectators_list, bool ), GET_VARIABLE( g_variables.m_spectators_list_type, int ) );


	

}

void n_misc::impl_t::on_fire_event( ) {


}

void n_misc::impl_t::practice_window_think( )
{
	if ( !GET_VARIABLE( g_variables.m_practice_window, bool ) )
		return;

	if ( !g_convars[ HASH_BT( "sv_cheats" ) ]->get_bool( ) || g_interfaces.m_engine_client->is_console_visible( ) )
		return;

	const auto cp_key = GET_VARIABLE( g_variables.m_practice_cp_key, key_bind_t ).m_key;

	const auto tp_key = GET_VARIABLE( g_variables.m_practice_tp_key, key_bind_t ).m_key;

	if ( g_input.is_key_released( cp_key ) ) {
		if ( !( g_ctx.m_local->get_flags( ) & fl_onground ) ) {
			g_logger.print( "you need to be onground to set a checkpoint.", "[practice]" );
			return;
		}

		c_angle saved_angles = { };
		g_interfaces.m_engine_client->get_view_angles( saved_angles );

		g_misc.practice.saved_angles   = saved_angles;
		g_misc.practice.saved_position = g_ctx.m_local->get_abs_origin( );

		g_logger.print( "saved checkpoint.", "[practice]" );
	} else if ( g_input.is_key_released( tp_key ) ) {
		if ( g_misc.practice.saved_angles.is_zero( ) || g_misc.practice.saved_position.is_zero( ) )
			return;

		g_interfaces.m_engine_client->client_cmd_unrestricted(
			std::string( "setpos " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_position.m_x, g_misc.practice.saved_position.m_y,
		                                                                  g_misc.practice.saved_position.m_z ) ) )
				.append( ";setang " )
				.append( std::vformat( "{} {} {}", std::make_format_args( g_misc.practice.saved_angles.m_x, g_misc.practice.saved_angles.m_y,
		                                                                  g_misc.practice.saved_angles.m_z ) ) )
				.c_str( ) );
	}
}

void n_misc::impl_t::disable_post_processing( )
{
	g_convars[ HASH_BT( "mat_postprocess_enable" ) ]->set_value( !( GET_VARIABLE( g_variables.m_disable_post_processing, bool ) ) );
}

void n_misc::impl_t::remove_panorama_blur( )
{
	g_convars[ HASH_BT( "@panorama_disable_blur" ) ]->set_value( GET_VARIABLE( g_variables.m_remove_panorama_blur, bool ) );
}

void n_misc::impl_t::draw_spectating_local( )
{
	int m_y = 5;

	std::vector< spectator_data_t > spectator_data{ };

	if ( !g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game( ) || !GET_VARIABLE( g_variables.m_spectators_list, bool ) ||
	     GET_VARIABLE( g_variables.m_spectators_list_type, int ) != 1 /*list local spectators*/ ) {
		if ( !spectator_data.empty( ) ) {
			g_ctx.m_last_spectators_y = 5;
			spectator_data.clear( );
		}
		return;
	}

	constexpr auto get_player_spec_type = [ & ]( int obs_mode ) -> std::string {
		switch ( obs_mode ) {
		case e_obs_mode::obs_mode_deathcam:
			return "deathcam";
		case e_obs_mode::obs_mode_freezecam:
			return "freezecam";
		case e_obs_mode::obs_mode_in_eye:
			return "first person";
		case e_obs_mode::obs_mode_chase:
			return "3rd person";
		case e_obs_mode::obs_mode_roaming:
			return "roaming";
		case e_obs_mode::obs_mode_fixed:
			return "fixed";
		default:
			return "first person";
		}
	};

	g_entity_cache.enumerate( e_enumeration_type::type_players, [ & ]( c_base_entity* entity ) {
		if ( !entity || entity->is_alive( ) || entity->is_dormant( ) )
			return;

		const auto entity_index = entity->get_index( );
		const auto entity_team  = entity->get_team( );

		const auto spectated_player = reinterpret_cast< c_base_entity* >(
			g_interfaces.m_client_entity_list->get_client_entity_from_handle( entity->get_observer_target_handle( ) ) );

		if ( !spectated_player || spectated_player != g_ctx.m_local )
			return;

		player_info_t spectating_info{ };
		g_interfaces.m_engine_client->get_player_info( entity_index, &spectating_info );

		if ( spectating_info.m_is_hltv )
			return;

		spectator_data.push_back(
			{ std::format( "{} | {}",  std::string( spectating_info.m_name ).substr( 0, 12 ).append( "..." ),
		                                                      get_player_spec_type( entity->get_observer_mode( ) )  ),
		      spectating_info.m_fake_player ? entity_team == e_team_id::team_tt   ? g_render.m_terrorist_avatar
		                                      : entity_team == e_team_id::team_ct ? g_render.m_counter_terrorist_avatar
		                                                                          : nullptr
		                                    : g_avatar_cache[ entity_index ],
		      GET_VARIABLE( g_variables.m_spectators_list_text_color_one, c_color ) } );
	} );

	if ( spectator_data.empty( ) ) {
		g_ctx.m_last_spectators_y = 5;
		return;
	}

	const auto draw_avatar            = GET_VARIABLE( g_variables.m_spectators_avatar, bool );
	constexpr static auto avatar_size = 14.f;

	for ( const auto& data : spectator_data ) {
		if ( draw_avatar )
			g_render.m_draw_data.emplace_back(
				e_draw_type::draw_type_texture,
				std::make_any< texture_draw_object_t >( c_vector_2d( 10, m_y ), c_vector_2d( avatar_size, avatar_size ),
			                                            ImColor( 1.f, 1.f, 1.f, 1.f ), data.m_avatar, 0.f, ImDrawFlags_::ImDrawFlags_None ) );

		g_render.m_draw_data.emplace_back(
			e_draw_type::draw_type_text,
			std::make_any< text_draw_object_t >( g_render.m_fonts[ e_font_names::font_name_tahoma_bd_12 ], c_vector_2d( draw_avatar ? 27 : 10, m_y ),
		                                         data.m_text.c_str( ), data.m_color.get_u32( ),
		                                         ImColor( 0.f, 0.f, 0.f, data.m_color.base< e_color_type::color_type_a >( ) ),
		                                         e_text_flags::text_flag_dropshadow ) );

		m_y += 15;

		g_ctx.m_last_spectators_y = m_y;
	}
}

void n_misc::impl_t::draw_spectator_list()
{
    if (!g_ctx.m_local || !g_interfaces.m_engine_client->is_in_game() || !GET_VARIABLE(g_variables.m_spectators_list, bool))
        return;

    constexpr float window_width = 200.0f;
    constexpr float window_rounding = 5.0f;
    constexpr float window_padding = 0.0f;
    constexpr float title_top_margin = 5.0f;
    constexpr float title_to_list_spacing = 15.0f;
    constexpr float item_left_margin = 10.0f;
    constexpr float item_height = 17.0f;
    constexpr float item_spacing = 10.0f;
    constexpr float bottom_padding = 10.0f;
    constexpr ImU32 bg_color = IM_COL32(13,13,13,255);
    constexpr ImU32 text_color = IM_COL32(255,255,255,255);

    // Собираем спектаторов
    std::vector<std::string> spectators;
    g_entity_cache.enumerate(e_enumeration_type::type_players, [&](c_base_entity* entity) {
        if (!entity || entity->is_alive() || entity->is_dormant())
            return;

        const auto entity_index = entity->get_index();
        const auto spectated_player = reinterpret_cast<c_base_entity*>(
            g_interfaces.m_client_entity_list->get_client_entity_from_handle(entity->get_observer_target_handle()));

        if (!spectated_player || !spectated_player->is_alive() || spectated_player != g_ctx.m_local)
            return;

        player_info_t info{};
        g_interfaces.m_engine_client->get_player_info(entity_index, &info);

        if (info.m_is_hltv)
            return;

        std::string name = std::string(info.m_name).substr(0, 24);
        spectators.push_back(name);
    });

    if (spectators.empty())
        return;

    // Высота окна: паддинг сверху + высота заголовка + отступ до первого ника + (item_height + item_spacing)*(n-1) + item_height + 10 (паддинг до низа)
    float window_height = title_top_margin + ImGui::GetFontSize() + title_to_list_spacing
        + spectators.size() * item_height
        + (spectators.size() > 1 ? (spectators.size() - 1) * item_spacing : 0)
        + bottom_padding;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);
    int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Spectators", nullptr, flags);
    {
        // Заголовок
        ImGui::PushFont(fonts_for_gui::regular_11);
        const char* title = "Spectators";
        ImVec2 title_size = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosY(title_top_margin);
        ImGui::SetCursorPosX((window_width - title_size.x) * 0.5f);
        ImGui::TextColored(ImColor(text_color), title);
        ImGui::PopFont();

        // Первый ник — строго на 20 пикселей ниже заголовка
        float y = title_top_margin + ImGui::GetFontSize() + title_to_list_spacing;
        for (size_t i = 0; i < spectators.size(); i++) {
            ImGui::SetCursorPosY(y + i * (item_height + item_spacing));
            ImGui::SetCursorPosX(item_left_margin);
            ImGui::PushFont(fonts_for_gui::regular_11);
            ImGui::TextColored(ImColor(text_color), spectators[i].c_str());
            ImGui::PopFont();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}
