#pragma region Copyright (c) 2014-2016 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#pragma warning(disable : 4996) // GetVersionExA deprecated

#include <time.h>
#include "audio/audio.h"
#include "audio/mixer.h"
#include "config.h"
#include "drawing/drawing.h"
#include "editor.h"
#include "game.h"
#include "input.h"
#include "interface/chat.h"
#include "interface/console.h"
#include "interface/viewport.h"
#include "intro.h"
#include "localisation/date.h"
#include "localisation/localisation.h"
#include "management/news_item.h"
#include "network/network.h"
#include "network/twitch.h"
#include "object.h"
#include "object/ObjectManager.h"
#include "openrct2.h"
#include "peep/staff.h"
#include "platform/platform.h"
#include "rct1.h"
#include "ride/ride.h"
#include "ride/track.h"
#include "ride/track_design.h"
#include "scenario.h"
#include "title.h"
#include "util/util.h"
#include "world/map.h"
#include "world/park.h"
#include "world/climate.h"
#include "world/scenery.h"
#include "world/sprite.h"

// rct2: 0x0097F67C
const char * const RCT2FilePaths[PATH_ID_END] = {
	"Data" PATH_SEPARATOR "g1.dat",
	"Data" PATH_SEPARATOR "plugin.dat",
	"Data" PATH_SEPARATOR "css1.dat",
	"Data" PATH_SEPARATOR "css2.dat",
	"Data" PATH_SEPARATOR "css4.dat",
	"Data" PATH_SEPARATOR "css5.dat",
	"Data" PATH_SEPARATOR "css6.dat",
	"Data" PATH_SEPARATOR "css7.dat",
	"Data" PATH_SEPARATOR "css8.dat",
	"Data" PATH_SEPARATOR "css9.dat",
	"Data" PATH_SEPARATOR "css11.dat",
	"Data" PATH_SEPARATOR "css12.dat",
	"Data" PATH_SEPARATOR "css13.dat",
	"Data" PATH_SEPARATOR "css14.dat",
	"Data" PATH_SEPARATOR "css15.dat",
	"Data" PATH_SEPARATOR "css3.dat",
	"Data" PATH_SEPARATOR "css17.dat",
	"Data" PATH_SEPARATOR "css18.dat",
	"Data" PATH_SEPARATOR "css19.dat",
	"Data" PATH_SEPARATOR "css20.dat",
	"Data" PATH_SEPARATOR "css21.dat",
	"Data" PATH_SEPARATOR "css22.dat",
	"Saved Games" PATH_SEPARATOR "scores.dat",
	"Data" PATH_SEPARATOR "css23.dat",
	"Data" PATH_SEPARATOR "css24.dat",
	"Data" PATH_SEPARATOR "css25.dat",
	"Data" PATH_SEPARATOR "css26.dat",
	"Data" PATH_SEPARATOR "css27.dat",
	"Data" PATH_SEPARATOR "css28.dat",
	"Data" PATH_SEPARATOR "css29.dat",
	"Data" PATH_SEPARATOR "css30.dat",
	"Data" PATH_SEPARATOR "css31.dat",
	"Data" PATH_SEPARATOR "css32.dat",
	"Data" PATH_SEPARATOR "css33.dat",
	"Data" PATH_SEPARATOR "css34.dat",
	"Data" PATH_SEPARATOR "css35.dat",
	"Data" PATH_SEPARATOR "css36.dat",
	"Data" PATH_SEPARATOR "css37.dat",
	"Data" PATH_SEPARATOR "css38.dat",
	"Data" PATH_SEPARATOR "CUSTOM1.WAV",
	"Data" PATH_SEPARATOR "CUSTOM2.WAV",
	"Data" PATH_SEPARATOR "css39.dat",
	"Data" PATH_SEPARATOR "css40.dat",
	"Data" PATH_SEPARATOR "css41.dat",
	"Scenarios" PATH_SEPARATOR "Six Flags Magic Mountain.SC6",
	"Data" PATH_SEPARATOR "css42.dat",
	"Data" PATH_SEPARATOR "css43.dat",
	"Data" PATH_SEPARATOR "css44.dat",
	"Data" PATH_SEPARATOR "css45.dat",
	"Data" PATH_SEPARATOR "css46.dat",
	"Data" PATH_SEPARATOR "css50.dat"
};

