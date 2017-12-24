//#define FASTLED_FORCE_SOFTWARE_SPI 1
#define FASTLED_ESP8266_DMA

#include <OneButton.h>
#include <EEPROM.h>
#include <Wire.h>

//#include <avr/wdt.h>
#include <FastLED.h>

// local extentions
#include "properties.h"
#include "mpu6050.h"
#include "audio.h"
#include "blades.h"

#define DEBUG

// default colour customization
//#define BLADE_BRIGHTNESS  127
//#define BLADE_SATURATION  255
//#define BLADE_HUE         144
// see properties.h for the presets list

// inactivity timeout
#define INACTIVITY_TIMEOUT 8000 // about 30 seconds

// voltage shutdown propertues
#ifndef DEBUG
#define VOLTAGE_SHUTDOWN       (3.6f * 3.0f) // minimum voltage required
#endif // !DEBUG

#define VOLTAGE_SENSOR_RATIO   (2.10f / 12.60f)  // ratio between sensor voltage and real battery voltage (1:6 by default)
#define VOLTAGE_SENSOR_PIN      A3               // pin used by voltage sensor
#define VOLTAGE_BEEPS                // do we beep when undervoltage?
#define VOLTAGE_BEEPS_ON        200  // noise time
#define VOLTAGE_BEEPS_OFF       400  // silence time
#define VOLTAGE_BEEPS_FREQ      256  // beep frequency

// when using the gyro values to calculate angular speed, we only want the "vertical" and "horizontal" swing axes, 
// not the "handle twist" rotation. Which of the three (0,1,2) axes to use?
#define GYRO_HORIZONTAL 0
#define GYRO_VERTICAL   2

// allocate the memory buffer for the LED array
//CRGB blade_leds[BLADE_LEDS_COUNT];
//Creating an array of Blades for multi blade setups
Blade blade_array[blade_count];

// enable rotaty encoder switch control
//#define CONTROL_ROTARY
#define CONTROL_BUTTON

//Switching over to a 2 button setup instead of the rotary encoder for 1, simplicity, 2 appearance
OneButton button1(D2, true);
OneButton button2(D3, true);

//Blade Effect Enable/disable
//Independent
//#define BLADE_BRIGHTNESS_SWING_MODULATION
//Either BLADE_FIRE for fire effect or BLADE_NOISE For noise effect
#define BLADE_NOISE
//#define BLADE_FIRE
//How far out on a blade you want it to be lit
//I doubt you want this lower than 100%
#define MAX_BLADE_PERCENTAGE 1000 //100% is the whole blade, set it higher to allow slower exten
//Brightness max, with modulation during swing you want to give it a big range to change brightness
#ifdef BLADE_BRIGHTNESS_SWING_MODULATION
//Default FASTLED max brightness, 0-255
int default_global_brightness = 225;
//Otherwise just let brightness be maxed out, approximately 72/255 or % based on config
#else
int default_global_brightness = 255;
#endif // BLADE_BRIGHTNESS_SWING_MODULATION

#ifdef CONTROL_ROTARY
// rotary control direction pins
#define ROTARY_D1_PIN    15
#define ROTARY_D2_PIN    16
#define ROTARY_GND1_PIN  14
// rotary control switch pins
#define ROTARY_SW_PIN    8
#define ROTARY_GND2_PIN  6

// flip these if your knob goes backwards from what you expect
#define ROTARY_DIR_A    -1
#define ROTARY_DIR_B    +1

#include "rotary.h"
#endif // CONTROL_ROTARY
#ifdef CONTROL_BUTTON
#include "button.h"
#endif // CONTROL_BUTTON



int ctrl_counter = 0;

int accel[3];
int accel_last[3];
int gyro[3];

float velocity[3];
float recent_impulse = 0;

float rotation_history = 0.0;
float rotation_offset = 0.0;
float rotation_factor = 0.0;

float rotation_echo = 0.0;

float velocity_offset = 26.6;
float velocity_factor = 0;

int gyro_hum1_volume = 0;
int accel_hum1_volume = 0;
int inactivity_counter = INACTIVITY_TIMEOUT;

