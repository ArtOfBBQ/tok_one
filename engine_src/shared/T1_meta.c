#include "T1_meta.h"

#define T1_META_ARRAY_SLICE_NONE UINT16_MAX
#define T1_META_SUBNAMES_CAP 2
typedef struct {
    char * name;
    char * subnames[T1_META_SUBNAMES_CAP];
    u16 subnames_enum_ids[T1_META_SUBNAMES_CAP];
    
    union {
        char * struct_type_name;
        char * enum_type_name;
    };
    union {
        u64 custom_uint_max;
        s64  custom_int_max;
        f64   custom_f32_max;
    };
    union {
        u64 custom_uint_min;
        s64  custom_int_min;
        f64   custom_f32_min;
    };
    u32 offset; // in field
    u16 parent_enum_id;
    u16 next_i;
    u16 array_sizes[T1_META_ARRAY_SIZES_CAP];
    T1MetaType data_type;
    u8 is_enum;
} MetaField;

typedef struct {
    char * name;
    s64 value;
    u16 metaenum_id;
} MetaEnumValue;

typedef struct {
    char * name;
    T1MetaType T1_type;
} MetaEnum;

typedef struct {
    char * name;
    u32 size_bytes;
    u16 head_fields_i;
} MetaStruct;

typedef struct {
    MetaStruct * internal_parent;
    MetaField * internal_field;
    u8 subname_i;
} T1MetaFieldInternal;

typedef struct {
    u32 parent_offset;
    u16 field_array_slices[T1_META_ARRAY_SIZES_CAP];
    u16 prefix_start_i;
    u16 field_i;
} StackedFieldEntry;

#define T1_META_SERIAL_FIELD_PREFIX_CAP 256
typedef struct {
    void * to_serialize;
    char * buffer;
    u32 * buffer_size;
    u8 * good;
    u32 buffer_cap;
    StackedFieldEntry field_stack[256];
    u16 field_stack_size;
    char field_prefix[T1_META_SERIAL_FIELD_PREFIX_CAP];
} T1MetaSerializationState;

typedef struct {
    T1MetaSerializationState ss;
    void *(* fp_memcpy)(void *, const void *, u64);
    void *(* fp_memset)(void *, s32, u64);
    s32 (* fp_strcmp)(const char *, const char *);
    u64 (* fp_strlen)(const char *);
    u64 (* fp_strtoull)(const char*, char**, s32);
    char * ascii_store;
    MetaEnum * meta_enums;
    MetaEnumValue * meta_enum_vals;
    MetaField * metafields_store;
    MetaStruct * metastructs;
    u32 ascii_store_cap;
    u16 meta_fields_store_cap;
    u16 meta_structs_cap;
    u16 meta_enums_cap;
    u16 meta_enum_vals_cap;
    u16 meta_fields_size;
    u16 meta_structs_size; // first memset target
    u16 meta_enums_size;
    u16 meta_enum_vals_size;
    u32 ascii_store_next_i;
} T1MetaState;

static T1MetaState * t1ms = NULL;

static void construct_metastruct(MetaStruct * to_construct) {
    t1ms->fp_memset(to_construct, 0, sizeof(MetaStruct));
    to_construct->head_fields_i = UINT16_MAX;
}

static void
metafield_construct(
    MetaField * to_construct)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(to_construct != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    t1ms->fp_memset(to_construct, 0, sizeof(MetaField));
    
    to_construct->next_i = UINT16_MAX;
    to_construct->data_type = T1_TYPE_NOTSET;
    to_construct->offset = UINT16_MAX;
    to_construct->parent_enum_id = UINT16_MAX;
}

static MetaField * metafield_i_to_ptr(const u16 field_i) {
    return field_i == UINT16_MAX ?
        NULL :
        &t1ms->metafields_store[field_i];
}

static void T1_meta_reset(void) {
    t1ms->fp_memset(
        (char *)t1ms +
            offsetof(T1MetaState, meta_structs_size),
            0,
            sizeof(T1MetaState) -
                offsetof(T1MetaState, meta_structs_size));
    
    t1ms->fp_memset(
        t1ms->metastructs, 0, t1ms->meta_structs_cap);
    t1ms->fp_memset(
        t1ms->meta_enums, 0, t1ms->meta_enums_cap);
    t1ms->fp_memset(
        t1ms->ascii_store, 0, t1ms->ascii_store_cap);
    
    for (u32 i = 0; i < t1ms->meta_fields_store_cap; i++) {
        metafield_construct(&t1ms->metafields_store[i]);
    }
}

void T1_meta_init(
    void *(* T1_meta_memcpy)(void *, const void *, u64),
    void *(* T1_meta_malloc_func)(u64),
    void *(* T1_meta_memset_func)(void *, s32, u64),
    s32 (* T1_meta_strcmp_func)(const char *, const char *),
    u64 (* T1_meta_strlen_func)(const char *),
    u64 (* T1_meta_strtoull_func)(const char*, char**, s32),
    const u32 ascii_store_cap,
    const u16 meta_structs_cap,
    const u16 meta_fields_cap,
    const u16 meta_enums_cap,
    const u16 meta_enum_vals_cap)
{
    t1ms = T1_meta_malloc_func(sizeof(T1MetaState));
    
    t1ms->fp_memcpy = T1_meta_memcpy;
    t1ms->fp_memset =  T1_meta_memset_func;
    t1ms->fp_strcmp = T1_meta_strcmp_func;
    t1ms->fp_strlen = T1_meta_strlen_func;
    t1ms->fp_strtoull = T1_meta_strtoull_func;
    
    t1ms->ascii_store_cap = ascii_store_cap;
    t1ms->ascii_store = T1_meta_malloc_func(t1ms->ascii_store_cap);
    
    t1ms->meta_structs_cap = meta_structs_cap;
    t1ms->metastructs = T1_meta_malloc_func(
        sizeof(MetaStruct) * t1ms->meta_structs_cap);
    
    t1ms->meta_fields_store_cap = meta_fields_cap;
    t1ms->metafields_store = T1_meta_malloc_func(
        sizeof(MetaField) * t1ms->meta_fields_store_cap);
    
    t1ms->meta_enums_cap = meta_enums_cap;
    t1ms->meta_enums = T1_meta_malloc_func(
        sizeof(MetaEnum) * t1ms->meta_enums_cap);
    
    t1ms->meta_enum_vals_cap = meta_enum_vals_cap;
    t1ms->meta_enum_vals = T1_meta_malloc_func(
        sizeof(MetaEnumValue) * t1ms->meta_enum_vals_cap);
    
    T1_meta_reset();
}

#if 0
static u64 T1_meta_string_to_u64(
    const char * input,
    u8 * good)
{
    *good = 0;
    
    if (input == NULL || input[0] == '\0') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return UINT64_MAX;
    }
    
    u64 return_value = 0;
    
    u32 i = 0;
    while (input[i] != '\0') {
        if (input[i] < '0' || input[i] > '9') {
            return UINT64_MAX;
        }
        
        return_value *= 10;
        return_value += (u64)(input[i] - '0');
        i++;
    }
    
    *good = 1;
    return return_value;
}
#endif

static f64 T1_meta_string_to_f64(
    const char * input,
    u8 * good)
{
    *good = 0;
    
    if (input == NULL || input[0] == '\0') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return UINT64_MAX;
    }
    
    f64 return_value = 0;
    
    u32 i = 0;
    u32 dot_loc = UINT32_MAX;
    f32 final_mult = 1.0f;
    
    if (input[i] == '-') {
        final_mult = -1.0f;
        i++;
    }
    
    while (input[i] != '\0') {
        return_value *= 10.0f;
        
        if (input[i] < '0' || input[i] > '9') {
            return return_value;
        }
        
        u8 val = (u8)(input[i] - '0');
        return_value += (f32)val;
        
        i++;
        
        if (input[i] == '.') {
            dot_loc = i;
            i++;
            break;
        }
    }
    
    if (dot_loc < UINT32_MAX) {
        f32 div = 1.0f;
        
        f32 below_decimal = 0.0f;
        
        while (input[i] != '\0') {
            if (input[i] < '0' || input[i] > '9') {
                return return_value;
            }
            
            u8 val = (u8)(input[i] - '0');
            below_decimal *= 10.0f;
            below_decimal += (f32)val;
            
            div *= 10.0f;
            i++;
        }
        
        below_decimal /= div;
        
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(below_decimal < 1.0f);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        
        return_value += below_decimal;
    }
    
    *good = 1;
    return return_value * final_mult;
}


