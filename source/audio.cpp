#include "audio.h"
#include "utils/logger.h"

#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3_ex.h>

#include <psp2/audioout.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <map>
#include <string>

#define MIX_RATE     44100
#define MIX_GRAIN    2048
#define MAX_VOICES   12
#define BGM_WIN      2048
#define SOFT_CLIP_THRESHOLD 0.92f

#define SOUND_DIR    "ux0:data/immortaldusk/sound"

// --- Engine State ---
static pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;
static bool gAudioReady = false;
static volatile int gQuit = 0;
static int gPort = -1;
static SceUID gThread = -1;

struct SfxSample {
    short *pcm;       // 16-bit PCM samples
    unsigned frames;  // Number of sample frames
    int channels;     // 1 (mono) or 2 (stereo)
    int rate;         // Native sample rate (e.g. 22050)
};

struct Voice {
    SfxSample *smp;   // NULL = free slot
    double pos;       // fractional frame position
    double step;      // pitch * rate / MIX_RATE
    float gl, gr;     // Volume gain
    bool loop;
    bool paused;
};

static std::map<int, SfxSample *> gSfxCache;
static Voice gVoices[MAX_VOICES];
static float gSfxVolume = 1.0f;

// --- BGM: Streamed decode from in-memory MP3 buffer ---
static unsigned char *gBgmMp3Buf = NULL;
static mp3dec_ex_t gBgmMp3;
static bool gBgmMp3Open = false;
static int gBgmChannels = 2;
static int gCurrentBgmId = -1;
static double gBgmStep = 1.0, gBgmReadPos = 0.0;
static short gBgmWin[BGM_WIN * 2];
static int gBgmAvail = 0;
static bool gBgmPlaying = false, gBgmPaused = false, gBgmLoop = true, gBgmEnded = false;
static float gBgmVolume = 1.0f;

// --- File I/O via sceIo ---
static unsigned char *read_entire_file(const char *path, int *out_len) {
    *out_len = 0;
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd < 0) {
        l_error("sceIoOpen failed for %s (0x%08X)", path, (unsigned)fd);
        return NULL;
    }
    SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);
    if (size <= 0 || size > 32 * 1024 * 1024) {
        l_error("Bad audio file size for %s: %lld", path, (long long)size);
        sceIoClose(fd);
        return NULL;
    }
    unsigned char *buf = (unsigned char *)malloc((size_t)size);
    if (!buf) {
        sceIoClose(fd);
        return NULL;
    }
    int off = 0;
    while (off < (int)size) {
        int n = sceIoRead(fd, buf + off, (SceSize)((int)size - off));
        if (n <= 0) {
            l_error("sceIoRead failed for %s at %d (0x%08X)", path, off, (unsigned)n);
            free(buf);
            sceIoClose(fd);
            return NULL;
        }
        off += n;
    }
    sceIoClose(fd);
    *out_len = (int)size;
    return buf;
}

// Parse WAV in-memory
static SfxSample *load_wav_memory(const unsigned char *data, int data_len) {
    if (data_len < 44) return NULL;
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) return NULL;

    int channels = *(uint16_t *)(data + 22);
    int rate = *(uint32_t *)(data + 24);
    int bits = *(uint16_t *)(data + 34);

    // Find "data" chunk
    const unsigned char *ptr = data + 12;
    const unsigned char *end = data + data_len;
    uint32_t pcm_size = 0;
    const unsigned char *pcm_data = NULL;

    while (ptr + 8 <= end) {
        uint32_t chunk_len = *(uint32_t *)(ptr + 4);
        if (memcmp(ptr, "data", 4) == 0) {
            pcm_data = ptr + 8;
            pcm_size = chunk_len;
            if (pcm_data + pcm_size > end) {
                pcm_size = (uint32_t)(end - pcm_data);
            }
            break;
        }
        ptr += 8 + chunk_len;
    }

    if (!pcm_data || bits != 16 || pcm_size == 0) return NULL;

    SfxSample *s = new SfxSample();
    s->channels = channels;
    s->rate = rate;
    s->frames = pcm_size / (channels * sizeof(short));
    s->pcm = (short *)malloc(pcm_size);
    memcpy(s->pcm, pcm_data, pcm_size);
    return s;
}