//Random Blade Noise Effect
uint16_t dist;         // A random number for our noise generator.

#ifdef VOLTAGE_SHUTDOWN
byte shutdown_state = 0;
int  shutdown_counter = 0;
#endif

DEFINE_GRADIENT_PALETTE(heatmap_gp) {
	0, 255, 26, 26,   //red
		128, 204, 0, 0,   //darker red
		255, 179, 0, 0 // very dark red
};

DEFINE_GRADIENT_PALETTE(heatmap_luke) {
	0, 16, 194, 239,   //blue
		128,0,204,255,
		255, 85, 250, 255 // lighter blue
};

double count_up = 0;

void init_leds() {
	// setup the blade strips
	blade_array[0].blade_led_count = 30;
	//Allocate the array of LEDs, shouldn't need to release as this only runs once
	blade_array[0].blade_leds = (CRGB*)malloc(blade_array[0].blade_led_count * sizeof(CRGB));
	//Technically unneeded
	blade_array[0].blade_red = 0;
	blade_array[0].blade_green = 205;
	blade_array[0].blade_blue = 255;
	blade_array[0].pin = RX;
	blade_array[0].myPal = heatmap_luke;

	//	LEDS.addLeds<WS2812, blade_array[0].pin, GRB>(blade_array[0].blade_leds, blade_array[0].blade_led_count);
	LEDS.addLeds<WS2812, RX, GRB>(blade_array[0].blade_leds, blade_array[0].blade_led_count);

#ifdef STATUS_LEDS
	// setup the status strip
	LEDS.addLeds<WS2812, STATUS_LEDS_PIN, GRB>(status_leds, STATUS_LEDS_COUNT);
#endif
	//Setting blade brightness with a limit so it can be modulated during swings
	set_blade_brightness(default_global_brightness);
	LEDS.setDither(0);
	blade_out_percentage = 0;
	//Turn LEDs off on startup
	LEDS.clear();
	LEDS.show();
}

void setup() {
	//delay(2000);
	dist = random16(12345);
	// start serial port?
	Serial.begin(9600);
	// enable watchdog timer
	//wdt_enable(WDTO_1S); // no, we cannot do this on a 32u4, it breaks the bootloader upload
	init_leds();
	// start i2c
	Wire.begin();
	MPU6050_start();
	// restore our saved state
	//eeprom_restore();
	// setup controls, either button or rotary switch
#ifdef CONTROL_ROTARY
	start_inputs_rotary();
#endif // ROTARY
#ifdef CONTROL_BUTTON
	setup_buttons();
#endif // CONTROL_BUTTON


	// start sound system
	snd_init();
#ifdef DEBUG
	Serial.println("Running");
	ignite();
#endif
}

