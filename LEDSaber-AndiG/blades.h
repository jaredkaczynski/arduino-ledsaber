// blade modes
#define BLADE_MODE_OFF        0
#define BLADE_MODE_ON         1
#define BLADE_MODE_IGNITE     2
#define BLADE_MODE_EXTINGUISH 3
#define BLADE_MODE_UNDERVOLT  4

// Total LED power limit, of all RGB LEDS together.
// If your strips are back-to-back in a tube without serious extra heatsinking, don't exceed 40% sustained power
#define BLADE_POWER_LIMIT     0.30f
// Seriously. I mean it. I heat-destroyed a blade at 100% so you don't have to. 
// It will run for a few minutes and then neopixels will start dying.
// we can also define limits for the individual channels, since that last 10% of brightness usually makes more heat than light (especially for red)
#define BLADE_POWER_LIMIT_RED      0.90f
#define BLADE_POWER_LIMIT_GREEN    0.90f
#define BLADE_POWER_LIMIT_BLUE     0.90f

// how many modes
#define MODE_COUNT 12

// blade state
int blade_mode = BLADE_MODE_OFF;
//Switch to percentage
int blade_out_percentage = 0;

int extend_speed = 2;

int blade_preset = 0;
//Number of blades
const int blade_count = 2;

typedef struct Blade
{
	//Pointer to array of LEDs
	CRGB* blade_leds = 0;
	// Allocation (let's suppose size contains some value discovered at runtime,
	// e.g. obtained from some external source)
	//	blade_leds = (CRGB*)malloc(blade_leds_size * sizeof(CRGB));
	//Number of LEDs in Pixel String
	int blade_led_count;
	//ms offset for animations
	int offset;
	//Which pin this blade is connected to on Arduino
	uint8_t pin;
	int blade_brightness;
	int blade_saturation;
	int blade_hue;
};

// mode light colour list
CRGB mode_color[] = {
	CRGB::White,  // mode 0 : extension
	CRGB::White,  // mode 1 : volume
	CRGB::Purple, // mode 2 : presets
	CRGB::Green,  // mode 3 : blade brightness
	CRGB::Blue,   // mode 4 : blade hue
	CRGB::Blue,   // mode 5 : blade saturation
	CRGB::Yellow, // mode 6 : buzz frequency
	CRGB::Orange, // mode 7 : hum1 frequency
	CRGB::Orange, // mode 8 : hum2 frequency
	CRGB::Red,    // mode 9 : doppler shift
	CRGB::Red,    // mode 10 : echo decay
	CRGB::Black,  // mode 11 : no action
};

// rotary knob state
int  button_mode = 0;
byte button_state = 0;

extern Blade blade_array[];

void update_blade(Blade b, CRGB color) {
	// start index
	int i = 0;
	// are we in menu selection mode?
	if (button_state == 2) {
		// show the mode dots
		for (int m = 0; m<MODE_COUNT; m++) {
			if (m == button_mode) {
				// current menu dot. both leds full brightness
				b.blade_leds[i++] = b.blade_leds[i++] = mode_color[m];
			}
			else {
				// not current item. low-intensity first dot
				b.blade_leds[i++] = CRGB(mode_color[m].r >> 4, mode_color[m].g >> 4, mode_color[m].b >> 4);
				b.blade_leds[i++] = CRGB::Black;
			}
			b.blade_leds[i++] = CRGB::Black;
		}
	}
	// set the remaining strip light values
	//This for loop is for ignite and deactivate
	//Iterates over every LED in the blade
	for (; i<b.blade_led_count; i++) {
		if (map(i, 1, b.blade_led_count, 1, 100) < blade_out_percentage) {
			b.blade_leds[i] = color;
		}
		else {
			b.blade_leds[i] = CRGB::Black;
			//Can I break here for efficiency?
		}
	}
	// update the LEDS now
	LEDS.show();
}

void update_blade_power_scale(Blade b) {
	// compute base color
	int H = b.blade_hue;
	int S = b.blade_saturation;
	int V = b.blade_brightness;
	CRGB color = CHSV(H, S, V);
	// limit the LED power
	float scale = 1.0;
	scale = min(scale, (BLADE_POWER_LIMIT_RED*255.0f) / (float)color.r);
	scale = min(scale, (BLADE_POWER_LIMIT_GREEN*255.0f) / (float)color.g);
	scale = min(scale, (BLADE_POWER_LIMIT_BLUE*255.0f) / (float)color.b);
	int power = (int)color.r + (int)color.g + (int)color.b;
	scale = min(scale, (BLADE_POWER_LIMIT*3.0f*255.0f) / (float)power);
	// rescale brightness;
	color = CHSV(H, S, scale * (float)V);
	update_blade(b,color);
}

void update_blade_power_scale_distortion(Blade b, int distortion) {
	// compute base color
	int H = b.blade_hue;
	int S = b.blade_saturation;
	int V = b.blade_brightness + distortion;
	CRGB color = CHSV(H, S, V);
	// limit the LED power
	float scale = 1.0;
	scale = min(scale, (BLADE_POWER_LIMIT_RED*255.0f) / (float)color.r);
	scale = min(scale, (BLADE_POWER_LIMIT_GREEN*255.0f) / (float)color.g);
	scale = min(scale, (BLADE_POWER_LIMIT_BLUE*255.0f) / (float)color.b);
	int power = (int)color.r + (int)color.g + (int)color.b;
	scale = min(scale, (BLADE_POWER_LIMIT*3.0f*255.0f) / (float)power);
	// rescale brightness;
	color = CHSV(H, S, scale * (float)V);
	update_blade(b, color);
}

//Used for extinguish/ignite
void update_blade_array() {
	for (int i = 0; i < blade_count; i++) {
		update_blade_power_scale(blade_array[i]);
	}
}

void update_blade_array_live(int distortion) {
	for (int i = 0; i < blade_count; i++) {
		update_blade_power_scale_distortion(blade_array[i],distortion);
	}
}

void update_blade_array(int brightness, int saturation, int hue) {
	for (int i = 0; i < blade_count; i++) {
		//Change the blade color
		if (brightness > 0) {
			blade_array[i].blade_brightness = brightness;
		}
		if (saturation > 0) {
			blade_array[i].blade_saturation = saturation;
		}
		if (hue > 0) {
			blade_array[i].blade_hue = hue;
		}
		//Update the blade color
		update_blade_power_scale(blade_array[i]);
	}
}

void update_blade_array(int brightness, int hue) {
	for (int i = 0; i < blade_count; i++) {
		//Change the blade color
		if (brightness > 0) {
			blade_array[i].blade_brightness = brightness;
		}
		if (hue > 0) {
			blade_array[i].blade_hue = hue;
		}
		//Update the blade color
		update_blade_power_scale(blade_array[i]);
	}
}




void ignite() {
	if (blade_mode == BLADE_MODE_OFF) {
		blade_mode = BLADE_MODE_IGNITE;
	}
}


void extinguish() {
	if (blade_mode == BLADE_MODE_ON) {
		blade_mode = BLADE_MODE_EXTINGUISH;
		// since this was done gracefully, store the current blade settings for next time
		eeprom_save();
	}
}