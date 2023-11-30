#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
  if (SDL_Init(SDL_INIT_AUDIO)) {
    printf("SDL init error\n");
    return -1;
  }

  // SDL_AudioSpec
  SDL_AudioSpec wanted_spec;
  SDL_zero(wanted_spec);
  wanted_spec.freq = 48000;
  wanted_spec.format = AUDIO_S16LSB;
  wanted_spec.channels = 2;
  wanted_spec.silence = 0;
  wanted_spec.samples = 960;
  wanted_spec.callback = NULL;

  SDL_AudioDeviceID deviceID = 0;
  // 打开设备
  if ((deviceID = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, NULL,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)) < 2) {
    printf("could not open audio device: %s\n", SDL_GetError());

    // 清除所有的子系统
    SDL_Quit();
    return 0;
  }

  SDL_PauseAudioDevice(deviceID, 0);

  FILE* fp = nullptr;

  fopen_s(&fp, "ls.pcm", "rb+");
  if (fp == NULL) {
    printf("cannot open this file\n");
    return -1;
  }

  if (fp == NULL) {
    printf("error \n");
  }
  Uint32 buffer_size = 4096;
  char* buffer = (char*)malloc(buffer_size);

  while (true) {
    if (fread(buffer, 1, buffer_size, fp) != buffer_size) {
      printf("end of file\n");
      break;
    }
    SDL_QueueAudio(deviceID, buffer, buffer_size);
  }

  printf("Play...\n");

  SDL_Delay(10000);

  // Uint32 residueAudioLen = 0;

  // while (true) {
  //   residueAudioLen = SDL_GetQueuedAudioSize(deviceID);
  //   printf("%10d\n", residueAudioLen);
  //   if (residueAudioLen <= 0) break;
  //   SDL_Delay(1);
  // }

  // while (true) {
  //   printf("1 暂停 2 继续  3 退出 \n");
  //   int flag = 0;

  //   scanf_s("%d", &flag);

  //   if (flag == 1)
  //     SDL_PauseAudioDevice(deviceID, 1);
  //   else if (flag == 2)
  //     SDL_PauseAudioDevice(deviceID, 0);
  //   else if (flag == 3)
  //     break;
  // }

  SDL_CloseAudio();
  SDL_Quit();
  fclose(fp);

  return 0;
}