void loop() {
	unsigned int time = 0;
	time = micros();
	int i, n, delta;
	float av, rv;
	// the program is alive
	//wdt_reset();
	// alternately read from the gyro and accelerometer
	ctrl_counter = ctrl_counter ^ 1;
	if ((ctrl_counter & 1) == 1) {
		// sample gyro 
		MPU6050_gyro_vector(gyro);
		add_entropy(gyro[0], 0x0F);
		// int3_print(gyro);
		// rotation vector, made from only two axis components (ignore 'twist')
		float gv[3];
		gv[0] = gyro[GYRO_HORIZONTAL];
		gv[1] = gyro[GYRO_VERTICAL];
		gv[2] = 0.0;
		// vector length
		float rot = vec3_length(gv);
		rotation_offset -= (rotation_offset - rot) / 300.0;
		// rotation_history = rotation_history * 0.95 + (rot - rotation_offset) / 1000.0;
		rv = (rot - rotation_offset) / 50.0;
		rv = (rotation_history + rv) / 2.0;
		rotation_history = rv;
		// Serial.println(rv);
	}
	else {
		// sample accel 
		MPU6050_accel_vector(accel);
		add_entropy(accel[0], 0x0F);
		// update the velocity vector
		vec3_addint(velocity, accel);
		vec3_scale(velocity, 0.99);
		// turn velocity vector into scalar factor
		av = vec3_length(velocity) / 10000.0;
		velocity_offset -= (velocity_offset - av) / 10.0;
		velocity_factor = sqrt(abs(velocity_offset - av));
		if (velocity_factor>1.0) velocity_factor = 1.0;
	}
//Use 2 button control
#ifdef CONTROL_BUTTON
	//check button status
	button1.tick();
	//Disabliing button2 for now as it's not that useful
	//button2.tick();
#endif // CONTROL_BUTTON
#ifdef CONTROL_ROTARY
	// read inputs
	check_button();
	check_rotary();
#endif // CONTROL_BUTTON


	// use some entropy to modulate the sound volume
	snd_buzz_speed = snd_buzz_freq + (entropy & 3);
	snd_hum1_speed = snd_hum1_freq;

#ifdef VOLTAGE_SHUTDOWN
	// read the current voltage
	n = analogRead(VOLTAGE_SENSOR_PIN);
	// below minimum volts?
	int undervolt = (int)((VOLTAGE_SHUTDOWN) * (VOLTAGE_SENSOR_RATIO) * 1024.0f / 5.0f);
	// Serial.print("Min "); Serial.print(undervolt); Serial.print("  Vin "); Serial.println(n); 
	if (n < undervolt) {
		if (shutdown_counter == 0) {
			// we are undervolt
			blade_mode = BLADE_MODE_UNDERVOLT;
			// flip mode
			if (shutdown_state == 0) {
				// beep
				shutdown_state = 1;
				shutdown_counter = VOLTAGE_BEEPS_ON;
	}
			else {
				// silence     
				shutdown_state = 0;
				shutdown_counter = VOLTAGE_BEEPS_OFF;
			}
}
		else {
			shutdown_counter--;
		}
	}
	else {
		// above minimum volts
		shutdown_counter = VOLTAGE_BEEPS_OFF;
		shutdown_state = 0;
		if (blade_mode == BLADE_MODE_UNDERVOLT) {
			// we're now just retracting
			blade_mode == BLADE_MODE_EXTINGUISH;
		}
	}
#endif


#ifdef DEBUG
	rotation_history = count_up;
	velocity_factor = (count_up / 100.0);
	count_up+=.1;
	if (count_up > 100) {
		count_up = 0;
	}
#endif

#ifdef CONTROL_ROTARY
	// check the controls
	check_rotary();
#endif  
	// current mode
	switch (blade_mode) {
	case BLADE_MODE_OFF:
		snd_buzz_volume = 0;
		snd_hum1_volume = 0;
		snd_hum2_volume = 0;
		rotation_echo = 0;
		// ignite if the rotation has exceeded the critical value
		if (rotation_history > 100.0) ignite();
		break;
	case BLADE_MODE_ON:
		// rotation hum and pitch-bend
		rv = rotation_history;
		//Should be 0.1 to avoid a crash from divide by zero me thinks?
		if (rv<0.0) rv = 0.1;
		if (rv>140.0) rv = 120.0;
		// update the rotation echo
		if (rv > rotation_echo) {
			// the echo is maximised
			rotation_echo = rv;
		}
		else {
			// decay the previous echo
			rotation_echo = rotation_echo * (0.975f + (float)snd_echo_decay / 10240.0f);
			// use the louder of the original value and 1/1.6 the echo
			rv = max(rv, rotation_echo / 1.6);
		}
		// rotation volume term
		rv = rv / 256.0 * global_volume;
		delta = 0;
		if (rv>snd_hum2_volume) { delta = 16; }
		else if (rv<snd_hum2_volume) { delta = -8; }
		snd_hum2_volume = value_delta(snd_hum2_volume, delta, 0, 255);

		snd_hum1_speed = snd_hum1_freq + (rotation_history / snd_hum2_doppler);
		snd_hum2_speed = snd_hum2_freq + (rotation_history / snd_hum2_doppler);

		// turn velocity into volume modifications
		av = velocity_factor;
		if (av>1.0) av = 1.0;
		snd_buzz_volume = 8 + (int)(av * 32.0); snd_buzz_volume = ((unsigned int)snd_buzz_volume*(unsigned int)global_volume) / 256;
		snd_hum1_volume = 12 + (int)(av * 40.0); snd_hum1_volume = max(((unsigned int)snd_hum1_volume*(unsigned int)global_volume) / 256, snd_hum2_volume);

		//Change Saber brightness during swing
		//update_blade_array_brightness((int)(rotation_history / snd_hum2_doppler));
		//velocity_factor or av range is 0-1.0
		//Sets blade brightness according to swing speed, modulating a range of 60,30 up, 30 down
		if ((ctrl_counter & 1) == 1) {//it's too fast now so i'm running it half time
#ifdef BLADE_BRIGHTNESS_SWING_MODULATION
			set_blade_brightness(default_global_brightness + (60 * av) - 30);
#endif
#ifdef BLADE_NOISE
			update_blade_array_noise();
#elif defined(BLADE_FIRE)
			update_blade_array_fire();
#else
#endif
		}

		// check for inactivity
		if ((velocity_factor < 0.4) && (rotation_history < 10.0)) {
			// inactive
			if (inactivity_counter == 0) {
				extinguish();
			}
			else {
				inactivity_counter--;
			}
		}
		else {
			// active
			inactivity_counter = INACTIVITY_TIMEOUT;
			// Serial.print(velocity_factor); Serial.print(' ');
			// Serial.println(rotation_history);
		}
		break;
	case BLADE_MODE_IGNITE:
		if (blade_out_percentage < MAX_BLADE_PERCENTAGE) {
			/*Serial.println(extend_speed);*/
			blade_out_percentage += extend_speed;
			//Serial.println(blade_out_percentage);
			if (blade_out_percentage > MAX_BLADE_PERCENTAGE) blade_out_percentage = MAX_BLADE_PERCENTAGE;
			update_blade_array();
			// loud volume
			snd_buzz_volume = (40 * (unsigned int)global_volume) / 256;
			snd_hum1_volume = (140 * (unsigned int)global_volume) / 256;
			snd_hum2_volume = (120 * (unsigned int)global_volume) / 256;
			// bend pitch
			snd_hum1_speed = snd_hum1_freq + (MAX_BLADE_PERCENTAGE - blade_out_percentage);
			snd_hum2_speed = snd_hum2_freq + (MAX_BLADE_PERCENTAGE - blade_out_percentage);
		}
		else {
			blade_mode = BLADE_MODE_ON;
			rotation_history = 100.0;
			inactivity_counter = INACTIVITY_TIMEOUT;
			//extinguish();
		}
		break;
	case BLADE_MODE_EXTINGUISH:
		if (blade_out_percentage > 0) {
			blade_out_percentage-=extend_speed; update_blade_array();
			// limit the volume on the way down
			int v = (blade_out_percentage * global_volume) / MAX_BLADE_PERCENTAGE;
			snd_buzz_volume = min(v, snd_buzz_volume);
			snd_hum1_volume = min(v, snd_hum1_volume);
			snd_hum2_volume = min(v, snd_hum2_volume);
			// bend pitch
			snd_hum1_speed = snd_hum1_freq + (MAX_BLADE_PERCENTAGE - blade_out_percentage);
			snd_hum2_speed = snd_hum2_freq + (MAX_BLADE_PERCENTAGE - blade_out_percentage);
		}
		else {
			blade_mode = BLADE_MODE_OFF;
			//ignite();
		}
		break;
#ifdef VOLTAGE_SHUTDOWN
	case BLADE_MODE_UNDERVOLT:
		// retract
		if (blade_out_percentage> 0) {
			blade_out_percentage--; update_blade_array();
		}
		// blink the light
		blade_array[0].blade_leds[0] = (shutdown_state == 0) ? CRGB::Black : CRGB::Red;
		LEDS.show();
		// use the beeps!
		snd_buzz_volume = 0;
		snd_hum1_volume = 0;
#ifdef VOLTAGE_BEEPS
		if (shutdown_state == 0) {
			snd_hum2_volume = 0;
		}
		else {
			snd_hum2_speed = VOLTAGE_BEEPS_FREQ;
			snd_hum2_volume = 255;
		}
#else
		snd_hum2_volume = 0;
#endif
		break;
#endif
	}	
	
	
	time = micros() - time;
	//Serial.println(time, DEC);
	}
