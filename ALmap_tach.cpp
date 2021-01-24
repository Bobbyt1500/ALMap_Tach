#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMDataAccess.h"
#include "LoadBMP/loadbmp.h"
#include "FontRenderer/font_renderer.h"
#include <string>
#include <windows.h>
#include <GL/gl.h>

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

// Ids for XPLM
static XPLMWindowID	window_id;

// References
const float radsec_revmin_conversion_factor = 9.5493;


// Callbacks must be declared
void				window_draw_callback(XPLMWindowID in_window_id, void* in_refcon);
int					mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void* in_refcon) { return 0; }
XPLMCursorStatus	cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void* in_refcon) { return xplm_CursorDefault; }
int					wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon) { return 0; }
void				key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void* in_refcon, int losing_focus) { }

// Textures

struct texture {
	unsigned int HEIGHT;
	unsigned int WIDTH;
	unsigned char* pixels = NULL;
	int id;
	const char* file_location;
} instrument_bg, font_bitmap, instrument_off;


void create_window() {
	XPLMCreateWindow_t params;
	params.structSize = sizeof(params);
	params.visible = 1;
	params.drawWindowFunc = window_draw_callback;
	params.handleMouseClickFunc = mouse_handler;
	params.handleRightClickFunc = mouse_handler;
	params.handleMouseWheelFunc = wheel_handler;
	params.handleKeyFunc = key_handler;
	params.handleCursorFunc = cursor_status_handler;
	params.refcon = NULL;
	params.layer = xplm_WindowLayerFloatingWindows;
	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;

	// Set the window's initial bounds
	params.left = 0;
	params.bottom = 0;
	params.right = 400;
	params.top = 400;

	window_id = XPLMCreateWindowEx(&params);

	XPLMSetWindowPositioningMode(window_id, xplm_WindowPositionFree, -1);

	XPLMSetWindowResizingLimits(window_id, 400, 400, 400, 400);
}

void load_textures() {
	// Regular instrument background
	XPLMGenerateTextureNumbers(&instrument_bg.id, 1);
	XPLMBindTexture2d(instrument_bg.id, 0);

	loadbmp_decode_file(instrument_bg.file_location, &instrument_bg.pixels, &instrument_bg.WIDTH, &instrument_bg.HEIGHT, LOADBMP_RGB);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, instrument_bg.WIDTH, instrument_bg.HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, instrument_bg.pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// The numeric characters font bitmap
	XPLMGenerateTextureNumbers(&font_bitmap.id, 1);
	XPLMBindTexture2d(font_bitmap.id, 0);

	loadbmp_decode_file(font_bitmap.file_location, &font_bitmap.pixels, &font_bitmap.WIDTH, &font_bitmap.HEIGHT, LOADBMP_RGB);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, font_bitmap.WIDTH, font_bitmap.HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, font_bitmap.pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// The instrument turned off background

	XPLMGenerateTextureNumbers(&instrument_off.id, 1);
	XPLMBindTexture2d(instrument_off.id, 0);

	loadbmp_decode_file(instrument_off.file_location, &instrument_off.pixels, &instrument_off.WIDTH, &instrument_off.HEIGHT, LOADBMP_RGB);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, instrument_off.WIDTH, instrument_off.HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, instrument_off.pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


PLUGIN_API int XPluginStart(
	char* outName,
	char* outSig,
	char* outDesc)
{
	strcpy(outName, "Aerospace Logic MAP & Tach");
	strcpy(outSig, "bobbyt15.al_map_tach");
	strcpy(outDesc, "A simple 2d instrument to display detailed manifold and RPM information");

	instrument_bg.HEIGHT = 400;
	instrument_bg.WIDTH = 400;
	instrument_bg.file_location = "C:/Program Files (x86)/Steam/steamapps/common/X-Plane 11/Resources/plugin_images/ALmap_tach_instrument_bg.bmp";

	font_bitmap.HEIGHT = 32;
	font_bitmap.WIDTH = 256;
	font_bitmap.file_location = "C:/Program Files (x86)/Steam/steamapps/common/X-Plane 11/Resources/fonts/Arial32Nums.bmp";

	instrument_off.HEIGHT = 400;
	instrument_off.WIDTH = 400;
	instrument_off.file_location = "C:/Program Files (x86)/Steam/steamapps/common/X-Plane 11/Resources/plugin_images/ALmap_tach_instrument_off.bmp";

	create_window();
	load_textures();

	return 1;
}

PLUGIN_API void	XPluginStop(void)
{
	XPLMDestroyWindow(window_id);
	window_id = NULL;
}

PLUGIN_API void XPluginDisable(void) { }
PLUGIN_API int  XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) { }

