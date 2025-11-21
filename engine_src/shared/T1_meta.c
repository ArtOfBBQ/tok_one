#include "T1_meta.h"

#define T1_META_ARRAY_SLICE_NONE UINT16_MAX

typedef struct {
    char * name;
    union {
        char * struct_type_name;
        char * enum_type_name;
    };
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
    uint32_t offset;
    uint16_t parent_enum_id;
    uint16_t next_i;
    uint16_t array_sizes[T1_META_ARRAY_SIZES_CAP];
    T1Type type;
} MetaField;

typedef struct {
    char * name;
    int64_t value;
    uint16_t metaenum_id;
} MetaEnumValue;

typedef struct {
    char * name;
    T1Type T1_type;
} MetaEnum;

typedef struct {
    char * name;
    uint32_t size_bytes;
    uint16_t head_fields_i;
} MetaStruct;

typedef struct {
    T1MetaField public;
    MetaStruct * internal_parent;
    MetaField * internal_field;
} T1MetaFieldInternal;

typedef struct {
    uint32_t parent_offset;
    uint16_t field_array_slices[T1_META_ARRAY_SIZES_CAP];
    uint16_t prefix_start_i;
    uint16_t field_i;
} StackedFieldEntry;

#define T1_META_SERIAL_FIELD_PREFIX_CAP 256
typedef struct {
    void * to_serialize;
    char * buffer;
    uint32_t * buffer_size;
    uint32_t * good;
    uint32_t buffer_cap;
    StackedFieldEntry field_stack[256];
    uint16_t field_stack_size;
    char field_prefix[T1_META_SERIAL_FIELD_PREFIX_CAP];
} T1MetaSerializationState;

typedef struct {
    T1MetaSerializationState ss;
    void *(* memcpy)(void *, const void *, size_t);
    void *(* memset)(void *, int, size_t);
    int (* strcmp)(const char *, const char *);
    size_t (* strlen)(const char *);
    unsigned long long int (* strtoull)(const char*, char**, int);
    char * ascii_store;
    MetaEnum * meta_enums;
    MetaEnumValue * meta_enum_vals;
    MetaField * metafields_store;
    MetaStruct * metastructs;
    uint32_t ascii_store_cap;
    uint16_t meta_fields_store_cap;
    uint16_t meta_structs_cap;
    uint16_t meta_enums_cap;
    uint16_t meta_enum_vals_cap;
    uint16_t meta_fields_size;
    uint16_t meta_structs_size; // first memset target
    uint16_t meta_enums_size;
    uint16_t meta_enum_vals_size;
    uint32_t ascii_store_next_i;
} T1MetaState;

static T1MetaState * t1ms = NULL;

static void construct_metastruct(MetaStruct * to_construct) {
    t1ms->memset(to_construct, 0, sizeof(MetaStruct));
    to_construct->head_fields_i = UINT16_MAX;
}

static void construct_public_metafield(T1MetaField * to_construct) {
    t1ms->memset(to_construct, 0, sizeof(T1MetaField));
    
    to_construct->data_type = T1_TYPE_NOTSET;
}

static void construct_metafield(MetaField * to_construct) {
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(to_construct != NULL);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    t1ms->memset(to_construct, 0, sizeof(MetaField));
    
    to_construct->next_i = UINT16_MAX;
    to_construct->type = T1_TYPE_NOTSET;
    to_construct->offset = UINT16_MAX;
    to_construct->parent_enum_id = UINT16_MAX;
}

static MetaField * metafield_i_to_ptr(const uint16_t field_i) {
    return field_i == UINT16_MAX ?
        NULL :
        &t1ms->metafields_store[field_i];
}

static void T1_meta_reset(void) {
    t1ms->memset(
        (char *)t1ms +
            offsetof(T1MetaState, meta_structs_size),
            0,
            sizeof(T1MetaState) -
                offsetof(T1MetaState, meta_structs_size));
    
    t1ms->memset(
        t1ms->metastructs, 0, t1ms->meta_structs_cap);
    t1ms->memset(
        t1ms->meta_enums, 0, t1ms->meta_enums_cap);
    t1ms->memset(
        t1ms->ascii_store, 0, t1ms->ascii_store_cap);
    
    for (uint32_t i = 0; i < t1ms->meta_fields_store_cap; i++) {
        construct_metafield(&t1ms->metafields_store[i]);
    }
}

