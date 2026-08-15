#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/power.h>
#include <psp2/io/stat.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>

#define SCREEN_W 800
#define SCREEN_H 480
#define VITA_W   960
#define VITA_H   544

int _newlib_heap_size_user = 256 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

#include <psp2/io/fcntl.h>

#define LOG_BUF_SIZE (64 * 1024)
static char log_mem_buffer[LOG_BUF_SIZE];
static size_t log_mem_pos = 0;
static uint64_t last_log_flush_time = 0;
static char current_log_path[256] = "";

static void flush_log_buffer(void) {
    if (log_mem_pos == 0) return;
    if (current_log_path[0] == '\0') {
        sceIoMkdir("ux0:data/immortaldusk", 0777);
        sceIoMkdir("ux0:data/immortaldusk/logs", 0777);
        for (int i = 1; i <= 999; i++) {
            char path[256];
            snprintf(path, sizeof(path), "ux0:data/immortaldusk/logs/game_%03d.log", i);
            SceIoStat stat;
            if (sceIoGetstat(path, &stat) < 0) {
                strncpy(current_log_path, path, sizeof(current_log_path) - 1);
                break;
            }
        }
        if (current_log_path[0] == '\0') {
            snprintf(current_log_path, sizeof(current_log_path), "ux0:data/immortaldusk/logs/game_001.log");
        }
    }

    SceUID fd = sceIoOpen(current_log_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, log_mem_buffer, log_mem_pos);
        sceIoClose(fd);
    }
    log_mem_pos = 0;
    last_log_flush_time = sceKernelGetProcessTimeWide();
}

void game_log(const char *fmt, ...) {
    char buf[1024];
    va_list list;
    va_start(list, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);
    sceClibPrintf("%s", buf);

    if (len > 0) {
        if (log_mem_pos + len >= LOG_BUF_SIZE - 1024) {
            flush_log_buffer();
        }
        if (log_mem_pos + len < LOG_BUF_SIZE) {
            memcpy(log_mem_buffer + log_mem_pos, buf, len);
            log_mem_pos += len;
        }
    }
}

void game_log_flush(void) {
    flush_log_buffer();
}

so_module so_mod;

/* Native JNI function pointers */
static void (*Java_com_gamevil_ast_global_SkeletonLauncher_initJni)(JNIEnv *env, jobject obj, jbyteArray path, jint telecom, jint game_type);
static void (*Java_com_gamevil_ast_global_SkeletonLauncher_startC)(JNIEnv *env, jobject obj, jint w, jint h);
static jint (*Java_com_gamevil_ast_global_SkeletonLauncher_runC)(JNIEnv *env, jobject obj, jshortArray sBuf);
static void (*Java_com_gamevil_ast_global_SkeletonLauncher_eventC)(JNIEnv *env, jobject obj, jint action, jint x, jint y);
static void (*Java_com_gamevil_ast_global_SkeletonLauncher_resumeC)(JNIEnv *env, jobject obj);
void (*Java_com_gamevil_ast_global_SkeletonLauncher_netState)(JNIEnv *env, jobject obj, jint nState, jint err) = NULL;
void (*Java_com_gamevil_ast_global_SkeletonLauncher_netRecv)(JNIEnv *env, jobject obj, jbyteArray buf) = NULL;

/* Controls Mapping Definitions */
#define DPAD_CENTER_X 110
#define DPAD_CENTER_Y 370
#define DPAD_RADIUS   60

#define BTN_ATTACK_X    700
#define BTN_ATTACK_Y    380

#define BTN_SKILL1_X    620
#define BTN_SKILL1_Y    420

#define BTN_SKILL2_X    620
#define BTN_SKILL2_Y    310

#define BTN_SKILL3_X    700
#define BTN_SKILL3_Y    260

#define BTN_SKILL4_X    770
#define BTN_SKILL4_Y    310

#define BTN_POTION_HP_X 420
#define BTN_POTION_HP_Y 450

#define BTN_POTION_MP_X 480
#define BTN_POTION_MP_Y 450

#define BTN_MENU_X      760
#define BTN_MENU_Y      35

typedef struct {
    uint32_t mask;
    int x;
    int y;
} ButtonMap;

static ButtonMap button_mappings[] = {
    { SCE_CTRL_CROSS,    BTN_ATTACK_X,    BTN_ATTACK_Y    },
    { SCE_CTRL_SQUARE,   BTN_SKILL1_X,    BTN_SKILL1_Y    },
    { SCE_CTRL_TRIANGLE, BTN_SKILL2_X,    BTN_SKILL2_Y    },
    { SCE_CTRL_CIRCLE,   BTN_SKILL3_X,    BTN_SKILL3_Y    },
    { SCE_CTRL_R1,       BTN_SKILL4_X,    BTN_SKILL4_Y    },
    { SCE_CTRL_L1,       BTN_POTION_HP_X, BTN_POTION_HP_Y },
    { SCE_CTRL_SELECT,   BTN_POTION_MP_X, BTN_POTION_MP_Y },
    { SCE_CTRL_START,    BTN_MENU_X,      BTN_MENU_Y      },
};
#define NUM_BUTTON_MAPPINGS (sizeof(button_mappings) / sizeof(button_mappings[0]))