static void T1_meta_uint_to_string(
    const u64 input,
    char * recipient,
    const u32 recipient_cap,
    u8 * good)
{
    *good = 0;
    
    u64 mult = 1;
    s32 recip_i = 0;
    
    // store chars right to left
    while (mult == 1 || mult <= input) {
        u32 rightmost = (input / mult) % 10;
        if (recip_i + 1 >= (s32)recipient_cap) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        if (rightmost > 9) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        recipient[recip_i++] = '0' + (u8)rightmost;
        mult *= 10;
    }
    recipient[recip_i] = '\0';
    
    // reverse chars
    s32 j = recip_i-1;
    recip_i = 0;
    
    while (recip_i < j) {
        char swap = recipient[recip_i];
        recipient[recip_i] = recipient[j];
        recipient[j] = swap;
        
        recip_i += 1;
        j -= 1;
    }
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(recipient[0] != '\0');
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    *good = 1;
}

static void T1_meta_int_to_string(
    s64 input,
    char * recipient,
    u32 recipient_cap,
    u8 * good)
{
    if (input < 0) {
        *recipient = '-';
        *recipient += 1;
        recipient_cap -= 1;
        input *= -1;
    }
    
    T1_meta_uint_to_string((u64)input, recipient, recipient_cap, good);
}

static void T1_meta_f32_to_string(
    f32 input,
    const u8 precision,
    char * recipient,
    u32 recipient_cap,
    u8 * good)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(precision <= 15);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    *good = 0;
    
    u64 rlen = 0;
    
    if (input < 0.0f) {
        recipient[rlen++] = '-';
        recipient[rlen] = '\0';
        input *= -1.0f;
    }
    
    u32 precision_mult = 1;
    for (u8 _ = 0; _ < precision; _++) {
        precision_mult *= 10;
    }
    
    f32 temp_above_decimal = (f32)(s32)input;
    u32 above_decimal = (u32)temp_above_decimal;
    // we're adding an extra '1' in front of the fractional part here, to
    // make it easier to work with leading zeros
    u32 below_decimal =
        (u32)(((input - temp_above_decimal)+1.0f) * precision_mult);
    
    T1_meta_uint_to_string(
        above_decimal,
        recipient + rlen,
        recipient_cap,
        good);
    rlen = t1ms->fp_strlen(recipient);
    
    if (below_decimal > 0) {
        recipient[rlen++] = '.';
        
        T1_meta_uint_to_string(
            below_decimal,
            recipient + rlen,
            recipient_cap - (u32)rlen,
            good);
        
        // We added a '1' at the start before, so we need to remove that
        while (recipient[rlen] != '\0') {
            recipient[rlen] = recipient[rlen + 1];
            rlen += 1;
        }
    }
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(recipient[0] != '.');
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    *good = 1;
}

static void T1_meta_reverse_array(
    char * array,
    u64 single_element_size,
    u32 array_size)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(array_size > 0);
    assert(single_element_size > 0);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    u32 i = 0;
    u32 j = array_size-1;
    
    char swap[single_element_size];
    
    while (i < j) {
        t1ms->fp_memcpy(
            swap,
            array + (i * single_element_size),
            single_element_size);
        
        t1ms->fp_memcpy(
            array + (i * single_element_size),
            array + (j * single_element_size),
            single_element_size);
        
        t1ms->fp_memcpy(
            array + (j * single_element_size),
            swap,
            single_element_size);
        
        i++;
        j--;
    }
}

static u8 T1_meta_string_starts_with(
    const char * string,
    const char * start_pattern)
{
    u32 i = 0;
    while (start_pattern[i] != '\0') {
        if (string[i] != start_pattern[i]) {
            return 0;
        }
        i++;
    }
    
    return 1;
}

static char * T1_meta_copy_str_to_store(
    const char * to_copy,
    u8 * good)
{
    *good = 0;
    
    u64 len = t1ms->fp_strlen(to_copy);
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(len > 0);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    if (t1ms->ascii_store_next_i + len >= t1ms->ascii_store_cap)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return NULL;
    }
    
    char * return_value = t1ms->ascii_store + t1ms->ascii_store_next_i;
    
    t1ms->fp_memcpy(return_value, to_copy, len);
    
    return_value[len] = '\0';
    
    t1ms->ascii_store_next_i += (len + 1);
    
    *good = 1;
    return return_value;
}

