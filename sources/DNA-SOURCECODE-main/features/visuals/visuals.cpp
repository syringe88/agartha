#include "../visuals/visuals.hpp"
#include "../../menu/config/config.hpp"
#include "../../menu/menu.hpp"
#include "../features/movement/movement.hpp"
//#include "../visuals/display/display.hpp"

mPlayer mplayer;

char* alloc_wcstcs(winrt::hstring source)
{
	char* string_alloc = (char*)malloc((source.size() + 1) * sizeof(char));
	wcstombs(string_alloc, source.c_str(), source.size() + 1);
	return string_alloc;
}

//TODO: UNDERSTAND WHY SOME SYMBOLS ARENT DECODING PROPERLY

std::string wstring_to_utf8(const std::wstring& wstr)
{
	if (wstr.empty()) {
		return std::string();
	}

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

	std::string strTo(size_needed, 0);

	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);

	return strTo;
}
void* LastThumb;
std::string PreviousTitle;
std::string PreviousArtist;

concurrency::task< void > mPlayer::Update(LPDIRECT3DDEVICE9 g_pd3dDevice)
{
	pool_.clear();

	static auto sessions = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
	auto currentSession = sessions.GetCurrentSession();
	mplayer.session = currentSession;

	if (this->session != nullptr) {
		this->HasMedia = true;
		this->SourceAppUserModeId = pool_.allocate(this->session->SourceAppUserModelId());

		auto info = this->session->TryGetMediaPropertiesAsync().get();

		std::string currentTitle = wstring_to_utf8(info.Title().c_str());
		std::string currentArtist = wstring_to_utf8(info.Artist().c_str());
		bool trackChanged = (currentTitle != PreviousTitle) || (currentArtist != PreviousArtist);

		this->Title = wstring_to_utf8(info.Title().c_str());
		this->Artist = wstring_to_utf8(info.Artist().c_str());
		this->AlbumArtist = pool_.allocate(info.AlbumArtist());
		this->AlbumTitle = pool_.allocate(info.AlbumTitle());
		this->TrackNumber = info.TrackNumber();
		this->AlbumTrackCount = info.AlbumTrackCount();

		if (info.Thumbnail() && trackChanged) {
			auto thumbnail_stream = info.Thumbnail().OpenReadAsync().get();
			this->Thumbnail_type = pool_.allocate(thumbnail_stream.ContentType());

			Buffer buffer = Buffer(thumbnail_stream.Size());
			thumbnail_stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::ReadAhead).get();

			if (this->Thumbnail_buffer != LastThumb) {
				free(this->Thumbnail_buffer);
				//this->thumb->Release();
				//this->thumb = nullptr;
			}

			this->Thumbnail_buffer = malloc(buffer.Length());
			memcpy(this->Thumbnail_buffer, buffer.data(), buffer.Length());
			this->Thumbnail_size = buffer.Length();
			if (this->Thumbnail_buffer != LastThumb) {
				D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, this->Thumbnail_buffer, this->Thumbnail_size, 30, 30, D3DX_DEFAULT, 1,
					D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &this->thumb);
				LastThumb = this->Thumbnail_buffer;
			}
			PreviousTitle = currentTitle;
			PreviousArtist = currentArtist;
		}
		else if (!info.Thumbnail()) {
			this->Thumbnail_size = 0;
			this->thumb = nullptr;
		}

		auto timelineProperties = this->session->GetTimelineProperties();
		auto duration = timelineProperties.EndTime() - timelineProperties.StartTime();
		this->TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		auto currentPos = timelineProperties.Position() - timelineProperties.StartTime();
		this->CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentPos).count();

		auto playbackInfo = this->session->GetPlaybackInfo();
		if (playbackInfo) {
			auto playbackStatus = playbackInfo.PlaybackStatus();
			this->isPlaying = (playbackStatus == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
		}
	}
	else {
		this->HasMedia = false;
	}
	co_return;
}

static std::chrono::steady_clock::time_point progressStartTime = std::chrono::steady_clock::now();
static double calculatedPositionMs = 0.0;
static std::string lastTitle = "";
int last;

void UpdateCalculatedTrackPosition(mPlayer& mplayer)
{
	if (lastTitle != mplayer.Title) {
		progressStartTime = std::chrono::steady_clock::now();
		lastTitle = mplayer.Title;
		calculatedPositionMs = static_cast<double>(mplayer.CurrentTime);
		last = mplayer.CurrentTime;
	}

	if (mplayer.isPlaying) {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> delta = now - progressStartTime;
		if (mplayer.CurrentTime != last) {
			calculatedPositionMs = static_cast<double>(mplayer.CurrentTime);
			last = mplayer.CurrentTime;
		}
		else {
			calculatedPositionMs += delta.count();
		}
		progressStartTime = now;
	}
}

void features::visuals::RenderMediaPlayer()
{
	if (!c::misc::show_spotify_currently_playing)
		return;

	UpdateCalculatedTrackPosition(mplayer);

	float progress = 0.0f;
	if (mplayer.TotalTime > 0) {
		progress = static_cast<float>(calculatedPositionMs) / static_cast<float>(mplayer.TotalTime);
		if (progress > 1.0f)
			progress = 1.0f;
	}

	static float smoothProgress = 0.0f;
	if (progress < smoothProgress)
		smoothProgress = progress;
	smoothProgress += (progress - smoothProgress) * 0.1f;

	static ImVec2 sz{ };
	int x, y;
	interfaces::engine->get_screen_size(x, y);
	float m = c::misc::watermark ? 30 : 0;
	ImGui::SetNextWindowPos({ x - sz.x + 2, 4.5f + m });

	ImGui::Begin("Media Player", nullptr,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	float padding = 10.0f;
	float imageWidth = 30.0f;
	float imageHeight = 30.0f;


	auto sizex1 = ImGui::CalcTextSize(strartist.c_str()).x;
	auto sizex2 = ImGui::CalcTextSize(strtitle.c_str()).x;

	float sizey1 = y * 1.5f / 720.f;
	float sizey2 = y * 1.5f / 720.f;

	auto text_size1 = im_render.measure_text(strartist.c_str(), fonts::esp_name_font, 12.f);
	auto text_size2 = im_render.measure_text(strtitle.c_str(), fonts::esp_name_font, 12.f);

	float windowWidth = ImGui::GetWindowSize().x;

	ImGui::PushFont(fonts::esp_name_font);

	if (albumArtTexture) {
		ImGui::SetCursorPos(ImVec2(windowWidth - imageWidth - padding, 3));
		ImGui::Image(albumArtTexture, ImVec2(imageWidth, imageWidth));
	}
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size1.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 + 1 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size1.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 + 1 });

	ImGui::TextColored(ImVec4(0.f, 0.f, 0.f, 0.7f), strartist.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size1.x - padding + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size1.x - padding + 2, 3 + imageHeight / 2 - (text_size1.y) / 2 + 6 });
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 0.7f), strartist.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size2.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 + 1 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size2.x - padding + 1 + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 + 1 });

	ImGui::TextColored(ImVec4(0.f, 0.f, 0.f, 0.7f), strtitle.c_str());
	if (albumArtTexture)
		ImGui::SetCursorPos({ windowWidth - imageWidth - padding - text_size2.x - padding + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 });
	else
		ImGui::SetCursorPos({ windowWidth - padding - text_size2.x - padding + 2, 3 + imageHeight / 2 - (text_size2.y) / 2 - 6 });
	ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.7f), strtitle.c_str());

	if (albumArtTexture && c::misc::progressbar_enable) {
		ImGui::PushItemWidth(108);
		ImGui::SetCursorPos({ sz.x - imageWidth - padding - 78, 40 });

		//should be here but idc
		//u can fix that if u want ig
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 1.f, 1.f, 0.15f));            // background
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.7f));       // progress fill
		ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.f, 1.f, 1.f, 0.7f)); // hover fill
		ImGui::ProgressBar(smoothProgress, ImVec2(0.0f, 2.0f));
		ImGui::PopStyleColor(3);
		ImGui::PopItemWidth();
	}
	ImGui::Spacing();

	ImGui::SetWindowSize({ 400, 100 });
	sz = ImGui::GetWindowSize();

	ImGui::PopFont();

	ImGui::End();
}

void features::visuals::display_spotify() {
	if (!c::misc::show_spotify_currently_playing)
		return;

	int w, h;
	std::string name;
	interfaces::engine->get_screen_size(w, h);
	h = c::misc::watermark ? 30 : 5;

	//simple check for " - " in the track name
	//cuz its usually used when artist name is in the track title
	if (strtitle.find(" - ") == std::string::npos && !strartist.empty()) {
		name = strartist + " - " + strtitle;
	}
	else {
		name = strtitle;
	}

	auto text_size = im_render.get_text_size(name.c_str(), fonts::esp_name_font, 0.f, 12.f);
	auto paused_size = im_render.get_text_size("", fonts::esp_name_font, 0.f, 12.f);

	if (mplayer.isPlaying) {
		//ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - text_size + 1, h + 1), ImColor(0, 0, 0, 255), name.c_str());
		//ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - text_size, h), ImColor(255, 255, 255, 255), name.c_str());
	}
	else {
		ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - paused_size + 1, h + 1), ImColor(0, 0, 0, 255), "");
		ImGui::GetBackgroundDrawList()->AddText(fonts::esp_name_font, 12.f, ImVec2(w - 6 - paused_size, h), ImColor(255, 255, 255, 255), "");
	}
}

void draw_screen_effect(i_material* material) {
	static auto fn = find_pattern("client.dll", "55 8B EC 83 E4 ? 83 EC ? 53 56 57 8D 44 24 ? 89 4C 24 ?");
	int w, h;
	interfaces::engine->get_screen_size(w, h);
	__asm {
		push h
		push w
		push 0
		xor edx, edx
		mov ecx, material
		call fn
		add esp, 12
	}
}

// Motion blur history structure
struct motion_blur_history_t {
	vec3_t last_position;
	float last_pitch;
	float last_yaw;
	float last_update_time;
	float disable_rotational_until;
	bool initialized;

	motion_blur_history_t() :
		last_position(0, 0, 0),
		last_pitch(0.0f),
		last_yaw(0.0f),
		last_update_time(0.0f),
		disable_rotational_until(0.0f),
		initialized(false) {}
};

static motion_blur_history_t g_mb_history;

