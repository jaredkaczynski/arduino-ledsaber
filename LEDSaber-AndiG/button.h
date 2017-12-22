//Handle button press
void button1_click() {
	// if we are properly off, then ignite
	switch (blade_mode) {
	case BLADE_MODE_OFF:
		ignite();
		break;
	}
}
//Handle long press 
void button1_held() {
	// toggle the blade
	switch (blade_mode) {
	case BLADE_MODE_OFF:
	case BLADE_MODE_EXTINGUISH:
		ignite();
		break;
	case BLADE_MODE_ON:
	case BLADE_MODE_IGNITE:
		extinguish();
		break;
	}
}

//Handle mode change click of button 2

void button2_click() {
	button_mode += 1;
	//MODE_COUNT
	button_mode %= MODE_COUNT;
	switch (button_mode) {
		// blade extension
	case 0:
		// turn blade on and off
		switch (blade_mode) {
		case BLADE_MODE_OFF:
		case BLADE_MODE_EXTINGUISH:
			ignite();
			break;
		case BLADE_MODE_ON:
			extinguish();
			break;
		case BLADE_MODE_IGNITE:
			extinguish();
			break;
		case BLADE_MODE_UNDERVOLT:
			blade_mode = BLADE_MODE_OFF;
			break;
		}
		break;
		// global volume
	case 1: global_volume = value_delta(global_volume, 1 * 16, 0, 255); break;
		// blade preset
	case 2:
		// change preset number
		blade_preset = value_delta(blade_preset, 1, 0, 7);
		// load preset values
		//Changing hue for all blades?
		update_blade_array(-1, preset_saturation[blade_preset], preset_hue[blade_preset]);
		extend_speed = preset_speed[blade_preset];
		snd_buzz_freq = preset_buzz[blade_preset];
		snd_hum1_freq = preset_hum1[blade_preset];
		snd_hum2_freq = preset_hum2[blade_preset];
		snd_hum2_doppler = preset_doppler[blade_preset];
		snd_echo_decay = preset_echo[blade_preset];
		break;
		// blade properties
	//case 3:
	//	//update hue
	//	update_blade_array(value_delta(blade_array[0].blade_blue, 1 * 8, 7, 255), -1, -1);
	//	break;
	//case 4:
	//	//update brightness/value
	//	update_blade_array(-1, -1, blade_array[0].blade_red += 1 * 4);
	//	break;
	//case 5:
	//	//update saturation
	//	update_blade_array(-1, value_delta(blade_array[0].blade_green, 1 * 8, 0, 255), -1);
	//	break;
	//	// sound properties
	//case 6: snd_buzz_freq += d; break;
	//case 7: snd_hum1_freq += d; break;
	//case 8: snd_hum2_freq += d; break;
	//case 9: snd_hum2_doppler += d; break;
	//case 10: snd_echo_decay = value_delta(snd_echo_decay, d, 0, 255); break;

	}
}

void setup_buttons() {
	button1.attachClick(button1_click);
	button1.attachDuringLongPress(button1_held);
	button2.attachClick(button2_click);

}