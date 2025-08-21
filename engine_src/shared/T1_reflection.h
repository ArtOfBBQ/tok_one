#ifndef T1_REFLECTION_H
#define T1_REFLECTION_H

#include <stddef.h>
#include <stdint.h>

#define T1_REFLECTION_ASSERTS 1
#if T1_REFLECTION_ASSERTS
#include <assert.h>
#endif


typedef enum T1DataType {
    T1_DATATYPE_NOTSET,
    T1_DATATYPE_STRUCT,
    T1_DATATYPE_STRING,
    T1_DATATYPE_F32,
    T1_DATATYPE_U64,
    T1_DATATYPE_U32,
    T1_DATATYPE_U16,
    T1_DATATYPE_U8,
    T1_DATATYPE_I64,
    T1_DATATYPE_I32,
    T1_DATATYPE_I16,
    T1_DATATYPE_I8,
} T1DataType;

void T1_reflection_init(
    void *(* T1_reflection_memcpy)(void *, const void *, size_t),
    void *(* malloc_func)(size_t),
    void *(* memset_func)(void *, int, size_t),
    int (* strcmp_func)(const char *, const char *),
    size_t (* strlen_func)(const char *));

void T1_reflection_reg(
    const char * struct_name,
    const char * property_name,
    const uint16_t property_offset,
    const T1DataType property_type,
    const uint16_t property_array_size,
    const uint32_t offset_i,
    uint32_t * good);

#endif // T1_REFLECTION_H