uint32 gCurrentDrawCount = 0;

uint8 gScreenFlags;
uint32 gScreenAge;
uint8 gSavePromptMode;
sint32 gScreenWidth;
sint32 gScreenHeight;

char gRCT2AddressAppPath[MAX_PATH];
char gRCT2AddressSavedGamesPath[MAX_PATH];
char gRCT2AddressSavedGamesPath2[MAX_PATH];
char gRCT2AddressScenariosPath[MAX_PATH];
char gRCT2AddressLandscapesPath[MAX_PATH];
char gRCT2AddressObjectDataPath[MAX_PATH];
char gRCT2AddressTracksPath[MAX_PATH];

typedef struct tm tm_t;

void print_launch_information();

int rct2_init_directories();
int rct2_startup_checks();

static void rct2_draw_fps(rct_drawpixelinfo *dpi);

void rct2_quit()
{
	gSavePromptMode = PM_QUIT;
	window_save_prompt_open();
}

void rct2_dispose()
{
	object_manager_unload_all_objects();
	gfx_object_check_all_images_freed();
	gfx_unload_g2();
	gfx_unload_g1();
}

bool rct2_init()
{
	log_verbose("initialising game");

	gScenarioTicks = 0;
	util_srand((unsigned int)time(0));

	config_reset_shortcut_keys();
	config_shortcut_keys_load();
	gInputPlaceObjectModifier = PLACE_OBJECT_MODIFIER_NONE;
	// config_load();

	if (!gfx_load_g1()) {
		return false;
	}
	if (!gfx_load_g2()) {
		return false;
	}

	object_list_load();
	scenario_load_list();
	track_design_index_create();

	font_sprite_initialise_characters();
	if (!gOpenRCT2Headless) {
		platform_init();
		audio_init_ride_sounds_and_info();
	}
	viewport_init_all();
	news_item_init_queue();
	reset_park_entrances();
	user_string_clear_all();
	reset_sprite_list();
	ride_init_all();
	window_guest_list_init_vars_a();
	staff_reset_modes();
	map_init(150);
	park_init();
	if (!gOpenRCT2Headless)
		window_title_menu_open();
	date_reset();
	climate_reset(CLIMATE_COOL_AND_WET);
	scenery_set_default_placement_configuration();
	window_new_ride_init_vars();
	window_guest_list_init_vars_b();
	window_staff_list_init_vars();

	log_verbose("initialising game finished");
	return true;
}

/**
 *
 *  rct2: 0x00683499
 */
