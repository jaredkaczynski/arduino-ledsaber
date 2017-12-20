// presets                   aqua green yell whit blue prpl orng red  
int   preset_hue[] =        { 144,  96,  64,  144, 160, 192,  32,   0 };
int   preset_saturation[] = { 255, 255, 255,  32,  255, 255, 255, 255 };
int   preset_buzz[] =       {  47,  48,  47,  50,  47,  50,   49,  45 };
int   preset_hum1[] =       {  50,  50,  55,  60,  50,  50,   50,  47 };
int   preset_hum2[] =       {  52,  51,  54,  60,  52,  57,   50,  47 };
int   preset_doppler[] =    {  30,  30,  35,  40,  40,  45,  -10,  30 };
int   preset_echo[] =       { 180, 200, 210, 220, 180, 192,  240, 210 };
int   preset_speed[] =      {   3,   2,   2,   3,   2,   3,   2,    1 };

// rotary knob state
int  button_mode = 0;
byte button_state = 0;

// audio state
byte global_volume = 240;
int snd_buzz_freq = 47; 
int snd_hum1_freq = 50; 
int snd_hum2_freq = 52; 
int snd_hum2_doppler = 40;
int snd_echo_decay = 128;
unsigned int entropy = 0;
void add_entropy(byte e, byte mask) {
  entropy = entropy << 1 ^ (e & mask);
}



// add a delta to a value, and limit the result to a range
int value_delta(int value, int delta, int vmin, int vmax) {
  int rd = 0;
  if(delta>0) {
    rd = min( vmax - value, delta);
  }
  if(delta<0) {
    rd = max( vmin - value, delta);
  }
  return value + rd;
}

/*
void update_property(int * v, int delta) {
  *v += delta;
  if(*v<0) *v=0;
  if(*v>=8192) *v=8191;
}

byte get_property(int * v) {
  int r = *v;
  return (r >> 5) & 255;
}
*/

void eeprom_restore() {
  // check the first two bytes for our magic value, indicating the eeprom isn't empty
  bool valid = (EEPROM.read(0) == 42) && (EEPROM.read(1) == !42);
  // if valid, load the properties
  if(valid) {
    button_mode = EEPROM.read(2);
    // audio state
    global_volume = EEPROM.read(3);
    snd_buzz_freq = EEPROM.read(4); 
    snd_hum1_freq = EEPROM.read(5); 
    snd_hum2_freq = EEPROM.read(6); 
    snd_hum2_doppler = EEPROM.read(7) - 128;
    snd_echo_decay = EEPROM.read(12); 
    // blade color
	//Unsave colors, will add back later if necessary
    //blade_hue = EEPROM.read(8); 
    //blade_saturation = EEPROM.read(9);
    //blade_brightness = EEPROM.read(10); 
    extend_speed = EEPROM.read(11);
  }
}

void eeprom_save() {
  // save the properties
  EEPROM.update(2,button_mode);
  // sound properties
  EEPROM.update(3,global_volume);
  EEPROM.update(4,snd_buzz_freq);
  EEPROM.update(5,snd_hum1_freq);
  EEPROM.update(6,snd_hum2_freq);
  EEPROM.update(7,snd_hum2_doppler);
  EEPROM.update(12,snd_echo_decay);
  // blade color
  //EEPROM.update(8,blade_hue);
  //EEPROM.update(9,blade_saturation);
  //EEPROM.update(10,blade_brightness);
  EEPROM.update(11,extend_speed);
  // commit with the valid token
  EEPROM.update(0,42);
  EEPROM.update(1,!42);
}



