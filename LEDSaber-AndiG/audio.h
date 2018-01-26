//#define AUDIO_PWM9
#define AUDIO_PWM8
//#define AUDIO_PWM7
//#define AUDIO_PWM6
//#define AUDIO_PWM4

#define USE_DMA_IRQ

#include <libmaple/dma.h>

// audio properties
int snd_buzz_speed = 47;
int snd_hum1_speed = 50;
int snd_hum2_speed = 52;

byte snd_buzz_volume = 0;
byte snd_hum1_volume = 0;
byte snd_hum2_volume = 0;

const uint8_t SPEAKER_PIN = PB6; // tim4 ch1 output



#define SAMPLING_RATE_HZ 8000 // todo: extract from WAV file

#define timer Timer4

#define TIMER_stop  (timer.pause)
#define TIMER_start (timer.resume)
#define DMA_BUF_SIZE 1

// sound fonts
#define BUZZ_WAVE_LENGTH 205

byte buzz_wave[BUZZ_WAVE_LENGTH] = {
	0x82, 0x82, 0x75, 0x65, 0x55, 0x4B, 0x5E, 0x74, 0x7C, 0x71, 0x57, 0x58,
	0x89, 0xAE, 0xB3, 0xC8, 0xD4, 0xBD, 0x83, 0x4B, 0x34, 0x42, 0x6D, 0x9F,
	0xD6, 0xF7, 0xF5, 0xD6, 0xB5, 0xA8, 0x9B, 0x8B, 0x77, 0x57, 0x31, 0x19,
	0x1B, 0x29, 0x3C, 0x64, 0x99, 0xBE, 0xBA, 0x9F, 0x87, 0x76, 0x6B, 0x66,
	0x67, 0x6E, 0x7E, 0x95, 0x9E, 0x8E, 0x75, 0x5C, 0x57, 0x5D, 0x5F, 0x63,
	0x62, 0x61, 0x5B, 0x56, 0x57, 0x56, 0x64, 0x82, 0x9F, 0xB8, 0xCF, 0xE0,
	0xE2, 0xD9, 0xC3, 0x9D, 0x84, 0x7D, 0x7A, 0x7A, 0x7B, 0x85, 0x90, 0x98,
	0x8E, 0x72, 0x61, 0x5C, 0x6A, 0x81, 0x94, 0x9D, 0x9F, 0x94, 0x7A, 0x5E,
	0x48, 0x3D, 0x38, 0x3E, 0x4A, 0x5D, 0x74, 0x80, 0x85, 0x82, 0x7C, 0x77,
	0x73, 0x76, 0x7D, 0x96, 0xB4, 0xC5, 0xC5, 0xBE, 0xB5, 0xA6, 0x93, 0x82,
	0x6E, 0x5A, 0x56, 0x59, 0x5C, 0x60, 0x66, 0x69, 0x6D, 0x6F, 0x69, 0x6C,
	0x78, 0x88, 0x93, 0xA1, 0xA7, 0xA4, 0xA0, 0x9D, 0x9E, 0x9E, 0x99, 0x8D,
	0x83, 0x7D, 0x73, 0x6D, 0x74, 0x7F, 0x80, 0x78, 0x6A, 0x57, 0x45, 0x37,
	0x36, 0x47, 0x64, 0x7A, 0x8B, 0x93, 0x8E, 0x8B, 0x8E, 0x99, 0xA8, 0xB5,
	0xBE, 0xC5, 0xC7, 0xBC, 0xA8, 0x98, 0x8E, 0x8B, 0x87, 0x7F, 0x79, 0x7F,
	0x80, 0x80, 0x7E, 0x72, 0x68, 0x5E, 0x59, 0x5C, 0x5C, 0x64, 0x6F, 0x7D,
	0x80, 0x76, 0x6B, 0x62, 0x69, 0x74, 0x7F, 0x8A, 0x92, 0x95, 0x90, 0x86,
	0x7B
};