void T1_meta_init(
    void *(* T1_meta_memcpy)(void *, const void *, size_t),
    void *(* T1_meta_malloc_func)(size_t),
    void *(* T1_meta_memset_func)(void *, int, size_t),
    int (* T1_meta_strcmp_func)(const char *, const char *),
    size_t (* T1_meta_strlen_func)(const char *),
    unsigned long long int (* T1_meta_strtoull_func)(const char*, char**, int),
    const uint32_t ascii_store_cap,
    const uint16_t meta_structs_cap,
    const uint16_t meta_fields_cap,
    const uint16_t meta_enums_cap,
    const uint16_t meta_enum_vals_cap)
{
    t1ms = T1_meta_malloc_func(sizeof(T1MetaState));
    
    t1ms->memcpy = T1_meta_memcpy;
    t1ms->memset =  T1_meta_memset_func;
    t1ms->strcmp = T1_meta_strcmp_func;
    t1ms->strlen = T1_meta_strlen_func;
    t1ms->strtoull = T1_meta_strtoull_func;
    
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
static uint64_t T1_meta_string_to_u64(
    const char * input,
    uint32_t * good)
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
    
    uint64_t return_value = 0;
    
    uint32_t i = 0;
    while (input[i] != '\0') {
        if (input[i] < '0' || input[i] > '9') {
            return UINT64_MAX;
        }
        
        return_value *= 10;
        return_value += (uint64_t)(input[i] - '0');
        i++;
    }
    
    *good = 1;
    return return_value;
}
#endif