void	window_draw_callback(XPLMWindowID in_window_id, void* in_refcon) {
	// Set and get necessary items for drawing
	glEnable(GL_TEXTURE_2D);
	XPLMSetGraphicsState(0, 1, 0, 0, 1, 0, 0);
	int l, t, r, b;
	XPLMGetWindowGeometry(window_id, &l, &t, &r, &b);

	// Get battery status
	XPLMDataRef battery_data_ref = XPLMFindDataRef("sim/cockpit2/electrical/battery_on");
	int battery_on;
	XPLMGetDatavi(battery_data_ref, &battery_on, 0, 1);

	// If the battery is not on
	if (battery_on == 0) {
		// Draw instrument turned off background
		XPLMBindTexture2d(instrument_off.id, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, instrument_off.WIDTH, instrument_off.HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, instrument_off.pixels);

		int x1 = l;
		int y1 = b;
		int x2 = x1 + 400;
		int y2 = y1 + 400;

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x1, y1);
		glTexCoord2f(0, 0);
		glVertex2f(x1, y2);
		glTexCoord2f(1, 0);
		glVertex2f(x2, y2);
		glTexCoord2f(1, 1);
		glVertex2f(x2, y1);
		glEnd();

		// Dont run rest of the drawing callback
		return;
	}

	// Draw instrument background


	XPLMBindTexture2d(instrument_bg.id, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, instrument_bg.WIDTH, instrument_bg.HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, instrument_bg.pixels);

	int x1 = l;
	int y1 = b;
	int x2 = x1 + 400;
	int y2 = y1 + 400;

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex2f(x1, y1);
	glTexCoord2f(0, 0);
	glVertex2f(x1, y2);
	glTexCoord2f(1, 0);
	glVertex2f(x2, y2);
	glTexCoord2f(1, 1);
	glVertex2f(x2, y1);
	glEnd();

	// Draw labels

	// Get Data

	XPLMDataRef rpm_data_ref = XPLMFindDataRef("sim/cockpit2/engine/indicators/prop_speed_rpm");
	XPLMDataRef MAP_data_ref = XPLMFindDataRef("sim/cockpit2/engine/indicators/MPR_in_hg");

	float current_rpm;
	float current_MAP;

	XPLMGetDatavf(rpm_data_ref, &current_rpm, 0, 1);
	XPLMGetDatavf(MAP_data_ref, &current_MAP, 0, 1);

	// If the rpm is less than 1, we must set the displayed value to zero
	if (current_rpm < 1) {
		current_rpm = 0;
	}


	// Render the numbers

	render_nums(font_bitmap.pixels, font_bitmap.id, static_cast<double> (current_rpm), l + 110, b + 142, false, true, 4);
	render_nums(font_bitmap.pixels, font_bitmap.id, static_cast<double> (current_MAP), l + 241, b + 142, true, true, 2);


	// Draw the visual bars
	glDisable(GL_TEXTURE_2D);

	// Get the max prop speed of this aircraft in RPMS for
	XPLMDataRef max_prp_data_ref = XPLMFindDataRef("sim/aircraft/engine/acf_RSC_redline_eng");
	int max_rpm = static_cast<int> (XPLMGetDataf(max_prp_data_ref) * radsec_revmin_conversion_factor);

	// Calculate how high the bar needs to be
	float rpm_percentage = (current_rpm / max_rpm) * 100;

	// If there is an error an the percentage is > 100, set it to max 100
	if (rpm_percentage > 100) {
		rpm_percentage = 100.0;
	}

	// Get the max manifold pressure of this aircraft in inHg
	XPLMDataRef max_MAP_data_ref = XPLMFindDataRef("sim/aircraft/limits/red_hi_MP");
	int max_MAP = static_cast<int> (XPLMGetDataf(max_MAP_data_ref));

	// Calculate how high the bar needs to be
	float MAP_percentage = (current_MAP / max_MAP) * 100;

	// If there is an error an the percentage is > 100, set it to max 100
	if (MAP_percentage > 100) {
		MAP_percentage = 100.0;
	}


	glColor3f(0, 1, 0);

	// RPM bar
	glBegin(GL_QUADS);
	{
		glVertex2f(l + 135, b + 178);
		glVertex2f(l + 135, b + 178 + rpm_percentage);
		glVertex2f(l + 145, b + 178 + rpm_percentage);
		glVertex2f(l + 145, b + 178);
	}
	glEnd();

	// MAP bar

	glBegin(GL_QUADS);
	{
		glVertex2f(l + 260, b + 178);
		glVertex2f(l + 260, b + 178 + MAP_percentage);
		glVertex2f(l + 270, b + 178 + MAP_percentage);
		glVertex2f(l + 270, b + 178);
	}
	glEnd();




}