#define HUM1_WAVE_LENGTH 133
byte hum1_wave[HUM1_WAVE_LENGTH] = {
	0x7F, 0x73, 0x65, 0x56, 0x4C, 0x49, 0x4B, 0x53, 0x60, 0x6F, 0x7C, 0x8B,
	0x98, 0xA4, 0xB0, 0xBA, 0xC6, 0xCA, 0xC0, 0xAF, 0x93, 0x71, 0x52, 0x38,
	0x26, 0x1E, 0x1D, 0x26, 0x32, 0x45, 0x5C, 0x73, 0x89, 0x98, 0xA2, 0xA9,
	0xAD, 0xA8, 0x9E, 0x92, 0x83, 0x78, 0x72, 0x6C, 0x6B, 0x71, 0x79, 0x82,
	0x8D, 0x96, 0x98, 0x94, 0x92, 0x93, 0x94, 0x99, 0xA0, 0xA7, 0xA8, 0xA5,
	0xA3, 0x9F, 0x98, 0x92, 0x8F, 0x88, 0x83, 0x7A, 0x6C, 0x5E, 0x50, 0x47,
	0x43, 0x44, 0x50, 0x61, 0x71, 0x83, 0x97, 0xA6, 0xB5, 0xC1, 0xCC, 0xD1,
	0xC8, 0xBA, 0xA0, 0x7E, 0x5E, 0x41, 0x2C, 0x1D, 0x17, 0x1B, 0x24, 0x34,
	0x4A, 0x63, 0x79, 0x8C, 0x9A, 0xA1, 0xA7, 0xA5, 0x9C, 0x90, 0x81, 0x76,
	0x6E, 0x68, 0x66, 0x6A, 0x72, 0x7A, 0x83, 0x8B, 0x8D, 0x89, 0x86, 0x87,
	0x88, 0x8E, 0x99, 0xA1, 0xA6, 0xA6, 0xA6, 0xA3, 0x9C, 0x98, 0x99, 0x96,
	0x91
};

#define HUM2_WAVE_LENGTH 133
byte hum2_wave[HUM2_WAVE_LENGTH] = {
	0x7A, 0x73, 0x6E, 0x62, 0x5D, 0x5F, 0x63, 0x70, 0x7B, 0x85, 0x93, 0x9E,
	0xAB, 0xB4, 0xC1, 0xC9, 0xCC, 0xCA, 0xC0, 0xB9, 0xB3, 0xAC, 0xAA, 0xA3,
	0xA0, 0xA1, 0x98, 0x91, 0x87, 0x76, 0x65, 0x53, 0x45, 0x37, 0x34, 0x2E,
	0x22, 0x25, 0x28, 0x2F, 0x3C, 0x48, 0x53, 0x60, 0x74, 0x7C, 0x8A, 0x95,
	0x99, 0xA4, 0xA2, 0x9F, 0x9A, 0x95, 0x90, 0x8B, 0x8D, 0x8D, 0x8F, 0x90,
	0x93, 0x96, 0x97, 0x97, 0x92, 0x8B, 0x86, 0x78, 0x64, 0x5A, 0x4E, 0x48,
	0x4A, 0x4D, 0x55, 0x5D, 0x6F, 0x85, 0x97, 0xAB, 0xBA, 0xC7, 0xCA, 0xCA,
	0xC9, 0xBC, 0xB6, 0xAE, 0xA2, 0x9E, 0x9E, 0x9D, 0x97, 0x92, 0x89, 0x78,
	0x6A, 0x55, 0x49, 0x41, 0x37, 0x36, 0x34, 0x33, 0x3D, 0x45, 0x50, 0x61,
	0x72, 0x82, 0x8F, 0x99, 0x9C, 0xA2, 0xA1, 0x98, 0x90, 0x88, 0x7F, 0x76,
	0x77, 0x76, 0x7B, 0x87, 0x82, 0x81, 0x7C, 0x7A, 0x87, 0x91, 0x9C, 0x99,
	0x8D
};

unsigned int sample;

#define dmaBuffer sample


byte sound_sample(int * index, byte * wave, int wave_speed, byte wave_length) {
	// interpolate next sample
	int i = *index;
	byte si1 = (i >> 8); // first sample index
	byte si2 = si1 + 1; if (si2 == wave_length) { si2 = 0; } // wrap next sample index
	int sj1 = i & 255; // sample sub-index
	int sj2 = 256 - sj1; // 
						 // mix the two sample values
	unsigned int s1 = wave[si1]; s1 *= sj2;
	unsigned int s2 = wave[si2]; s2 *= sj1;
	unsigned int sample = s1 + s2;
	// increment index, wrap at limit
	i += wave_speed;
	unsigned int index_limit = wave_length << 8;
	if (i >= index_limit) i -= index_limit;
	*index = i;
	// return the top byte
	return sample / 256;
}