// Normalize angle to -180 to 180 range
inline float normalize_angle(float angle) {
	angle = fmodf(angle, 360.0f);
	if (angle > 180.0f)
		angle -= 360.0f;
	else if (angle < -180.0f)
		angle += 360.0f;
	return angle;
}

// Calculate shortest angle difference
inline float angle_diff(float target, float current) {
	float diff = normalize_angle(target - current);
	return diff;
}

void features::visuals::motion_blur(view_setup_t* setup) {
	// Early exit checks
	if (!c::visuals::mbenabled)
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (!g::local || !g::local->is_alive()) {
		g_mb_history.initialized = false;
		return;
	}

	static float motion_blur_values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Setup phase - calculate motion blur values
	if (setup) {
		const float current_time = interfaces::globals->realtime;
		const float time_delta = current_time - g_mb_history.last_update_time;

		// Initialize on first run or after respawn
		if (!g_mb_history.initialized || time_delta > 1.0f) {
			g_mb_history.last_position = setup->origin;
			g_mb_history.last_pitch = normalize_angle(setup->view.x);
			g_mb_history.last_yaw = normalize_angle(setup->view.y);
			g_mb_history.last_update_time = current_time;
			g_mb_history.initialized = true;

			// Reset blur values
			memset(motion_blur_values, 0, sizeof(motion_blur_values));
			return;
		}

		// Get current camera state
		const vec3_t current_position = setup->origin;
		const float current_pitch = normalize_angle(setup->view.x);
		const float current_yaw = normalize_angle(setup->view.y);

		// Calculate position change
		vec3_t position_delta = g_mb_history.last_position - current_position;
		const float position_change = position_delta.length();

		// Detect teleportation or major disruptions
		const float teleport_threshold = 30.0f;
		const float max_frame_time = 1.0f / 10.0f; // 10 FPS minimum

		if (position_change > teleport_threshold || time_delta > max_frame_time) {
			// Reset on teleport/disruption
			g_mb_history.last_position = current_position;
			g_mb_history.last_pitch = current_pitch;
			g_mb_history.last_yaw = current_yaw;
			g_mb_history.last_update_time = current_time;
			memset(motion_blur_values, 0, sizeof(motion_blur_values));
			return;
		}

		// Detect very fast movement (noclip, respawn, etc)
		const float fast_movement_threshold = 50.0f;
		if (position_change > fast_movement_threshold) {
			g_mb_history.disable_rotational_until = current_time + 1.0f;
		}

		// Calculate view vectors
		vec3_t forward_vec, right_vec, up_vec;
		math::angle_vectors(setup->view, &forward_vec, &right_vec, &up_vec);

		// Calculate FOV values
		const float horizontal_fov = setup->fov;
		const float vertical_fov = (setup->aspect_ratio > 0.0f)
			? (setup->fov / setup->aspect_ratio)
			: setup->fov;

		// Calculate angle deltas using shortest path
		const float pitch_delta = angle_diff(g_mb_history.last_pitch, current_pitch);
		const float yaw_delta = angle_diff(g_mb_history.last_yaw, current_yaw);

		// Calculate motion components
		const float forward_motion = forward_vec.dot_product(position_delta);
		const float side_motion = right_vec.dot_product(position_delta);

		// YAW BLUR (horizontal rotation)
		float yaw_blur = yaw_delta;

		// Adjust yaw based on sideways movement
		yaw_blur += (side_motion / 3.0f);

		// Clamp adjustment to prevent over-compensation
		if (yaw_delta < 0.0f)
			yaw_blur = std::clamp(yaw_blur, yaw_delta, 0.0f);
		else
			yaw_blur = std::clamp(yaw_blur, 0.0f, yaw_delta);

		// Normalize by FOV and reduce based on pitch (looking up/down reduces horizontal blur)
		const float pitch_factor = 1.0f - (fabsf(current_pitch) / 90.0f);
		motion_blur_values[0] = (yaw_blur / horizontal_fov) * pitch_factor;

		// PITCH BLUR (vertical rotation)
		float pitch_blur = pitch_delta;

		// Compensate for forward/backward motion when looking up/down
		const float pitch_compensation = 1.0f - powf(1.0f - fabsf(forward_vec.z), 2.0f);
		const float motion_adjustment = (forward_motion / 2.0f) * pitch_compensation;

		if (current_pitch > 0.0f)
			pitch_blur -= motion_adjustment;
		else
			pitch_blur += motion_adjustment;

		// Clamp adjustment
		if (pitch_delta < 0.0f)
			pitch_blur = std::clamp(pitch_blur, pitch_delta, 0.0f);
		else
			pitch_blur = std::clamp(pitch_blur, 0.0f, pitch_delta);

		motion_blur_values[1] = pitch_blur / vertical_fov;

		// FORWARD/FALLING BLUR
		if (c::visuals::mbforwardEnabled && time_delta > 0.0f) {
			float forward_blur = forward_motion / (time_delta * 30.0f);

			// Apply falling min/max range
			const float falling_range = c::visuals::mbfallingMax - c::visuals::mbfallingMin;
			if (falling_range > 0.0f) {
				const float normalized = (fabsf(forward_blur) - c::visuals::mbfallingMin) / falling_range;
				forward_blur = std::clamp(normalized, 0.0f, 1.0f) * (forward_blur >= 0.0f ? 1.0f : -1.0f);
			}

			motion_blur_values[2] = forward_blur / 60.0f;
		}
		else {
			motion_blur_values[2] = 0.0f;
		}

		// ROLL BLUR (yaw-based rolling effect at extreme pitches)
		motion_blur_values[3] = (yaw_delta / horizontal_fov) * powf(fabsf(current_pitch) / 90.0f, 3.0f);

		// Apply intensity multipliers
		const float rotation_intensity = c::visuals::mbrotationIntensity * 0.15f * c::visuals::mbstrength;
		const float falling_intensity = c::visuals::mbfallingIntensity * 0.15f * c::visuals::mbstrength;

		motion_blur_values[0] *= rotation_intensity;
		motion_blur_values[1] *= rotation_intensity;
		motion_blur_values[2] *= rotation_intensity;
		motion_blur_values[3] *= falling_intensity;

		// Disable rotational blur during fast movement
		if (current_time < g_mb_history.disable_rotational_until) {
			motion_blur_values[0] = 0.0f;
			motion_blur_values[1] = 0.0f;
			motion_blur_values[3] = 0.0f;
		}

		// Update history
		g_mb_history.last_position = current_position;
		g_mb_history.last_pitch = current_pitch;
		g_mb_history.last_yaw = current_yaw;
		g_mb_history.last_update_time = current_time;

		return;
	}

	// Render phase - apply motion blur material
	i_material* material = interfaces::material_system->find_material("dev/motion_blur", "RenderTargets", false);

	if (!material || material->is_error_material())
		return;

	// Set motion blur values
	const auto motion_blur_internal = material->find_var("$MotionBlurInternal", nullptr, false);
	if (motion_blur_internal) {
		motion_blur_internal->set_vec_component_value(motion_blur_values[0], 0);
		motion_blur_internal->set_vec_component_value(motion_blur_values[1], 1);
		motion_blur_internal->set_vec_component_value(motion_blur_values[2], 2);
		motion_blur_internal->set_vec_component_value(motion_blur_values[3], 3);
	}

	// Set viewport
	const auto motion_blur_viewport = material->find_var("$MotionBlurViewportInternal", nullptr, false);
	if (motion_blur_viewport) {
		motion_blur_viewport->set_vec_component_value(0.0f, 0);
		motion_blur_viewport->set_vec_component_value(0.0f, 1);
		motion_blur_viewport->set_vec_component_value(1.0f, 2);
		motion_blur_viewport->set_vec_component_value(1.0f, 3);
	}

	// Handle depth buffer for specific adapters
	if (c::visuals::mb_video_adapter == 0 || c::visuals::mb_video_adapter == 3) {
		static auto mat_resolve = interfaces::console->get_convar("mat_resolveFullFrameDepth");
		if (mat_resolve)
			mat_resolve->set_value(0);
	}

	// Apply the effect
	draw_screen_effect(material);
}

struct camera_stabilization_t {
	vec3_t last_position;
	float last_pitch;
	float last_yaw;
	float last_update_time;
	bool initialized;

	camera_stabilization_t() :
		last_position(0, 0, 0),
		last_pitch(0.0f),
		last_yaw(0.0f),
		last_update_time(0.0f),
		initialized(false) {}
};
static camera_stabilization_t g_camera_stab;

void features::visuals::camera_fix(view_setup_t* setup) {
	if (!c::visuals::camera_fix_enabled)
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (!g::local || !g::local->is_alive()) {
		g_camera_stab.initialized = false;
		return;
	}

	if (!setup)
		return;

	const float current_time = interfaces::globals->realtime;
	const float time_delta = current_time - g_camera_stab.last_update_time;

	// Initialize on first run or after long delay
	if (!g_camera_stab.initialized || time_delta > 1.0f) {
		g_camera_stab.last_position = setup->origin;
		g_camera_stab.last_pitch = normalize_angle(setup->view.x);
		g_camera_stab.last_yaw = normalize_angle(setup->view.y);
		g_camera_stab.last_update_time = current_time;
		g_camera_stab.initialized = true;
		return;
	}

	// Get current camera state
	vec3_t current_position = setup->origin;
	float current_pitch = normalize_angle(setup->view.x);
	float current_yaw = normalize_angle(setup->view.y);

	// Calculate position change
	vec3_t position_delta = g_camera_stab.last_position - current_position;
	const float position_change = position_delta.length();

	// Detect teleportation (map change, respawn, noclip)
	const float teleport_threshold = 100.0f;
	const float max_frame_time = 0.5f;

	if (position_change > teleport_threshold || time_delta > max_frame_time) {
		// Reset on teleport
		g_camera_stab.last_position = current_position;
		g_camera_stab.last_pitch = current_pitch;
		g_camera_stab.last_yaw = current_yaw;
		g_camera_stab.last_update_time = current_time;
		return;
	}

	// CAMERA STABILIZATION
	// Smooth out jittery camera movement
	if (c::visuals::camera_smoothing > 0.0f && c::visuals::camrea_enable_smoothing) {
		const float smooth_factor = std::clamp<float>(c::visuals::camera_smoothing, 0.0f, 1.f);

		// Smooth position
		setup->origin.x = current_position.x * (1.0f - smooth_factor) + g_camera_stab.last_position.x * smooth_factor;
		setup->origin.y = current_position.y * (1.0f - smooth_factor) + g_camera_stab.last_position.y * smooth_factor;
		setup->origin.z = current_position.z * (1.0f - smooth_factor) + g_camera_stab.last_position.z * smooth_factor;

		// Smooth angles using shortest path
		float pitch_delta = angle_diff(current_pitch, g_camera_stab.last_pitch);
		float yaw_delta = angle_diff(current_yaw, g_camera_stab.last_yaw);

		setup->view.x = normalize_angle(g_camera_stab.last_pitch + pitch_delta * (1.0f - smooth_factor));
		setup->view.y = normalize_angle(g_camera_stab.last_yaw + yaw_delta * (1.0f - smooth_factor));
	}

	// ANGLE WRAPPING FIX
	// Prevent 180-degree snapping
	if (c::visuals::camera_angle_fix) {
		setup->view.x = normalize_angle(setup->view.x);
		setup->view.y = normalize_angle(setup->view.y);
		setup->view.z = normalize_angle(setup->view.z);
	}

	// TELEPORT COMPENSATION
	// Gradually adjust view after sudden position changes
	if (c::visuals::camera_teleport_fix && position_change > 10.0f) {
		const float compensation = std::clamp(position_change / 50.0f, 0.0f, 1.0f);
		// Slightly adjust angles to compensate for position jump
		setup->view.x *= (1.0f - compensation * 0.1f);
	}

	// Update history
	g_camera_stab.last_position = setup->origin;
	g_camera_stab.last_pitch = setup->view.x;
	g_camera_stab.last_yaw = setup->view.y;
	g_camera_stab.last_update_time = current_time;
}

