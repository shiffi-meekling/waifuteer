#include "sound.h"

static float sound_level;

float get_sound_level() {
	return sound_level;
}

void audioRecordingCallback( void* userdata, Uint8* stream, int len ) {
	sound_level = 0;
	for (int i =0; i<len; i++) {
		char x = stream[i];
		if (sound_level < x) sound_level = x;
	}
};

static SDL_AudioSpec audio_spec;

void sound_init() {

	SDL_AudioSpec want;
	SDL_AudioDeviceID device;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = 48000;
	want.format = AUDIO_S8;
	want.channels = 1;
	want.samples = 512;
	want.callback = audioRecordingCallback;  // you wrote this function elsewhere.

	device = SDL_OpenAudioDevice(NULL, true, &want, &audio_spec, 0);
	//start the device
	SDL_PauseAudioDevice(device, 0);

}



