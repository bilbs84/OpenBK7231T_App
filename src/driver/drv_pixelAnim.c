// WS2812B etc animations
// For the animations themselves credit goes to https://github.com/Electriangle
#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
// Commands register, execution API and cmd tokenizer
#include "../cmnds/cmd_public.h"
#include "../mqtt/new_mqtt.h"
#include "../logging/logging.h"
#include "drv_local.h"
#include "../hal/hal_pins.h"
#include <math.h>

/*
// Usage:
startDriver SM16703P
SM16703P_Init 16
startDriver PixelAnim

*/

// Credit: https://github.com/Electriangle/RainbowCycle_Main
byte *RainbowWheel_Wheel(byte WheelPosition) {
	static byte c[3];

	if (WheelPosition < 85) {
		c[0] = WheelPosition * 3;
		c[1] = 255 - WheelPosition * 3;
		c[2] = 0;
	}
	else if (WheelPosition < 170) {
		WheelPosition -= 85;
		c[0] = 255 - WheelPosition * 3;
		c[1] = 0;
		c[2] = WheelPosition * 3;
	}
	else {
		WheelPosition -= 170;
		c[0] = 0;
		c[1] = WheelPosition * 3;
		c[2] = 255 - WheelPosition * 3;
	}

	return c;
}
uint16_t j = 0;

void RainbowWheel_Run() {
	byte *c;
	uint16_t i;

	for (i = 0; i < pixel_count; i++) {
		c = RainbowWheel_Wheel(((i * 256 / pixel_count) + j) & 255);
		SM16703P_setPixelWithBrig(pixel_count - 1 - i, *c, *(c + 1), *(c + 2));
	}
	SM16703P_Show();
	j++;
	j %= 256;
}

void Fire_setPixelHeatColor(int Pixel, byte temperature) {
	// Rescale heat from 0-255 to 0-191
	byte t192 = round((temperature / 255.0) * 191);

	// Calculate ramp up from
	byte heatramp = t192 & 0x3F; // 0...63
	heatramp <<= 2; // scale up to 0...252

	// Figure out which third of the spectrum we're in:
	if (t192 > 0x80) {                    // hottest
		SM16703P_setPixelWithBrig(Pixel, 255, 255, heatramp);
	}
	else if (t192 > 0x40) {               // middle
		SM16703P_setPixelWithBrig(Pixel, 255, heatramp, 0);
	}
	else {                               // coolest
		SM16703P_setPixelWithBrig(Pixel, heatramp, 0, 0);
	}
}
// FlameHeight - Use larger value for shorter flames, default=50.
// Sparks - Use larger value for more ignitions and a more active fire (between 0 to 255), default=100.
// DelayDuration - Use larger value for slower flame speed, default=10.
int FlameHeight = 50;
int Sparks = 100;
static byte heat[32];
int RandomRange(int min, int max) {
	int r = rand() % (max - min);
	return min + r;
}
void Fire_Run() {
	int cooldown;

	// Cool down each cell a little
	for (int i = 0; i < pixel_count; i++) {
		cooldown = RandomRange(0, ((FlameHeight * 10) / pixel_count) + 2);

		if (cooldown > heat[i]) {
			heat[i] = 0;
		}
		else {
			heat[i] = heat[i] - cooldown;
		}
	}

	// Heat from each cell drifts up and diffuses slightly
	for (int k = (pixel_count - 1); k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Randomly ignite new Sparks near bottom of the flame
	if (rand()%255 < Sparks) {
		int y = rand()%7;
		heat[y] = heat[y] + RandomRange(160, 255);
	}

	// Convert heat to LED colors
	for (int j = 0; j < pixel_count; j++) {
		Fire_setPixelHeatColor(j, heat[j]);
	}

}

// startDriver PixelAnim

typedef struct ledAnim_s {
	const char *name;
	void(*runFunc)();
} ledAnim_t;

int activeAnim = -1;
ledAnim_t g_anims[] = {
	{ "Rainbow Wheel", RainbowWheel_Run },
	{ "Fire", Fire_Run }
};
int g_numAnims = sizeof(g_anims) / sizeof(g_anims[0]);

void PixelAnim_Init() {

}
extern byte g_lightEnableAll;
extern byte g_lightMode;
void PixelAnim_CreatePanel(http_request_t *request) {
	const char* activeStr = "";
	int i;

	if (g_lightMode == Light_Anim) {
		activeStr = "[ACTIVE]";
	}
	poststr(request, "<tr><td>");
	hprintf255(request, "<h5>LED Animation %s</h5>", activeStr);

	for (i = 0; i < g_numAnims; i++) {
		const char* c;
		if (i == activeAnim) {
			c = "bgrn";
		}
		else {
			c = "bred";
		}
		poststr(request, "<form action=\"index\">");
		hprintf255(request, "<input type=\"hidden\" name=\"an\" value=\"%i\">", i);
		hprintf255(request, "<input class=\"%s\" type=\"submit\" value=\"%s\"/></form>",
			c, g_anims[i].name);
	}
	poststr(request, "</td></tr>");
}
void PixelAnim_Run(int j) {
	activeAnim = j;
	g_lightMode = Light_Anim;
}
void PixelAnim_RunQuickTick() {
	if (g_lightEnableAll == 0) {
		// disabled
		return;
	}
	if (g_lightMode != Light_Anim) {
		// disabled
		return;
	}
	if (activeAnim != -1) {
		g_anims[activeAnim].runFunc();
	}
}