void features::visuals::apply_zoom() {
	if (!interfaces::engine->is_in_game()) {
		return;
	}

	if (c::visuals::apply_zoom && g::local) {
		if ((g::local->fov() == 90 || g::local->fov_start() == 90)) {
			if (menu::checkkey(c::visuals::apply_zoom_key, c::visuals::apply_zoom_key_s)) {
				g::local->fov() = 40;
				g::local->fov_start() = 40;
			}
		}
	}
}

void features::visuals::fog() {
	static auto fog_override = interfaces::console->get_convar("fog_override");

	if (!c::visuals::fog) {
		fog_override->set_value(0);
		return;
	}
	fog_override->set_value(1);

	static auto fog_start = interfaces::console->get_convar("fog_start");

	fog_start->set_value(0);

	static auto fog_end = interfaces::console->get_convar("fog_end");

	fog_end->set_value(c::visuals::fog_distance);

	static auto fog_maxdensity = interfaces::console->get_convar("fog_maxdensity");

	fog_maxdensity->set_value((float)c::visuals::fog_density * 0.01f);

	int red = c::visuals::fog_color[0] * 255;
	int green = c::visuals::fog_color[1] * 255;
	int blue = c::visuals::fog_color[2] * 255;
	char buffer_color[12];
	sprintf_s(buffer_color, 12, "%i %i %i", red, green, blue);

	static auto fog_color = interfaces::console->get_convar("fog_color");

	fog_color->set_value(buffer_color);
}

void features::visuals::gravity_ragdoll() {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game())
		return;

	static auto ragdollGravity = interfaces::console->get_convar("cl_ragdoll_gravity");
	ragdollGravity->set_value(c::visuals::gravity_ragdoll ? -600 : 600);
}

