#ifndef T1_META_H
#define T1_META_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define T1_META_ASSERTS T1_ACTIVE
#if T1_META_ASSERTS == T1_ACTIVE
#include <assert.h>
#elif T1_META_ASSERTS == T1_INACTIVE
#else
#error
#endif

typedef enum : uint8_t {
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

#define T1_meta_enum(enum_type_name, T1_data_type, good) T1_meta_reg_enum(#enum_type_name, T1_data_type, sizeof(enum_type_name), good)
void T1_meta_reg_enum(
    const char * enum_type_name,
    const T1Type T1_type,
    const uint32_t type_size_check,
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
#define T1_meta_enum_field(parent_type_name, field_enum_name, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_ENUM, #field_enum_name, 1, 1, 1, good)
#define T1_meta_enum_array(parent_type_name, field_enum_name, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_ENUM, #field_enum_name, array_size, 1, 1, good)
#define T1_meta_struct_field(parent_type_name, field_type_or_NULL, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, 1, 1, 1, good)
#define T1_meta_array(parent_type_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, array_size, 1, 1, good)
#define T1_meta_struct_array(parent_type_name, field_type_or_NULL, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, array_size, 1, 1, good)
#define T1_meta_multi_array(parent_type_name, field_T1_type, field_struct_type_or_NULL, field_name, array_size_1, array_size_2, array_size_3, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_struct_type_or_NULL, array_size_1, array_size_2, array_size_3, good)
void T1_meta_reg_field(
    const char * field_name,
    const uint32_t field_offset,
    const T1Type field_type,
    const char * field_struct_type_name_or_null,
    const uint16_t field_array_size_1,
    const uint16_t field_array_size_2,
    const uint16_t field_array_size_3,
    uint32_t * good);

void T1_meta_reg_custom_float_limits_for_last_field(
    const double floating_min,
    const double floating_max,
    uint32_t * good);

void T1_meta_reg_custom_int_limits_for_last_field(
    const int64_t int_min,
    const int64_t int_max,
    uint32_t * good);

void T1_meta_reg_custom_uint_limits_for_last_field(
    const uint64_t uint_min,
    const uint64_t uint_max,
    uint32_t * good);

#define T1_META_ARRAY_SIZES_CAP 3
typedef struct {
    union {
        uint64_t custom_uint_max;
        int64_t  custom_int_max;
        double   custom_float_max;
    };
    union {
        uint64_t custom_uint_min;
        int64_t  custom_int_min;
        double   custom_float_min;
    };
    int64_t offset; // -1 if no such field
    char * name;
    char * struct_type_name;
    uint16_t array_sizes[T1_META_ARRAY_SIZES_CAP];
    T1Type data_type;
} T1MetaField;

#define T1_meta_get_field(struct_name, field_name, good) T1_meta_get_field_from_strings(#struct_name, #field_name, good)
T1MetaField T1_meta_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    uint32_t * good);

void T1_meta_write_to_known_field_str(
    const char * target_parent_type,
    const char * target_field_name,
    const char * value_to_write_str,
    void * target_parent_ptr,
    uint32_t * good);

#define T1_meta_get_num_of_fields_in_struct(struct_type) internal_T1_meta_get_num_of_fields_in_struct(#struct_type)
uint32_t internal_T1_meta_get_num_of_fields_in_struct(
    const char * struct_name);

T1MetaField T1_meta_get_field_at_index(
    char * parent_name_str,
    uint32_t at_index);

void T1_meta_serialize_instance_to_buffer(
    const char * struct_name,
    void * to_serialize,
    char * buffer,
    uint32_t * buffer_size,
    uint32_t buffer_cap,
    uint32_t * good);

#endif // T1_META_H