// Soft-knee limiter
static inline short soft_clip16(int v) {
    const float full = 32768.0f;
    float x = (float) v / full;
    float ax = fabsf(x);
    if (ax <= SOFT_CLIP_THRESHOLD) {
        if (v > 32767) return 32767;
        if (v < -32768) return -32768;
        return (short) v;
    }
    float sign = (x < 0.0f) ? -1.0f : 1.0f;
    float over = (ax - SOFT_CLIP_THRESHOLD) / (1.0f - SOFT_CLIP_THRESHOLD);
    float compressed = SOFT_CLIP_THRESHOLD + (1.0f - SOFT_CLIP_THRESHOLD) * tanhf(over);
    float outv = sign * compressed * full;
    if (outv > 32767.0f) outv = 32767.0f;
    if (outv < -32768.0f) outv = -32768.0f;
    return (short) outv;
}

static int bgm_refill_from_decoder(int frames_wanted) {
    int space = BGM_WIN - gBgmAvail;
    if (frames_wanted > space) frames_wanted = space;
    if (frames_wanted <= 0) return 0;

    if (gBgmChannels == 2) {
        size_t got = mp3dec_ex_read(&gBgmMp3, gBgmWin + gBgmAvail * 2, (size_t) frames_wanted * 2);
        int gotFrames = (int)(got / 2);
        gBgmAvail += gotFrames;
        return gotFrames;
    }

    static short mono[BGM_WIN];
    size_t got = mp3dec_ex_read(&gBgmMp3, mono, (size_t) frames_wanted);
    for (size_t i = 0; i < got; i++) {
        gBgmWin[(gBgmAvail + (int) i) * 2 + 0] = mono[i];
        gBgmWin[(gBgmAvail + (int) i) * 2 + 1] = mono[i];
    }
    gBgmAvail += (int) got;
    return (int) got;
}

extern "C" void game_log(const char *fmt, ...);

static bool bgm_ensure_window(void) {
    int stall_count = 0;
    for (;;) {
        int idx = (int)gBgmReadPos;
        if (idx + 1 < gBgmAvail)
            return true;

        int keep = (gBgmAvail > 0) ? 1 : 0;
        if (keep && idx < gBgmAvail) {
            gBgmWin[0] = gBgmWin[(gBgmAvail - 1) * 2];
            gBgmWin[1] = gBgmWin[(gBgmAvail - 1) * 2 + 1];
        }
        gBgmReadPos -= (gBgmAvail > keep) ? (double)(gBgmAvail - keep) : 0.0;
        if (gBgmReadPos < 0.0) gBgmReadPos = 0.0;
        gBgmAvail = keep;

        int got = bgm_refill_from_decoder(BGM_WIN - gBgmAvail);
        if (got > 0) {
            stall_count = 0;
            continue;
        }

        if (gBgmLoop) {
            mp3dec_ex_seek(&gBgmMp3, 0);
            stall_count++;
            if (stall_count > 8) {
                game_log("DIAG: bgm_ensure_window stalled after loop-seek, giving up (bgm id=%d)\n",
                         gCurrentBgmId);
                gBgmEnded = true;
                return false;
            }
            continue;
        }
        return false;
    }
}

static void mix_bgm(int *acc, int frames) {
    if (!gBgmMp3Open || !gBgmPlaying || gBgmPaused || gBgmEnded)
        return;
    for (int i = 0; i < frames; i++) {
        if (!bgm_ensure_window()) {
            gBgmEnded = true;
            return;
        }
        int idx = (int)gBgmReadPos;
        float frac = (float)(gBgmReadPos - idx);
        const short *a = &gBgmWin[idx * 2];
        const short *b = &gBgmWin[(idx + 1) * 2];
        float l = a[0] + (b[0] - a[0]) * frac;
        float r = a[1] + (b[1] - a[1]) * frac;
        acc[i * 2]     += (int)(l * gBgmVolume);
        acc[i * 2 + 1] += (int)(r * gBgmVolume);
        gBgmReadPos += gBgmStep;
    }
}

static void mix_voice(Voice *v, int *acc, int frames) {
    SfxSample *s = v->smp;
    for (int i = 0; i < frames; i++) {
        unsigned idx = (unsigned)v->pos;
        if (idx + 1 >= s->frames) {
            if (v->loop && s->frames > 1) {
                v->pos -= (double)(s->frames - 1);
                if (v->pos < 0.0) v->pos = 0.0;
                idx = (unsigned)v->pos;
            } else {
                v->smp = NULL;
                return;
            }
        }
        float frac = (float)(v->pos - idx);
        float l, r;
        if (s->channels == 2) {
            const short *a = &s->pcm[idx * 2];
            const short *b = &s->pcm[(idx + 1) * 2];
            l = a[0] + (b[0] - a[0]) * frac;
            r = a[1] + (b[1] - a[1]) * frac;
        } else {
            float m = s->pcm[idx] + (s->pcm[idx + 1] - s->pcm[idx]) * frac;
            l = r = m;
        }
        acc[i * 2]     += (int)(l * v->gl);
        acc[i * 2 + 1] += (int)(r * v->gr);
        v->pos += v->step;
    }
}