/*
// sound ring buffer (record our own output for echo/harmonic effects)
byte sound_ring[1024];
unsigned int sound_ring_index = 0;
inline int sound_ring_sample(int ago) {
unsigned int i = (sound_ring_index - ago ) & 0x03ff;
return sound_ring[i];
}
*/

// sound generators
int snd_index_1 = 0;
int snd_index_2 = 0;
int snd_index_3 = 0;

//isr(timer1_compa_vect) {
//	//digitalwrite(8,high);
//	// combine the wave and global volume into channel volumes
//	unsigned int v1 = snd_buzz_volume; //v1 *= global_volume; v1 = v1 >> 8;
//	unsigned int v2 = snd_hum1_volume; //v2 *= global_volume; v2 = v2 >> 8;
//	unsigned int v3 = snd_hum2_volume; //v3 *= global_volume; v3 = v3 >> 8;
//									   // sample our primary waveforms, and multiply by their master volumes
//	int s1 = (sound_sample(&snd_index_1, buzz_wave, snd_buzz_speed, buzz_wave_length) - 128) * v1;
//	int s2 = (sound_sample(&snd_index_2, hum1_wave, snd_hum1_speed, hum1_wave_length) - 128) * v2;
//	int s3 = (sound_sample(&snd_index_3, hum2_wave, snd_hum2_speed, hum2_wave_length) - 128) * v3;
//	// combine the samples together
//	unsigned int sample = 0x8000 + s1 + s2 + s3;
//
//
//
//	// update the pwm value with the top few bits
//#ifdef audio_pwm9
//	ocr4b = (sample >> 7) & 0x1ff;
//#endif
//#ifdef audio_pwm8
//	ocr4b = (sample >> 8) & 0xff;
//#endif
//#ifdef audio_pwm7
//	ocr4b = (sample >> 9) & 0x7f;
//#endif
//#ifdef audio_pwm6
//	ocr4b = (sample >> 10) & 0x3f;
//#endif
//#ifdef audio_pwm4
//	ocr4b = (sample >> 12) & 0x0f;
//#endif
//	/*
//	// store the sample in the ring
//	sound_ring[sound_ring_index] = sample >> 8;
//	sound_ring_index = (sound_ring_index + 1) & 0x03ff;
//	*/
//	// done
//	//digitalwrite(8,low);
//}

#ifdef USE_DMA_IRQ
//-----------------------------------------------------------------------------
// This is our DMA interrupt handler.
volatile uint8_t dma_full_complete;
//-----------------------------------------------------------------------------
void DMA_isr(void)
{
	unsigned int v1 = snd_buzz_volume; //v1 *= global_volume; v1 = v1 >> 8;
	unsigned int v2 = snd_hum1_volume; //v2 *= global_volume; v2 = v2 >> 8;
	unsigned int v3 = snd_hum2_volume; //v3 *= global_volume; v3 = v3 >> 8;
									   // sample our primary waveforms, and multiply by their master volumes
	int s1 = (sound_sample(&snd_index_1, buzz_wave, snd_buzz_speed, 205) - 128) * v1;
	int s2 = (sound_sample(&snd_index_2, hum1_wave, snd_hum1_speed, 133) - 128) * v2;
	int s3 = (sound_sample(&snd_index_3, hum2_wave, snd_hum2_speed, 133) - 128) * v3;
	// combine the samples together
	sample = 0x8000 + s1 + s2 + s3;
	uint32_t dma_isr = dma_get_isr_bits(DMA1, DMA_CH7);
	//if (dma_isr&DMA_ISR_HTIF1) {
	//	dma_half_complete = true;
	//}
	if (dma_isr&DMA_ISR_TCIF1) {
		dma_full_complete = true;
		// here we could stop the timer
	}
	dma_clear_isr_bits(DMA1, DMA_CH7);
}
#else
#define dma_full_complete (DMA1->regs->CNDTR7==0)
#endif

void enable_intr() {
}

