#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

void audio_init(void);
void audio_shutdown(void);
void audio_play(int snd_id, int type);
void audio_stop(int type);
void audio_pause(void);
void audio_resume(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