bool update = false;
void features::visuals::skybox_changer() {
	if (!g::local)
		update = true;
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(find_pattern("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));
	static const auto sv_skyname = interfaces::console->get_convar("sv_skyname");
	static std::string skybox_name;
	static int saved_skybox = c::visuals::skybox;
	switch (c::visuals::skybox) {
	case 1: skybox_name = "cs_tibet"; break;
	case 2: skybox_name = "cs_baggage_skybox_"; break;
	case 3: skybox_name = "italy"; break;
	case 4: skybox_name = "jungle"; break;
	case 5: skybox_name = "office"; break;
	case 6: skybox_name = "sky_cs15_daylight01_hdr"; break;
	case 7: skybox_name = "sky_cs15_daylight02_hdr"; break;
	case 8: skybox_name = "vertigoblue_hdr"; break;
	case 9: skybox_name = "vertigo"; break;
	case 10: skybox_name = "sky_day02_05_hdr"; break;
	case 11: skybox_name = "nukeblank"; break;
	case 12: skybox_name = "sky_venice"; break;
	case 13: skybox_name = "sky_cs15_daylight03_hdr"; break;
	case 14: skybox_name = "sky_cs15_daylight04_hdr"; break;
	case 15: skybox_name = "sky_csgo_cloudy01"; break;
	case 16: skybox_name = "sky_csgo_night02"; break;
	case 17: skybox_name = "sky_csgo_night02b"; break;
	case 18: skybox_name = "sky_csgo_night_flat"; break;
	case 19: skybox_name = "sky_dust"; break;
	case 20: skybox_name = "vietnam"; break;
	}

	if (g::local && (update || saved_skybox != c::visuals::skybox)) {
		if (c::visuals::skybox == 0)
			load_skybox(sv_skyname->string);
		else
			load_skybox(skybox_name.c_str());
		saved_skybox = c::visuals::skybox;
	}
}

void features::visuals::notification_system() {
	if (!g::local || !c::visuals::notifcation_system)
		return;

	static float fadeAlpha = 0.0f;  // Transparency value for fade effect
	static float last_update_time = 0;
	static const float fade_in_speed = 0.007f;
	static const float fade_out_speed = 0.007f;
	static const float display_time = 2.75f; // Time the notification is visible

	static std::string current_text = "";
	static std::string notified_text = "";
	static bool is_visible = false;

	if (!notified_text.empty() && notified_text != current_text) {
		// New notification
		current_text = notified_text;
		fadeAlpha = 0.0f;
		last_update_time = interfaces::globals->realtime;
		is_visible = false;
	}

	float current_time = interfaces::globals->realtime;

	// Manage fade in and fade out
	if (is_visible) {
		if (current_time - last_update_time < display_time) {
			// Fade in
			fadeAlpha = min(fadeAlpha + fade_in_speed, 1.0f);
		}
		else {
			// Fade out
			fadeAlpha = max(fadeAlpha - fade_out_speed, 0.0f);
			if (fadeAlpha <= 0.0f) {
				is_visible = false;
				current_text = "";
			}
		}
	}

	if (c::visuals::movement_noti) {
		if (features::movement::should_edge_bug) {
			is_visible = true;
			notified_text = "Linear has assisted a Edgebug";
		}
		else if (features::movement::should_ps) {
			is_visible = true;
			notified_text = "Linear has assisted a Pixelsurf";
		}
		else if (features::movement::detected_normal_jump_bug) {
			is_visible = true;
			notified_text = "Linear has assisted a Jumpbug";
		}
		else if (features::movement::should_mj) {
			is_visible = true;
			notified_text = "Linear has assisted a Minijump";
		}
		else if (features::movement::should_lj) {
			is_visible = true;
			notified_text = "Linear has assisted a Longjump";
		}
	}

	if (!is_visible) return;

	// Screen size
	int w, h;
	interfaces::engine->get_screen_size(w, h);

	// Text and box size
	auto text_size = im_render.measure_text(current_text.c_str(), fonts::watermark_font, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(4, 3);
	static const int box_offset = 10;

	// Centered lower position
	float center_x = (w - text_size.x - padding.x * 2.f) / 2.0f;
	float center_y = h - 175.0f;

	// Colors with alpha fading
	ImColor bg_color(0.11f, 0.11f, 0.11f, fadeAlpha);
	ImColor outline_color(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], fadeAlpha);
	ImColor text_color(255, 255, 255, static_cast<int>(fadeAlpha * 220));

	// Draw notification box
	ImGui::GetBackgroundDrawList()->AddRectFilled(
		{ center_x - 2.f, center_y - 2.f },
		{ center_x + text_size.x + padding.x * 2.f + 2.f, center_y + text_size.y + padding.y * 2.f + 2.f },
		bg_color,
		6.f
	);
	ImGui::GetBackgroundDrawList()->AddRect(
		{ center_x - 3.f, center_y - 3.f },
		{ center_x + text_size.x + padding.x * 2.f + 3.f, center_y + text_size.y + padding.y * 2.f + 3.f },
		outline_color,
		6.f
	);

	// Draw text
	ImGui::PushFont(fonts::watermark_font);
	ImGui::GetBackgroundDrawList()->AddText(
		fonts::watermark_font,
		15.f,
		ImVec2(center_x + padding.x, center_y + padding.y),
		text_color,
		current_text.c_str()
	);
	ImGui::PopFont();
}

void features::visuals::points_system() {
	if (!g::local || !c::movement::points_system)
		return;

	static int points = 0;
	static int combo_count = 0;
	static std::string last_trick_type = "";
	static bool last_ps = false;
	static bool last_air = false;
	static bool last_edge_bug = false;
	static bool last_jump_bug = false;
	static bool last_fireman = false;
	static float fade_alpha = 0.f;
	static float combo_fade_alpha = 0.f;
	static float last_point_time = 0.f;
	static float last_combo_time = 0.f;

	bool gained_points = false;
	std::string current_trick = "";

	// Add points on state change (only when becoming true)
	if (features::movement::should_ps && !last_ps) {
		points += 125;
		current_trick = "ps";
		gained_points = true;
	}
	else if (features::movement::should_air && !last_air) {
		points += 150;
		current_trick = "air";
		gained_points = true;
	}
	else if (features::movement::should_edge_bug && !last_edge_bug) {
		points += 125;
		current_trick = "edge_bug";
		gained_points = true;
	}
	else if (features::movement::detected_normal_jump_bug && !last_jump_bug) {
		points += 50;
		current_trick = "jump_bug";
		gained_points = true;
	}
	else if (features::movement::should_fireman && !last_fireman) {
		points += 125;
		current_trick = "fireman";
		gained_points = true;
	}

	// Combo logic: only combo if same trick type, reset if different
	if (gained_points) {
		if (current_trick == last_trick_type) {
			combo_count++;
		}
		else {
			combo_count = 1;
			last_trick_type = current_trick;
		}
	}

	// Update last states
	last_ps = features::movement::should_ps;
	last_air = features::movement::should_air;
	last_edge_bug = features::movement::should_edge_bug;
	last_jump_bug = features::movement::detected_normal_jump_bug;
	last_fireman = features::movement::should_fireman;

	// Update fade alpha and last point time
	float current_time = interfaces::globals->realtime;
	if (gained_points) {
		fade_alpha = 1.f;
		combo_fade_alpha = 1.f;
		last_point_time = current_time;
		last_combo_time = current_time;
	}

	// Fade out after 3 seconds
	const float display_duration = 3.f;
	const float fade_speed = 3.f;
	if (current_time - last_point_time > display_duration) {
		fade_alpha = std::max<float>(0.f, fade_alpha - fade_speed * interfaces::globals->frame_time);
		// Reset points when fully faded
		if (fade_alpha <= 0.01f) {
			points = 0;
			combo_count = 0;
			last_trick_type = "";
		}
	}

	// Combo fade (slightly faster)
	const float combo_fade_speed = 2.5f;
	if (current_time - last_combo_time > display_duration) {
		combo_fade_alpha = std::max<float>(0.f, combo_fade_alpha - combo_fade_speed * interfaces::globals->frame_time);
	}

	// Don't render if fully faded
	if (fade_alpha <= 0.01f)
		return;

	// Get screen dimensions
	int w, h;
	interfaces::engine->get_screen_size(w, h);

	// Format points text
	std::string points_text = std::to_string(points) + " pts";

	ImGui::PushFont(fonts::points_big_font);
	auto text_size = ImGui::CalcTextSize(points_text.c_str());
	ImGui::PopFont();

	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);
	static const float spacing = -30.f; // Space between points system and watermark

	// Calculate watermark width (approximate from your watermark function)
	std::string wm_text = "dna | beta | 000 fps"; // Approximate max size
	ImGui::PushFont(fonts::points_big_font);
	auto wm_size = ImGui::CalcTextSize(wm_text.c_str());
	ImGui::PopFont();
	float wm_width = wm_size.x + padding.x + margin.x * 2.f;

	// Position to the left of watermark
	float points_right_x = w - wm_width - spacing;
	ImVec2 text_pos = ImVec2(points_right_x - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(points_right_x - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(points_right_x - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// Calculate sine wave for pulsing effect
	float sine_value = sinf(current_time * 3.f) * 0.5f + 0.5f; // 0 to 1 range
	float pulse_intensity = 0.3f + sine_value * 0.7f; // 0.3 to 1.0 range

	// Apply fade to all alpha values
	pulse_intensity *= fade_alpha;

	// Colors
	ImColor accent(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], fade_alpha);
	ImColor white(1.f, 1.f, 1.f, pulse_intensity);
	ImColor shadow(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], pulse_intensity);
	ImColor fg_color(10, 10, 12, 255 * (int)fade_alpha);
	ImColor fg2_color(10, 10, 12, 255 * (int)fade_alpha);
	ImColor bg_color(0.14f, 0.14f, 0.14f, fade_alpha);
	ImColor bgfade_color(10, 10, 12, 50 * (int)fade_alpha);
	ImColor bg2_color(10, 10, 12, 200 * (int)fade_alpha);
	ImColor bg3_color(4, 4, 5, 50 * (int)fade_alpha);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
	// Glow fade multiplier (slower than main fade for lingering effect)
	float glow_fade_multiplier = 0.5f; // 0.5 = glow fades at half the speed
	float glow_alpha_final = fade_alpha + (1.f - fade_alpha) * glow_fade_multiplier;
	glow_alpha_final = std::min<float>(glow_alpha_final, 1.f); // clamp to 1.0

	if (c::visuals::style_points_background) {
		// Pulsing glow effect
		if (pulse_intensity > 0.15f) {
			ImColor glow(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				glow_alpha_final
			);

			const float glow_expand = 7.f * glow_alpha_final;
			const int layers = 10;

			for (int i = 0; i < layers; i++) {
				float t = float(i) / float(layers - 1);
				float expand = glow_expand * t;
				float alpha = glow_alpha_final * 0.5f * (1.f - t);

				ImColor col(glow.Value.x, glow.Value.y, glow.Value.z, alpha);

				draw_list->AddRectFilled(
					ImVec2(top_left.x - expand, top_left.y - expand),
					ImVec2(bottom_right.x + expand, bottom_right.y + expand),
					col,
					5.5f,
					ImDrawFlags_RoundCornersAll
				);
			}
		}
		// Background
		draw_list->AddRectFilledMultiColorRounded(
			top_left, bottom_right,
			bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
			5.5f, ImDrawFlags_RoundCornersAll
		);
		draw_list->AddRectFilledMultiColorRounded(
			top_left, bottom_right,
			bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
			5.5f, ImDrawFlags_RoundCornersAll
		);
		draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
		draw_list->AddRect(
			{ top_left.x - 1.f, top_left.y - 1.f },
			{ bottom_right.x + 1.f, bottom_right.y + 1.f },
			bg3_color, 5.5f
		);
		draw_list->AddRect(
			{ top_left.x - 2.f, top_left.y - 2.f },
			{ bottom_right.x + 2.f, bottom_right.y + 2.f },
			bg2_color, 5.5f
		);

		// Gradient bar at top
		const float grad_width = text_size.x * 1.1f;
		const float grad_height = 3.f;
		const float center_x = text_pos.x + text_size.x * 0.5f;
		const float grad_top_y = text_pos.y - grad_height - 3.f;
		const float grad_bottom_y = grad_top_y + grad_height;
		const int slices = 60;

		for (int i = 0; i < slices; i++) {
			float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
			float slice_x1 = slice_x0 + (grad_width / slices);

			float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
			float alpha = 1.f - dist;
			if (alpha < 0.f) alpha = 0.f;

			ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * alpha * pulse_intensity);

			draw_list->AddRectFilled(
				ImVec2(slice_x0, grad_top_y),
				ImVec2(slice_x1, grad_bottom_y),
				col,
				2.0f, 0
			);
		}
	}

	// Draw text with wave per letter
	ImGui::PushFont(fonts::points_big_font);

	float letter_spacing = 0.f;

	// Wave settings
	float wave_amplitude = 4.f;   // height of wave
	float wave_speed = 6.f;   // speed of movement
	float wave_offset = current_time * wave_speed;

	// Ghost shadow settings
	int shadow_layers = 4;           // number of shadow "ghosts"
	float shadow_step = 1.15f;       // spacing between ghost layers
	ImColor ghost_color = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], menu::menu_accent[3]);  // your accent color

	for (size_t i = 0; i < points_text.size(); i++)
	{
		const char c = points_text[i];
		std::string s(1, c);

		// Wave offset for this letter
		float offset_y = sinf(wave_offset + i * 0.15f) * wave_amplitude;

		ImVec2 letter_pos(
			text_pos.x + letter_spacing,
			text_pos.y + offset_y
		);

		// ===== GHOST TRAILS ABOVE & BELOW =====
		for (int g = 1; g <= shadow_layers; g++)
		{
			float t = float(g) / float(shadow_layers + 1);
			float ghost_offset = shadow_step * g;
			float ghost_offset_down = 0.7f * g;
			float alpha = (1.f - t) * fade_alpha;

			// GHOST BELOW   (positive Y)
			ImVec2 ghost_pos_down(letter_pos.x, letter_pos.y + ghost_offset_down);

			// GHOST ABOVE   (negative Y)
			ImVec2 ghost_pos_up(letter_pos.x, letter_pos.y - ghost_offset);

			ImU32 faded_ghost = ImGui::GetColorU32(ImVec4(
				((ghost_color >> 0) & 0xFF) / 255.f,
				((ghost_color >> 8) & 0xFF) / 255.f,
				((ghost_color >> 16) & 0xFF) / 255.f,
				alpha * 0.7f  // smaller alpha → closer to original text
			));

			draw_list->AddText(ghost_pos_down, faded_ghost, s.c_str());
			draw_list->AddText(ghost_pos_up, faded_ghost, s.c_str());
		}

		// ===== MAIN TEXT =====
		draw_list->AddText(
			ImVec2(letter_pos.x, letter_pos.y),
			IM_COL32(255, 255, 255, 255 * fade_alpha),
			s.c_str()
		);

		// Advance spacing
		letter_spacing += ImGui::CalcTextSize(s.c_str()).x;
	}

	ImGui::PopFont();

	// Draw combo text if combo > 1
	if (c::visuals::style_points_combo && combo_count > 1 && combo_fade_alpha > 0.01f) {
		std::string combo_text = "x" + std::to_string(combo_count) + " combo";

		ImGui::PushFont(fonts::points_big_font);
		auto combo_size = ImGui::CalcTextSize(combo_text.c_str());

		// Position below main text
		float combo_y_offset = text_size.y + 8.f;
		ImVec2 combo_base_pos(
			text_pos.x + (text_size.x - combo_size.x) * 0.5f, // Center it
			text_pos.y + combo_y_offset
		);

		float combo_letter_spacing = 0.f;

		// Combo wave settings (same as main text but phase offset)
		float combo_wave_amplitude = 4.f;
		float combo_wave_speed = 6.f;
		float combo_wave_offset = current_time * combo_wave_speed + 1.5f; // Phase offset

		// Combo ghost settings (same as main text)
		int combo_shadow_layers = 4;
		float combo_shadow_step = 1.15f;
		ImColor combo_ghost_color = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], menu::menu_accent[3]);

		for (size_t i = 0; i < combo_text.size(); i++)
		{
			const char c = combo_text[i];
			std::string s(1, c);

			// Wave offset for this letter
			float offset_y = sinf(combo_wave_offset + i * 0.15f) * combo_wave_amplitude;

			ImVec2 letter_pos(
				combo_base_pos.x + combo_letter_spacing,
				combo_base_pos.y + offset_y
			);

			// ===== GHOST TRAILS ABOVE & BELOW =====
			for (int g = 1; g <= combo_shadow_layers; g++)
			{
				float t = float(g) / float(combo_shadow_layers + 1);
				float ghost_offset = combo_shadow_step * g;
				float ghost_offset_down = 0.7f * g;
				float alpha = (1.f - t) * combo_fade_alpha;

				// GHOST BELOW
				ImVec2 ghost_pos_down(letter_pos.x, letter_pos.y + ghost_offset_down);

				// GHOST ABOVE
				ImVec2 ghost_pos_up(letter_pos.x, letter_pos.y - ghost_offset);

				ImU32 faded_ghost = ImGui::GetColorU32(ImVec4(
					((combo_ghost_color >> 0) & 0xFF) / 255.f,
					((combo_ghost_color >> 8) & 0xFF) / 255.f,
					((combo_ghost_color >> 16) & 0xFF) / 255.f,
					alpha * 0.7f
				));

				draw_list->AddText(ghost_pos_down, faded_ghost, s.c_str());
				draw_list->AddText(ghost_pos_up, faded_ghost, s.c_str());
			}

			// ===== MAIN COMBO TEXT =====
			draw_list->AddText(
				letter_pos,
				IM_COL32(255, 255, 255, 255 * combo_fade_alpha),
				s.c_str()
			);

			combo_letter_spacing += ImGui::CalcTextSize(s.c_str()).x;
		}

		ImGui::PopFont();
	}
}

