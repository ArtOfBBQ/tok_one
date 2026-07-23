#ifndef T1_META_H
#define T1_META_H

#include <stddef.h>

#include "T1_types_public.h"

#define T1_META_ASSERTS T1_ACTIVE
#if T1_META_ASSERTS == T1_ACTIVE
#include <assert.h>
#elif T1_META_ASSERTS == T1_INACTIVE
#else
#error
#endif

void T1_meta_init(
    void *(* T1_meta_memcpy)(void *, const void *, u64),
    void *(* malloc_func)(u64),
    void *(* memset_func)(void *, s32, u64),
    s32 (* strcmp_func)(const char *, const char *),
    u64 (* strlen_func)(const char *),
    u64 (* T1_meta_strtoull_func)(const char*, char**, s32),
    const u32 ascii_store_cap,
    const u16 meta_structs_cap,
    const u16 meta_fields_cap,
    const u16 meta_enums_cap,
    const u16 meta_enum_vals_cap);

#define T1_meta_enum(enum_type_name, T1_data_type, good) T1_meta_reg_enum(#enum_type_name, T1_data_type, sizeof(enum_type_name), good)
void T1_meta_reg_enum(
    const char * enum_type_name,
    const T1MetaType T1_type,
    const u32 type_size_check,
    u8 * good);
#define T1_meta_enum_value(enum_type_name, enum_value, good) T1_meta_reg_enum_value(#enum_type_name, #enum_value, enum_value, good)
void T1_meta_reg_enum_value(
    const char * enum_type_name,
    const char * value_name,
    const s64 value,
    u8 * good);

#define T1_meta_struct(struct_name, good) T1_meta_reg_struct(#struct_name, sizeof(struct_name), good)
void T1_meta_reg_struct(
    const char * struct_name,
    const u32 size_bytes,
    u8 * good);

//#define T1_meta_field(parent_type_name, field_T1_type, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, 1, 1, 1, false, good)
//#define T1_meta_enum_field(parent_type_name, enum_name, field_T1_type, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #enum_name, 1, 1, 1, true, good)
//#define T1_meta_enum_array(parent_type_name, field_enum_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_enum_name, array_size, 1, 1, true, good)
//#define T1_meta_struct_field(parent_type_name, field_type_or_NULL, field_name, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, 1, 1, 1, false, good)
//#define T1_meta_array(parent_type_name, field_T1_type, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, NULL, array_size, 1, 1, false, good)
//#define T1_meta_struct_array(parent_type_name, field_type_or_NULL, field_name, array_size, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), T1_TYPE_STRUCT, #field_type_or_NULL, array_size, 1, 1, false, good)
//#define T1_meta_multi_array(parent_type_name, field_T1_type, field_struct_type_or_NULL, field_name, array_size_1, array_size_2, array_size_3, good) T1_meta_reg_field(#field_name, offsetof(parent_type_name, field_name), field_T1_type, #field_struct_type_or_NULL, array_size_1, array_size_2, array_size_3, false, good)
void T1_meta_reg_field(
    const char * field_name,
    const u32 field_offset,
    const T1MetaType field_type,
    const char * field_struct_type_name_or_null,
    const u16 field_array_size_1,
    const u16 field_array_size_2,
    const u16 field_array_size_3,
    const u8 is_enum,
    u8 * good);

void T1_meta_reg_f32_limits_for_last_field(
    const f64 min,
    const f64 max,
    u8 * good);

void T1_meta_reg_int_limits_for_last_field(
    const s64 min,
    const s64 max,
    u8 * good);

void T1_meta_reg_uint_limits_for_last_field(
    const u64 min,
    const u64 max,
    u8 * good);

void
T1_meta_reg_u4_subname_for_last_field(
    const char * subname,
    const char * enum_name_if_any,
    const u8 is_right_nibble,
    u8 * good);

#define T1_META_ARRAY_SIZES_CAP 3
#if 1
typedef struct {
    union {
        u64 custom_umax;
        s64 custom_imax;
        f64 custom_fmax;
    };
    union {
        u64 custom_umin;
        s64 custom_imin;
        f64 custom_fmin;
    };
    s64 offset; // -1 if no such field
    char * name;
    union {
        char * struct_type_name;
        char * enum_type_name;
    };
    u16 array_sizes[T1_META_ARRAY_SIZES_CAP];
    T1MetaType data_type;
    u8 is_enum;
} T1MetaField;
#endif

#if 0
#define T1_meta_get_field(struct_name, field_name, good) T1_meta_get_field_from_strings(#struct_name, #field_name, good)
T1MetaField T1_meta_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    u8 * good);
#endif

void T1_meta_write_to_known_field_str(
    const char * target_parent_type,
    const char * target_field_name,
    const char * value_to_write_str,
    void * target_parent_ptr,
    u8 * good);

void T1_meta_write_to_known_field_uint(
    const char * target_parent_type,
    const char * target_field_name,
    u64 value_to_write_uint,
    void * target_parent_ptr,
    u8 * good);

#define T1_meta_get_num_of_fields_in_struct(struct_type) internal_T1_meta_get_num_of_fields_in_struct(#struct_type)
u32 internal_T1_meta_get_num_of_fields_in_struct(
    const char * struct_name);

void
T1_meta_get_offset_and_type(
    const char * struct_name,
    const char * field_name,
    s32 * out_offset,
    T1MetaType * out_data_type);

#if 0
T1MetaField T1_meta_get_field_at_index(
    char * parent_name_str,
    u32 at_index);
#endif

void T1_meta_serialize_instance_to_buffer(
    const char * struct_name,
    void * to_serialize,
    char * buffer,
    u32 * buffer_size,
    u32 buffer_cap,
    u8 * good);

void T1_meta_deserialize_instance_from_buffer(
    const char * struct_name,
    void * recipient,
    char * buffer,
    const u32 buffer_size,
    u8 * good);

char * T1_meta_enum_uint_to_string(
    const char * enum_type_name,
    const u64 value,
    u8 * good);

#endif // T1_META_H
