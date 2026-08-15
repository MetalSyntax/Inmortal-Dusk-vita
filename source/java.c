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

static void jni_noop_void(jmethodID id, va_list args) {
    // Stubs for policy_chk, netClose, review_popup, netSend, netBuy, ipnetSend, hide_new, display_new
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
    { METHOD_NET_CONN,      jni_noop_void },
    { METHOD_SELECTED_LANG, jni_selected_lang },
    { METHOD_POLICY_CHK,    jni_noop_void },
    { METHOD_NET_CLOSE,     jni_noop_void },
    { METHOD_REVIEW_POPUP,  jni_noop_void },
    { METHOD_NET_SEND,      jni_noop_void },
    { METHOD_NET_BUY,       jni_noop_void },
    { METHOD_IPNET_SEND,    jni_noop_void },
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