void features::visuals::jump_distance_display()
{
	if (!g::local || !g::local->is_alive())
		return;

	static vec3_t takeoff_pos = { };
	static float jump_distance = 0.f;
	static bool was_onground = false;

	static float fade = 0.f;
	static float land_time = 0.f;

	float current_time = interfaces::globals->realtime;

	bool onground = (g::local->flags() & fl_onground);

	// ================================
	//   REAL GOKZ TAKEOFF CAPTURE
	// ================================
	if (was_onground && !onground)
	{
		vec3_t vel = g::local->velocity();

		if (vel.z > 1.f && (g::cmd->buttons & in_jump))
			takeoff_pos = g::local->origin();
	}

	// ================================
	//   REAL GOKZ LANDING CAPTURE
	// ================================
	if (!was_onground && onground)
	{
		vec3_t land_pos = g::local->origin();

		float dx = land_pos.x - takeoff_pos.x;
		float dy = land_pos.y - takeoff_pos.y;

		float planar = sqrtf(dx * dx + dy * dy);

		// Player horizontal hull radius in Source
		constexpr float PLAYER_RADIUS = 16.f;

		// Add radius at takeoff + landing
		jump_distance = planar + (PLAYER_RADIUS * 2.f);

		fade = 1.f;
		land_time = current_time;
	}

	was_onground = onground;

	// Fade behavior
	if (current_time - land_time > 3.f)
		fade = std::max<float>(0.f, fade - 3.f * interfaces::globals->frame_time);

	if (fade <= 0.01f)
		return;

	// ======================
	//  RENDER (unchanged)
	// ======================
	std::string text = std::to_string((int)jump_distance) + "u";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	ImVec2 base_pos(w * 0.75f, 90.f);

	float wave_amp = 4.f;
	float wave_speed = 6.f;
	float wave_off = current_time * wave_speed;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImGui::PushFont(fonts::points_big_font);

	float spacing = 0.f;

	for (size_t i = 0; i < text.size(); i++)
	{
		std::string s(1, text[i]);

		float offset_y = sinf(wave_off + i * 0.15f) * wave_amp;

		ImVec2 pos(base_pos.x + spacing, base_pos.y + offset_y);

		// Ghost trails identical to your points
		const int ghost_layers = 4;
		for (int g = 1; g <= ghost_layers; g++)
		{
			float t = float(g) / (ghost_layers + 1);
			float alpha = (1.f - t) * fade;

			ImU32 ghost_col = ImGui::GetColorU32(ImVec4(
				menu::menu_accent[0],
				menu::menu_accent[1],
				menu::menu_accent[2],
				alpha
			));

			float up = 1.15f * g;
			float down = 0.7f * g;

			draw->AddText(ImVec2(pos.x, pos.y - up), ghost_col, s.c_str());
			draw->AddText(ImVec2(pos.x, pos.y + down), ghost_col, s.c_str());
		}

		draw->AddText(pos, IM_COL32(255, 255, 255, 255 * fade), s.c_str());

		spacing += ImGui::CalcTextSize(s.c_str()).x;
	}

	ImGui::PopFont();
}

void features::visuals::watermark() {
	if (!c::movement::billware_wm)
		return;

	if (!(c::movement::watermark_type == 0))
		return;

	static float frame_rate = 0.f;
	static float last_update_time = 0.f;
	static const float update_interval = 0.5f;

	float current_time = interfaces::globals->realtime;

	// prevent division by zero
	float ft = interfaces::globals->frame_time;
	if (ft <= 0.f)
		ft = 0.0001f;

	if (current_time - last_update_time >= update_interval) {
		const float alpha = 0.7f;
		frame_rate = alpha * frame_rate + (1.f - alpha) * (1.f / ft);

		last_update_time = current_time;
	}

	int fps = static_cast<int>(frame_rate);
	std::string fps_number = std::to_string(fps);
	std::string fps_label = " fps";

	std::string part1 = "dna";
	std::string sep1 = " | ";
	std::string part2 = "beta";
	std::string sep2 = " | ";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	std::string full_text = part1 + sep1 + part2 + sep2 + fps_number + fps_label;
	auto text_size = im_render.measure_text(full_text.c_str(), fonts::watermark_font, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);

	ImVec2 text_pos = ImVec2(w - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(w - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(w - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// === BACKGROUND DRAWING ===
	ImColor accent(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 1.f);
	ImColor darkgrey(0.4f, 0.4f, 0.4f, 1.f);
	ImColor white(0.8f, 0.8f, 0.8f, 1.f);
	ImColor fg_color(10, 10, 12, 255);
	ImColor fg2_color(10, 10, 12, 255);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 1.f);
	ImColor bgfade_color(10, 10, 12, 50);
	ImColor bg2_color(10, 10, 12, 200);
	ImColor bg3_color(4, 4, 5, 50);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	// Track when single-tick events occurred
	static int last_jump_bug_tick = 0;
	static int last_fireman_tick = 0;
	static int last_air_tick = 0;
	static int last_edgebug_tick = 0;

	// Update saved ticks when events occur
	if (features::movement::detected_normal_jump_bug)
		last_jump_bug_tick = interfaces::globals->tick_count;
	if (features::movement::should_fireman)
		last_fireman_tick = interfaces::globals->tick_count;
	if (features::movement::should_air)
		last_air_tick = interfaces::globals->tick_count;
	if (features::movement::should_edge_bug)
		last_edgebug_tick = interfaces::globals->tick_count;

	// Check if we're within the glow duration for single-tick events
	bool jump_bug_active = (interfaces::globals->tick_count - last_jump_bug_tick) < c::movement::detection_saved_tick;
	bool fireman_active = (interfaces::globals->tick_count - last_fireman_tick) < c::movement::detection_saved_tick;
	bool air_active = (interfaces::globals->tick_count - last_air_tick) < c::movement::detection_saved_tick;
	bool edgebug_active = (interfaces::globals->tick_count - last_edgebug_tick) < c::movement::detection_saved_tick;

	// fade strength
	static float glow_strength = 0.f;
	const float fade_speed = 3.f; // faster fade (increase for faster)
	bool spotted = features::visuals::is_spotted ||
		features::movement::should_ps ||
		edgebug_active ||
		jump_bug_active ||
		fireman_active ||
		air_active;

	// fade in/out
	if (spotted)
		glow_strength = std::min<float>(1.f, glow_strength + fade_speed * interfaces::globals->frame_time);
	else
		glow_strength = std::max<float>(0.f, glow_strength - fade_speed * interfaces::globals->frame_time);

	// ===== ROUNDED-RECT NEON GLOW (matches background shape) =====
	if (glow_strength > 0.01f)
	{
		ImColor neon(
			menu::menu_accent[0],
			menu::menu_accent[1],
			menu::menu_accent[2],
			1.f
		);

		const float base_alpha = 0.75f * glow_strength;
		const float base_expand = 7.f * glow_strength; // how far glow expands

		const int layers = 10;  // more = smoother
		for (int i = 0; i < layers; i++)
		{
			float t = float(i) / float(layers - 1);

			float expand = base_expand * t;           // outer expansion size
			float alpha = base_alpha * (1.f - t);    // fade outward

			ImColor col(
				neon.Value.x,
				neon.Value.y,
				neon.Value.z,
				alpha
			);

			draw_list->AddRectFilled(
				ImVec2(top_left.x - expand, top_left.y - expand),
				ImVec2(bottom_right.x + expand, bottom_right.y + expand),
				col,
				5.5f, // <= matches background perfectly
				ImDrawFlags_RoundCornersAll
			);
		}

		// strong inner neon punch
		draw_list->AddRectFilled(
			ImVec2(top_left.x - 2, top_left.y - 2),
			ImVec2(bottom_right.x + 2, bottom_right.y + 2),
			ImColor(neon.Value.x, neon.Value.y, neon.Value.z, glow_strength * 0.9f),
			5.5f,
			ImDrawFlags_RoundCornersAll
		);
	}

	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
	draw_list->AddRect(
		{ top_left.x - 1.f, top_left.y - 1.f },
		{ bottom_right.x + 1.f, bottom_right.y + 1.f },
		bg3_color, 5.5f
	);
	draw_list->AddRect(
		{ top_left.x - 2.5f, top_left.y - 2.5f },
		{ bottom_right.x + 2.5f, bottom_right.y + 2.5f },
		bg2_color, 5.5f
	);

	// === SNOW EFFECT ===
	static std::vector<ImVec2> snow_positions;
	static const int max_snowflakes = 10;
	static const float snow_speed = 0.07f;
	static const float snow_opacity = 0.4f;

	if (snow_positions.empty()) {
		for (int i = 0; i < max_snowflakes; ++i) {
			float x = top_left.x + (rand() % static_cast<int>(bottom_right.x - top_left.x));
			float y = top_left.y + (rand() % static_cast<int>(bottom_right.y - top_left.y));
			snow_positions.push_back(ImVec2(x, y));
		}
	}

	for (auto& pos : snow_positions) {
		ImGui::PushFont(fonts::dna_icon_rain);
		//draw_list->AddText(pos, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 50.f), "A");
		ImGui::PopFont();
		//draw_list->AddCircleFilled(pos, 0.5f, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], snow_opacity));

		pos.y += snow_speed;
		if (pos.y > bottom_right.y - 1) {
			pos.y = top_left.y + (rand() % static_cast<int>(bottom_right.y - top_left.y));
			pos.x = top_left.x + (rand() % static_cast<int>(bottom_right.x - top_left.x));
		}
	}

	// === GRADIENT CUT (TOP BAR) ===
	const ImColor bloodRedStrong = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f);
	const float grad_width = text_size.x * 1.1f;
	const float grad_height = 3.f;
	const float center_x = text_pos.x + text_size.x * 0.5f;
	const float grad_top_y = text_pos.y - grad_height - 3.f;
	const float grad_bottom_y = grad_top_y + grad_height;
	const int slices = 60;

	for (int i = 0; i < slices; i++) {
		float slice_x0 = center_x - grad_width * 0.5f + (grad_width / slices) * i;
		float slice_x1 = slice_x0 + (grad_width / slices);

		float dist = fabsf((slice_x0 + slice_x1) * 0.5f - center_x) / (grad_width * 0.5f);
		float alpha = 1.f - dist;
		if (alpha < 0.f) alpha = 0.f;

		ImColor col = ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.75f * alpha);

		draw_list->AddRectFilled(
			ImVec2(slice_x0, grad_top_y),
			ImVec2(slice_x1, grad_bottom_y),
			col,
			2.0f, 0
		);
	}

	//// === ICON (PARTIALLY CLIPPED, SAME AS CLARITY) ===
	//{
	//	ImGui::PushFont(fonts::logo_watermark); // same icon font

	//	const char* icon = "A";
	//	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	//	// same visible ratio as clarity
	//	const float visible_ratio = 0.7f;

	//	// position identical logic
	//	ImVec2 icon_pos(
	//		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
	//		text_pos.y - 25.f
	//	);

	//	// clip strictly to watermark bounds
	//	draw_list->PushClipRect(top_left, bottom_right, true);
	//	draw_list->AddText(icon_pos,
	//		ImColor(
	//			menu::menu_accent[0],
	//			menu::menu_accent[1],
	//			menu::menu_accent[2],
	//			0.15f // same subtle fade as clarity
	//		),
	//		icon
	//	);
	//	draw_list->PopClipRect();

	//	ImGui::PopFont();
	//}

	// === TEXT DRAWING ===
	ImGui::PushFont(fonts::watermark_font);

	ImVec2 cursor = text_pos;

	// Draw text parts with alternating colors
	draw_list->AddText(ImVec2(cursor.x + 1, cursor.y + 1), white, part1.c_str());
	draw_list->AddText(cursor, accent, part1.c_str());
	cursor.x += ImGui::CalcTextSize(part1.c_str()).x;

	draw_list->AddText(cursor, darkgrey, sep1.c_str());
	cursor.x += ImGui::CalcTextSize(sep1.c_str()).x;

	draw_list->AddText(cursor, darkgrey, part2.c_str());
	cursor.x += ImGui::CalcTextSize(part2.c_str()).x;

	draw_list->AddText(cursor, darkgrey, sep2.c_str());
	cursor.x += ImGui::CalcTextSize(sep2.c_str()).x;

	draw_list->AddText(cursor, white, fps_number.c_str());
	cursor.x += ImGui::CalcTextSize(fps_number.c_str()).x;

	draw_list->AddText(cursor, darkgrey, fps_label.c_str());

	ImGui::PopFont();
}

