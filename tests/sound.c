#include "sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
//   sound_play("tests/data/example.wav");
   thread_t *sound = sound_async_play("tests/data/example.wav");

   for (int i = 0; i < 10; i++)
     {
        printf("DOING SOME THINGS!\n");
        sleep(1);
     }

   thread_wait(sound);

   exit(EXIT_SUCCESS);
}