int rct2_init_directories()
{
	// windows_get_registry_install_info((rct2_install_info*)0x009AA10C, "RollerCoaster Tycoon 2 Setup", "MS Sans Serif", 0);

	char separator[] = {platform_get_path_separator(), 0};

	if (str_is_null_or_empty(gCustomRCT2DataPath)) {
		// check install directory
		if (!platform_original_game_data_exists(gConfigGeneral.game_path)) {
			log_verbose("install directory does not exist or invalid directory selected, %s", gConfigGeneral.game_path);
			if (!config_find_or_browse_install_directory()) {
				utf8 path[MAX_PATH];
				config_get_default_path(path);
				log_fatal("Invalid RCT2 installation path. Please correct \"game_path\" in %s.", path);
				return 0;
			}
		}
		strcpy(gRCT2AddressAppPath, gConfigGeneral.game_path);
	} else {
		strcpy(gRCT2AddressAppPath, gCustomRCT2DataPath);
	}
	strcat(gRCT2AddressAppPath, separator);

	strcpy(gRCT2AddressSavedGamesPath, gRCT2AddressAppPath);
	strcat(gRCT2AddressSavedGamesPath, "Saved Games");
	strcat(gRCT2AddressSavedGamesPath, separator);

	strcpy(gRCT2AddressScenariosPath, gRCT2AddressAppPath);
	strcat(gRCT2AddressScenariosPath, "Scenarios");
	strcat(gRCT2AddressScenariosPath, separator);
	strcat(gRCT2AddressScenariosPath, "*.SC6");

	strcpy(gRCT2AddressLandscapesPath, gRCT2AddressAppPath);
	strcat(gRCT2AddressLandscapesPath, "Landscapes");
	strcat(gRCT2AddressLandscapesPath, separator);
	strcat(gRCT2AddressLandscapesPath, "*.SC6");

	strcpy(gRCT2AddressObjectDataPath, gRCT2AddressAppPath);
	strcat(gRCT2AddressObjectDataPath, "ObjData");
	strcat(gRCT2AddressObjectDataPath, separator);
	strcat(gRCT2AddressObjectDataPath, "*.DAT");

	strcpy(gRCT2AddressTracksPath, gRCT2AddressAppPath);
	strcat(gRCT2AddressTracksPath, "Tracks");
	strcat(gRCT2AddressTracksPath, separator);
	strcat(gRCT2AddressTracksPath, "*.TD?");

	strcpy(gRCT2AddressSavedGamesPath2, gRCT2AddressSavedGamesPath);
	return 1;
}

void substitute_path(char *dest, const char *path, const char *filename)
{
	while (*path != '*') {
		*dest++ = *path++;
	}
	strcpy(dest, filename);
}

/**
 *
 *  rct2: 0x00674B42
 */
int rct2_startup_checks()
{
	if (!check_file_paths())
		return 0;

	return 1;
}

void rct2_draw(rct_drawpixelinfo *dpi)
{
	if (gIntroState != INTRO_STATE_NONE) {
		return;
	}

	// redraw_rain();
	// window_update_all();
	// gfx_invalidate_pickedup_peep();
	// gfx_draw_pickedup_peep();
	// update_rain_animation();
	update_palette_effects();

	chat_draw(dpi);
	console_draw(dpi);

	if ((gScreenFlags & SCREEN_FLAGS_TITLE_DEMO) && !gTitleHideVersionInfo) {
		DrawOpenRCT2(dpi, 0, gScreenHeight - 20);
	}

	if (gConfigGeneral.show_fps) {
		rct2_draw_fps(dpi);
	}

	gCurrentDrawCount++;
}

static time_t _lastSecond;
static int _currentFPS;
static int _frames;

static void rct2_measure_fps()
{
	_frames++;

	time_t currentTime = time(NULL);

	if (currentTime != _lastSecond) {
		_currentFPS = _frames;
		_frames = 0;
	}

	_lastSecond = currentTime;
}

static void rct2_draw_fps(rct_drawpixelinfo *dpi)
{
	int x = gScreenWidth / 2;
	int y = 2;

	// Measure FPS
	rct2_measure_fps();

	// Format string
	utf8 buffer[64];
	utf8 *ch = buffer;
	ch = utf8_write_codepoint(ch, FORMAT_MEDIUMFONT);
	ch = utf8_write_codepoint(ch, FORMAT_OUTLINE);
	ch = utf8_write_codepoint(ch, FORMAT_WHITE);
	
	sprintf(ch, "%d", _currentFPS);

	// Draw Text
	int stringWidth = gfx_get_string_width(buffer);
	x = x - (stringWidth / 2);
	gfx_draw_string(dpi, buffer, 0, x, y);

	// Make area dirty so the text doesn't get drawn over the last
	gfx_set_dirty_blocks(x - 16, y - 4, gLastDrawStringX + 16, 16);
}