void features::visuals::clarity_watermark() {
	if (!c::movement::billware_wm)
		return;

	if (!(c::movement::watermark_type == 1))
		return;

	static float frame_rate = 0.f;
	static float last_update_time = 0.f;
	static const float update_interval = 0.5f;

	float current_time = interfaces::globals->realtime;

	// prevent division by zero
	float ft = interfaces::globals->frame_time;
	if (ft <= 0.f)
		ft = 0.0001f;

	if (current_time - last_update_time >= update_interval) {
		const float alpha = 0.7f;
		frame_rate = alpha * frame_rate + (1.f - alpha) * (1.f / ft);

		last_update_time = current_time;
	}

	int fps = static_cast<int>(frame_rate);
	std::string fps_number = std::to_string(fps);
	std::string fps_label = " fps";

	std::string part1 = "clarity";
	std::string sep1 = " | ";
	std::string part2 = "dev";
	std::string sep2 = " | ";

	int w, h;
	interfaces::engine->get_screen_size(w, h);

	std::string full_text = part1 + sep1 + part2 + sep2 + fps_number + fps_label;
	auto text_size = im_render.measure_text(full_text.c_str(), fonts::clarity_watermark, 15.f);
	static const ImVec2 padding = ImVec2(7, 7);
	static const ImVec2 margin = ImVec2(3, 3);

	ImVec2 text_pos = ImVec2(w - text_size.x - padding.x - margin.x, padding.y + margin.y);

	ImVec2 top_left(w - text_size.x - padding.x - margin.x * 2.f - 2.f, padding.y - 2.f);
	ImVec2 bottom_right(w - padding.x + 2.f, text_size.y + padding.y + margin.y * 2.f + 2.f);

	// === BACKGROUND DRAWING ===
	ImColor accent(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 1.f);
	ImColor accent2(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], 0.15f);
	ImColor darkgrey(0.5f, 0.5f, 0.5f, 1.f);
	ImColor darkgrey2(0.2f, 0.2f, 0.2f, 1.f);
	ImColor white(0.8f, 0.8f, 0.8f, 1.f);
	ImColor fg_color(10, 10, 12, 255);
	ImColor fg2_color(10, 10, 12, 255);
	ImColor bg_color(0.14f, 0.14f, 0.14f, 1.f);
	ImColor bgfade_color(10, 10, 12, 50);
	ImColor bg2_color(20, 20, 22, 250);
	ImColor bg3_color(4, 4, 5, 100);

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, fg2_color, fg2_color, bg_color, bg_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRectFilledMultiColorRounded(
		top_left, bottom_right,
		bgfade_color, bg_color, bg_color, fg2_color, fg2_color,
		5.5f, ImDrawFlags_RoundCornersAll
	);
	draw_list->AddRect(top_left, bottom_right, bg2_color, 5.5f);
	draw_list->AddRect(
		{ top_left.x - 1.f, top_left.y - 1.f },
		{ bottom_right.x + 1.f, bottom_right.y + 1.f },
		bg3_color, 5.5f
	);
	draw_list->AddRect(
		{ top_left.x - 2.5f, top_left.y - 2.5f },
		{ bottom_right.x + 2.5f, bottom_right.y + 2.5f },
		bg2_color, 5.5f
	);

	// === TEXT DRAWING ===
	ImGui::PushFont(fonts::clarity_watermark);

	ImVec2 cursor = text_pos;

	// === ICON (PARTIALLY CLIPPED, NO BOX) ===
	ImGui::PushFont(fonts::watermark_icons);

	const char* icon = "A";
	ImVec2 icon_size = ImGui::CalcTextSize(icon);

	// show only ~70% of icon width
	const float visible_ratio = 0.7f;
	const float advance_x = icon_size.x * visible_ratio;

	// draw icon shifted left so part is clipped
	ImVec2 icon_pos(
		top_left.x + 6.f - (icon_size.x * (1.f - visible_ratio)),
		cursor.y - 17.f
	);

	// clip strictly to watermark bounds
	draw_list->PushClipRect(top_left, bottom_right, true);
	draw_list->AddText(icon_pos, accent2, icon);
	draw_list->PopClipRect();

	// advance cursor by visible portion only
	//cursor.x = top_left.x + 6.f + advance_x + 4.f;

	ImGui::PopFont();

	// Draw text parts with alternating colors
	draw_list->AddText(cursor, accent, part1.c_str());
	cursor.x += ImGui::CalcTextSize(part1.c_str()).x;

	draw_list->AddText(cursor, darkgrey2, sep1.c_str());
	cursor.x += ImGui::CalcTextSize(sep1.c_str()).x;

	draw_list->AddText(cursor, darkgrey, part2.c_str());
	cursor.x += ImGui::CalcTextSize(part2.c_str()).x;

	draw_list->AddText(cursor, darkgrey2, sep2.c_str());
	cursor.x += ImGui::CalcTextSize(sep2.c_str()).x;

	draw_list->AddText(cursor, white, fps_number.c_str());
	cursor.x += ImGui::CalcTextSize(fps_number.c_str()).x;

	draw_list->AddText(cursor, darkgrey, fps_label.c_str());

	ImGui::PopFont();
}

