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
	CRGBPalette16 myPal;
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

void update_blade_color(Blade b, CRGB color) {
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
//Update the blade with brightness distortion
void update_blade(Blade b) {
	// compute base color
	int H = b.blade_hue;
	int S = b.blade_saturation;
	int V = b.blade_brightness;
	CRGB color = CHSV(H, S, V);

	color = CHSV(H, S, V);
	update_blade_color(b, color);
}

//Sets brightness limit globally using inbuilt function
void set_blade_brightness(int limit) {
	//limit the LED power using inbuilt brightness functions
	//Default would be limit*.3 for 255 this is 76.5
	LEDS.setBrightness(limit * BLADE_POWER_LIMIT);
}

//Used for extinguish/ignite effect
void update_blade_array() {
	for (int i = 0; i < blade_count; i++) {
		update_blade(blade_array[i]);
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
		update_blade(blade_array[i]);
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
		update_blade(blade_array[i]);
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

//Random noise generation done in setup()
extern uint16_t dist;
// Wouldn't recommend changing this on the fly, or the animation will be really blocky
uint16_t scale = 30;

//Noise Effect, AKA Kylo Ren presumably
void fillnoise8(Blade b) {
	for (int i = 0; i < b.blade_led_count; i++) {                                      // Just ONE loop to fill up the LED array as all of the pixels change.
		uint8_t index = inoise8(i*scale, dist + i*scale) % 255;                  // Get a value from the noise function. I'm using both x and y axis.
		b.blade_leds[i] = ColorFromPalette(b.myPal, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
	}
	dist += beatsin8(10, 1, 4);                                               // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
																			  // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  10

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

//Fire effect, not sure yet
void Fire2012(Blade b)
{
	// Array of temperature readings at each simulation cell
	//Hardcoding as 144 LEDs for simplicity. Basically 0 blades will be > 144 and this isn't too memory intensive.
	byte heat[144];

	// Step 1.  Cool down every cell a little
	for (int i = 0; i < b.blade_led_count; i++) {
		heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / b.blade_led_count) + 2));
	}

	// Step 2.  Heat from each cell drifts 'up' and diffuses a little
	for (int k = b.blade_led_count - 1; k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Step 3.  Randomly ignite new 'sparks' of heat near the bottom
	if (random8() < SPARKING) {
		int y = random8(7);
		heat[y] = qadd8(heat[y], random8(160, 255));
	}

	// Step 4.  Map from heat cells to LED colors
	for (int j = 0; j < b.blade_led_count; j++) {
		//CRGB color = HeatColor(heat[j]);
		CRGB color = ColorFromPalette(b.myPal, heat[j]);
		int pixelnumber;
		pixelnumber = j;
		b.blade_leds[pixelnumber] = color;
	}
}
void update_blade_array_noise() {
	for (int i = 0; i < blade_count; i++) {
		fillnoise8(blade_array[i]);
	}
	LEDS.show();
}

void update_blade_array_fire() {
	for (int i = 0; i < blade_count; i++) {
		Fire2012(blade_array[i]);
	}
	LEDS.show();
}