bool rct2_open_file(const char *path)
{
	char *extension = strrchr(path, '.');
	if (extension == NULL) {
		return false;
	}
	extension++;

	if (_stricmp(extension, "sv6") == 0) {
		strcpy((char*)gRCT2AddressSavedGamesPath2, path);
		if (game_load_save(path)) {
			gFirstTimeSave = 0;
			return true;
		}
	} else if (_stricmp(extension, "sc6") == 0) {
		// TODO scenario install
		rct_scenario_basic scenarioBasic;
		strcpy(scenarioBasic.path, path);
		if (scenario_load_and_play_from_path(scenarioBasic.path)) {
			return true;
		}
	} else if (_stricmp(extension, "td6") == 0 || _stricmp(extension, "td4") == 0) {
		// TODO track design install
		return true;
	} else if (_stricmp(extension, "sv4") == 0) {
		if (rct1_load_saved_game(path)) {
			game_load_init();
		}
	} else if (_stricmp(extension, "sc4") == 0) {
		if (rct1_load_scenario(path)) {
			scenario_begin();
		}
	}

	return false;
}

/**
 *
 *  rct2: 0x00674C95
 */
int check_file_paths()
{
	for (int pathId = 0; pathId < PATH_ID_END; pathId++) {
		if (!check_file_path(pathId)) {
			return 0;
		}
	}
	return 1;
}

/**
 *
 *  rct2: 0x00674CA5
 */
int check_file_path(int pathId)
{
	const utf8* path = get_file_path(pathId);
	SDL_RWops *file = SDL_RWFromFile(path, "rb");

	switch (pathId) {
	case PATH_ID_G1:
		if (file == NULL) {
			log_fatal("Could not find file %s", path);
			return 0;
		}
		break;

	case PATH_ID_CUSTOM1:
		if (file != NULL) {
			// Store file size in music_custom1_size @ 0x009AF164
			gRideMusicInfoList[36]->length = (uint32)SDL_RWsize(file);
		}
		break;

	case PATH_ID_CUSTOM2:
		if (file != NULL) {
			// Store file size in music_custom2_size @ 0x009AF16E
			gRideMusicInfoList[37]->length = (uint32)SDL_RWsize(file);
		}
		break;
	}

	if (file != NULL) {
		SDL_RWclose(file);
	}

	return 1;
}

void rct2_update()
{
	int tickCount = SDL_GetTicks();
	gTicksSinceLastUpdate = min(tickCount - gLastTickCount, 500);
	gLastTickCount = tickCount;
	if (game_is_not_paused()) {
		gPaletteEffectFrame += gTicksSinceLastUpdate;
	}

	date_update_real_time_of_day();

	// TODO: screenshot countdown process

	network_update();

	// check_cmdline_arg();
	// Screens
	if (gIntroState != INTRO_STATE_NONE) {
		intro_update();
	} else if ((gScreenFlags & SCREEN_FLAGS_TITLE_DEMO) && !gOpenRCT2Headless) {
		title_update();
	} else {
		game_update();
	}

	//stop_completed_sounds(); // removes other sounds that are no longer playing in directsound

	twitch_update();
	chat_update();
	console_update();
}

/**
 *
 *  rct2: 0x00674E6C
 */
const utf8 *get_file_path(int pathId)
{
	static utf8 path[MAX_PATH];
	strcpy(path, gRCT2AddressAppPath);
	safe_strcat_path(path, RCT2FilePaths[pathId], sizeof(path));
	return path;
}

uint32 get_file_extension_type(const utf8 *path)
{
	const utf8 *extension = path_get_extension(path);
	if (strcicmp(extension, ".dat") == 0) return FILE_EXTENSION_DAT;
	if (strcicmp(extension, ".sc4") == 0) return FILE_EXTENSION_SC4;
	if (strcicmp(extension, ".sv4") == 0) return FILE_EXTENSION_SV4;
	if (strcicmp(extension, ".td4") == 0) return FILE_EXTENSION_TD4;
	if (strcicmp(extension, ".sc6") == 0) return FILE_EXTENSION_SC6;
	if (strcicmp(extension, ".sv6") == 0) return FILE_EXTENSION_SV6;
	if (strcicmp(extension, ".td6") == 0) return FILE_EXTENSION_TD6;
	return FILE_EXTENSION_UNKNOWN;
}
