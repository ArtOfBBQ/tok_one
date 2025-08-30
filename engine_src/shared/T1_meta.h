#ifndef T1_META_H
#define T1_META_H

#include <stddef.h>
#include <stdint.h>

#define T1_META_ASSERTS 1
#if T1_META_ASSERTS
#include <assert.h>
#endif


typedef enum T1Type {
    T1_TYPE_NOTSET,
    T1_TYPE_ENUM,
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

void T1_meta_init(
    void *(* T1_meta_memcpy)(void *, const void *, size_t),
    void *(* malloc_func)(size_t),
    void *(* memset_func)(void *, int, size_t),
    int (* strcmp_func)(const char *, const char *),
    size_t (* strlen_func)(const char *),
    const uint32_t ascii_store_cap,
    const uint16_t meta_structs_cap,
    const uint16_t meta_fields_cap,
    const uint16_t meta_enums_cap,
    const uint16_t meta_enum_vals_cap);

#define T1_meta_enum(enum_type_name, good) T1_meta_reg_enum(#enum_type_name, sizeof(enum_type_name), good)
void T1_meta_reg_enum(
    const char * enum_type_name,
    const uint32_t size_bytes,
    uint32_t * good);
#define T1_meta_enum_value(enum_type_name, enum_value, good) T1_meta_reg_enum_value(#enum_type_name, #enum_value, enum_value, good)
void T1_meta_reg_enum_value(
    const char * enum_type_name,
    const char * value_name,
    const int64_t value,
    uint32_t * good);

#define T1_meta_struct(struct_name, good) T1_meta_reg_struct(#struct_name, sizeof(struct_name), good)
void T1_meta_reg_struct(
    const char * struct_name,
    const uint32_t size_bytes,
    uint32_t * good);

#define T1_meta_field(parent_type_name, field_T1_type, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, 1, 1, 1, good)
#define T1_meta_struct_field(parent_type_name, field_T1_type, field_type_or_NULL, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_type_or_NULL, 1, 1, 1, good)
#define T1_meta_array(parent_type_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, array_size, 1, 1, good)
#define T1_meta_struct_array(parent_type_name, field_T1_type, field_type_or_NULL, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_type_or_NULL, array_size, 1, 1, good)
#define T1_meta_multi_array(parent_type_name, field_T1_type, field_type_or_NULL, field_name, array_size_1, array_size_2, array_size_3, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_type_or_NULL, array_size_1, array_size_2, array_size_3, good)
void T1_meta_reg_field(
    const char * parent_type_name,
    const uint32_t field_offset,
    const T1Type field_type,
    const char * field_name_or_null,
    const uint16_t field_array_size_1,
    const uint16_t field_array_size_2,
    const uint16_t field_array_size_3,
    uint32_t * good);

#define T1_REFL_MAX_ARRAY_SIZES 3
typedef struct {
    T1Type data_type;
    int64_t offset; // -1 if no such field
    uint16_t array_sizes[T1_REFL_MAX_ARRAY_SIZES];
} T1MetaField;

#define T1_meta_get_field(struct_name, field_name, good) T1_meta_get_field_from_strings(#struct_name, #field_name, good)
T1MetaField T1_meta_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    uint32_t * good);

#endif // T1_META_H
