#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI.h>
#include "utils/logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>

#define ASSETS_PATH "ux0:data/immortaldusk/assets/"

enum MethodIDs {
    METHOD_GET_RES = 1,
    METHOD_SND_PLAY,
    METHOD_SND_STOP,
    METHOD_VIB_START,
    METHOD_NET_CONN,
    METHOD_SELECTED_LANG,
    METHOD_SELECT_LANG,
    METHOD_POLICY_CHK,
    METHOD_NET_CLOSE,
    METHOD_REVIEW_POPUP,
    METHOD_NET_SEND,
    METHOD_NET_BUY,
    METHOD_GET_NUMBER,
    METHOD_GET_MODEL,
    METHOD_IPNET_SEND,
    METHOD_HIDE_NEW,
    METHOD_DISPLAY_NEW,
    METHOD_CHK_IMEMORY,
};

/* JNI Methods Mapping */
NameToMethodID nameToMethodId[] = {
    { METHOD_GET_RES,       "getRes",        METHOD_TYPE_OBJECT },
    { METHOD_SND_PLAY,      "sndPlay",       METHOD_TYPE_VOID },
    { METHOD_SND_STOP,      "sndStop",       METHOD_TYPE_VOID },
    { METHOD_VIB_START,     "vibStart",      METHOD_TYPE_VOID },
    { METHOD_NET_CONN,      "netConn",       METHOD_TYPE_VOID },
    { METHOD_SELECTED_LANG, "selected_lang", METHOD_TYPE_VOID },
    { METHOD_SELECT_LANG,   "select_lang",   METHOD_TYPE_INT },
    { METHOD_POLICY_CHK,    "policy_chk",    METHOD_TYPE_VOID },
    { METHOD_NET_CLOSE,     "netClose",      METHOD_TYPE_VOID },
    { METHOD_REVIEW_POPUP,  "review_popup",  METHOD_TYPE_VOID },
    { METHOD_NET_SEND,      "netSend",       METHOD_TYPE_VOID },
    { METHOD_NET_BUY,       "netBuy",        METHOD_TYPE_VOID },
    { METHOD_GET_NUMBER,    "getNumber",     METHOD_TYPE_OBJECT },
    { METHOD_GET_MODEL,     "getModel",      METHOD_TYPE_OBJECT },
    { METHOD_IPNET_SEND,    "ipnetSend",     METHOD_TYPE_VOID },
    { METHOD_HIDE_NEW,      "hide_new",      METHOD_TYPE_VOID },
    { METHOD_DISPLAY_NEW,   "display_new",   METHOD_TYPE_VOID },
    { METHOD_CHK_IMEMORY,   "chkImemory",    METHOD_TYPE_INT },
};

/* Methods Implementation */

static jobject jni_getRes(jmethodID id, va_list args) {
    JNIEnv *env = &jni;
    jbyteArray name_arr = va_arg(args, jbyteArray);
    if (!name_arr) {
        l_error("jni_getRes: NULL name array");
        return NULL;
    }

    jsize name_len = (*env)->GetArrayLength(env, name_arr);
    char *name = (char *)malloc(name_len + 1);
    (*env)->GetByteArrayRegion(env, name_arr, 0, name_len, (jbyte *)name);
    name[name_len] = '\0';

    const char *sanitized_name = name;
    if (sanitized_name[0] == '/')
        sanitized_name++;

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", ASSETS_PATH, sanitized_name);

    SceUID fd = sceIoOpen(full_path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        snprintf(full_path, sizeof(full_path), "ux0:data/immortaldusk/%s", sanitized_name);
        fd = sceIoOpen(full_path, SCE_O_RDONLY, 0777);
    }

    if (fd < 0) {
        l_warn("jni_getRes: File not found: %s (requested: %s)", full_path, name);
        free(name);
        return NULL;
    }

    SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);

    jbyteArray ret = (*env)->NewByteArray(env, (jsize)size);
    if (ret && size > 0) {
        JavaDynArray *jda = jda_find(ret);
        if (jda && jda->array) {
            sceIoRead(fd, jda->array, size);
        }
    }
    sceIoClose(fd);
    free(name);
    return ret;
}