static int mixer_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    static int mixAcc[MIX_GRAIN * 2];
    static short outBuf[MIX_GRAIN * 2];

    while (!gQuit) {
        memset(mixAcc, 0, sizeof(mixAcc));

        pthread_mutex_lock(&gLock);

        mix_bgm(mixAcc, MIX_GRAIN);

        for (int i = 0; i < MAX_VOICES; i++) {
            Voice *v = &gVoices[i];
            if (v->smp && !v->paused) {
                mix_voice(v, mixAcc, MIX_GRAIN);
            }
        }

        pthread_mutex_unlock(&gLock);

        for (int i = 0; i < MIX_GRAIN * 2; i++) {
            outBuf[i] = soft_clip16(mixAcc[i]);
        }

        sceAudioOutOutput(gPort, outBuf);
    }
    return 0;
}

extern "C" void audio_init(void) {
    if (gAudioReady) return;

    gPort = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, MIX_GRAIN, MIX_RATE, SCE_AUDIO_OUT_MODE_STEREO);
    if (gPort < 0) {
        l_error("audio_init: sceAudioOutOpenPort failed (0x%08X)", (unsigned)gPort);
        return;
    }

    int vol[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(gPort, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol);

    gQuit = 0;
    gThread = sceKernelCreateThread("immortal_audio_mixer", mixer_thread, 0x10000100, 64 * 1024, 0, 0, NULL);
    if (gThread < 0) {
        l_error("audio_init: sceKernelCreateThread failed (0x%08X)", (unsigned)gThread);
        sceAudioOutReleasePort(gPort);
        gPort = -1;
        return;
    }
    sceKernelStartThread(gThread, 0, NULL);
    gAudioReady = true;
    l_info("Native audio mixer started (44.1kHz stereo, %d grain, port %d).", MIX_GRAIN, gPort);
}

extern "C" void audio_play(int snd_id, int type) {
    if (!gAudioReady) return;

    if (type == 0) {
        // SFX: snd0.wav .. snd61.wav
        pthread_mutex_lock(&gLock);

        SfxSample *smp = NULL;
        auto it = gSfxCache.find(snd_id);
        if (it != gSfxCache.end()) {
            smp = it->second;
        } else {
            char path[256];
            snprintf(path, sizeof(path), "%s/snd%d.wav", SOUND_DIR, snd_id);
            int len = 0;
            unsigned char *data = read_entire_file(path, &len);
            if (!data) {
                snprintf(path, sizeof(path), "%s/snd%d.mp3", SOUND_DIR, snd_id);
                data = read_entire_file(path, &len);
                if (data) {
                    mp3dec_t mp3d;
                    mp3dec_init(&mp3d);
                    mp3dec_file_info_t info;
                    if (mp3dec_load_buf(&mp3d, data, (size_t)len, &info, NULL, NULL) == 0) {
                        smp = new SfxSample();
                        smp->channels = info.channels;
                        smp->rate = info.hz;
                        smp->frames = (unsigned)info.samples / info.channels;
                        smp->pcm = info.buffer;
                    }
                    free(data);
                }
            } else {
                smp = load_wav_memory(data, len);
                free(data);
            }

            if (smp) {
                gSfxCache[snd_id] = smp;
            }
        }

        if (smp) {
            int slot = -1;
            for (int i = 0; i < MAX_VOICES; i++) {
                if (gVoices[i].smp == NULL) {
                    slot = i;
                    break;
                }
            }
            if (slot < 0) slot = 0;

            Voice *v = &gVoices[slot];
            v->smp = smp;
            v->pos = 0.0;
            v->step = (double)smp->rate / (double)MIX_RATE;
            v->gl = gSfxVolume;
            v->gr = gSfxVolume;
            v->loop = false;
            v->paused = false;
            l_info("SFX playing: snd%d (rate: %d, channels: %d)", snd_id, smp->rate, smp->channels);
        } else {
            l_warn("audio_play: Failed to play SFX %d", snd_id);
        }

        pthread_mutex_unlock(&gLock);
    } else {
        // BGM: snd100.mp3 .. snd122.mp3 (with .wav fallback for snd120.wav)
        pthread_mutex_lock(&gLock);

        if (gBgmMp3Open && gCurrentBgmId == snd_id) {
            gBgmPaused = false;
            gBgmPlaying = true;
            pthread_mutex_unlock(&gLock);
            return;
        }

        if (gBgmMp3Open) {
            mp3dec_ex_close(&gBgmMp3);
            gBgmMp3Open = false;
        }
        if (gBgmMp3Buf) {
            free(gBgmMp3Buf);
            gBgmMp3Buf = NULL;
        }

        char path[256];
        snprintf(path, sizeof(path), "%s/snd%d.mp3", SOUND_DIR, snd_id);
        int len = 0;
        unsigned char *data = read_entire_file(path, &len);
        if (!data) {
            snprintf(path, sizeof(path), "%s/snd%d.wav", SOUND_DIR, snd_id);
            data = read_entire_file(path, &len);
        }

        if (data) {
            gBgmMp3Buf = data;
            if (mp3dec_ex_open_buf(&gBgmMp3, gBgmMp3Buf, (size_t)len, MP3D_SEEK_TO_SAMPLE) == 0) {
                gBgmMp3Open = true;
                gCurrentBgmId = snd_id;
                gBgmChannels = gBgmMp3.info.channels;
                gBgmStep = (double)gBgmMp3.info.hz / (double)MIX_RATE;
                gBgmReadPos = 0.0;
                gBgmAvail = 0;
                gBgmPlaying = true;
                gBgmPaused = false;
                gBgmLoop = true;
                gBgmEnded = false;
                l_info("BGM playing: %s (%d channels, %d Hz)", path, gBgmChannels, gBgmMp3.info.hz);
            } else {
                l_error("mp3dec_ex_open_buf failed for %s", path);
                free(gBgmMp3Buf);
                gBgmMp3Buf = NULL;
            }
        } else {
            l_warn("BGM file not found for id %d", snd_id);
        }

        pthread_mutex_unlock(&gLock);
    }
}

