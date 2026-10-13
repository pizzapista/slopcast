#ifndef DC_VITA_AUDIO_H
#define DC_VITA_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

/* Output is interleaved stereo int16 at 44100 Hz (the Dreamcast's native
 * AICA rate). We keep the SceAudioOut grain at 512 frames per channel. */
#define VITA_AUDIO_SAMPLE_RATE 44100
#define VITA_AUDIO_GRAIN       512

/* Fill callback: produced VITA_AUDIO_GRAIN stereo frames. `out` is
 * interleaved L,R,L,R,... and must be written in full. Runs on the audio
 * thread; keep it short. */
typedef void (*vita_audio_fill_cb)(int16_t *out, int frames, void *user);

bool vita_audio_init(void);
void vita_audio_shutdown(void);

/* Must be called before the first vita_audio_output() call. */
void vita_audio_set_fill(vita_audio_fill_cb cb, void *user);

/* Blocking pump: calls the fill callback once and submits the grain to
 * SceAudioOut. Returns false if the port is dead. Calling this once per
 * 11.6 ms of audio time keeps the queue fed. */
bool vita_audio_output(void);

/* True if audio was successfully opened at init time. */
bool vita_audio_available(void);

#endif /* DC_VITA_AUDIO_H */