static jobject jni_getNumber(jmethodID id, va_list args) {
    JNIEnv *env = &jni;
    const char *dummy_num = "01012345678";
    jsize len = (jsize)strlen(dummy_num);
    jbyteArray ret = (*env)->NewByteArray(env, len);
    if (ret) {
        (*env)->SetByteArrayRegion(env, ret, 0, len, (const jbyte *)dummy_num);
    }
    return ret;
}

static jobject jni_getModel(jmethodID id, va_list args) {
    JNIEnv *env = &jni;
    const char *model_str = "PlayStation Vita";
    jsize len = (jsize)strlen(model_str);
    jbyteArray ret = (*env)->NewByteArray(env, len);
    if (ret) {
        (*env)->SetByteArrayRegion(env, ret, 0, len, (const jbyte *)model_str);
    }
    return ret;
}

static jint jni_select_lang(jmethodID id, va_list args) {
    // 0 = Korean, 1 = English
    return 1;
}

static jint jni_chkImemory(jmethodID id, va_list args) {
    return 0x7FFFFFFF;
}

#include "audio.h"

static void jni_sndPlay(jmethodID id, va_list args) {
    int snd = va_arg(args, int);
    int type = va_arg(args, int);
    l_debug("sndPlay: snd=%d, type=%d", snd, type);
    audio_play(snd, type);
}

extern void game_log(const char *fmt, ...);
extern void game_log_flush(void);

static void jni_sndStop(jmethodID id, va_list args) {
    int type = va_arg(args, int);
    game_log("DIAG: jni_sndStop enter type=%d\n", type);
    game_log_flush();
    audio_stop(type);
    game_log("DIAG: jni_sndStop exit type=%d\n", type);
    game_log_flush();
}

static void jni_vibStart(jmethodID id, va_list args) {
    int val = va_arg(args, int);
    l_debug("vibStart: %d", val);
    SceCtrlActuator act;
    act.small = (val > 0) ? 100 : 0;
    act.large = (val > 0) ? 100 : 0;
    sceCtrlSetActuator(0, &act);
}

static void jni_selected_lang(jmethodID id, va_list args) {
    int kind = va_arg(args, int);
    l_info("selected_lang: %d", kind);
}

extern void (*Java_com_gamevil_ast_global_SkeletonLauncher_netState)(JNIEnv *env, jobject obj, jint nState, jint err);
extern void (*Java_com_gamevil_ast_global_SkeletonLauncher_netRecv)(JNIEnv *env, jobject obj, jbyteArray buf);

static void bridge_netState(int nState, int err) {
    if (Java_com_gamevil_ast_global_SkeletonLauncher_netState) {
        game_log("DIAG: bridge_netState(state=%d, err=%d)\n", nState, err);
        game_log_flush();
        Java_com_gamevil_ast_global_SkeletonLauncher_netState(&jni, (jobject)0x1000, nState, err);
    }
}

static void bridge_netRecv(const void *data, int len) {
    if (Java_com_gamevil_ast_global_SkeletonLauncher_netRecv) {
        JNIEnv *env = &jni;
        jbyteArray jret = (*env)->NewByteArray(env, (jsize)len);
        if (jret) {
            JavaDynArray *jda = jda_find(jret);
            if (jda && jda->array) {
                memcpy(jda->array, data, len);
            }
            game_log("DIAG: bridge_netRecv(len=%d)\n", len);
            game_log_flush();
            Java_com_gamevil_ast_global_SkeletonLauncher_netRecv(env, (jobject)0x1000, jret);
        }
    }
}

static void jni_netConn(jmethodID id, va_list args) {
    int type = va_arg(args, int);
    l_info("jni_netConn: type=%d", type);
    game_log("DIAG: jni_netConn enter type=%d\n", type);
    game_log_flush();
    // Signal connection established (state=1, err=0) so engine proceeds with its packet flow
    bridge_netState(1, 0);
}

