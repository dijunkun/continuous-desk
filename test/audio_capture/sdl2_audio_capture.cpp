#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_AudioDeviceID input_dev;
static SDL_AudioDeviceID output_dev;

static Uint8 *buffer = 0;
static int in_pos = 0;
static int out_pos = 0;

char *out = "audio_old.pcm";
FILE *outfile = fopen(out, "wb+");

void cb_in(void *userdata, Uint8 *stream, int len) {
  // If len < 4, the printf below will probably segfault

  // fwrite(stream, 1, len, outfile);
  // fflush(outfile);

  // SDL_memcpy(stream, buffer + in_pos, len);
  // in_pos += len;
  // printf("IN:  %d\t%d %d %d %d\n", in_pos, stream[0], stream[1], stream[2],
  //        stream[3]);
}

void cb_out(void *userdata, Uint8 *stream, int len) {
  // If len < 4, the printf below will probably segfault
  fwrite(stream, 1, len, outfile);
  fflush(outfile);

  // if (out_pos >= in_pos) {
  //   // Output is way ahead of input; fill with emptiness
  //   memset(buffer + out_pos, 0, len * sizeof(Uint8));
  //   printf("OUT: %d\t(Empty)\n", out_pos);
  // } else if (out_pos + len > in_pos) {
  //   // Output is reaching input; read until reaching input, and leave the
  //   rest
  //   // empty
  //   memset(buffer + out_pos, 0, len * sizeof(Uint8));
  //   SDL_memcpy(buffer + out_pos, stream, in_pos - out_pos);
  //   out_pos = in_pos;
  //   printf("OUT: %d\t%d %d %d %d (Partial)\n", out_pos, stream[0], stream[1],
  //          stream[2], stream[3]);
  // } else {
  //   // Input is way ahead of output; read as much as requested
  //   SDL_memcpy(buffer + out_pos, stream, len);
  //   out_pos += len;
  //   printf("OUT: %d\t%d %d %d %d\n", out_pos, stream[0], stream[1],
  //   stream[2],
  //          stream[3]);
  // }

  // This is to make sure the output device works
  // for (int i = 0; i < len; i++)
  //    stream[i] = (Uint8) random();
}

int main() {
  SDL_Init(SDL_INIT_AUDIO);

  // 16Mb should be enough; the test lasts 5 seconds
  buffer = (Uint8 *)malloc(16777215);

  SDL_AudioSpec want_in, want_out, have_in, have_out;

  SDL_zero(want_out);
  want_out.freq = 48000;
  want_out.format = AUDIO_U16LSB;
  want_out.channels = 2;
  want_out.samples = 960;
  want_out.callback = cb_out;

  output_dev = SDL_OpenAudioDevice(NULL, 0, &want_out, &have_out,
                                   SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (output_dev == 0) {
    SDL_Log("Failed to open output: %s", SDL_GetError());
    return 1;
  }

  SDL_zero(want_in);
  want_in.freq = 48000;
  want_in.format = AUDIO_U16LSB;
  want_in.channels = 2;
  want_in.samples = 960;
  want_in.callback = cb_in;

  input_dev = SDL_OpenAudioDevice(NULL, 1, &want_in, &have_in,
                                  SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (input_dev == 0) {
    SDL_Log("Failed to open input: %s", SDL_GetError());
    return 1;
  }

  SDL_PauseAudioDevice(input_dev, 0);
  SDL_PauseAudioDevice(output_dev, 0);

  SDL_Delay(5000);

  SDL_CloseAudioDevice(output_dev);
  SDL_CloseAudioDevice(input_dev);
  free(buffer);

  fclose(outfile);
}