void features::visuals::init() {
	static bool show_init_screen = true;
	static float init_start_time = interfaces::globals->realtime;
	const float init_duration = 5.0f; // extended duration for longer load
	static constexpr int max_particles = 75;
	static struct particle_t {
		ImVec2 pos;
		ImVec2 velocity;
		float alpha;
		float size;
	} particles[max_particles];
	static bool initialized_particles = false;

	float elapsed = interfaces::globals->realtime - init_start_time;
	float alpha = 1.0f;

	// Fade out
	if (elapsed >= init_duration) {
		alpha = 1.0f - (elapsed - init_duration);
		if (alpha <= 0.0f) {
			show_init_screen = false;
			has_initalized = true;
			initialized_particles = false;
			return;
		}
	}

	// Screen setup
	int screen_w, screen_h;
	interfaces::engine->get_screen_size(screen_w, screen_h);
	ImVec2 center(screen_w / 2.f, screen_h / 2.f);
	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	// Background overlay
	draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(screen_w, screen_h), ImColor(0.02f, 0.02f, 0.02f, 0.8f * alpha));

	const float spinner_radius = 35.f;
	const float thickness = 4.f;
	const float rotation_speed = 0.4f; // slower rotation
	const float arc_length = m_pi * 1.2f; // length of the arc in radians
	float current_time = interfaces::globals->realtime;

	// Calculate start angle for rotation
	float start_angle = current_time * rotation_speed;
	float end_angle = start_angle + arc_length;

	// Center Text
	std::string loading_text = "initializing...";
	std::string software_text = "dna";
	auto text_size = ImGui::CalcTextSize(loading_text.c_str());
	ImVec2 text_pos(center.x - text_size.x / 2.f, center.y + spinner_radius + 30.f);
	draw_list->AddText(ImVec2(text_pos.x - 1, text_pos.y - 1), ImColor(1.f, 1.f, 1.f, alpha * 0.4f), loading_text.c_str());	
	draw_list->AddText(text_pos, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), loading_text.c_str());

	ImGui::PushFont(fonts::logo);
	draw_list->AddText(ImVec2(text_pos.x - 7, text_pos.y - 100), ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), "A");
	ImGui::PopFont();

	// "DNA" below
	auto software_size = ImGui::CalcTextSize(software_text.c_str());
	draw_list->AddText(ImVec2(center.x - software_size.x / 2.f - 6, text_pos.y + text_size.y + 10.f),
		ImColor(1.f, 1.f, 1.f, alpha * 0.7f), software_text.c_str());

	draw_list->AddText(ImVec2(center.x - software_size.x / 2.f - 7, text_pos.y + text_size.y + 11.f),
		ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha * 0.7f), software_text.c_str());

	// Initialize particles once
	if (!initialized_particles) {
		for (auto& p : particles) {
			p.pos = ImVec2(rand() % screen_w, rand() % screen_h);
			p.velocity = ImVec2((rand() % 10 - 5) * 0.15f, -(0.8f + (rand() % 5) * 0.15f));
			p.alpha = 0.4f + (rand() % 60) / 100.f;
			p.size = 1.8f + (rand() % 50) / 30.f;
		}
		initialized_particles = true;
	}

	// Animate particles
	for (auto& p : particles) {
		p.pos.x += p.velocity.x;
		p.pos.y += p.velocity.y;
		if (p.pos.y < 0 || p.pos.x < 0 || p.pos.x > screen_w) {
			p.pos = ImVec2(rand() % screen_w, screen_h + (rand() % 50));
			p.velocity = ImVec2((rand() % 10 - 5) * 0.15f, -(0.8f + (rand() % 5) * 0.15f));
			p.alpha = 0.4f + (rand() % 60) / 100.f;
			p.size = 1.8f + (rand() % 50) / 30.f;
		}
		draw_list->AddCircleFilled(p.pos, p.size, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], p.alpha * alpha));
	}

	// Progress calculation
	float progress = (elapsed > init_duration) ? 1.f : (elapsed / init_duration);
	const float bar_width = screen_w * 0.6f;
	const float bar_height = 8.f;
	ImVec2 bar_pos(center.x - bar_width / 2.f, text_pos.y + 50.f);

	// Draw background of progress bar
	draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_width, bar_pos.y + bar_height), ImColor(0.1f, 0.1f, 0.1f, alpha), 4.f);

	// Glowing layered effect for the progress fill
	const int glow_layers = 4;
	for (int i = glow_layers; i > 0; --i) {
		float layer_opacity = alpha * 0.2f / i;
		float layer_size = (bar_width * progress) * (1.f + 0.2f * i);
		draw_list->AddRectFilled(
			ImVec2(bar_pos.x, bar_pos.y),
			ImVec2(bar_pos.x + bar_width * progress, bar_pos.y + bar_height),
			ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], layer_opacity),
			4.f
		);
	}

	// Main fill
	ImVec2 fill_end = ImVec2(bar_pos.x + bar_width * progress, bar_pos.y + bar_height);
	draw_list->AddRectFilled(bar_pos, fill_end, ImColor(menu::menu_accent[0], menu::menu_accent[1], menu::menu_accent[2], alpha), 4.f);
}

void features::visuals::low_fps_warning() {
	if (!g::local || !g::local->is_alive())
		return;

	float frame_time = interfaces::globals->frame_time;
	if (frame_time <= 0.0f)
		return;

	float fps = 1.0f / frame_time;
	if (fps >= 60.0f)
		return; // No warning needed

	// Get screen size
	int screen_w, screen_h;
	interfaces::engine->get_screen_size(screen_w, screen_h);
	ImDrawList* draw = ImGui::GetBackgroundDrawList();

	// Text segments
	const char* left = "! ";
	const char* mid = "LOW FPS";
	const char* right = " !";

	std::string full = std::string(left) + mid + right;
	ImVec2 full_size = ImGui::CalcTextSize(full.c_str());

	// Centered position
	ImVec2 center_pos = ImVec2(screen_w / 2.f - full_size.x / 2.f, screen_h / 2.f - full_size.y / 2.f);
	ImVec2 segment_pos = center_pos;

	// Pulsing alpha for red LOW FPS
	float pulse = 0.5f + 0.5f * std::sin(ImGui::GetTime() * 4.f); // 0..1
	float red_alpha = 0.5f + 0.5f * pulse;                        // 0.5..1

	ImFont* font = fonts::watermark_font;
	if (font)
		ImGui::PushFont(font);

	// Left white "!"
	draw->AddText(segment_pos, ImColor(1.f, 1.f, 1.f, 1.f), left);
	segment_pos.x += ImGui::CalcTextSize(left).x;

	// Red pulsing "LOW FPS"
	draw->AddText(segment_pos, ImColor(0.95f, 0.1f, 0.1f, red_alpha), mid);
	segment_pos.x += ImGui::CalcTextSize(mid).x;

	// Right white "!"
	draw->AddText(segment_pos, ImColor(1.f, 1.f, 1.f, 1.f), right);

	if (font)
		ImGui::PopFont();
}

bool once = false;
void features::visuals::removals() {
	if (!c::visuals::enable_removals || !interfaces::engine->is_in_game() || !g::local)
		return;

	static convar* shadows = interfaces::console->get_convar("cl_csm_enabled");
	static convar* sway = interfaces::console->get_convar("cl_wpn_sway_scale");
	static convar* postproccesing = interfaces::console->get_convar("mat_postprocess_enable");
	static convar* sky = interfaces::console->get_convar("r_3dsky");
	static convar* mat_postprocess_enable = interfaces::console->get_convar("mat_disable_bloom");
	static convar* blur = interfaces::console->get_convar("@panorama_disable_blur");
	static bool* disable_postprocessing = *reinterpret_cast<bool**>(find_pattern("client.dll", "83 EC 4C 80 3D") + 0x5);
	*disable_postprocessing = c::visuals::removals[1];

	if (c::misc::swayscale) {
		sway->set_value(0);
	}
	else {
		sway->set_value(1);
	}

	if (c::visuals::skybox || c::visuals::remove_3dsky) {
		sky->set_value(0);
	}
	else {
		sky->set_value(1);
	}
	if (c::visuals::remove_shadows) {
		shadows->set_value(0);
	}
	else {
		shadows->set_value(1);
	}
	if (c::visuals::remove_post_processing) {
		mat_postprocess_enable->set_value(0);
	}
	else {
		mat_postprocess_enable->set_value(1);
	}
	if (c::visuals::remove_panorama_blur && !once) {
		blur->set_value(0);
		once = true;
	}
	else if (once && !c::visuals::remove_panorama_blur) {
		blur->set_value(1);
		once = false;
	}
}

void features::visuals::fullbright() {
	static auto bright = interfaces::console->get_convar("mat_fullbright");
	bright->set_value(c::visuals::fullbright ? 1 : 0);
}

void features::visuals::flashalpha() {
	if (!interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || !g::local)
		return;

	if (c::visuals::change_flashalpha) {

		g::local->flash_alpha() = 0;
	}
}

void features::visuals::nosmoke() {
	static auto linegoesthrusmoke = find_pattern("client.dll", "A3 ? ? ? ? 57 8B CB");
	static bool set = true;
	std::vector<const char*> vistasmoke_wireframe = {
		"particle/vistasmokev1/vistasmokev1_smokegrenade",
	};

	std::vector<const char*> vistasmoke_nodraw = {
		"particle/vistasmokev1/vistasmokev1_fire",
		"particle/vistasmokev1/vistasmokev1_emods",
		"particle/vistasmokev1/vistasmokev1_emods_impactdust",
	};

	if (!c::visuals::nosmoke) {
		if (set) {
			for (auto material_name : vistasmoke_wireframe) {
				i_material* smoke = interfaces::material_system->find_material(material_name, TEXTURE_GROUP_OTHER);
				smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, false);
			}
			for (auto material_name : vistasmoke_nodraw) {
				i_material* smoke = interfaces::material_system->find_material(material_name, TEXTURE_GROUP_OTHER);
				smoke->set_material_var_flag(material_var_flags_t::material_var_no_draw, false);
			}
			set = false;
		}
		return;
	}

	set = true;

	for (auto mat_s : vistasmoke_wireframe) {
		i_material* smoke = interfaces::material_system->find_material(mat_s, TEXTURE_GROUP_OTHER);
		smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, true);
	}

	for (auto mat_n : vistasmoke_nodraw) {
		i_material* smoke = interfaces::material_system->find_material(mat_n, TEXTURE_GROUP_OTHER);
		smoke->set_material_var_flag(material_var_flags_t::material_var_wireframe, true);
	}

	static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x1);
	*(int*)(smokecout) = 0;
}

std::tuple<float, float, float> draw_rainbow(float speed) {
	constexpr float pi = std::numbers::pi_v<float>;
	float r = std::sin(speed * interfaces::globals->realtime) * 0.5f + 0.5f;
	float g = std::sin(speed * interfaces::globals->realtime + 2 * pi / 3) * 0.5f + 0.5f;
	float b = std::sin(speed * interfaces::globals->realtime + 4 * pi / 3) * 0.5f + 0.5f;

	r = std::pow(r, 2.2f);
	g = std::pow(g, 2.2f);
	b = std::pow(b, 2.2f);

	return std::make_tuple(r, g, b);
}

void features::visuals::jump_trail() {
	if (!g::local || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected() || !g::local->is_alive())
		return;

	if (c::visuals::trails) {
		const int move_type = g::local->move_type();

		vec3_t origin = g::local->origin();
		int vel = g::local->velocity().length_2d();

		const auto [r, g, b] { draw_rainbow(4.f) };
		if (!(g::local->flags() & fl_onground) && tick == 0) {

			color_t rainbow_col = color_t(r * 255, g * 255, b * 255);

			BeamInfo_t beam_info;
			beam_info.m_nType = 0;
			beam_info.m_pszModelName = "sprites/physbeam.vmt";
			beam_info.m_nModelIndex = -1;
			beam_info.m_flHaloScale = 0.0;
			beam_info.m_flLife = 2.5f;
			beam_info.m_flWidth = 5;
			beam_info.m_flEndWidth = 5;
			beam_info.m_flFadeLength = 0.0;
			beam_info.m_flAmplitude = 2.0;
			beam_info.m_flBrightness = 255.f;
			beam_info.m_flSpeed = 0.5;
			beam_info.m_nStartFrame = 0.;
			beam_info.m_flFrameRate = 0.;
			beam_info.m_flRed = (float)rainbow_col.r();
			beam_info.m_flGreen = (float)rainbow_col.g();
			beam_info.m_flBlue = (float)rainbow_col.b();
			beam_info.m_nSegments = 2;
			beam_info.m_bRenderable = true;
			beam_info.m_nFlags = 0;
			beam_info.m_vecStart = origin_old;
			beam_info.m_vecEnd = origin;

			Beam_t* myBeam = interfaces::render_beams->create_beam_point(beam_info);
			if (myBeam && !(move_type == movetype_ladder || move_type == movetype_noclip || move_type == movetype_observer)) {
				interfaces::render_beams->draw_beam(myBeam);
			}

			velocity_old = vel;
		}
		if (tick == 0) {
			origin_old = origin;
			tick = igonre_ticks + 1;
		}
		tick = tick - 1;
	}
}