static MetaStruct * find_struct_by_name(
    const char * struct_name)
{
    MetaStruct * return_value = NULL;
    
    for (s32 i = 0; i < (s32)t1ms->meta_structs_size; i++) {
        if (
            t1ms->metastructs[i].name != NULL &&
            t1ms->metastructs[i].name[0] != '\0' &&
            t1ms->fp_strcmp(
                t1ms->metastructs[i].name,
                struct_name) == 0)
        {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(return_value == 0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            
            return_value = t1ms->metastructs + i;
        }
    }
    
    return return_value;
}

static MetaField * find_field_in_struct_by_name(
    const MetaStruct * in_struct,
    const char * property_name,
    u8 * hit_subname_i)
{
    *hit_subname_i = UINT8_MAX;
    MetaField * return_value = NULL;
    
    MetaField * i = metafield_i_to_ptr(in_struct->head_fields_i);
    
    while (i != NULL)
    {
        u8 match_name =
            i->name != NULL &&
                t1ms->fp_strcmp(
                    i->name,
                    property_name) == 0;
        
        for (u8 j = 0; j < T1_META_SUBNAMES_CAP; j++) {
            if (
                i->subnames[j] != NULL &&
                i->subnames[j][0] != '\0' &&
                t1ms->fp_strcmp(
                    i->subnames[j],
                    property_name) == 0)
            {
                *hit_subname_i = j;
            }
        }
        
        if (match_name || (*hit_subname_i < UINT8_MAX))
        {
            return_value = i;
            break;
        }
        
        i = metafield_i_to_ptr(i->next_i);
    }
    
    return return_value;
}

void T1_meta_reg_enum(
    const char * enum_type_name,
    const T1MetaType T1_type,
    const u32 type_size_check,
    u8 * good)
{
    *good = 0;
    
    if (enum_type_name == NULL || enum_type_name[0] == '\0') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // invalid argument
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    switch (T1_type) {
        case T1_TYPE_U4x2:
            if (type_size_check != 1) {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0); // Incorrect T1_TYPE passed? sizeof() is not 8
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
        break;
        case T1_TYPE_I8:
        case T1_TYPE_U8:
            if (type_size_check != 1) {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0); // Incorrect T1_TYPE passed? sizeof() is not 8
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
        break;
        case T1_TYPE_I16:
        case T1_TYPE_U16:
            if (type_size_check != 2) {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0); // Incorrect T1_TYPE passed? sizeof() is not 16
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
        break;
        case T1_TYPE_I32:
        case T1_TYPE_U32:
            if (type_size_check != 4) {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0); // Incorrect T1_TYPE passed? sizeof() is not 32
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
        break;
        default:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // Unsupported type for enums
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            break;
    }
    
    if (t1ms->meta_enums_size + 1 >= t1ms->meta_enums_cap) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // not enough memory, init() with higher meta_enums_cap
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    MetaEnum * new_enum = &t1ms->meta_enums[t1ms->meta_enums_size];
    t1ms->meta_enums_size += 1;
    
    new_enum->name = T1_meta_copy_str_to_store(enum_type_name, good);
    if (!*good) { return; } else { *good = 0; }
    
    new_enum->T1_type = T1_type;
    
    *good = 1;
}

void T1_meta_reg_enum_value(
    const char * enum_type_name,
    const char * value_name,
    const s64 value,
    u8 * good)
{
    *good = 0;
    
    if (enum_type_name == NULL || enum_type_name[0] == '\0') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // invalid argument
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (value_name == NULL || value_name[0] == '\0') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // invalid argument
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (t1ms->meta_enum_vals_size + 1 >= t1ms->meta_enum_vals_cap) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // not enough memory, init() with higher meta_enums_cap
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    MetaEnumValue * new_value = &t1ms->
        meta_enum_vals[t1ms->meta_enum_vals_size];
    t1ms->meta_enum_vals_size += 1;
    
    new_value->name = T1_meta_copy_str_to_store(value_name, good);
    if (!*good) { return; } else { *good = 0; }
    
    new_value->value = value;
    new_value->metaenum_id = UINT16_MAX;
    for (u16 i = 0; i < t1ms->meta_enums_size; i++) {
        if (
            t1ms->fp_strcmp(
                t1ms->meta_enums[i].name,
                enum_type_name) == 0)
        {
            new_value->metaenum_id = i;
            break;
        }
    }
    
    if (new_value->metaenum_id >= UINT16_MAX) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // Tried to register enum value to non-existant enum
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    *good = 1;
}

void T1_meta_reg_struct(
    const char * struct_name,
    const u32 size_bytes,
    u8 * good)
{
    MetaStruct * target_mstruct = find_struct_by_name(struct_name);
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(target_mstruct == NULL); // name already taken
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    if (target_mstruct == NULL) {
        if (
            t1ms->meta_structs_size + 1 >= t1ms->meta_structs_cap)
        {
            *good = 0;
            return;
        }
        
        target_mstruct = t1ms->metastructs +
            t1ms->meta_structs_size;
        t1ms->meta_structs_size += 1;
        construct_metastruct(target_mstruct);
        target_mstruct->name = T1_meta_copy_str_to_store(
            struct_name,
            good);
        target_mstruct->size_bytes = size_bytes;
        if (!*good) {
            #if T1_META_ASSERTS == T1_ACTIVE
            // nout enough ascii store for a struct name should
            // probably never happen
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        *good = 0;
    }
    
    *good = 1;
}

void T1_meta_reg_field(
    const char * field_name,
    const u32 field_offset,
    const T1MetaType field_type,
    const char * field_struct_type_name_or_null,
    const u16 field_array_size_1,
    const u16 field_array_size_2,
    const u16 field_array_size_3,
    const u8 is_enum,
    u8 * good)
{
    *good = 0;
    
    if (
        field_struct_type_name_or_null != NULL &&
        field_struct_type_name_or_null[0] == 'N' &&
        field_struct_type_name_or_null[1] == 'U' &&
        field_struct_type_name_or_null[2] == 'L' &&
        field_struct_type_name_or_null[3] == 'L' &&
        field_struct_type_name_or_null[4] == '\0')
    {
        field_struct_type_name_or_null = NULL;
    }
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(field_name != NULL);
    // When invoking the macro versions of this function, call using
    // the actual name, not a string containing the name
    assert(field_name[0] != '"');
    if (field_struct_type_name_or_null != NULL) {
        // When invoking the macro versions of this function, call using
        // the actual type name, not a string containing the type name
        assert(field_struct_type_name_or_null[0] != '"');
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    // There should be no '.' in the property, because substructs
    // should be registered separately
    for (u32 i = 0; i < UINT16_MAX; i++) {
        if (field_name[i] == '.') {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        if (field_name[i] == '\0') {
            break;
        }
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(i < 250); // 250 character property is absurd
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    if (t1ms->meta_structs_size < 1) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // use T1_meta_reg_struct() first
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        *good = 0;
        return;
    }
    
    // We only support registering to the most recently registered struct
    MetaStruct * target_mstruct = &t1ms->
        metastructs[t1ms->meta_structs_size-1];
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(target_mstruct != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    // check for existing field name in that struct
    u8 target_subname_i;
    MetaField * target_mfield = find_field_in_struct_by_name(
        target_mstruct,
        field_name,
        &target_subname_i);
    
    if (target_mfield == NULL) {
        if (t1ms->meta_fields_size + 1 >= t1ms->meta_fields_store_cap)
        {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // provide more memory on init()
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        target_mfield = t1ms->metafields_store + t1ms->meta_fields_size;
        u16 target_mfield_i = t1ms->meta_fields_size;
        t1ms->meta_fields_size += 1;
        
        metafield_construct(target_mfield);
        
        MetaField * previous_link = metafield_i_to_ptr(
            target_mstruct->head_fields_i);
        if (previous_link == NULL) {
            target_mstruct->head_fields_i = target_mfield_i;
        } else {
            while (
                metafield_i_to_ptr(
                    previous_link->next_i) != NULL)
            {
                previous_link = metafield_i_to_ptr(
                    previous_link->next_i);
            }
            previous_link->next_i = target_mfield_i;
        }
    }
    
    target_mfield->data_type = field_type;
    target_mfield->is_enum = is_enum;
    switch (target_mfield->data_type) {
        case T1_TYPE_NOTSET:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // field type should have been set on registration
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        break;
        case T1_TYPE_STRUCT:
            // no min/max needed
            target_mfield->custom_uint_max = 0;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_U64:
            target_mfield->custom_uint_max = UINT64_MAX;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_U32:
            target_mfield->custom_uint_max = UINT32_MAX;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_U16:
            target_mfield->custom_uint_max = UINT16_MAX;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_U8:
            target_mfield->custom_uint_max = UINT8_MAX;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_U4x2:
            target_mfield->custom_uint_max = 15;
            target_mfield->custom_uint_min = 0;
        break;
        case T1_TYPE_I64:
            target_mfield->custom_int_max = INT64_MAX;
            target_mfield->custom_int_min = INT64_MIN;
        break;
        case T1_TYPE_I32:
            target_mfield->custom_int_max = INT32_MAX;
            target_mfield->custom_int_min = INT32_MIN;
        break;
        case T1_TYPE_I16:
            target_mfield->custom_int_max = INT16_MAX;
            target_mfield->custom_int_min = INT16_MIN;
        break;
        case T1_TYPE_I8:
        case T1_TYPE_CHAR:
            target_mfield->custom_int_max = INT8_MAX;
            target_mfield->custom_int_min = INT8_MIN;
        break;
        case T1_TYPE_F32:
            target_mfield->custom_f32_max = 3.40282347E+38f;
            target_mfield->custom_f32_min = -3.40282347E+38f;
        break;
    }
    target_mfield->offset = field_offset;
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(target_mfield->offset < (1 << 24));
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    target_mfield->array_sizes[0] = field_array_size_1;
    target_mfield->array_sizes[1] = field_array_size_2;
    target_mfield->array_sizes[2] = field_array_size_3;
    #if T1_META_ASSERTS == T1_ACTIVE
    if (target_mfield->array_sizes[1] < 2) {
        assert(target_mfield->array_sizes[2] < 2);
    }
    if (target_mfield->array_sizes[0] < 2) {
        assert(target_mfield->array_sizes[1] < 2);
    }
    for (u32 i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
        assert(target_mfield->array_sizes[i] > 0);
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    target_mfield->name = T1_meta_copy_str_to_store(
        field_name,
        good);
    if (!*good) { return; }
    *good = 0;
    
    if (target_mfield->data_type == T1_TYPE_STRUCT) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(field_struct_type_name_or_null != NULL);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        if (
            field_struct_type_name_or_null == NULL ||
            field_struct_type_name_or_null[0] == '\0')
        {
            *good = 0;
            return;
        }
        
        target_mfield->struct_type_name =
            T1_meta_copy_str_to_store(
                /* const char * to_copy: */
                    field_struct_type_name_or_null,
                /* u8 * good: */
                    good);
        if (!*good) { return; }
        *good = 0;
    } else if (is_enum) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(field_struct_type_name_or_null != NULL);
        assert(target_mfield->is_enum);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        if (field_struct_type_name_or_null == NULL) {
            *good = 0;
            return;
        }
        
        target_mfield->parent_enum_id = UINT16_MAX;
        for (u16 i = 0; i < t1ms->meta_enums_size; i++) {
            if (
                t1ms->fp_strcmp(
                    field_struct_type_name_or_null,
                    t1ms->meta_enums[i].name) == 0)
            {
                target_mfield->enum_type_name =
                    t1ms->meta_enums[i].name;
                target_mfield->parent_enum_id = i;
                if (
                    t1ms->meta_enums[i].T1_type != 
                        target_mfield->data_type)
                {
                    #if T1_META_ASSERTS == T1_ACTIVE
                    assert(0); // enum's data type is mismatched
                    #elif T1_META_ASSERTS == T1_INACTIVE
                    #else
                    #error
                    #endif
                    return;
                }
                break;
            }
        }
        
        if (target_mfield->parent_enum_id >= UINT16_MAX) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // no such parent enum
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
    } else if (field_struct_type_name_or_null != NULL) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // not a struct, expected no type name
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
    }
    
    #if T1_META_ASSERTS == T1_ACTIVE
    MetaField * previous_link = metafield_i_to_ptr(
        target_mstruct->head_fields_i);
    while (previous_link != NULL) {
        MetaField * next_link = metafield_i_to_ptr(previous_link->next_i);
        // This forces the caller of T1_meta_reg_field()
        // to register their fields in the same order as the original
        // struct declaration, which is not strictly necessary but
        // probably for the best
        if (next_link != NULL) {
            assert(previous_link->offset < next_link->offset);
        }
        previous_link = next_link;
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    *good = 1;
}

void T1_meta_reg_f32_limits_for_last_field(
    const f64 min,
    const f64 max,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    // We only support registering to the most recently registered struct
    MetaStruct * target_mstruct = &t1ms->metastructs[t1ms->meta_structs_size-1];
    assert(target_mstruct != NULL);
    if (target_mstruct == NULL) { return; }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    //  We only support registering to the most recently registered field
    MetaField * target_mfield = &t1ms->metafields_store[t1ms->meta_fields_size-1];
    
    if (target_mfield == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set limits, no such field
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (target_mfield->data_type != T1_TYPE_F32) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set f32 limits, not an f32/f64
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (min >= max) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    target_mfield->custom_f32_max = (f32)max;
    target_mfield->custom_f32_min = (f32)min;
    
    *good = 1;
}

void T1_meta_reg_int_limits_for_last_field(
    const s64 int_min,
    const s64 int_max,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    // We only support registering to the most recently registered struct
    MetaStruct * target_mstruct = &t1ms->
        metastructs[t1ms->meta_structs_size-1];
    assert(target_mstruct != NULL);
    if (target_mstruct == NULL) { return; }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    //  We only support registering to the most recently registered field
    MetaField * target_mfield = &t1ms->metafields_store[t1ms->meta_fields_size-1];
    
    if (target_mfield == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set limits, no such field
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (
        target_mfield->data_type != T1_TYPE_I64 &&
        target_mfield->data_type != T1_TYPE_I32 &&
        target_mfield->data_type != T1_TYPE_I16 &&
        target_mfield->data_type != T1_TYPE_I8)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set s32 limits, not an s32 field
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (int_min >= int_max) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    target_mfield->custom_int_max = int_max;
    target_mfield->custom_int_min = int_min;
    
    *good = 1;
}

void T1_meta_reg_uint_limits_for_last_field(
    const u64 uint_min,
    const u64 uint_max,
    u8 * good)
{
    *good = 0;
    
    // We only support registering to the most recently registered struct
    
    #if T1_META_ASSERTS == T1_ACTIVE
    MetaStruct * target_mstruct = &t1ms->
        metastructs[t1ms->meta_structs_size-1];
    assert(target_mstruct != NULL);
    if (target_mstruct == NULL) { return; }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    //  We only support registering to the most recently registered field
    MetaField * target_mfield = &t1ms->metafields_store[t1ms->meta_fields_size-1];
    
    if (target_mfield == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set limits, no such field
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (
        target_mfield->data_type != T1_TYPE_U64 &&
        target_mfield->data_type != T1_TYPE_U32 &&
        target_mfield->data_type != T1_TYPE_U16 &&
        target_mfield->data_type != T1_TYPE_U8)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set unsigned limits on field type
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (uint_min >= uint_max) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    target_mfield->custom_uint_max = uint_max;
    target_mfield->custom_uint_min = uint_min;
    
    *good = 1;
}

void
T1_meta_reg_u4_subname_for_last_field(
    const char * subname,
    const char * enum_name_if_any,
    const u8 is_right_nibble,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    MetaStruct * target_mstruct = &t1ms->
        metastructs[t1ms->meta_structs_size-1];
    assert(target_mstruct != NULL);
    if (target_mstruct == NULL) { return; }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    //  register to the most recently registered field
    MetaField * target_mfield = 
        &t1ms->metafields_store[t1ms->meta_fields_size-1];
    
    if (target_mfield->data_type != T1_TYPE_U4x2) {
        return;
    }
    
    if (!is_right_nibble) {
        assert(target_mfield->subnames[0] == NULL);
        assert(target_mfield->subnames[1] == NULL);
        
        target_mfield->subnames[0] = T1_meta_copy_str_to_store(
            subname, good);
        if (!*good) { return; } else { *good = 0; }
    } else {
        assert(target_mfield->subnames[0] != NULL);
        assert(target_mfield->subnames[1] == NULL);
        
        target_mfield->subnames[1] = T1_meta_copy_str_to_store(
            subname, good);
        if (!*good) { return; } else { *good = 0; }
    }
    
    if (enum_name_if_any != NULL) {
        u16 enum_i = UINT16_MAX;
        for (
            u16 me_i = 0;
            me_i < t1ms->meta_enums_size;
            me_i++)
        {
            if (t1ms->fp_strcmp(t1ms->meta_enums[me_i].name, enum_name_if_any) == 0)
            {
                enum_i = me_i;
            }
        }
        
        if (enum_i >= UINT16_MAX) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // no such enum
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        target_mfield->subnames_enum_ids[is_right_nibble] = enum_i;
    }
    
    *good = 1;
}

static void strip_array_brackets_and_get_array_indices(
    char * to_strip,
    u32 * array_indices,
    u32 * array_indices_found)
{
    u64 field_name_len = t1ms->fp_strlen(to_strip);
    u8 is_array = 0;
    t1ms->fp_memset(
        array_indices,
        0,
        sizeof(u32) * T1_META_ARRAY_SIZES_CAP);
    
    while (
        field_name_len > 3 &&
        to_strip[field_name_len-1] == ']' &&
        to_strip[field_name_len-2] >= '0' &&
        to_strip[field_name_len-2] <= '9')
    {
        for (u32 i = T1_META_ARRAY_SIZES_CAP-1; i >= 1; i--) {
            array_indices[i] = array_indices[i-1];
        }
        
        u32 mult = 1;
        u32 total = 0;
        
        u64 i = field_name_len-2;
        while (
            to_strip[i] >= '0'
            && to_strip[i] <= '9' &&
            i > 2)
        {
            total += ((u32)(to_strip[i] - '0') * mult);
            mult *= 10;
            i--;
        }
        
        if (to_strip[i] == '[') {
            is_array = 1;
            array_indices[0] = total;
            to_strip[i] = '\0';
            field_name_len -= 3;
            *array_indices_found += 1;
        } else {
            break;
        }
    }
    
    return;
}

static u32 array_indices_to_flat_array_index(
    u32 * array_indices,
    MetaField * field)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    for (u32 i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
        assert(array_indices[i] < field->array_sizes[i]);
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    u32 return_val = 0;
    
    for (s32 i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
        u32 offset_per_slice = 1;
        for (s32 j = i+1; j < T1_META_ARRAY_SIZES_CAP; j++) {
            offset_per_slice *= field->array_sizes[j];
        }
        
        return_val += (offset_per_slice * array_indices[i]);
    }
    
    return return_val;
}

// returns cumulative offset
static u32 T1_meta_get_field_recursive(
    T1MetaFieldInternal * return_value,
    const char * struct_name,
    const char * field_name,
    u8 * good)
{
    u32 out_cumul_offset = 0;
    
    *good = 0;
    
    MetaStruct * metastruct = find_struct_by_name(struct_name);
    return_value->internal_parent = metastruct;
    
    if (metastruct == NULL) {
        return_value->internal_field = NULL;
        return_value->internal_parent = NULL;
        return UINT32_MAX;
    }
    
    // The field name may include a dot, in which case we only
    // search for the first part, then do a recursion
    u32 pop_ascii_store_next_i = t1ms->ascii_store_next_i;
    char * first_part = T1_meta_copy_str_to_store(
        field_name,
        good);
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(good);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    *good = 0;
    
    u32 dot_i = 0;
    while (first_part[dot_i] != '.' && first_part[dot_i] != '\0')
    {
        dot_i += 1;
    }
    char * second_part = NULL;
    if (first_part[dot_i] == '.') {
        first_part[dot_i] = '\0';
        second_part = T1_meta_copy_str_to_store(
            first_part + dot_i + 1,
            good);
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(good);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        *good = 0;
    }
    
    u32 array_indices[T1_META_ARRAY_SIZES_CAP];
    u32 array_indices_found = 0;
    strip_array_brackets_and_get_array_indices(
        first_part,
        array_indices,
        &array_indices_found);
    
    MetaField * metafield = find_field_in_struct_by_name(
        metastruct, first_part, &return_value->subname_i);
    
    if (metafield == NULL) {
        return_value->internal_field = NULL;
        *good = 0;
        t1ms->ascii_store_next_i = pop_ascii_store_next_i;
        return UINT32_MAX;
    }
    
    return_value->internal_field = metafield;
    
    u32 flat_array_index = array_indices_to_flat_array_index(
        array_indices,
        metafield);
    
    s32 offset_per_array_index = 0;
    if (flat_array_index > 0 ) {
        MetaStruct * substruct = NULL;
        switch (metafield->data_type) {
            case T1_TYPE_I8:
            case T1_TYPE_U8:
            case T1_TYPE_CHAR:
                offset_per_array_index = 1;
            break;
            case T1_TYPE_I16:
            case T1_TYPE_U16:
                offset_per_array_index = 2;
            break;
            case T1_TYPE_I32:
            case T1_TYPE_U32:
            case T1_TYPE_F32:
                offset_per_array_index = 4;
            break;
            case T1_TYPE_STRUCT:
                substruct = find_struct_by_name(
                    metafield->struct_type_name);
                if (substruct == NULL) {
                    #if T1_META_ASSERTS == T1_ACTIVE
                    // our struct has an unregistered struct as
                    // a property, that shouldn't be possible
                    assert(0);
                    #elif T1_META_ASSERTS == T1_INACTIVE
                    #else
                    #error
                    #endif
                    *good = 0;
                    t1ms->ascii_store_next_i = pop_ascii_store_next_i;
                    return UINT32_MAX;
                }
                offset_per_array_index = (s32)substruct->size_bytes;
                break;
            case T1_TYPE_NOTSET:
            default:
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                *good = 0;
                t1ms->ascii_store_next_i = pop_ascii_store_next_i;
                return UINT32_MAX;
        }
        
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(offset_per_array_index > 0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    if (second_part != NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(return_value->internal_field->data_type == T1_TYPE_STRUCT);
        assert(metafield->struct_type_name != NULL);
        assert(metafield->struct_type_name[0] != '\0');
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        
        out_cumul_offset += T1_meta_get_field_recursive(
            return_value,
            metafield->struct_type_name,
            second_part,
            good);
        if (!*good) {
            t1ms->ascii_store_next_i = pop_ascii_store_next_i;
            return UINT32_MAX;
        }
    }
    
    t1ms->ascii_store_next_i = pop_ascii_store_next_i;
    *good = 1;
    out_cumul_offset +=
        metafield->offset +
            (
                (u32)offset_per_array_index *
                flat_array_index
            );
    return out_cumul_offset;
}

static u64 T1_meta_shared_get_element_size_bytes(
    T1MetaType data_type,
    const char * struct_name_or_null)
{
    switch (data_type) {
        case T1_TYPE_STRUCT:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(struct_name_or_null != NULL);
            assert(struct_name_or_null[0] != '\0');
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            
            MetaStruct * mstruct = find_struct_by_name(struct_name_or_null);
            
            if (mstruct == NULL)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                break;
            }
            
            return mstruct->size_bytes;
        break;
        case T1_TYPE_U64:
            return 8;
        break;
        case T1_TYPE_U32:
            return 4;
        break;
        case T1_TYPE_U16:
            return 2;
        break;
        case T1_TYPE_U8:
        case T1_TYPE_U4x2:
            return 1;
        break;
        case T1_TYPE_I64:
            return 8;
        break;
        case T1_TYPE_I32:
            return 4;
        break;
        case T1_TYPE_I16:
            return 2;
        break;
        case T1_TYPE_I8:
            return 1;
        break;
        case T1_TYPE_F32:
            return 4;
        case T1_TYPE_CHAR:
            return 1;
        break;
        case T1_TYPE_NOTSET:
            // Reminder: enum should never happen here because it should be
            // adjusted above at the top of this function
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        break;
    }
    
    return 0;
}

#if 0
static u64 T1_meta_field_get_element_size_bytes(T1MetaField * field) {
    u64 return_value = 0;
    
    switch (field->data_type) {
        case T1_TYPE_STRUCT:
            // field->struct_type_name;
            assert(0);
        break;
        case T1_TYPE_ENUM:
            
            assert(0);
        break;
        case T1_TYPE_U64:
            return 8;
        break;
        case T1_TYPE_U32:
            return 4;
        break;
        case T1_TYPE_U16:
            return 2;
        break;
        case T1_TYPE_U8:
            return 1;
        break;
        case T1_TYPE_I64:
            return 8;
        break;
        case T1_TYPE_I32:
            return 4;
        break;
        case T1_TYPE_I16:
            return 2;
        break;
        case T1_TYPE_I8:
            return 1;
        break;
        case T1_TYPE_F32:
            return 4;
        case T1_TYPE_CHAR:
            return 1;
        break;
        case T1_TYPE_NOTSET:
        case T1_TYPE_STRING:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        break;
    }
    
    return return_value;
}
#endif

static u64 T1_meta_internal_field_get_element_size_bytes(MetaField * field) {
    return T1_meta_shared_get_element_size_bytes(
        field->data_type,
        field->struct_type_name);
}

typedef struct {
    u64 value_u64;
    s64  value_i64;
    f64   value_f64;
    u8  value_is_i64;
    u8  value_is_u64;
    u8  value_is_f64;
} T1MetaParsedvalue;

static void parse_string_to_value(
    T1MetaParsedvalue * recipient,
    const char * string)
{
    u8 is_f64 = 0;
    recipient->value_f64 = T1_meta_string_to_f64(string, &is_f64);
    recipient->value_is_f64 = (u8)is_f64;
    
    u8 val_starts_minus = string[0] == '-';
    if (val_starts_minus) { string++; }
    
    char * endptr = NULL;
    recipient->value_u64 = (u64)t1ms->fp_strtoull(string, &endptr, 10);
    recipient->value_is_u64 = *endptr == '\0' && !val_starts_minus;
    
    recipient->value_is_i64 =
        *endptr == '\0' &&
        recipient->value_u64 < INT64_MAX;
    
    recipient->value_i64 = (s64)recipient->value_u64;
    if (val_starts_minus) { recipient->value_i64 *= -1; }
}

void T1_meta_write_to_known_field_str(
    const char * target_parent_type,
    const char * target_field_name,
    const char * value_to_write_str,
    void * target_parent_ptr,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(value_to_write_str != NULL);
    assert(value_to_write_str[0] != '\0');
    assert(target_parent_type != NULL);
    assert(target_parent_type[0] != '"');
    assert(target_field_name != NULL);
    assert(target_field_name[0] != '"');
    assert(target_parent_ptr != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    T1MetaFieldInternal field;
    field.internal_field = NULL;
    field.internal_parent = NULL;
    
    // T1_meta_get_field_recursive() will push to the ascii store
    // as if it were stack memory
    u32 before_recursion_ascii_store_next_i = t1ms->ascii_store_next_i;
    
    u32 total_offset = T1_meta_get_field_recursive(
        &field,
        target_parent_type,
        target_field_name,
        good);
    
    if (!*good) {
        return;
    }
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(field.internal_field->name != NULL);
    assert(field.internal_field->data_type != T1_TYPE_NOTSET);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    // discard or "pop" T1_meta_get_field_recursive()'s strings
    t1ms->ascii_store_next_i = before_recursion_ascii_store_next_i;
    
    char * value_adj = (char *)value_to_write_str;
    while (value_adj[0] == ' ') { value_adj++; }
    
    T1MetaParsedvalue parsed;
    parse_string_to_value(&parsed, value_adj);
    
    MetaEnum * parent_enum = NULL;
    T1MetaType type_adj = field.internal_field->data_type;
    u8 found_enum_field = 0;
        
    if (field.internal_field->is_enum)
    {
        u16 enum_id = field.internal_field->parent_enum_id;
        
        if (
            field.subname_i < T1_META_SUBNAMES_CAP &&
            field.internal_field->
                subnames_enum_ids[field.subname_i] < UINT16_MAX)
        {
            enum_id = field.internal_field->
                subnames_enum_ids[field.subname_i];
        }
        
        if (
            field.internal_field == NULL ||
            enum_id >= t1ms->meta_enums_size ||
            enum_id >= t1ms->meta_enums_cap)
        {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        }
        parent_enum = &t1ms->meta_enums[enum_id];
        
        for (
            u32 i = 0;
            i < t1ms->meta_enum_vals_size;
            i++)
        {
            if (
                t1ms->meta_enum_vals[i].
                    metaenum_id == enum_id &&
                t1ms->fp_strcmp(
                    t1ms->meta_enum_vals[i].name,
                    value_adj) == 0)
            {
                found_enum_field = 1;
                parsed.value_i64 =
                    t1ms->meta_enum_vals[i].value;
                parsed.value_u64 = (u64)t1ms->meta_enum_vals[i].value;
                parsed.value_is_i64 = 1;
                parsed.value_is_u64 = parsed.value_i64 >= 0;
                break;
            }
        }
        
        if (!found_enum_field) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0); // enum is registered, but that value isn't
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        type_adj = parent_enum->T1_type;
    } else if (
        field.internal_field->data_type != T1_TYPE_CHAR &&
        !parsed.value_is_u64 &&
        !parsed.value_is_i64 &&
        !parsed.value_is_f64)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // trying to write string to non-enum non-char field?
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    s32 rightmost_array_i = 2;
    switch (type_adj) {
        case T1_TYPE_NOTSET:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        case T1_TYPE_F32:
            if (
                !parsed.value_is_f64 ||
                parsed.value_f64 > 3.4028235+36 ||
                parsed.value_f64 < -3.40282347E+36)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            f32 value_f32 = (f32)parsed.value_f64;
            t1ms->fp_memcpy(
                (f32 *)((char *)target_parent_ptr + total_offset),
                &value_f32,
                4);
        break;
        case T1_TYPE_I8:
            if (
                !parsed.value_is_i64 ||
                parsed.value_i64 > INT8_MAX ||
                parsed.value_i64 < INT8_MIN)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            s8 value_i8 = (s8)parsed.value_i64;
            t1ms->fp_memcpy(
                (s8 *)((char *)target_parent_ptr + total_offset),
                &value_i8,
                1);
        break;
        case T1_TYPE_I16:
            if (
                !parsed.value_is_i64 ||
                parsed.value_i64 > INT16_MAX ||
                parsed.value_i64 < INT16_MIN)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            s16 value_i16 = (s16)parsed.value_i64;
            t1ms->fp_memcpy(
                (s16 *)((char *)target_parent_ptr + total_offset),
                &value_i16,
                2);
        break;
        case T1_TYPE_I32:
            if (
                !parsed.value_is_i64 ||
                parsed.value_i64 > INT32_MAX ||
                parsed.value_i64 < INT32_MIN)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            s32 value_s32 = (s32)parsed.value_i64;
            t1ms->fp_memcpy(
                (s32 *)((char *)target_parent_ptr + total_offset),
                &value_s32,
                4);
        break;
        case T1_TYPE_I64:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        break;
        case T1_TYPE_U4x2:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > 15)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            u8 value_u4 = (u8)parsed.value_u64;
            
            s32 value_recip = *(u8 *)(
                (char *)target_parent_ptr + total_offset);
            
            if (field.subname_i == 0) {
                value_recip =
                    (value_u4 << 4) |
                    (value_recip & 0x0F);
            } else if (
                field.subname_i == 1)
            {
                value_recip =
                    (value_recip & 0xF0) |
                    (value_u4 & 0x0F);
            } else
            {
                // trying to write a u4, but subfield > 1
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            } 
            
            value_u4 = (u8)value_recip;
            
            t1ms->fp_memcpy(
                (u8 *)((char *)target_parent_ptr + total_offset),
                &value_u4,
                1);
        break;
        case T1_TYPE_U8:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT8_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            u8 value_u8 = (u8)parsed.value_u64;
            t1ms->fp_memcpy(
                (u8 *)((char *)target_parent_ptr + total_offset),
                &value_u8,
                1);
        break;
        case T1_TYPE_U16:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT16_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            u16 value_u16 = (u16)parsed.value_u64;
            t1ms->fp_memcpy(
                (u16 *)((char *)target_parent_ptr + total_offset),
                &value_u16,
                2);
        break;
        case T1_TYPE_U32:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT32_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            t1ms->fp_memcpy(
                (u32 *)((char *)target_parent_ptr + total_offset),
                &parsed.value_u64,
                4);
        break;
        case T1_TYPE_U64:
            if (
                !parsed.value_is_u64)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            t1ms->fp_memcpy(
                (u64 *)((char *)target_parent_ptr + total_offset),
                &parsed.value_u64,
                8);
        break;
        case T1_TYPE_CHAR:
            rightmost_array_i = 2;
            while (
                field.internal_field->array_sizes[rightmost_array_i] < 2)
            {
                rightmost_array_i -= 1;
            }
            
            if (
                t1ms->fp_strlen(value_to_write_str) >
                    field.internal_field->array_sizes[rightmost_array_i])
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(t1ms->fp_memcpy != NULL);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            t1ms->fp_memcpy(
                ((char *)target_parent_ptr + total_offset),
                value_to_write_str,
                field.internal_field->array_sizes[rightmost_array_i]);
        break;
        default:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
    }
    
    *good = 1;
}

void T1_meta_write_to_known_field_uint(
    const char * target_parent_type,
    const char * target_field_name,
    const u64 value_to_write_uint,
    void * target_parent_ptr,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(target_parent_type != NULL);
    assert(target_parent_type[0] != '"');
    assert(target_field_name != NULL);
    assert(target_field_name[0] != '"');
    assert(target_parent_ptr != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    T1MetaFieldInternal field;
    field.internal_field = NULL;
    field.internal_parent = NULL;
    
    // T1_meta_get_field_recursive() will push to the ascii store
    // as if it were stack memory
    u32 before_recursion_ascii_store_next_i = t1ms->ascii_store_next_i;
    
    u32 total_offset = T1_meta_get_field_recursive(
        &field,
        target_parent_type,
        target_field_name,
        good);
    
    if (!*good) {
        return;
    }
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(field.internal_field->name != NULL);
    assert(field.internal_field->data_type != T1_TYPE_NOTSET);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    // discard or "pop" T1_meta_get_field_recursive()'s strings
    t1ms->ascii_store_next_i = before_recursion_ascii_store_next_i;
    
    T1MetaParsedvalue parsed;
    parsed.value_u64 = value_to_write_uint;
    parsed.value_is_i64 = 0;
    parsed.value_is_u64 = 1;
    parsed.value_is_f64 = 0;
    
    T1MetaType type_adj = field.internal_field->data_type;
    
    if (field.internal_field->is_enum)
    {
        return;
    } else if (
        field.internal_field->data_type != T1_TYPE_CHAR &&
        !parsed.value_is_u64 &&
        !parsed.value_is_i64 &&
        !parsed.value_is_f64)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // trying to write string to non-enum non-char field?
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    switch (type_adj) {
        case T1_TYPE_NOTSET:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        case T1_TYPE_F32:
            return;
        break;
        case T1_TYPE_I8:
            return;
        break;
        case T1_TYPE_I16:
            return;
        break;
        case T1_TYPE_I32:
            return;
        break;
        case T1_TYPE_I64:
            return;
        break;
        case T1_TYPE_U4x2:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > 15)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            u8 value_u4 = (u8)parsed.value_u64;
            
            s32 value_recip = *(u8 *)(
                (char *)target_parent_ptr + total_offset);
            
            if (field.subname_i == 0) {
                value_recip =
                    (value_u4 << 4) |
                    (value_recip & 0x0F);
            } else if (
                field.subname_i == 1)
            {
                value_recip =
                    (value_recip & 0xF0) |
                    (value_u4 & 0x0F);
            } else
            {
                // trying to write a u4, but subfield > 1
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            } 
            
            value_u4 = (u8)value_recip;
            
            t1ms->fp_memcpy(
                (u8 *)((char *)target_parent_ptr + total_offset),
                &value_u4,
                1);
        break;
        case T1_TYPE_U8:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT8_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            u8 value_u8 = (u8)parsed.value_u64;
            t1ms->fp_memcpy(
                (u8 *)((char *)target_parent_ptr + total_offset),
                &value_u8,
                1);
        break;
        case T1_TYPE_U16:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT16_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            u16 value_u16 = (u16)parsed.value_u64;
            t1ms->fp_memcpy(
                (u16 *)((char *)target_parent_ptr + total_offset),
                &value_u16,
                2);
        break;
        case T1_TYPE_U32:
            if (
                !parsed.value_is_u64 ||
                parsed.value_u64 > UINT32_MAX)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            t1ms->fp_memcpy(
                (u32 *)((char *)target_parent_ptr + total_offset),
                &parsed.value_u64,
                4);
        break;
        case T1_TYPE_U64:
            if (
                !parsed.value_is_u64)
            {
                #if T1_META_ASSERTS == T1_ACTIVE
                assert(0);
                #elif T1_META_ASSERTS == T1_INACTIVE
                #else
                #error
                #endif
                return;
            }
            
            t1ms->fp_memcpy(
                (u64 *)((char *)target_parent_ptr + total_offset),
                &parsed.value_u64,
                8);
        break;
        case T1_TYPE_CHAR:
            return;
        break;
        default:
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
    }
    
    *good = 1;
}

u32 internal_T1_meta_get_num_of_fields_in_struct(
    const char * struct_name)
{
    MetaStruct * parent = find_struct_by_name(struct_name);
    
    if (parent == NULL) { return 0; }
    
    u32 fields_size = 0;
    MetaField * field = metafield_i_to_ptr(parent->head_fields_i);
    while (field != NULL) {
        fields_size += 1;
        field = metafield_i_to_ptr(field->next_i);
    }
    
    return fields_size;
}

void
T1_meta_get_offset_and_type(
    const char * struct_name,
    const char * field_name,
    s32 * out_offset,
    T1MetaType * out_data_type)
{
    T1MetaFieldInternal field;
    
    u8 good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(field_name != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    T1_meta_get_field_recursive(
        &field,
        struct_name,
        field_name,
        &good);
    
    if (good) {
        *out_data_type = field.internal_field->data_type;
        *out_offset = (s32)field.internal_field->offset;
    } else {
        *out_data_type = T1_TYPE_NOTSET;
        *out_offset = INT32_MAX;
    }
}

static void T1_meta_serialize_cat_str_to_buf(
    const char * to_copy)
{
    *t1ms->ss.good = 0;
    
    u64 to_copy_len = t1ms->fp_strlen(to_copy);
    
    if (*t1ms->ss.buffer_size + to_copy_len + 1 >= t1ms->ss.buffer_cap) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    t1ms->fp_memcpy(
        t1ms->ss.buffer + *t1ms->ss.buffer_size,
        to_copy,
        to_copy_len);
    
    *t1ms->ss.buffer_size += to_copy_len;
    
    t1ms->ss.buffer[*t1ms->ss.buffer_size] = '\0';
    
    *t1ms->ss.good = 1;
    return;
}

static void T1_meta_serialize_cat_str_to_prefix(
    const char * to_copy)
{
    *t1ms->ss.good = 0;
    
    u64 to_copy_len = t1ms->fp_strlen(to_copy);
    
    u64 field_prefix_len = t1ms->fp_strlen(t1ms->ss.field_prefix);
    
    if (
        to_copy_len + field_prefix_len + 1 >=
            T1_META_SERIAL_FIELD_PREFIX_CAP)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    t1ms->fp_memcpy(
        t1ms->ss.field_prefix + field_prefix_len,
        to_copy,
        to_copy_len);
    t1ms->ss.field_prefix[field_prefix_len + to_copy_len] = '\0';
    
    *t1ms->ss.good = 1;
    return;
}

static void T1_meta_serialize_add_struct_fields_to_stack(
    const char * struct_name,
    const char * field_text,
    u16 field_array_sizes[T1_META_ARRAY_SIZES_CAP],
    const char * field_dot_or_arrow,
    const u32 parent_offset)
{
    *t1ms->ss.good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(struct_name != NULL);
    assert(struct_name[0] != '\0');
    assert(field_text != NULL);
    assert(field_text[0] != '\0');
    assert(field_dot_or_arrow != NULL);
    assert(field_dot_or_arrow[0] != '\0');
    assert((field_dot_or_arrow[0] == '.' || field_dot_or_arrow[0] == '-'));
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    MetaStruct * parent = find_struct_by_name(struct_name);
    if (parent == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    T1_meta_serialize_cat_str_to_prefix(field_text);
    if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
    
    char slice_text[32];
    t1ms->fp_memset(slice_text, 0, 32);
    
    if (field_array_sizes[0] != T1_META_ARRAY_SLICE_NONE) {
        slice_text[0] = '[';
        T1_meta_int_to_string(
            field_array_sizes[0],
            slice_text+1,
            31,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        slice_text[t1ms->fp_strlen(slice_text)] = ']';
    }
    u64 len = t1ms->fp_strlen(slice_text);
    if (field_array_sizes[1] != T1_META_ARRAY_SLICE_NONE) {
        slice_text[len++] = '[';
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(len < 31);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        T1_meta_int_to_string(
            field_array_sizes[1],
            slice_text+len,
            32 - (u32)len,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        len = t1ms->fp_strlen(slice_text);
        slice_text[len++] = ']';
    }
    if (field_array_sizes[2] != T1_META_ARRAY_SLICE_NONE) {
        slice_text[len++] = '[';
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(len < 31);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        T1_meta_int_to_string(
            field_array_sizes[2],
            slice_text+len,
            32 - (u32)len,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        len = t1ms->fp_strlen(slice_text);
        slice_text[len++] = ']';
    }
    
    T1_meta_serialize_cat_str_to_prefix(slice_text);
    if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
    
    T1_meta_serialize_cat_str_to_prefix(field_dot_or_arrow);
    if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
    
    MetaField * field = metafield_i_to_ptr(parent->head_fields_i);
    if (field == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // this type has no fields registered to it?
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    u32 to_reverse_start_i = t1ms->ss.field_stack_size;
    u16 prefix_start_i = (u16)t1ms->fp_strlen(
        t1ms->ss.field_prefix);
    
    u16 field_i = parent->head_fields_i;
    
    u32 array_slices[T1_META_ARRAY_SIZES_CAP];
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(T1_META_ARRAY_SIZES_CAP == 3);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    while (field_i != UINT16_MAX) {
        field = metafield_i_to_ptr(field_i);
        
        for (
            array_slices[0] = 0;
            array_slices[0] < field->array_sizes[0];
            array_slices[0]++)
        {
        for (
            array_slices[1] = 0;
            array_slices[1] < field->array_sizes[1];
            array_slices[1]++)
        {
        for (
            array_slices[2] = 0;
            array_slices[2] < field->array_sizes[2];
            array_slices[2]++)
        {
            u32 flat_i = array_indices_to_flat_array_index(
                array_slices,
                field);
            
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].parent_offset =
                parent_offset + (
                    flat_i * (u32)
                        T1_meta_internal_field_get_element_size_bytes(field));
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[0] = field->array_sizes[0] > 1 ?
                    (u16)array_slices[0] :
                    T1_META_ARRAY_SLICE_NONE;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[1] = field->array_sizes[1] > 1 ?
                    (u16)array_slices[1] :
                    T1_META_ARRAY_SLICE_NONE;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[2] = field->array_sizes[2] > 1 ?
                    (u16)array_slices[2] :
                    T1_META_ARRAY_SLICE_NONE;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].field_i = field_i;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].prefix_start_i =
                prefix_start_i;
            t1ms->ss.field_stack_size += 1;
        }
        }
        }
        
        field_i = field->next_i;
    }
    
    T1_meta_reverse_array(
        (char *)(t1ms->ss.field_stack + to_reverse_start_i),
        sizeof(StackedFieldEntry),
        t1ms->ss.field_stack_size - to_reverse_start_i);
    
    *t1ms->ss.good = 1;
    return;
}

void T1_meta_serialize_instance_to_buffer(
    const char * struct_name,
    void * to_serialize,
    char * buffer,
    u32 * buffer_size,
    u32 buffer_cap,
    u8 * good)
{
    *good = 0;
    *buffer_size = 0;
    
    t1ms->ss.to_serialize = to_serialize;
    t1ms->ss.buffer = buffer;
    t1ms->ss.buffer_size = buffer_size;
    t1ms->ss.buffer_cap = buffer_cap;
    t1ms->ss.good = good;
    t1ms->ss.field_prefix[0] = '\0';
    t1ms->ss.field_stack_size = 0;
    
    T1_meta_serialize_cat_str_to_buf("T1_META_START\n");
    
    T1_meta_serialize_cat_str_to_buf(struct_name);
    if (!*good) { return; } else { *good = 0; }
    
    T1_meta_serialize_cat_str_to_buf("\n");
    if (!*good) { return; } else { *good = 0; }
    
    MetaStruct * parent = find_struct_by_name(struct_name);
    if (parent == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    MetaField * field = metafield_i_to_ptr(parent->head_fields_i);
    if (field == NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // this type has no fields registered to it?
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(T1_META_ARRAY_SIZES_CAP == 3);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    u16 slices_i[T1_META_ARRAY_SIZES_CAP];
    slices_i[0] = T1_META_ARRAY_SLICE_NONE;
    slices_i[1] = T1_META_ARRAY_SLICE_NONE;
    slices_i[2] = T1_META_ARRAY_SLICE_NONE;
    
    T1_meta_serialize_add_struct_fields_to_stack(
        parent->name,
        "s",
        slices_i,
        "->",
        0);
    if (!*good) { return; } else { *good = 0; }
    
    f32 value_f32;
    u64 value_u64;
    s64 value_i64;
    
    while (t1ms->ss.field_stack_size > 0) {
        
        StackedFieldEntry top =
            t1ms->ss.field_stack[t1ms->ss.field_stack_size-1];
        t1ms->ss.field_stack_size -= 1;
        t1ms->ss.field_prefix[top.prefix_start_i] = '\0';
        
        field = metafield_i_to_ptr(top.field_i);
        
        if (field->data_type == T1_TYPE_STRUCT) {
                
            t1ms->ss.field_prefix[top.prefix_start_i] = '\0';
            
            T1_meta_serialize_add_struct_fields_to_stack(
                field->struct_type_name,
                field->name,
                top.field_array_slices,
                ".",
                field->offset + top.parent_offset);
            if (!*good) { return; } else { *good = 0; }
            
            continue;
        } else {
            T1_meta_serialize_cat_str_to_buf(t1ms->ss.field_prefix);
            T1_meta_serialize_cat_str_to_buf(field->name);
            
            char value_as_str[128];
            t1ms->fp_memset(value_as_str, 0, 128);
            
            if (top.field_array_slices[0] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[0],
                    /* char * recipient: */
                        value_as_str,
                    /* u32 recipient_cap: */
                        128,
                    /* u8 * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->fp_memset(value_as_str, 0, 128);
            }
            if (top.field_array_slices[1] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[1],
                    /* char * recipient: */
                        value_as_str,
                    /* u32 recipient_cap: */
                        128,
                    /* u8 * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->fp_memset(value_as_str, 0, 128);
            }
            if (top.field_array_slices[2] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[2],
                    /* char * recipient: */
                        value_as_str,
                    /* u32 recipient_cap: */
                        128,
                    /* u8 * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->fp_memset(value_as_str, 0, 128);
            }
            
            T1_meta_serialize_cat_str_to_buf(" = ");
            
            if (field->parent_enum_id != UINT16_MAX) {
                u8 val_u8 = *(u8 *)(((char *)to_serialize) +
                    field->offset + top.parent_offset);
                for (
                    u32 mev_i = 0;
                    mev_i < t1ms->meta_enum_vals_size;
                    mev_i++)
                {
                    if (
                        t1ms->meta_enums[field->parent_enum_id].T1_type !=
                            T1_TYPE_U8 ||
                        field->data_type != T1_TYPE_U8)
                    {
                        #if T1_META_ASSERTS == T1_ACTIVE
                        assert(0); // only supporting u8 enums for now
                        #elif T1_META_ASSERTS == T1_INACTIVE
                        #else
                        #error
                        #endif
                    }
                    
                    if (
                        t1ms->meta_enum_vals[mev_i].metaenum_id == field->parent_enum_id &&
                        t1ms->meta_enum_vals[mev_i].value == val_u8)
                    {
                        #if T1_META_ASSERTS == T1_ACTIVE
                        assert(t1ms->meta_enum_vals[mev_i].name != NULL);
                        #elif T1_META_ASSERTS == T1_INACTIVE
                        #else
                        #error
                        #endif
                        
                        u64 enumvalstrlen = t1ms->fp_strlen(t1ms->meta_enum_vals[mev_i].name);
                        
                        #if T1_META_ASSERTS == T1_ACTIVE
                        assert(enumvalstrlen > 0);
                        #elif T1_META_ASSERTS == T1_INACTIVE
                        #else
                        #error
                        #endif
                        
                        t1ms->fp_memcpy(
                            value_as_str,
                            t1ms->meta_enum_vals[mev_i].name,
                            enumvalstrlen);
                        break;
                    }
                }
            } else {
                switch (field->data_type) {
                    case T1_TYPE_F32:
                        value_f32 = *(f32 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_f32_to_string(
                            /* f32 input: */
                                value_f32,
                            /* const u8 precision: */
                                4,
                            /* char * recipient: */
                                value_as_str,
                            /* u32 recipient_cap: */
                                128,
                            /* u8 * good: */
                                good);
                        if (!*good) { return; } else { *good = 0; }
                        
                        u64 vaslen = t1ms->fp_strlen(value_as_str);
                        
                        if (vaslen + 1 >= 128) {
                            return;
                        }
                        
                        value_as_str[vaslen] = 'f';
                        value_as_str[vaslen+1] = '\0';
                    break;
                    case T1_TYPE_U64:
                        value_u64 = *(u64 *)(((char *)to_serialize) +
                        field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(
                            value_u64, value_as_str, 128, good);
                        
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U32:
                        value_u64 = *(u32 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U16:
                        value_u64 = *(u16 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U8:
                        value_u64 = *(u8 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I64:
                        value_i64 = *(s64 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I32:
                        value_i64 = *(s32 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I16:
                        value_i64 = *(s16 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I8:
                        value_i64 = *(s8 *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    default:
                        #if T1_META_ASSERTS == T1_ACTIVE
                        assert(0); // this type has no fields registered to it?
                        #elif T1_META_ASSERTS == T1_INACTIVE
                        #else
                        #error
                        #endif
                        return;
                }
            }
            
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(value_as_str[0] != '\0');
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            
            T1_meta_serialize_cat_str_to_buf(value_as_str);
            
            T1_meta_serialize_cat_str_to_buf(";\n");
        }
    }
    
    T1_meta_serialize_cat_str_to_buf("T1_META_END\n");
    
    *good = 1;
}

void T1_meta_deserialize_instance_from_buffer(
    const char * struct_name,
    void * recipient,
    char * buffer,
    const u32 buffer_size,
    u8 * good)
{
    *good = 0;
    
    u32 at_i = 0;
    if (!T1_meta_string_starts_with(buffer + at_i, "T1_META_START\n")) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        *good = 0;
        return;
    }
    at_i += 14;
    
    if (!T1_meta_string_starts_with(buffer + at_i, struct_name)) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        *good = 0;
        return;
    }
    at_i += t1ms->fp_strlen(struct_name);
    
    if (!T1_meta_string_starts_with(buffer + at_i, "\n")) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        *good = 0;
        return;
    }
    at_i += 1;
    
    char recursive_field_name[256];
    char value_to_assign[128];
    u32 write_i;
    
    while (buffer[at_i] == 's') {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(at_i < buffer_size);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        
        if (!T1_meta_string_starts_with(buffer + at_i, "s->")) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        at_i += 3;
        
        write_i = 0;
        while (
            buffer[at_i] != ' ' &&
            buffer[at_i] != '\n' &&
            buffer[at_i] != '\0')
        {
            recursive_field_name[write_i++] = buffer[at_i];
            at_i++;
        }
        recursive_field_name[write_i] = '\0';
        
        if (!T1_meta_string_starts_with(buffer + at_i, " = ")) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        at_i += 3;
        
        write_i = 0;
        while (
            buffer[at_i] != ';' &&
            buffer[at_i] != ' ' &&
            buffer[at_i] != '\n' &&
            buffer[at_i] != '\0')
        {
            value_to_assign[write_i++] = buffer[at_i];
            at_i++;
        }
        value_to_assign[write_i] = '\0';
        
        if (write_i < 1) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            return;
        }
        
        if (value_to_assign[write_i-1] == 'f')
        {
            value_to_assign[write_i-1] = '\0';
            
            T1MetaFieldInternal field;
            T1_meta_get_field_recursive(
                &field,
                struct_name,
                recursive_field_name,
                good);
            
            if (field.internal_field->data_type != T1_TYPE_F32 ||
                !*good)
            {
                return;
            }
            *good = 0;
        }
        
        T1_meta_write_to_known_field_str(
            /* const char * target_parent_type: */
                struct_name,
            /* const char * target_field_name: */
                recursive_field_name,
            /* const char * value_to_write_str: */
                value_to_assign,
            /* void * target_parent_ptr: */
                recipient,
            /* u8 * good: */
                good);
        if (!*good) { return; } else { *good = 1; }
        
        if (!T1_meta_string_starts_with(buffer + at_i, ";\n")) {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            *good = 0;
            return;
        }
        at_i += 2;
    }
    
    *good = 1;
}

char * T1_meta_enum_uint_to_string(
    const char * enum_type_name,
    const u64 value,
    u8 * good)
{
    *good = 0;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(enum_type_name != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    if (enum_type_name == NULL) {
        *good = 0;
        return NULL;
    }
    
    u16 parent_enum_id = UINT16_MAX;
    for (u16 i = 0; i < t1ms->meta_enums_size; i++) {
        if (
            t1ms->fp_strcmp(
                t1ms->meta_enums[i].name,
                enum_type_name) == 0)
        {
            parent_enum_id = i;
            break;
        }
    }
    
    if (parent_enum_id == UINT16_MAX) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // no such enum
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return NULL;
    }
    
    for (u16 i = 0; i < t1ms->meta_enum_vals_size; i++) {
        if (
            t1ms->meta_enum_vals[i].metaenum_id == parent_enum_id &&
            t1ms->meta_enum_vals[i].value == (s64)value)
        {
            *good = 1;
            return t1ms->meta_enum_vals[i].name;
        }
    }
    
    return NULL;
}