/* Framebuffer Quad Renderer */
static GLuint fb_texture = 0;

static const GLfloat quad_vertices[] = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f, -1.0f
};

static const GLfloat quad_texcoords[] = {
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f
};

static void init_renderer(void) {
    glGenTextures(1, &fb_texture);
    glBindTexture(GL_TEXTURE_2D, fb_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glViewport(0, 0, VITA_W, VITA_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void render_screen(const void *pixels) {
    glViewport(0, 0, VITA_W, VITA_H);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fb_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, quad_vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, quad_texcoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_TEXTURE_2D);
}

/* Single pointer touch state coordinator */
static int current_pointer_down = 0;
static int current_pointer_x = 0;
static int current_pointer_y = 0;

static void send_touch_event(JNIEnv *env, jobject act, int action, int x, int y) {
    if (action == 1) { // DOWN
        if (current_pointer_down) {
            Java_com_gamevil_ast_global_SkeletonLauncher_eventC(env, act, 2, current_pointer_x, current_pointer_y);
        }
        current_pointer_down = 1;
        current_pointer_x = x;
        current_pointer_y = y;
        Java_com_gamevil_ast_global_SkeletonLauncher_eventC(env, act, 1, x, y);
    } else if (action == 3) { // MOVE
        if (!current_pointer_down) {
            current_pointer_down = 1;
            current_pointer_x = x;
            current_pointer_y = y;
            Java_com_gamevil_ast_global_SkeletonLauncher_eventC(env, act, 1, x, y);
        } else if (current_pointer_x != x || current_pointer_y != y) {
            current_pointer_x = x;
            current_pointer_y = y;
            Java_com_gamevil_ast_global_SkeletonLauncher_eventC(env, act, 3, x, y);
        }
    } else if (action == 2) { // UP / RELEASE
        if (current_pointer_down) {
            Java_com_gamevil_ast_global_SkeletonLauncher_eventC(env, act, 2, current_pointer_x, current_pointer_y);
            current_pointer_down = 0;
        }
    }
}

int main(void) {
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    sceIoMkdir("ux0:data/immortaldusk", 0777);
    sceIoMkdir("ux0:data/immortaldusk/saves", 0777);
    sceIoMkdir("ux0:data/immortaldusk/logs", 0777);

    // Ensure main save file AIST exists with valid structure (24000 bytes: 3 slots x 8000 bytes)
    const char *aist_path = "ux0:data/immortaldusk/saves/AIST";
    SceIoStat aist_stat;
    if (sceIoGetstat(aist_path, &aist_stat) < 0 || aist_stat.st_size < 24000) {
        l_info("Creating initial save data file AIST (24,000 bytes)...");
        SceUID fd = sceIoOpen(aist_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (fd >= 0) {
            char zero_buf[1000];
            memset(zero_buf, 0, sizeof(zero_buf));
            for (int i = 0; i < 24; i++) {
                sceIoWrite(fd, zero_buf, sizeof(zero_buf));
            }
            sceIoClose(fd);
            l_info("Save data file AIST created successfully.");
        }
    }

    l_info("Initializing soloader...");
    soloader_init_all();

    // Resolve native entrypoints
    Java_com_gamevil_ast_global_SkeletonLauncher_initJni = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_initJni");
    Java_com_gamevil_ast_global_SkeletonLauncher_startC = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_startC");
    Java_com_gamevil_ast_global_SkeletonLauncher_runC = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_runC");
    Java_com_gamevil_ast_global_SkeletonLauncher_eventC = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_eventC");
    Java_com_gamevil_ast_global_SkeletonLauncher_resumeC = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_resumeC");
    Java_com_gamevil_ast_global_SkeletonLauncher_netState = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_netState");
    Java_com_gamevil_ast_global_SkeletonLauncher_netRecv = (void *)so_symbol(&so_mod, "Java_com_gamevil_ast_global_SkeletonLauncher_netRecv");

    if (!Java_com_gamevil_ast_global_SkeletonLauncher_initJni || !Java_com_gamevil_ast_global_SkeletonLauncher_runC) {
        l_fatal("Failed to find native symbols in libaist-jni.so");
        return -1;
    }

    // Initialize Video & Audio
    l_info("Initializing OpenGL and Audio subsystems...");
    gl_init();
    init_renderer();
    audio_init();

    JNIEnv *env = &jni;
    jobject fake_activity = (jobject)0x1000;

    // Prepare save directory path for initJni
    const char *save_path = "ux0:data/immortaldusk/saves/";
    jsize save_path_len = (jsize)strlen(save_path);
    jbyteArray j_save_path = (*env)->NewByteArray(env, save_path_len);
    (*env)->SetByteArrayRegion(env, j_save_path, 0, save_path_len, (const jbyte *)save_path);

    l_info("Calling initJni...");
    Java_com_gamevil_ast_global_SkeletonLauncher_initJni(env, fake_activity, j_save_path, 0, 0);

    l_info("Calling startC (800x480)...");
    Java_com_gamevil_ast_global_SkeletonLauncher_startC(env, fake_activity, SCREEN_W, SCREEN_H);

    // Allocate Pixel Framebuffer for runC
    jshortArray sBuf = (*env)->NewShortArray(env, SCREEN_W * SCREEN_H);
    if (!sBuf) {
        l_fatal("Failed to allocate framebuffer short array");
        return -1;
    }

    SceTouchData touch;
    SceCtrlData pad;

    l_info("Entering main game loop...");
    last_log_flush_time = sceKernelGetProcessTimeWide();

    while (1) {
        uint64_t frame_start = sceKernelGetProcessTimeWide();

        // 1. Poll Inputs
        sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
        sceCtrlPeekBufferPositive(0, &pad, 1);

        int target_down = 0;
        int target_action = 0;
        int target_x = 0;
        int target_y = 0;

        // Front Touch takes highest priority
        if (touch.reportNum > 0) {
            int tx = (touch.report[0].x * SCREEN_W) / 1920;
            int ty = (touch.report[0].y * SCREEN_H) / 1088;
            if (tx < 0) tx = 0;
            if (tx >= SCREEN_W) tx = SCREEN_W - 1;
            if (ty < 0) ty = 0;
            if (ty >= SCREEN_H) ty = SCREEN_H - 1;

            target_down = 1;
            target_action = current_pointer_down ? 3 : 1;
            target_x = tx;
            target_y = ty;
        } else {
            // Physical Buttons
            int button_pressed = 0;
            for (size_t i = 0; i < NUM_BUTTON_MAPPINGS; i++) {
                if (pad.buttons & button_mappings[i].mask) {
                    target_down = 1;
                    target_action = current_pointer_down ? 3 : 1;
                    target_x = button_mappings[i].x;
                    target_y = button_mappings[i].y;
                    button_pressed = 1;
                    break;
                }
            }

            // Direction Joystick (Analog Stick / DPad)
            if (!button_pressed) {
                float dx = 0.0f;
                float dy = 0.0f;
                if (pad.buttons & SCE_CTRL_UP)    dy -= 1.0f;
                if (pad.buttons & SCE_CTRL_DOWN)  dy += 1.0f;
                if (pad.buttons & SCE_CTRL_LEFT)  dx -= 1.0f;
                if (pad.buttons & SCE_CTRL_RIGHT) dx += 1.0f;

                int sx = (int)pad.lx - 128;
                int sy = (int)pad.ly - 128;
                if (abs(sx) > 35 || abs(sy) > 35) {
                    dx = (float)sx / 128.0f;
                    dy = (float)sy / 128.0f;
                }

                float len = sqrtf(dx * dx + dy * dy);
                if (len > 0.1f) {
                    if (len > 1.0f) {
                        dx /= len;
                        dy /= len;
                    }
                    target_down = 1;
                    target_action = current_pointer_down ? 3 : 1;
                    target_x = DPAD_CENTER_X + (int)(dx * DPAD_RADIUS);
                    target_y = DPAD_CENTER_Y + (int)(dy * DPAD_RADIUS);
                }
            }
        }

        if (target_down) {
            send_touch_event(env, fake_activity, target_action, target_x, target_y);
        } else if (current_pointer_down) {
            send_touch_event(env, fake_activity, 2, current_pointer_x, current_pointer_y);
        }

        // 2. Execute Engine Frame
        int sleep_ms = Java_com_gamevil_ast_global_SkeletonLauncher_runC(env, fake_activity, sBuf);

        // 3. Upload and Render Frame
        jshort *pixels = (*env)->GetShortArrayElements(env, sBuf, NULL);
        if (pixels) {
            render_screen(pixels);
            (*env)->ReleaseShortArrayElements(env, sBuf, pixels, JNI_ABORT);
        }

        gl_swap();

        // 4. Periodic non-blocking log buffer flush (every 3 seconds)
        uint64_t now = sceKernelGetProcessTimeWide();
        if (now - last_log_flush_time > 3000000) {
            flush_log_buffer();
        }

        // 5. Frame limiter (smooth 60 FPS)
        uint64_t elapsed = sceKernelGetProcessTimeWide() - frame_start;
        uint32_t target_us = (sleep_ms > 0 && sleep_ms < 60) ? (uint32_t)sleep_ms * 1000 : 16666;
        if (elapsed < target_us) {
            sceKernelDelayThread(target_us - (uint32_t)elapsed);
        }
    }

    flush_log_buffer();
    audio_shutdown();
    sceKernelExitDeleteThread(0);
    return 0;
}