void features::visuals::dlights(player_t* entity) {
	if (!c::visuals::dlight || !interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	if (interfaces::engine->is_in_game() && interfaces::engine->is_connected()) {
		vec3_t getorig = entity->origin();
		vec3_t  getheadorig = entity->get_eye_pos();

		if (entity->is_enemy() && !entity->dormant()) {

			dlight_t* elight = interfaces::effects->cl_alloc_elight(entity->index());
			elight->color.r = float(c::visuals::dlight_clr[0] * 255.f);
			elight->color.g = float(c::visuals::dlight_clr[1] * 255.f);;
			elight->color.b = float(c::visuals::dlight_clr[2] * 255.f);;
			elight->color.exponent = 8.f;
			elight->direction = getheadorig;
			elight->origin = getheadorig;
			elight->radius = 200.0f;
			elight->die_time = interfaces::globals->cur_time + 0.1f;
			elight->decay = 50.0f;
			elight->key = entity->index();

			dlight_t* dlight = interfaces::effects->cl_alloc_dlight(entity->index());
			dlight->color.r = float(c::visuals::dlight_clr[0] * 255.f);
			dlight->color.g = float(c::visuals::dlight_clr[1] * 255.f);;
			dlight->color.b = float(c::visuals::dlight_clr[2] * 255.f);;
			dlight->color.exponent = 8.f;
			dlight->direction = getorig;
			dlight->origin = getorig;
			dlight->radius = 100.f;
			dlight->die_time = interfaces::globals->cur_time + 0.1f;
			dlight->decay = dlight->radius / 2.f;
			dlight->key = entity->index();
		}
	}
}

static int buttons = 0;
void features::visuals::run_freecam(c_usercmd* cmd, vec3_t angles) {
	static vec3_t currentviewangles = vec3_t{};
	static vec3_t realviewangles = vec3_t{};
	static bool wascrouching = false;
	static bool washoldingattack = false;
	static bool washoldinguse = false;
	static bool hassetangles = false;
	buttons = cmd->buttons;

	if (!c::misc::freecam || !menu::checkkey(c::misc::freecam_key, c::misc::freecam_key_s)) {
		if (hassetangles) {
			interfaces::engine->set_view_angles(realviewangles);
			cmd->view_angles = currentviewangles;
			if (wascrouching)
				cmd->buttons |= in_duck;
			if (washoldingattack)
				cmd->buttons |= in_attack;
			if (washoldinguse)
				cmd->buttons |= in_use;
			wascrouching = false;
			washoldingattack = false;
			washoldinguse = false;
			hassetangles = false;
		}
		currentviewangles = vec3_t{};
		return;
	}

	if (!g::local || !g::local->is_alive())
		return;

	if (currentviewangles.null()) {
		currentviewangles = cmd->view_angles;
		realviewangles = angles;
		wascrouching = cmd->buttons & in_duck;
	}

	cmd->forward_move = 0;
	cmd->side_move = 0;
	cmd->buttons = 0;

	if (wascrouching)
		cmd->buttons |= in_duck;

	if (washoldingattack)
		cmd->buttons |= in_attack;

	if (washoldinguse)
		cmd->buttons |= in_use;

	cmd->view_angles = currentviewangles;
	hassetangles = true;
}

void features::visuals::freecam(view_setup_t* setup) {
	static vec3_t origin = vec3_t{ };

	if (!c::misc::freecam || !GetAsyncKeyState(c::misc::freecam_key)) {
		origin = vec3_t{ };
		return;
	}

	if (!g::local || !g::local->is_alive())
		return;

	float cam_speed = fabsf(static_cast<float>(2)); // cfg later 

	if (origin.null())
		origin = setup->origin;

	vec3_t forward{ }, right{ }, up{ };

	math::angle_vectors_alternative(setup->view, &forward, &right, &up);

	const bool inback = buttons & in_back;
	const bool inforward = buttons & in_forward;
	const bool rightBtn = buttons & in_moveright;
	const bool inleft = buttons & in_moveleft;
	const bool inshift = buttons & in_speed;
	const bool induck = buttons & in_duck;
	const bool injump = buttons & in_jump;

	if (induck)
		cam_speed *= 0.45f;

	if (inshift)
		cam_speed *= 1.65f;

	if (inforward)
		origin += forward * cam_speed;

	if (rightBtn)
		origin += right * cam_speed;

	if (inleft)
		origin -= right * cam_speed;

	if (inback)
		origin -= forward * cam_speed;

	if (injump)
		origin += up * cam_speed;

	setup->origin = origin;
}

void features::visuals::key_strokes() {
	if (!c::movement::key_strokes)
		return;

	if (!g::local || !g::local->is_alive())
		return;

	if (!interfaces::engine->is_in_game() || !interfaces::engine->is_connected())
		return;

	int w, h;
	interfaces::engine->get_screen_size(w, h);
	c_usercmd* cmd = g::cmd;

	color_t clr;

	if (cmd->buttons & in_moveleft && cmd->buttons & in_moveright)
		clr = color_t(0.6f, 0.2f, 0.2f);
	else
		clr = color_t(1.f, 1.f, 1.f);

	im_render.text(w / 2 + 13, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 - 14, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 - 14, c::movement::key_strokes_position + 14, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);
	im_render.text(w / 2 + 13, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "_", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_jump)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "J", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_duck)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "C", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_forward)
		im_render.text(w / 2, c::movement::key_strokes_position, 12, fonts::key_strokes_font, "W", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_moveleft)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position + 14, 12, fonts::key_strokes_font, "A", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_back)
		im_render.text(w / 2, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "S", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->buttons & in_moveright)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position + 13, 12, fonts::key_strokes_font, "D", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->mouse_dx < 0.f)
		im_render.text(w / 2 - 14, c::movement::key_strokes_position + 28, 12, fonts::key_strokes_font, "<", true, color_t(1.f, 1.f, 1.f), true);

	if (cmd->mouse_dx > 0.f)
		im_render.text(w / 2 + 13, c::movement::key_strokes_position + 28, 12, fonts::key_strokes_font, ">", true, color_t(1.f, 1.f, 1.f), true);
}

void features::visuals::console() {
	static i_material* material[5];
	static float time = 0.f;

	// Define two colors for interpolation
	const float color1[4] = { c::misc::custom_console_clr[0], c::misc::custom_console_clr[1], c::misc::custom_console_clr[2], c::misc::custom_console_clr[3] }; // Example: Red
	const float color2[4] = { c::misc::custom_console_clr2[0], c::misc::custom_console_clr2[1], c::misc::custom_console_clr2[2], c::misc::custom_console_clr2[3] }; // Example: Blue

	// Increment time for interpolation
	time += interfaces::globals->frame_time * 0.5f; // Adjust speed
	if (time > 1.0f)
		time = 0.0f; // Reset time to loop effect

	// Compute interpolated color
	float interp_color[4];
	for (int i = 0; i < 4; i++)
		interp_color[i] = (1.0f - time) * color1[i] + time * color2[i];

	// Initialize materials if not already set
	if (!material[0] || !material[1] || !material[2] || !material[3] || !material[4]) {
		for (material_handle_t h = interfaces::material_system->first_material();
			h != interfaces::material_system->invalid_material_handle();
			h = interfaces::material_system->next_material(h)) {

			const auto mat = interfaces::material_system->get_material(h);
			if (!mat)
				continue;

			if (strstr(mat->get_name(), "vgui_white"))
				material[0] = mat;
			else if (strstr(mat->get_name(), "800corner1"))
				material[1] = mat;
			else if (strstr(mat->get_name(), "800corner2"))
				material[2] = mat;
			else if (strstr(mat->get_name(), "800corner3"))
				material[3] = mat;
			else if (strstr(mat->get_name(), "800corner4"))
				material[4] = mat;
		}
	}
	else {
		for (unsigned int num = 0; num < 5; num++) {
			if (!c::misc::custom_console || !interfaces::engine->is_console_visible()) {
				material[num]->color_modulate(1.f, 1.f, 1.f);
				material[num]->alpha_modulate(1.f);
				continue;
			}

			// Apply interpolated gradient color
			material[num]->color_modulate(interp_color[0], interp_color[1], interp_color[2]);
			material[num]->alpha_modulate(interp_color[3]);
		}
	}
}

float calculate_sun_position(float base_angle) {
	// Use global real-time to animate the sun's rotation
	float time = interfaces::globals->realtime;

	// Modify this speed multiplier as needed for faster/slower rotation
	constexpr float rotation_speed = 0.1f;

	// Add time-based rotation to the base angle
	float animated_angle = base_angle + (time * rotation_speed);

	// Keep it within 0-360 degrees
	if (animated_angle > 360.f)
		animated_angle = std::fmod(animated_angle, 360.f);

	return animated_angle;
}

void features::visuals::custom_sun() {
	auto cl_csm_rot_override = interfaces::console->get_convar(xs("cl_csm_rot_override"));
	auto cl_csm_max_shadow_dist = interfaces::console->get_convar(xs("cl_csm_max_shadow_dist"));
	auto cl_csm_rot_x = interfaces::console->get_convar(xs("cl_csm_rot_x"));
	auto cl_csm_rot_y = interfaces::console->get_convar(xs("cl_csm_rot_y"));

	if (!c::visuals::custom_sun) {
		cl_csm_rot_override->set_value(0);
		return;
	}

	cl_csm_max_shadow_dist->set_value(c::visuals::custom_sun_dist);
	cl_csm_rot_override->set_value(1);
	cl_csm_rot_x->set_value(calculate_sun_position(c::visuals::custom_sun_x));
	cl_csm_rot_y->set_value(calculate_sun_position(c::visuals::custom_sun_y));
}

