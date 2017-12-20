typedef struct blade
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
	int pin;
};
