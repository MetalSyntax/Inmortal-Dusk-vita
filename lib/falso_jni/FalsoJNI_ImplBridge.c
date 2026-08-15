/*
 * FalsoJNI_ImplBridge.c
 *
 * Fake Java Native Interface, providing JavaVM and JNIEnv objects.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2021 Rinnegatamante
 * Copyright (C) 2022 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "FalsoJNI_Impl.h"
#include "FalsoJNI_Logger.h"

#include "FalsoJNI_ImplBridge.h"

#include <string.h>
#include <malloc.h>
#include <pthread.h>

JavaDynArray** javaDynArrays = NULL;
pthread_mutex_t* javaDynArrays_mutex = NULL;
size_t javaDynArrays_capacity = 0;
size_t javaDynArrays_count = 0;

jfieldID getFieldIdByName(const char* name) {
    for (int i = 0; i < nameToFieldId_size() / sizeof(NameToFieldID); i++) {
        if (strcmp(name, nameToFieldId[i].name) == 0) {
            return (jfieldID) nameToFieldId[i].id;
        }
    }

    fjni_logv_warn("Unknown field name \"%s\"", name);
    return NULL;
}

const char* fieldTypeToStr(FIELD_TYPE t) {
    switch (t) {
        case FIELD_TYPE_INT:
            return "FIELD_TYPE_INT";
        case FIELD_TYPE_OBJECT:
            return "FIELD_TYPE_OBJECT";
        case FIELD_TYPE_BOOLEAN:
            return "FIELD_TYPE_BOOLEAN";
        case FIELD_TYPE_BYTE:
            return "FIELD_TYPE_BYTE";
        case FIELD_TYPE_CHAR:
            return "FIELD_TYPE_CHAR";
        case FIELD_TYPE_SHORT:
            return "FIELD_TYPE_SHORT";
        case FIELD_TYPE_LONG:
            return "FIELD_TYPE_LONG";
        case FIELD_TYPE_FLOAT:
            return "FIELD_TYPE_FLOAT";
        case FIELD_TYPE_DOUBLE:
            return "FIELD_TYPE_DOUBLE";
        default:
            return "FIELD_TYPE_UNKNOWN";
    }
}

jsize getFieldTypeSize(FIELD_TYPE fieldType) {
    switch (fieldType) {
        case FIELD_TYPE_OBJECT:
            return sizeof(jobject);
        case FIELD_TYPE_BOOLEAN:
            return sizeof(jboolean);
        case FIELD_TYPE_BYTE:
            return sizeof(jbyte);
        case FIELD_TYPE_CHAR:
            return sizeof(jchar);
        case FIELD_TYPE_SHORT:
            return sizeof(jshort);
        case FIELD_TYPE_INT:
            return sizeof(jint);
        case FIELD_TYPE_LONG:
            return sizeof(jlong);
        case FIELD_TYPE_FLOAT:
            return sizeof(jfloat);
        case FIELD_TYPE_DOUBLE:
            return sizeof(jdouble);
        default:
            return sizeof(void *);
    }
}

jobject getObjectFieldValueById(jfieldID id) {
    getFieldValueById(jobject, FIELD_TYPE_OBJECT, FieldsObject, fieldsObject, fieldsObject_size, id, (jobject)0x42424242);
}

jint getIntFieldValueById(jfieldID id) {
    getFieldValueById(jint, FIELD_TYPE_INT, FieldsInt, fieldsInt, fieldsInt_size, id, 1);
}

jboolean getBooleanFieldValueById(jfieldID id) {
    getFieldValueById(jboolean, FIELD_TYPE_BOOLEAN, FieldsBoolean, fieldsBoolean, fieldsBoolean_size, id, JNI_FALSE);
}

jbyte getByteFieldValueById(jfieldID id) {
    getFieldValueById(jbyte, FIELD_TYPE_BYTE, FieldsByte, fieldsByte, fieldsByte_size, id, 'a');
}

jchar getCharFieldValueById(jfieldID id) {
    getFieldValueById(jchar, FIELD_TYPE_CHAR, FieldsChar, fieldsChar, fieldsChar_size, id, 'b');
}

jshort getShortFieldValueById(jfieldID id) {
    getFieldValueById(jshort, FIELD_TYPE_SHORT, FieldsShort, fieldsShort, fieldsShort_size, id, 1);
}

jlong getLongFieldValueById(jfieldID id) {
    getFieldValueById(jlong, FIELD_TYPE_LONG, FieldsLong, fieldsLong, fieldsLong_size, id, 1);
}

jfloat getFloatFieldValueById(jfieldID id) {
    getFieldValueById(jfloat, FIELD_TYPE_FLOAT, FieldsFloat, fieldsFloat, fieldsFloat_size, id, 1.0f);
}

jdouble getDoubleFieldValueById(jfieldID id) {
    getFieldValueById(jdouble, FIELD_TYPE_DOUBLE, FieldsDouble, fieldsDouble, fieldsDouble_size, id, 1);
}

void setObjectFieldValueById(jfieldID id, jobject value) {
    setFieldValueById(jobject, FIELD_TYPE_OBJECT, FieldsObject, fieldsObject, fieldsObject_size, id, value);
}

void setIntFieldValueById(jfieldID id, jint value) {
    setFieldValueById(jint, FIELD_TYPE_INT, FieldsInt, fieldsInt, fieldsInt_size, id, value);
}

void setBooleanFieldValueById(jfieldID id, jboolean value) {
    setFieldValueById(jboolean, FIELD_TYPE_BOOLEAN, FieldsBoolean, fieldsBoolean, fieldsBoolean_size, id, value);
}

void setByteFieldValueById(jfieldID id, jbyte value) {
    setFieldValueById(jbyte, FIELD_TYPE_BYTE, FieldsByte, fieldsByte, fieldsByte_size, id, value);
}

void setCharFieldValueById(jfieldID id, jchar value) {
    setFieldValueById(jchar, FIELD_TYPE_CHAR, FieldsChar, fieldsChar, fieldsChar_size, id, value);
}

void setShortFieldValueById(jfieldID id, jshort value) {
    setFieldValueById(jshort, FIELD_TYPE_SHORT, FieldsShort, fieldsShort, fieldsShort_size, id, value);
}

void setLongFieldValueById(jfieldID id, jlong value) {
    setFieldValueById(jlong, FIELD_TYPE_LONG, FieldsLong, fieldsLong, fieldsLong_size, id, value);
}

void setFloatFieldValueById(jfieldID id, jfloat value) {
    setFieldValueById(jfloat, FIELD_TYPE_FLOAT, FieldsFloat, fieldsFloat, fieldsFloat_size, id, value);
}

void setDoubleFieldValueById(jfieldID id, jdouble value) {
    setFieldValueById(jdouble, FIELD_TYPE_DOUBLE, FieldsDouble, fieldsDouble, fieldsDouble_size, id, value);
}

jmethodID getMethodIdByName(const char* name) {
    for (int i = 0; i < nameToMethodId_size() / sizeof(NameToMethodID); i++) {
        if (strcmp(name, nameToMethodId[i].name) == 0) {
            return (jmethodID) nameToMethodId[i].id;
        }
    }
    return NULL;
}

jobject methodObjectCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsObject_size() / sizeof(MethodsObject); i++) {
        if (methodsObject[i].id == (int)id) {
            return methodsObject[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return NULL;
}

void methodVoidCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsVoid_size() / sizeof(MethodsVoid); i++) {
        if (methodsVoid[i].id == (int)id) {
            return methodsVoid[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
}

jboolean methodBooleanCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsBoolean_size() / sizeof(MethodsBoolean); i++) {
        if (methodsBoolean[i].id == (int)id) {
            return methodsBoolean[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return JNI_FALSE;
}

jbyte methodByteCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsByte_size() / sizeof(MethodsByte); i++) {
        if (methodsByte[i].id == (int)id) {
            return methodsByte[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return 0;
}

jshort methodShortCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsShort_size() / sizeof(MethodsShort); i++) {
        if (methodsShort[i].id == (int)id) {
            return methodsShort[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return 0;
}

jdouble methodDoubleCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsDouble_size() / sizeof(MethodsDouble); i++) {
        if (methodsDouble[i].id == (int)id) {
            return methodsDouble[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return 0;
}

jchar methodCharCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsChar_size() / sizeof(MethodsChar); i++) {
        if (methodsChar[i].id == (int)id) {
            return methodsChar[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return 0;
}

jlong methodLongCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsLong_size() / sizeof(MethodsLong); i++) {
        if (methodsLong[i].id == (int)id) {
            return methodsLong[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return -1;
}

jint methodIntCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsInt_size() / sizeof(MethodsInt); i++) {
        if (methodsInt[i].id == (int)id) {
            return methodsInt[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return -1;
}

jfloat methodFloatCall(jmethodID id, va_list args) {
    for (int i = 0; i < methodsFloat_size() / sizeof(MethodsFloat); i++) {
        if (methodsFloat[i].id == (int)id) {
            return methodsFloat[i].Method(id, args);
        }
    }

    fjni_logv_warn("method ID %i not found!", (int)id);
    return -1;
}

void jda_lock() {
    if (javaDynArrays_mutex == NULL) {
        pthread_mutex_t initTmpNormal;
        javaDynArrays_mutex = malloc(sizeof(pthread_mutex_t));
        memcpy(javaDynArrays_mutex, &initTmpNormal, sizeof(pthread_mutex_t));

        if (pthread_mutex_init(javaDynArrays_mutex, NULL) != 0) {
            fjni_log_err("Failed to allocate dynarr mutex");
            javaDynArrays_mutex = NULL;
            return;
        }
    }

    pthread_mutex_lock(javaDynArrays_mutex);
}

void jda_unlock() {
    if (javaDynArrays_mutex != NULL) {
        pthread_mutex_unlock(javaDynArrays_mutex);
    }
}

// [Zenonia3] Capacidad inicial subida de 16 a 1024: jda_alloc() devuelve
// punteros DENTRO de javaDynArrays[], y jda_extend() hace realloc() de esa
// tabla cuando se llena -- invalidando cualquier JavaDynArray* que un caller
// haya cacheado entre llamadas (java.c cachea los de GFA_GetPixels32/16 y
// los persistentes de DrawFont/DrawText/MeasureText). Con 1024 entradas y
// los jda de GFA reutilizados, la tabla no crece nunca en la practica; el
// realloc de jda_extend queda solo como fallback (y si llegara a dispararse,
// los punteros cacheados volverian a invalidarse -- sintoma: "Array 0x...
// not found" en el log, ver port_progress.md Fase 4.1).
#define JDA_INITIAL_CAPACITY 4096

jboolean jda_tryinit() {
    if (javaDynArrays == NULL) {
        javaDynArrays = (JavaDynArray**) malloc(JDA_INITIAL_CAPACITY * sizeof(JavaDynArray*));
        if (!javaDynArrays)
            return JNI_FALSE;

        for (int i = 0; i < JDA_INITIAL_CAPACITY; ++i) {
            javaDynArrays[i] = NULL;
        }

        javaDynArrays_capacity = JDA_INITIAL_CAPACITY;
        javaDynArrays_count = 0;
    }

    return JNI_TRUE;
}

jboolean jda_extend() {
    if (jda_tryinit() == JNI_FALSE)
        return JNI_FALSE;

    if (javaDynArrays_count >= javaDynArrays_capacity) {
        size_t new_cap = javaDynArrays_capacity + 4096;
        JavaDynArray** new_arr = (JavaDynArray**) realloc(javaDynArrays, new_cap * sizeof(JavaDynArray*));
        if (new_arr == NULL)
            return JNI_FALSE;

        for (size_t i = javaDynArrays_capacity; i < new_cap; ++i) {
            new_arr[i] = NULL;
        }

        javaDynArrays = new_arr;
        javaDynArrays_capacity = new_cap;
    }
    return JNI_TRUE;
}

JavaDynArray * jda_alloc(jsize len, FIELD_TYPE type) {
    jda_lock();

    void * array = malloc(len * getFieldTypeSize(type));
    if (!array && len > 0) {
        jda_unlock();
        return NULL;
    }

    if (jda_extend() == JNI_FALSE) {
        jda_unlock();
        if (array) free(array);
        return NULL;
    }

    JavaDynArray * jda = (JavaDynArray *) malloc(sizeof(JavaDynArray));
    if (!jda) {
        jda_unlock();
        if (array) free(array);
        return NULL;
    }

    jda->array = array;
    jda->len = len;
    jda->type = type;

    int index = -1;
    for (size_t i = 0; i < javaDynArrays_capacity; ++i) {
        if (javaDynArrays[i] == NULL) {
            index = (int)i;
            break;
        }
    }

    if (index == -1) {
        jda_unlock();
        if (array) free(array);
        free(jda);
        return NULL;
    }

    javaDynArrays[index] = jda;
    javaDynArrays_count++;

    jda_unlock();
    return jda;
}

jsize jda_sizeof(JavaDynArray * jda) {
    if (!jda) return -1;
    return jda->len;
}

jboolean jda_free(JavaDynArray * jda) {
    if (!jda) return JNI_FALSE;
    if (!javaDynArrays) return JNI_FALSE;

    jda_lock();
    for (size_t i = 0; i < javaDynArrays_capacity; ++i) {
        if (javaDynArrays[i] == jda) {
            javaDynArrays[i] = NULL;
            javaDynArrays_count--;
            if (jda->array) free(jda->array);
            free(jda);
            jda_unlock();
            return JNI_TRUE;
        }
    }

    jda_unlock();
    return JNI_FALSE;
}

JavaDynArray * jda_find(void * arr) {
    if (!arr) return NULL;
    if (!javaDynArrays) return NULL;

    jda_lock();
    for (size_t i = 0; i < javaDynArrays_capacity; ++i) {
        if (javaDynArrays[i] == (JavaDynArray*)arr) {
            jda_unlock();
            return (JavaDynArray*)arr;
        }
    }

    jda_unlock();
    return NULL;
}

va_list _AtoV(int dummy, ...) {
    va_list args1;
    va_start(args1, dummy);
    va_list args2;
    va_copy(args2, args1);
    va_end(args1);
    return args2;
}
