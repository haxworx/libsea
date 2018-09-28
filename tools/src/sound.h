#ifndef __SOUND_H__
#define __SOUND_H__

/**
 * @file
 * @brief Sound playback and recording.
 */

/**
 * @brief Sound playback and recording.
 * @defgroup Sound
 *
 * @{
 *
 * Playback of audio and recording.
 */
#include "thread.h"

/**
 * Play a sound file and wait for it to complete.
 *
 * @param filename The location of the file to play.
 *
 * @return Returns -1 on failure and 0 on success.
 */
int      sound_play(const char *filename);

/**
 * Play a RIFF/WAVE file once, waiting for it to complete.
 *
 * @param filename The RIFF/WAVE file to play.
 *
 * @return Returns -1 on failure or zero on success.
 */
int      sound_wave_file_play(const char *filename);

/**
 * Play a sound file in the background.
 *
 * @param filename The location of the file to play.
 *
 * @return A pointer to the thread associated with the playback.
 */
thread_t *sound_async_play(const char *filename);

/**
 * @}
 */
#endif