void DMA_Setup() {
	dma_init(DMA1);
	dma_disable(DMA1, DMA_CH7);
	uint32_t flags = (DMA_FROM_MEM | DMA_MINC_MODE); // | DMA_CIRC_MODE));
#ifdef USE_DMA_IRQ
	flags |= DMA_TRNS_CMPLT;
	dma_attach_interrupt(DMA1, DMA_CH7, DMA_isr);
#endif
	dma_setup_transfer(DMA1, DMA_CH7,
		&((TIMER4->regs).gen->CCR1), DMA_SIZE_16BITS, // peripheral
		(uint8_t *)dmaBuffer, DMA_SIZE_8BITS, // memory
		flags);
	dma_set_priority(DMA1, DMA_CH7, DMA_PRIORITY_HIGH);
}

//-----------------------------------------------------------------------------
void DMA_start()
{
#ifdef USE_DMA_IRQ
	dma_full_complete = false;
#endif
	dma_disable(DMA1, DMA_CH7);
	dma_set_num_transfers(DMA1, DMA_CH7, DMA_BUF_SIZE);
	dma_enable(DMA1, DMA_CH7);
}
//-----------------------------------------------------------------------------
// Set TIM4_UPDATE event to trigger DMA1 channel 7
//-----------------------------------------------------------------------------
void TIMER_Setup()
{
	pinMode(SPEAKER_PIN, OUTPUT);
	pinMode(SPEAKER_PIN, PWM); // activate output
	timer.pause();
	// try to set the reload register to a value as close as possible to 256
	// to get full range audio 0 - 3.3V for 0 - 255 PWM values (8 bit resolution)
	uint32_t prescaler = F_CPU / SAMPLING_RATE_HZ / 256;
	timer.setPrescaleFactor(prescaler);
	uint32_t overflow = F_CPU / SAMPLING_RATE_HZ / prescaler;
	timer.setOverflow(overflow);
	//	Serial.print("prescaler: "); Serial.println(timer.getPrescaleFactor());
	//	Serial.print("reload register: "); Serial.println(timer.getOverflow());
	//	Serial.print("sampling rate: "); Serial.println((uint32_t)(F_CPU/timer.getPrescaleFactor()/timer.getOverflow()));
	timer.enableDMA(0); // for udate event use channel 0
}

//void snd_init() {
//	//pinMode(8, OUTPUT);
//	//digitalWrite(8,LOW);
//	pinMode(10, OUTPUT);
//	/*
//	// init the internal PLL
//	PLLFRQ = _BV(PDIV2);
//	PLLCSR = _BV(PLLE);
//	while(!(PLLCSR & _BV(PLOCK)));
//	PLLFRQ |= _BV(PLLTM0); // PCK 48MHz
//	*/
//	// setup timer4 
//	TCCR4A = (1 << PWM4B) | (0 << COM4B1) | (1 << COM4B0);
//	TCCR4B = (0 << CS43) | (0 << CS42) | (0 << CS41) | (1 << CS40);
//	TCCR4E = (1 << ENHC4);
//	// TCCR4D = (1<<WGM40); //set it to phase and frequency correct mode    
//	TCCR4D = (0 << WGM41) | (0 << WGM40); //set it to fast PWM mode    
//										  // TCCR4C = 0;
//#ifdef AUDIO_PWM9
//	OCR4C = 255; // timer max
//#endif
//#ifdef AUDIO_PWM8
//	OCR4C = 128; // timer max
//#endif
//#ifdef AUDIO_PWM7
//	OCR4C = 64; // timer max
//#endif
//#ifdef AUDIO_PWM6
//	OCR4C = 32; // timer max
//#endif
//#ifdef AUDIO_PWM4
//	OCR4C = 8; // timer max
//#endif
//
//			   // TCCR4C |= (1<<COM4B1); // start timer
//
//			   // setup timer 1
//	TCCR1A = (0 << WGM11) | (0 << WGM10);
//	TCCR1B = (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10);
//	OCR1A = 512; // 1 * 1024;
//				 // ICR1 = 64;
//				 // TCCR1B = (1<<WGM13) | (0<<WGM12) | (0<<CS12) | (1<<CS11) | (0<<CS10);
//				 // enable timer 1 interrupt
//				 // TIMSK1 |= _BV(TOIE1);
//	TIMSK1 |= (1 << OCIE1A);
//
//	// 
//}
//
//void snd_signal(unsigned int sample) {
//	OCR4D = (sample) & 127;
//}
//
//void snd_stop() {
//	TCCR4A &= ~(_BV(COM4D1));
//}