static void jni_netClose(jmethodID id, va_list args) {
    l_info("jni_netClose called");
    game_log("DIAG: jni_netClose called\n");
    game_log_flush();
}

static void jni_netSend(jmethodID id, va_list args) {
    JNIEnv *env = &jni;
    jbyteArray buf_arr = va_arg(args, jbyteArray);
    int len = va_arg(args, int);

    if (!buf_arr || len <= 0) {
        l_warn("jni_netSend: empty buffer");
        bridge_netState(0, 100);
        return;
    }

    uint8_t buf[1500];
    if (len > (int)sizeof(buf)) len = sizeof(buf);

    JavaDynArray *jda = jda_find(buf_arr);
    if (jda && jda->array) {
        memcpy(buf, jda->array, len);
    } else {
        (*env)->GetByteArrayRegion(env, buf_arr, 0, len, (jbyte *)buf);
    }

    short cmd = 0;
    if (len >= 4) {
        cmd = (short)((buf[2] << 8) | (buf[3] & 0xff));
    }
    if (cmd == 0 && len >= 2) {
        cmd = (short)((buf[0] << 8) | (buf[1] & 0xff));
    }

    l_info("jni_netSend: len=%d, cmd=%d (0x%04X)", len, cmd, (uint16_t)cmd);
    game_log("DIAG: jni_netSend len=%d, cmd=%d (0x%04X)\n", len, cmd, (uint16_t)cmd);
    game_log_flush();

    if (cmd == 1000) {
        // PK_LOGIN / HUD Shop trigger (log_in_kind = 6 or 7)
        // Calling netState(5, 0) resets connect_chk to 0 without disconnecting when log_in_kind <= 9,
        // which leaves the Shop screen (gameflag = 5, now_layer = 0xd) open and interactive!
        l_info("jni_netSend: handling cmd 1000 (Shop / Login) -> netState(5, 0)");
        bridge_netState(5, 0);
    }
    else if (cmd == 0x36b4) {
        // PK_BUY_C2S: Cash Item Purchase with K currency
        l_info("jni_netSend: handling cmd 0x36b4 (Cash Item Purchase) -> sending 0x36b5");
        uint8_t pkt[8];
        pkt[0] = 0x00; pkt[1] = 0x08; // total length
        pkt[2] = 0x36; pkt[3] = 0xb5; // PK_BUY_S2C (14005)
        // Return 999,999 K points balance
        pkt[4] = 0x00; pkt[5] = 0x0f; pkt[6] = 0x42; pkt[7] = 0x3f;

        bridge_netRecv(pkt, sizeof(pkt));
        bridge_netState(3, 0);
    }
    else if (cmd == 0x36b2 || cmd == 14002) {
        // PK_K_BUY_C2S: Recharge K points
        l_info("jni_netSend: handling cmd 0x36b2 (K Points Recharge) -> sending 0x36b3");
        uint8_t pkt[112];
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = 0x00; pkt[1] = 0x70; // 112 bytes
        pkt[2] = 0x36; pkt[3] = 0xb3; // PK_K_BUY_S2C (14003)
        pkt[4] = 0; pkt[5] = 0; pkt[6] = 0; pkt[7] = 0; // result = 0 (success)
        snprintf((char *)&pkt[8], 100, "Recharge successful! Added 999,999 K.");
        pkt[108] = 0x00; pkt[109] = 0x0f; pkt[110] = 0x42; pkt[111] = 0x3f; // 999,999 K

        bridge_netRecv(pkt, sizeof(pkt));
        bridge_netState(3, 0);
    }
    else if (cmd == 14000) {
        // PK_K_POINT_INFO (0x36b0): Cash Shop Info request
        l_info("jni_netSend: handling cmd 14000 -> sending 14001 (0x36b1)");
        uint8_t pkt[310];
        memset(pkt, 0, sizeof(pkt));
        pkt[0] = 0x01; pkt[1] = 0x36; // 310 bytes
        pkt[2] = 0x36; pkt[3] = 0xb1; // 0x36b1
        pkt[4] = 1; // k_pointer_info_chk = 1
        snprintf((char *)&pkt[5], 300, "Welcome to the Immortal Dusk Shop!");
        pkt[305] = 0; // k_pointer_cnt = 0

        bridge_netRecv(pkt, sizeof(pkt));
        bridge_netState(3, 0);
    }
    else {
        // For unhandled offline features (ranking, cloud save, net search), safely disconnect
        // so connect_chk is reset and the user is NEVER stuck in an infinite loop!
        l_warn("jni_netSend: unhandled cmd 0x%04X, safely disconnecting", (uint16_t)cmd);
        bridge_netState(0, 100);
    }
}