extern "C" void game_log(const char *fmt, ...);
extern "C" void game_log_flush(void);

extern "C" void audio_stop(int type) {
    game_log("DIAG: audio_stop enter type=%d gAudioReady=%d gBgmMp3Open=%d gBgmEnded=%d\n",
              type, (int)gAudioReady, (int)gBgmMp3Open, (int)gBgmEnded);
    game_log_flush();
    if (!gAudioReady) return;
    game_log("DIAG: audio_stop acquiring lock\n");
    game_log_flush();
    pthread_mutex_lock(&gLock);
    game_log("DIAG: audio_stop lock acquired\n");
    game_log_flush();
    if (type == 0) {
        for (int i = 0; i < MAX_VOICES; i++) {
            gVoices[i].smp = NULL;
        }
    } else if (type == 1) {
        if (gBgmMp3Open) {
            gBgmPlaying = false;
            gBgmPaused = true;
        }
    }
    pthread_mutex_unlock(&gLock);
    game_log("DIAG: audio_stop exit\n");
    game_log_flush();
}

extern "C" void audio_pause(void) {
    if (!gAudioReady) return;
    pthread_mutex_lock(&gLock);
    gBgmPaused = true;
    pthread_mutex_unlock(&gLock);
}

extern "C" void audio_resume(void) {
    if (!gAudioReady) return;
    pthread_mutex_lock(&gLock);
    gBgmPaused = false;
    pthread_mutex_unlock(&gLock);
}

extern "C" void audio_shutdown(void) {
    if (!gAudioReady) return;
    gQuit = 1;
    if (gThread >= 0) {
        sceKernelWaitThreadEnd(gThread, NULL, NULL);
        sceKernelDeleteThread(gThread);
        gThread = -1;
    }

    pthread_mutex_lock(&gLock);
    if (gBgmMp3Open) {
        mp3dec_ex_close(&gBgmMp3);
        gBgmMp3Open = false;
    }
    if (gBgmMp3Buf) {
        free(gBgmMp3Buf);
        gBgmMp3Buf = NULL;
    }

    for (auto &pair : gSfxCache) {
        if (pair.second) {
            free(pair.second->pcm);
            delete pair.second;
        }
    }
    gSfxCache.clear();

    pthread_mutex_unlock(&gLock);

    if (gPort >= 0) {
        sceAudioOutReleasePort(gPort);
        gPort = -1;
    }
    gAudioReady = false;
}