static double T1_meta_string_to_double(
    const char * input,
    uint32_t * good)
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
    
    double return_value = 0;
    
    uint32_t i = 0;
    uint32_t dot_loc = UINT32_MAX;
    float final_mult = 1.0f;
    
    if (input[i] == '-') {
        final_mult = -1.0f;
        i++;
    }
    
    while (input[i] != '\0') {
        return_value *= 10.0f;
        
        if (input[i] < '0' || input[i] > '9') {
            return return_value;
        }
        
        uint8_t val = (uint8_t)(input[i] - '0');
        return_value += (float)val;
        
        i++;
        
        if (input[i] == '.') {
            dot_loc = i;
            i++;
            break;
        }
    }
    
    if (dot_loc < UINT32_MAX) {
        float div = 1.0f;
        
        float below_decimal = 0.0f;
        
        while (input[i] != '\0') {
            if (input[i] < '0' || input[i] > '9') {
                return return_value;
            }
            
            uint8_t val = (uint8_t)(input[i] - '0');
            below_decimal *= 10.0f;
            below_decimal += (float)val;
            
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
    const uint64_t input,
    char * recipient,
    const uint32_t recipient_cap,
    uint32_t * good)
{
    *good = 0;
    
    uint64_t mult = 1;
    int32_t recip_i = 0;
    
    // store chars right to left
    while (mult == 1 || mult <= input) {
        uint32_t rightmost = (input / mult) % 10;
        if (recip_i + 1 >= (int32_t)recipient_cap) {
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
        
        recipient[recip_i++] = '0' + (uint8_t)rightmost;
        mult *= 10;
    }
    recipient[recip_i] = '\0';
    
    // reverse chars
    int32_t j = recip_i-1;
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
    int64_t input,
    char * recipient,
    uint32_t recipient_cap,
    uint32_t * good)
{
    if (input < 0) {
        *recipient = '-';
        *recipient += 1;
        recipient_cap -= 1;
        input *= -1;
    }
    
    T1_meta_uint_to_string((uint64_t)input, recipient, recipient_cap, good);
}

static void T1_meta_float_to_string(
    float input,
    const uint8_t precision,
    char * recipient,
    uint32_t recipient_cap,
    uint32_t * good)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(precision <= 15);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    *good = 0;
    
    size_t rlen = 0;
    
    if (input < 0.0f) {
        recipient[rlen++] = '-';
        recipient[rlen] = '\0';
        input *= -1.0f;
    }
    
    uint32_t precision_mult = 1;
    for (uint8_t _ = 0; _ < precision; _++) {
        precision_mult *= 10;
    }
    
    float temp_above_decimal = (float)(int32_t)input;
    uint32_t above_decimal = (uint32_t)temp_above_decimal;
    // we're adding an extra '1' in front of the fractional part here, to
    // make it easier to work with leading zeros
    uint32_t below_decimal =
        (uint32_t)(((input - temp_above_decimal)+1.0f) * precision_mult);
    
    T1_meta_uint_to_string(
        above_decimal,
        recipient + rlen,
        recipient_cap,
        good);
    rlen = t1ms->strlen(recipient);
    
    if (below_decimal > 0) {
        recipient[rlen++] = '.';
        
        T1_meta_uint_to_string(
            below_decimal,
            recipient + rlen,
            recipient_cap - (uint32_t)rlen,
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
    size_t single_element_size,
    uint32_t array_size)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(array_size > 0);
    assert(single_element_size > 0);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    uint32_t i = 0;
    uint32_t j = array_size-1;
    
    char swap[single_element_size];
    
    while (i < j) {
        t1ms->memcpy(
            swap,
            array + (i * single_element_size),
            single_element_size);
        
        t1ms->memcpy(
            array + (i * single_element_size),
            array + (j * single_element_size),
            single_element_size);
        
        t1ms->memcpy(
            array + (j * single_element_size),
            swap,
            single_element_size);
        
        i++;
        j--;
    }
}

static uint8_t T1_meta_string_starts_with(
    const char * string,
    const char * start_pattern)
{
    uint32_t i = 0;
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
    uint32_t * good)
{
    *good = 0;
    
    size_t len = t1ms->strlen(to_copy);
    
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
        #elif T1_META_ASSERTS == T1_ACTIVE
        #else
        #error
        #endif
        return NULL;
    }
    
    char * return_value = t1ms->ascii_store + t1ms->ascii_store_next_i;
    
    t1ms->memcpy(return_value, to_copy, len);
    
    return_value[len] = '\0';
    
    t1ms->ascii_store_next_i += (len + 1);
    
    *good = 1;
    return return_value;
}

static MetaStruct * find_struct_by_name(
    const char * struct_name)
{
    MetaStruct * return_value = NULL;
    
    for (int32_t i = 0; i < (int32_t)t1ms->meta_structs_size; i++) {
        if (
            t1ms->strcmp(
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
    const char * property_name)
{
    MetaField * return_value = NULL;
    
    MetaField * i = metafield_i_to_ptr(in_struct->head_fields_i);
    
    while (i != NULL)
    {
        if (
            i->name != NULL &&
            t1ms->strcmp(
                i->name,
                property_name) == 0)
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
    const T1Type T1_type,
    const uint32_t type_size_check,
    uint32_t * good)
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
    const int64_t value,
    uint32_t * good)
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
    for (uint16_t i = 0; i < t1ms->meta_enums_size; i++) {
        if (
            t1ms->strcmp(
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
    const uint32_t size_bytes,
    uint32_t * good)
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
    const uint32_t field_offset,
    const T1Type field_type,
    const char * field_struct_type_name_or_null,
    const uint16_t field_array_size_1,
    const uint16_t field_array_size_2,
    const uint16_t field_array_size_3,
    const uint8_t is_enum,
    uint32_t * good)
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
    for (uint32_t i = 0; i < UINT16_MAX; i++) {
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
    MetaField * target_mfield = find_field_in_struct_by_name(
        target_mstruct,
        field_name);
    
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
        uint16_t target_mfield_i = t1ms->meta_fields_size;
        t1ms->meta_fields_size += 1;
        
        construct_metafield(target_mfield);
        
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
    
    target_mfield->type = field_type;
    switch (target_mfield->type) {
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
            target_mfield->custom_float_max = 3.40282347E+38f;
            target_mfield->custom_float_min = -3.40282347E+38f;
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
    for (uint32_t i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
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
    
    if (target_mfield->type == T1_TYPE_STRUCT) {
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
                /* uint32_t * good: */
                    good);
        if (!*good) { return; }
        *good = 0;
    } else if (is_enum) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(field_struct_type_name_or_null != NULL);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        if (field_struct_type_name_or_null == NULL) {
            *good = 0;
            return;
        }
        
        target_mfield->parent_enum_id = UINT16_MAX;
        for (uint16_t i = 0; i < t1ms->meta_enums_size; i++) {
            if (
                t1ms->strcmp(
                    field_struct_type_name_or_null,
                    t1ms->meta_enums[i].name) == 0)
            {
                target_mfield->enum_type_name =
                    t1ms->meta_enums[i].name;
                target_mfield->parent_enum_id = i;
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

void T1_meta_reg_float_limits_for_last_field(
    const double floating_min,
    const double floating_max,
    uint32_t * good)
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
    
    if (target_mfield->type != T1_TYPE_F32) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set float limits, not a floating point field
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    if (floating_min >= floating_max) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    target_mfield->custom_float_max = (float)floating_max;
    target_mfield->custom_float_min = (float)floating_min;
    
    *good = 1;
}

void T1_meta_reg_int_limits_for_last_field(
    const int64_t int_min,
    const int64_t int_max,
    uint32_t * good)
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
        target_mfield->type != T1_TYPE_I64 &&
        target_mfield->type != T1_TYPE_I32 &&
        target_mfield->type != T1_TYPE_I16 &&
        target_mfield->type != T1_TYPE_I8)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set int limits, not an int field
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
    const uint64_t uint_min,
    const uint64_t uint_max,
    uint32_t * good)
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
        target_mfield->type != T1_TYPE_U64 &&
        target_mfield->type != T1_TYPE_U32 &&
        target_mfield->type != T1_TYPE_U16 &&
        target_mfield->type != T1_TYPE_U8)
    {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0); // can't set int limits, not an int field
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

static void strip_array_brackets_and_get_array_indices(
    char * to_strip,
    uint32_t * array_indices,
    uint32_t * array_indices_found)
{
    size_t field_name_len = t1ms->strlen(to_strip);
    uint8_t is_array = 0;
    t1ms->memset(
        array_indices,
        0,
        sizeof(uint32_t) * T1_META_ARRAY_SIZES_CAP);
    
    while (
        field_name_len > 3 &&
        to_strip[field_name_len-1] == ']' &&
        to_strip[field_name_len-2] >= '0' &&
        to_strip[field_name_len-2] <= '9')
    {
        for (uint32_t i = T1_META_ARRAY_SIZES_CAP-1; i >= 1; i--) {
            array_indices[i] = array_indices[i-1];
        }
        
        uint32_t mult = 1;
        uint32_t total = 0;
        
        size_t i = field_name_len-2;
        while (
            to_strip[i] >= '0'
            && to_strip[i] <= '9' &&
            i > 2)
        {
            total += ((uint32_t)(to_strip[i] - '0') * mult);
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

static uint32_t array_indices_to_flat_array_index(
    uint32_t * array_indices,
    MetaField * field)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    for (uint32_t i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
        assert(array_indices[i] < field->array_sizes[i]);
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    uint32_t return_val = 0;
    
    for (int32_t i = 0; i < T1_META_ARRAY_SIZES_CAP; i++) {
        uint32_t offset_per_slice = 1;
        for (int32_t j = i+1; j < T1_META_ARRAY_SIZES_CAP; j++) {
            offset_per_slice *= field->array_sizes[j];
        }
        
        return_val += (offset_per_slice * array_indices[i]);
    }
    
    return return_val;
}

static void copy_internal_field_to_public_field(
    const MetaField * internal,
    T1MetaField * public)
{
    public->offset = internal->offset;
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(internal->offset >= 0);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    public->custom_int_max   = internal->custom_int_max;
    public->custom_int_min   = internal->custom_int_min;
    public->name             = internal->name;
    public->struct_type_name = internal->struct_type_name;

    public->array_sizes[0] = internal->array_sizes[0];
    public->array_sizes[1] = internal->array_sizes[1];
    public->array_sizes[2] = internal->array_sizes[2];
    public->data_type = internal->type;
    public->is_enum = internal->parent_enum_id < t1ms->meta_enums_size;
    
    #if T1_META_ASSERTS == T1_ACTIVE
    switch (internal->type) {
        case T1_TYPE_STRUCT:
        case T1_TYPE_CHAR:
        break;
        case T1_TYPE_F32:
            assert(internal->custom_float_max > internal->custom_float_min);
        break;
        case T1_TYPE_I64:
        case T1_TYPE_I32:
        case T1_TYPE_I16:
        case T1_TYPE_I8:
            assert(internal->custom_int_max > internal->custom_int_min);
        break;
        case T1_TYPE_U64:
        case T1_TYPE_U32:
        case T1_TYPE_U16:
        case T1_TYPE_U8:
            assert(internal->custom_uint_max > internal->custom_uint_min);
        break;
        case T1_TYPE_NOTSET:
            assert(0);
        break;
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    #if T1_META_ASSERTS == T1_ACTIVE
    if (public->data_type == T1_TYPE_STRUCT) {
        assert(public->struct_type_name != NULL);
        assert(public->struct_type_name[0] != '\0');
    }
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    
    /*
    return_value->public.name = return_value->internal_field->name;
    return_value->public.offset += (uint32_t)metafield->offset +
        (offset_per_array_index * flat_array_index);
    return_value->public.custom_uint_max = metafield->custom_uint_max;
    return_value->public.custom_uint_min = metafield->custom_uint_min;
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(return_value->public.offset >= 0);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    return_value->public.data_type = metafield->type;
    return_value->public.array_sizes[0] = metafield->array_sizes[0];
    return_value->public.array_sizes[1] = metafield->array_sizes[1];
    return_value->public.array_sizes[2] = metafield->array_sizes[2];
    while (array_indices_found > 0) {
        return_value->public.array_sizes[0] = return_value->public.array_sizes[1];
        return_value->public.array_sizes[1] = return_value->public.array_sizes[2];
        return_value->public.array_sizes[2] = 1;
        array_indices_found -= 1;
    }
     */
}

static void T1_meta_get_field_recursive(
    T1MetaFieldInternal * return_value,
    const char * struct_name,
    const char * field_name,
    uint32_t * good)
{
    *good = 0;
    
    MetaStruct * metastruct = find_struct_by_name(struct_name);
    return_value->internal_parent = metastruct;
    
    if (metastruct == NULL) {
        construct_public_metafield(&return_value->public);
        return_value->internal_field = NULL;
        return_value->internal_parent = NULL;
        return;
    }
    
    // The field name may include a dot, in which case we only
    // search for the first part, then do a recursion
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
    
    uint32_t dot_i = 0;
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
    
    uint32_t array_indices[T1_META_ARRAY_SIZES_CAP];
    uint32_t array_indices_found = 0;
    strip_array_brackets_and_get_array_indices(
        first_part,
        array_indices,
        &array_indices_found);
    
    MetaField * metafield = find_field_in_struct_by_name(
        metastruct, first_part);
    
    if (metafield == NULL) {
        construct_public_metafield(&return_value->public);
        return_value->internal_field = NULL;
        *good = 1;
        return;
    }
    
    return_value->internal_field = metafield;
    
    uint32_t flat_array_index = array_indices_to_flat_array_index(
        array_indices,
        metafield);
    
    uint32_t offset_per_array_index = 0;
    if (flat_array_index > 0 ) {
        MetaStruct * substruct = NULL;
        switch (metafield->type) {
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
                    return;
                }
                offset_per_array_index = substruct->size_bytes;
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
                return;
        }
        
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(offset_per_array_index > 0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
    }
    
    int64_t previous_offset = return_value->public.offset;
    copy_internal_field_to_public_field(
        return_value->internal_field,
        &return_value->public);
    
    return_value->public.offset += previous_offset;
    
    return_value->public.offset += (offset_per_array_index * flat_array_index);
    
    if (second_part != NULL) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(return_value->public.data_type == T1_TYPE_STRUCT);
        assert(metafield->struct_type_name != NULL);
        assert(metafield->struct_type_name[0] != '\0');
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        
        T1_meta_get_field_recursive(
            return_value,
            metafield->struct_type_name,
            second_part,
            good);
        if (!*good) { return; }
    }
    
    *good = 1;
}

static void construct_T1_metafield(
    T1MetaField * to_construct)
{
    t1ms->memset(to_construct, 0, sizeof(T1MetaField));
}

T1MetaField T1_meta_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    uint32_t * good)
{
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(struct_name != NULL);
    assert(struct_name[0] != '"');
    assert(field_name != NULL);
    assert(field_name[0] != '"');
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
    
    T1MetaFieldInternal return_value;
    construct_T1_metafield(&return_value.public);
    
    // T1_meta_get_field_recursive() will push to the ascii store
    // as if it were stack memory
    uint32_t before_recursion_ascii_store_next_i = t1ms->ascii_store_next_i;
    
    T1_meta_get_field_recursive(
        &return_value,
        struct_name,
        field_name,
        good);
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(good);
    #elif T1_META_ASSERTS == T1_INACTIVE
    #else
    #error
    #endif
        
    // discard or "pop" T1_meta_get_field_recursive()'s strings
    t1ms->ascii_store_next_i = before_recursion_ascii_store_next_i;
    
    *good = 1;
    return return_value.public;
}

static size_t T1_meta_shared_get_element_size_bytes(
    T1Type data_type,
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
static size_t T1_meta_field_get_element_size_bytes(T1MetaField * field) {
    size_t return_value = 0;
    
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

static size_t T1_meta_internal_field_get_element_size_bytes(MetaField * field) {
    return T1_meta_shared_get_element_size_bytes(
        field->type,
        field->struct_type_name);
}

typedef struct {
    uint64_t value_u64;
    int64_t  value_i64;
    double   value_f64;
    uint8_t  value_is_i64;
    uint8_t  value_is_u64;
    uint8_t  value_is_f64;
} T1MetaParsedvalue;

static void parse_string_to_value(
    T1MetaParsedvalue * recipient,
    const char * string)
{
    uint32_t is_f64 = 0;
    recipient->value_f64 = T1_meta_string_to_double(string, &is_f64);
    recipient->value_is_f64 = (uint8_t)is_f64;
    
    uint8_t val_starts_minus = string[0] == '-';
    if (val_starts_minus) { string++; }
    
    char * endptr = NULL;
    recipient->value_u64 = (uint64_t)t1ms->strtoull(string, &endptr, 10);
    recipient->value_is_u64 = *endptr == '\0' && !val_starts_minus;
    
    recipient->value_is_i64 =
        *endptr == '\0' &&
        recipient->value_u64 < INT64_MAX;
    
    recipient->value_i64 = (int64_t)recipient->value_u64;
    if (val_starts_minus) { recipient->value_i64 *= -1; }
}

void T1_meta_write_to_known_field_str(
    const char * target_parent_type,
    const char * target_field_name,
    const char * value_to_write_str,
    void * target_parent_ptr,
    uint32_t * good)
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
    field.public.data_type = T1_TYPE_NOTSET;
    field.public.array_sizes[0] = 0;
    field.public.array_sizes[1] = 0;
    field.public.array_sizes[2] = 0;
    field.public.offset = 0;
    field.internal_field = NULL;
    field.internal_parent = NULL;
    
    // T1_meta_get_field_recursive() will push to the ascii store
    // as if it were stack memory
    uint32_t before_recursion_ascii_store_next_i = t1ms->ascii_store_next_i;
    
    T1_meta_get_field_recursive(
        &field,
        target_parent_type,
        target_field_name,
        good);
    
    #if T1_META_ASSERTS == T1_ACTIVE
    assert(good);
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
    T1Type type_adj = field.public.data_type;
    uint8_t found_enum_field = 0;
    
    if (field.public.is_enum) {
        if (
            field.internal_field == NULL ||
            field.internal_field->parent_enum_id >=
                t1ms->meta_enums_size ||
            field.internal_field->parent_enum_id >=
                t1ms->meta_enums_cap)
        {
            #if T1_META_ASSERTS == T1_ACTIVE
            assert(0);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
        }
        parent_enum = &t1ms->meta_enums[field.internal_field->parent_enum_id];
        
        for (
            uint32_t i = 0;
            i < t1ms->meta_enum_vals_size;
            i++)
        {
            if (
                t1ms->meta_enum_vals[i].
                    metaenum_id ==
                        field.internal_field->
                            parent_enum_id &&
                t1ms->strcmp(
                    t1ms->meta_enum_vals[i].name,
                    value_adj) == 0)
            {
                found_enum_field = 1;
                parsed.value_i64 =
                    t1ms->meta_enum_vals[i].value;
                parsed.value_u64 = (uint64_t)t1ms->meta_enum_vals[i].value;
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
        field.public.data_type != T1_TYPE_CHAR &&
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
    
    int32_t rightmost_array_i = 2;
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
            
            float value_f32 = (float)parsed.value_f64;
            t1ms->memcpy(
                (float *)((char *)target_parent_ptr + field.public.offset),
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
            int8_t value_i8 = (int8_t)parsed.value_i64;
            t1ms->memcpy(
                (int8_t *)((char *)target_parent_ptr + field.public.offset),
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
            int16_t value_i16 = (int16_t)parsed.value_i64;
            t1ms->memcpy(
                (int16_t *)((char *)target_parent_ptr + field.public.offset),
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
            int32_t value_i32 = (int32_t)parsed.value_i64;
            t1ms->memcpy(
                (int32_t *)((char *)target_parent_ptr + field.public.offset),
                &value_i32,
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
            uint8_t value_u8 = (uint8_t)parsed.value_u64;
            t1ms->memcpy(
                (uint8_t *)((char *)target_parent_ptr + field.public.offset),
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
            uint16_t value_u16 = (uint16_t)parsed.value_u64;
            t1ms->memcpy(
                (uint16_t *)((char *)target_parent_ptr + field.public.offset),
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
            t1ms->memcpy(
                (uint32_t *)((char *)target_parent_ptr + field.public.offset),
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
            
            t1ms->memcpy(
                (uint64_t *)((char *)target_parent_ptr + field.public.offset),
                &parsed.value_u64,
                8);
        break;
        case T1_TYPE_CHAR:
            rightmost_array_i = 2;
            while (field.public.array_sizes[rightmost_array_i] < 2) {
                rightmost_array_i -= 1;
            }
            
            if (
                t1ms->strlen(value_to_write_str) >
                    field.public.array_sizes[rightmost_array_i])
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
            assert(t1ms->memcpy != NULL);
            #elif T1_META_ASSERTS == T1_INACTIVE
            #else
            #error
            #endif
            t1ms->memcpy(
                ((char *)target_parent_ptr + field.public.offset),
                value_to_write_str,
                field.public.array_sizes[rightmost_array_i]);
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

uint32_t internal_T1_meta_get_num_of_fields_in_struct(
    const char * struct_name)
{
    MetaStruct * parent = find_struct_by_name(struct_name);
    
    if (parent == NULL) { return 0; }
    
    uint32_t fields_size = 0;
    MetaField * field = metafield_i_to_ptr(parent->head_fields_i);
    while (field != NULL) {
        fields_size += 1;
        field = metafield_i_to_ptr(field->next_i);
    }
    
    return fields_size;
}

T1MetaField T1_meta_get_field_at_index(
    char * parent_name_str,
    uint32_t at_index)
{
    T1MetaField return_value;
    construct_T1_metafield(&return_value);
    
    uint32_t i = 0;
    
    MetaStruct * parent = find_struct_by_name(parent_name_str);
    
    if (parent == NULL) { return return_value; }
    
    MetaField * field = metafield_i_to_ptr(parent->head_fields_i);
    if (field == NULL) { return return_value; }
    
    while (i < at_index) {
        field = metafield_i_to_ptr(field->next_i);
        if (field == NULL) { return return_value; }
        
        i++;
    }
    
    copy_internal_field_to_public_field(field, &return_value);
    
    return return_value;
}

static void T1_meta_serialize_cat_str_to_buf(
    const char * to_copy)
{
    *t1ms->ss.good = 0;
    
    size_t to_copy_len = t1ms->strlen(to_copy);
    
    if (*t1ms->ss.buffer_size + to_copy_len + 1 >= t1ms->ss.buffer_cap) {
        #if T1_META_ASSERTS == T1_ACTIVE
        assert(0);
        #elif T1_META_ASSERTS == T1_INACTIVE
        #else
        #error
        #endif
        return;
    }
    
    t1ms->memcpy(
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
    
    size_t to_copy_len = t1ms->strlen(to_copy);
    
    size_t field_prefix_len = t1ms->strlen(t1ms->ss.field_prefix);
    
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
    
    t1ms->memcpy(
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
    uint16_t field_array_sizes[T1_META_ARRAY_SIZES_CAP],
    const char * field_dot_or_arrow,
    const uint32_t parent_offset)
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
    t1ms->memset(slice_text, 0, 32);
    
    if (field_array_sizes[0] != T1_META_ARRAY_SLICE_NONE) {
        slice_text[0] = '[';
        T1_meta_int_to_string(
            field_array_sizes[0],
            slice_text+1,
            31,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        slice_text[t1ms->strlen(slice_text)] = ']';
    }
    size_t len = t1ms->strlen(slice_text);
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
            32 - (uint32_t)len,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        len = t1ms->strlen(slice_text);
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
            32 - (uint32_t)len,
            t1ms->ss.good);
        if (!*t1ms->ss.good) { return; } else { *t1ms->ss.good = 0; }
        len = t1ms->strlen(slice_text);
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
    
    uint32_t to_reverse_start_i = t1ms->ss.field_stack_size;
    uint16_t prefix_start_i = (uint16_t)t1ms->strlen(
        t1ms->ss.field_prefix);
    
    uint16_t field_i = parent->head_fields_i;
    
    uint32_t array_slices[T1_META_ARRAY_SIZES_CAP];
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
            uint32_t flat_i = array_indices_to_flat_array_index(
                array_slices,
                field);
            
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].parent_offset =
                parent_offset + (
                    flat_i * (uint32_t)
                        T1_meta_internal_field_get_element_size_bytes(field));
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[0] = field->array_sizes[0] > 1 ?
                    (uint16_t)array_slices[0] :
                    T1_META_ARRAY_SLICE_NONE;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[1] = field->array_sizes[1] > 1 ?
                    (uint16_t)array_slices[1] :
                    T1_META_ARRAY_SLICE_NONE;
            t1ms->ss.field_stack[t1ms->ss.field_stack_size].
                field_array_slices[2] = field->array_sizes[2] > 1 ?
                    (uint16_t)array_slices[2] :
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
    uint32_t * buffer_size,
    uint32_t buffer_cap,
    uint32_t * good)
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
    uint16_t slices_i[T1_META_ARRAY_SIZES_CAP];
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
    
    float value_f32;
    uint64_t value_u64;
    int64_t value_i64;
    
    while (t1ms->ss.field_stack_size > 0) {
        
        StackedFieldEntry top =
            t1ms->ss.field_stack[t1ms->ss.field_stack_size-1];
        t1ms->ss.field_stack_size -= 1;
        t1ms->ss.field_prefix[top.prefix_start_i] = '\0';
        
        field = metafield_i_to_ptr(top.field_i);
        
        if (field->type == T1_TYPE_STRUCT) {
                
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
            t1ms->memset(value_as_str, 0, 128);
            
            if (top.field_array_slices[0] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[0],
                    /* char * recipient: */
                        value_as_str,
                    /* uint32_t recipient_cap: */
                        128,
                    /* uint32_t * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->memset(value_as_str, 0, 128);
            }
            if (top.field_array_slices[1] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[1],
                    /* char * recipient: */
                        value_as_str,
                    /* uint32_t recipient_cap: */
                        128,
                    /* uint32_t * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->memset(value_as_str, 0, 128);
            }
            if (top.field_array_slices[2] != T1_META_ARRAY_SLICE_NONE) {
                T1_meta_serialize_cat_str_to_buf("[");
                T1_meta_int_to_string(
                        top.field_array_slices[2],
                    /* char * recipient: */
                        value_as_str,
                    /* uint32_t recipient_cap: */
                        128,
                    /* uint32_t * good: */
                        good);
                if (!*good) { return; } else { *good = 0; }
                T1_meta_serialize_cat_str_to_buf(value_as_str);
                T1_meta_serialize_cat_str_to_buf("]");
                t1ms->memset(value_as_str, 0, 128);
            }
            
            T1_meta_serialize_cat_str_to_buf(" = ");
            
            if (field->parent_enum_id != UINT16_MAX) {
                uint8_t val_u8 = *(uint8_t *)(((char *)to_serialize) +
                    field->offset + top.parent_offset);
                for (
                    uint32_t mev_i = 0;
                    mev_i < t1ms->meta_enum_vals_size;
                    mev_i++)
                {
                    if (
                        t1ms->meta_enums[field->parent_enum_id].T1_type !=
                            T1_TYPE_U8 ||
                        field->type != T1_TYPE_U8)
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
                        
                        size_t enumvalstrlen = t1ms->strlen(t1ms->meta_enum_vals[mev_i].name);
                        
                        #if T1_META_ASSERTS == T1_ACTIVE
                        assert(enumvalstrlen > 0);
                        #elif T1_META_ASSERTS == T1_INACTIVE
                        #else
                        #error
                        #endif
                        
                        t1ms->memcpy(
                            value_as_str,
                            t1ms->meta_enum_vals[mev_i].name,
                            enumvalstrlen);
                        break;
                    }
                }
            } else {
                switch (field->type) {
                    case T1_TYPE_F32:
                        value_f32 = *(float *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_float_to_string(
                            /* float input: */
                                value_f32,
                            /* const uint8_t precision: */
                                4,
                            /* char * recipient: */
                                value_as_str,
                            /* uint32_t recipient_cap: */
                                128,
                            /* uint32_t * good: */
                                good);
                        if (!*good) { return; } else { *good = 0; }
                        
                        size_t vaslen = t1ms->strlen(value_as_str);
                        
                        if (vaslen + 1 >= 128) {
                            return;
                        }
                        
                        value_as_str[vaslen] = 'f';
                        value_as_str[vaslen+1] = '\0';
                    break;
                    case T1_TYPE_U64:
                        value_u64 = *(uint64_t *)(((char *)to_serialize) +
                        field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(
                            value_u64, value_as_str, 128, good);
                        
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U32:
                        value_u64 = *(uint32_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U16:
                        value_u64 = *(uint16_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_U8:
                        value_u64 = *(uint8_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_uint_to_string(value_u64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I64:
                        value_i64 = *(int64_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I32:
                        value_i64 = *(int32_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I16:
                        value_i64 = *(int16_t *)(((char *)to_serialize) +
                            field->offset + top.parent_offset);
                        
                        T1_meta_int_to_string(value_i64, value_as_str, 128, good);
                        if (!*good) { return; } else { *good = 0; }
                    break;
                    case T1_TYPE_I8:
                        value_i64 = *(int8_t *)(((char *)to_serialize) +
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
    const uint32_t buffer_size,
    uint32_t * good)
{
    *good = 0;
    
    uint32_t at_i = 0;
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
    at_i += t1ms->strlen(struct_name);
    
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
    uint32_t write_i;
    
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
            
            if (field.public.data_type != T1_TYPE_F32 ||
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
            /* uint32_t * good: */
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
    const uint64_t value,
    uint32_t * good)
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
    
    uint16_t parent_enum_id = UINT16_MAX;
    for (uint16_t i = 0; i < t1ms->meta_enums_size; i++) {
        if (
            t1ms->strcmp(
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
    
    for (uint16_t i = 0; i < t1ms->meta_enum_vals_size; i++) {
        if (
            t1ms->meta_enum_vals[i].metaenum_id == parent_enum_id &&
            t1ms->meta_enum_vals[i].value == (int64_t)value)
        {
            *good = 1;
            return t1ms->meta_enum_vals[i].name;
        }
    }
    
    return NULL;
}