static void jni_netBuy(jmethodID id, va_list args) {
    JNIEnv *env = &jni;
    jbyteArray buf_arr = va_arg(args, jbyteArray);
    int idx = va_arg(args, int);
    int len = va_arg(args, int);
    int type = va_arg(args, int);
    l_info("jni_netBuy: idx=%d, len=%d, type=%d -> granting 999,999 K points", idx, len, type);
    game_log("DIAG: jni_netBuy idx=%d, len=%d, type=%d\n", idx, len, type);
    game_log_flush();

    // Grant 999,999 K points instantly
    uint8_t pkt[112];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = 0x00; pkt[1] = 0x70; // 112 bytes
    pkt[2] = 0x36; pkt[3] = 0xb3; // PK_K_BUY_S2C (14003)
    pkt[4] = 0; pkt[5] = 0; pkt[6] = 0; pkt[7] = 0; // result = 0 (success)
    snprintf((char *)&pkt[8], 100, "Purchase Successful! Added 999,999 K.");
    pkt[108] = 0x00; pkt[109] = 0x0f; pkt[110] = 0x42; pkt[111] = 0x3f; // 999,999 K

    bridge_netRecv(pkt, sizeof(pkt));
    bridge_netState(3, 0);
}

static void jni_ipnetSend(jmethodID id, va_list args) {
    l_info("jni_ipnetSend called");
    game_log("DIAG: jni_ipnetSend called\n");
    game_log_flush();
    bridge_netState(0, 100);
}

static void jni_noop_void(jmethodID id, va_list args) {
    // Stubs for policy_chk, review_popup, hide_new, display_new
}

/* Dispatch Tables */

MethodsBoolean methodsBoolean[] = {};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {};

MethodsInt methodsInt[] = {
    { METHOD_SELECT_LANG, jni_select_lang },
    { METHOD_CHK_IMEMORY, jni_chkImemory },
};

MethodsLong methodsLong[] = {};

MethodsObject methodsObject[] = {
    { METHOD_GET_RES,    jni_getRes },
    { METHOD_GET_NUMBER, jni_getNumber },
    { METHOD_GET_MODEL,  jni_getModel },
};

MethodsShort methodsShort[] = {};

MethodsVoid methodsVoid[] = {
    { METHOD_SND_PLAY,      jni_sndPlay },
    { METHOD_SND_STOP,      jni_sndStop },
    { METHOD_VIB_START,     jni_vibStart },
    { METHOD_NET_CONN,      jni_netConn },
    { METHOD_SELECTED_LANG, jni_selected_lang },
    { METHOD_POLICY_CHK,    jni_noop_void },
    { METHOD_NET_CLOSE,     jni_netClose },
    { METHOD_REVIEW_POPUP,  jni_noop_void },
    { METHOD_NET_SEND,      jni_netSend },
    { METHOD_NET_BUY,       jni_netBuy },
    { METHOD_IPNET_SEND,    jni_ipnetSend },
    { METHOD_HIDE_NEW,      jni_noop_void },
    { METHOD_DISPLAY_NEW,   jni_noop_void },
};

/* JNI Fields */
char WINDOW_SERVICE[] = "window";
const int SDK_INT = 19;

NameToFieldID nameToFieldId[] = {
    { 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
    { 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
    { 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
    { 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
