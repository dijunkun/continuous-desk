#include <SDL2/SDL.h>

int main(int argc, char *argv[]) {
  int ret;
  SDL_AudioSpec wanted_spec, obtained_spec;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s",
                 SDL_GetError());
    return -1;
  }

  // Set audio format
  wanted_spec.freq = 44100;  // Sample rate
  wanted_spec.format =
      AUDIO_F32SYS;          // Sample format (32-bit float, system byte order)
  wanted_spec.channels = 2;  // Number of channels (stereo)
  wanted_spec.samples = 1024;   // Buffer size (in samples)
  wanted_spec.callback = NULL;  // Audio callback function (not used here)

  // Open audio device
  ret = SDL_OpenAudio(&wanted_spec, &obtained_spec);
  if (ret < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Failed to open audio device: %s", SDL_GetError());
    return -1;
  }

  // Start playing audio
  SDL_PauseAudio(0);

  // Write PCM data to audio buffer
  float *pcm_data = ...;    // PCM data buffer (float, interleaved)
  int pcm_data_size = ...;  // Size of PCM data buffer (in bytes)
  int bytes_written = SDL_QueueAudio(0, pcm_data, pcm_data_size);

  // Wait until audio buffer is empty
  while (SDL_GetQueuedAudioSize(0) > 0) {
    SDL_Delay(100);
  }

  // Stop playing audio
  SDL_PauseAudio(1);

  // Close audio device
  SDL_CloseAudio();

  // Quit SDL
  SDL_Quit();

  return 0;
}
