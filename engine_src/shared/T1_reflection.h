#ifndef T1_REFLECTION_H
#define T1_REFLECTION_H

#include <stddef.h>
#include <stdint.h>

#define T1_REFLECTION_ASSERTS 1
#if T1_REFLECTION_ASSERTS
#include <assert.h>
#endif


typedef enum T1Type {
    T1_TYPE_NOTSET,
    T1_TYPE_STRUCT,
    T1_TYPE_STRING,
    T1_TYPE_F32,
    T1_TYPE_U64,
    T1_TYPE_U32,
    T1_TYPE_U16,
    T1_TYPE_U8,
    T1_TYPE_I64,
    T1_TYPE_I32,
    T1_TYPE_I16,
    T1_TYPE_I8,
    T1_TYPE_CHAR,
} T1Type;

void T1_reflection_init(
    void *(* T1_reflection_memcpy)(void *, const void *, size_t),
    void *(* malloc_func)(size_t),
    void *(* memset_func)(void *, int, size_t),
    int (* strcmp_func)(const char *, const char *),
    size_t (* strlen_func)(const char *));

#define T1_reflection_struct(struct_name, good) T1_reflection_reg_struct(#struct_name, sizeof(struct_name), good)
void T1_reflection_reg_struct(
    const char * struct_name,
    const uint32_t size_bytes,
    uint32_t * good);

#define T1_reflection_field(struct_name, data_type, prop_name, good) T1_reflection_reg_field(#prop_name, offsetof(struct_name, prop_name), data_type, NULL, 1, 1, 1, good)
#define T1_reflection_struct_field(struct_name, data_type, data_name_if_struct, prop_name, good) T1_reflection_reg_field(#prop_name, offsetof(struct_name, prop_name), data_type, data_name_if_struct, 1, 1, 1, good)
#define T1_reflection_array(struct_name, data_type, prop_name, array_size, good) T1_reflection_reg_field(#prop_name, offsetof(struct_name, prop_name), data_type, NULL, array_size, 1, 1, good)
#define T1_reflection_struct_array(struct_name, data_type, data_name_if_struct, prop_name, array_size, good) T1_reflection_reg_field(#prop_name, offsetof(struct_name, prop_name), data_type, data_name_if_struct, array_size, 1, 1, good)
#define T1_reflection_multi_array(struct_name, data_type, data_name_if_struct, prop_name, array_size_1, array_size_2, array_size_3, good) T1_reflection_reg_field(#prop_name, offsetof(struct_name, prop_name), data_type, data_name_if_struct, array_size_1, array_size_2, array_size_3, good)
void T1_reflection_reg_field(
    const char * property_name,
    const uint32_t property_offset,
    const T1Type property_type,
    const char * property_struct_name,
    const uint16_t property_array_size_1,
    const uint16_t property_array_size_2,
    const uint16_t property_array_size_3,
    uint32_t * good);

#define T1_REFL_MAX_ARRAY_SIZES 3
typedef struct {
    T1Type data_type;
    int64_t offset; // -1 if no such field
    uint16_t array_sizes[T1_REFL_MAX_ARRAY_SIZES];
} T1ReflectedField;

T1ReflectedField T1_reflection_get_field(
    const char * struct_name,
    const char * field_name,
    uint32_t * good);

#endif // T1_REFLECTION_H
