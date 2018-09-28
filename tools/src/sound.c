#include <SDL.h>
#include <SDL_mixer.h>
#include "thread.h"
#include "file.h"
#include "kiss.h"

struct wav_t
{
   char      chunk_id[4];
   int       chunk_size;
   char      format[4];
   char      subchunk1_id[4];
   int       subchunk1_size;
   short int audio_format;
   short int num_channels;
   int       sample_rate;
   int       byte_rate;
   short int block_align;
   short int bits_per_sample;
   char      subchunk2_id[4];
   int       subchunk2_size;
};

int
sound_wave_file_play(const char *filename)
{
   FILE *f = NULL;
   size_t bytes;
   const char *ext;
   Mix_Music *music = NULL;
   struct wav_t header;
   char *path = NULL;
   int ret = -1;

   if (!file_exists(filename))
     goto error;

   path = strdup(filename);

   ext = strrchr(path, '.');
   if (!ext || strcasecmp(ext, ".wav"))
     goto error;

   f = fopen(path, "r");
   if (!f) goto error;

   bytes = fread(&header, sizeof(struct wav_t), 1, f);
   if (bytes != 1) goto error;
   fclose(f);
   f = NULL;

   SDL_CHECK(SDL_Init(SDL_INIT_AUDIO) == 0);
   SDL_CHECK(Mix_OpenAudio(header.sample_rate, MIX_DEFAULT_FORMAT, header.num_channels, 2048) == 0);
   SDL_CHECK(music = Mix_LoadMUS(path));

   Mix_PlayMusic(music, 1);
   while (1)
     {
        if (Mix_PlayingMusic() == 0)
          break;
        SDL_Delay(1000);
     }

   ret = 0;
error:
   if (path)
     free(path);
   if (f)
     fclose(f);
   if (music)
     Mix_FreeMusic(music);

   Mix_CloseAudio();
   SDL_Quit();
   return ret;
}

static void *
_sound_wave_file_play(thread_t *thread, void *data)
{
   (void) thread;
   sound_wave_file_play((char *) data);

   return NULL;
}

int
sound_play(const char *path)
{
   return sound_wave_file_play(path);
}

thread_t *
sound_async_play(char *path)
{
   thread_t *thread = thread_run(_sound_wave_file_play, NULL, NULL, path);

   return thread;
}
