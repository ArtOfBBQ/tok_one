#include "T1_meta.h"

typedef struct {
    char * name;
    char * struct_type_name;
    T1Type type;
    uint32_t offset;
    uint16_t next_i;
    uint16_t array_sizes[T1_REFL_MAX_ARRAY_SIZES];
} MetaField;

typedef struct {
    char * name;
    int64_t value;
    uint16_t metaenum_id;
} MetaEnumValue;

typedef struct {
    char * name;
    uint32_t size_bytes;
} MetaEnum;

typedef struct {
    char * name;
    uint32_t size_bytes;
    uint16_t head_fields_i;
} MetaStruct;

typedef struct T1ReflectionState {
    void *(* memcpy)(void *, const void *, size_t);
    void *(* memset)(void *, int, size_t);
    int (* strcmp)(const char *, const char *);
    size_t (* strlen)(const char *);
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
} T1ReflectionState;

static T1ReflectionState * t1rs = NULL;

static void construct_metastruct(MetaStruct * to_construct) {
    t1rs->memset(to_construct, 0, sizeof(MetaStruct));
    to_construct->head_fields_i = UINT16_MAX;
}

static void construct_metafield(MetaField * to_construct) {
    #if T1_META_ASSERTS
    assert(to_construct != NULL);
    #endif
    
    t1rs->memset(to_construct, 0, sizeof(MetaField));
    
    to_construct->next_i = UINT16_MAX;
    to_construct->type = T1_TYPE_NOTSET;
    to_construct->offset = UINT16_MAX;
}

static MetaField * metafield_i_to_ptr(const uint16_t field_i) {
    return field_i == UINT16_MAX ?
        NULL :
        &t1rs->metafields_store[field_i];
}

static void T1_refl_reset(void) {
    t1rs->memset(
        (char *)t1rs +
            offsetof(T1ReflectionState, meta_structs_size),
            0,
            sizeof(T1ReflectionState) -
                offsetof(T1ReflectionState, meta_structs_size));
    
    t1rs->memset(
        t1rs->metastructs, 0, t1rs->meta_structs_cap);
    t1rs->memset(
        t1rs->meta_enums, 0, t1rs->meta_enums_cap);
    t1rs->memset(
        t1rs->ascii_store, 0, t1rs->ascii_store_cap);
    
    for (uint32_t i = 0; i < t1rs->meta_fields_store_cap; i++) {
        construct_metafield(&t1rs->metafields_store[i]);
    }
}

void T1_meta_init(
    void *(* T1_meta_memcpy)(void *, const void *, size_t),
    void *(* T1_meta_malloc_func)(size_t),
    void *(* T1_meta_memset_func)(void *, int, size_t),
    int (* T1_meta_strcmp_func)(const char *, const char *),
    size_t (* T1_meta_strlen_func)(const char *),
    const uint32_t ascii_store_cap,
    const uint16_t meta_structs_cap,
    const uint16_t meta_fields_cap,
    const uint16_t meta_enums_cap,
    const uint16_t meta_enum_vals_cap)
{
    t1rs = T1_meta_malloc_func(sizeof(T1ReflectionState));
    
    t1rs->memcpy = T1_meta_memcpy;
    t1rs->memset =  T1_meta_memset_func;
    t1rs->strcmp = T1_meta_strcmp_func;
    t1rs->strlen = T1_meta_strlen_func;
    
    t1rs->ascii_store_cap = ascii_store_cap;
    t1rs->ascii_store = T1_meta_malloc_func(
        t1rs->ascii_store_cap);
    
    t1rs->meta_structs_cap = meta_structs_cap;
    t1rs->metastructs = T1_meta_malloc_func(
        sizeof(MetaStruct) * t1rs->meta_structs_cap);
    
    t1rs->meta_fields_store_cap = meta_fields_cap;
    t1rs->metafields_store = T1_meta_malloc_func(
        sizeof(MetaField) * t1rs->meta_fields_store_cap);
    
    t1rs->meta_enums_cap = meta_enums_cap;
    t1rs->meta_enums = T1_meta_malloc_func(
        sizeof(MetaEnum) * t1rs->meta_enums_cap);
    
    t1rs->meta_enum_vals_cap = meta_enum_vals_cap;
    t1rs->meta_enum_vals = T1_meta_malloc_func(
        sizeof(MetaEnumValue) * t1rs->meta_enum_vals_cap);
    
    T1_refl_reset();
}

static char * T1_refl_copy_str_to_store(
    const char * to_copy,
    uint32_t * good)
{
    *good = 0;
    
    size_t len = t1rs->strlen(to_copy);
    
    #if T1_META_ASSERTS
    assert(len > 0);
    #endif
    
    if (t1rs->ascii_store_next_i + len >= t1rs->ascii_store_cap)
    {
        #if T1_META_ASSERTS
        assert(0);
        #endif
        return NULL;
    }
    
    char * return_value = t1rs->ascii_store + t1rs->ascii_store_next_i;
    
    t1rs->memcpy(return_value, to_copy, len);
    
    return_value[len] = '\0';
    
    t1rs->ascii_store_next_i += (len + 1);
    
    *good = 1;
    return return_value;
}

static MetaStruct * find_struct_by_name(
    const char * struct_name)
{
    MetaStruct * return_value = NULL;
    
    for (int32_t i = 0; i < (int32_t)t1rs->meta_structs_size; i++) {
        if (
            t1rs->strcmp(
                t1rs->metastructs[i].name,
                struct_name) == 0)
        {
            #if T1_META_ASSERTS
            assert(return_value == 0);
            #endif
            
            return_value = t1rs->metastructs + i;
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
            t1rs->strcmp(
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
    const uint32_t size_bytes,
    uint32_t * good)
{
    *good = 0;
    
    if (enum_type_name == NULL || enum_type_name[0] == '\0') {
        #if T1_META_ASSERTS
        assert(0); // invalid argument
        #endif
        return;
    }
    
    if (t1rs->meta_enums_size + 1 >= t1rs->meta_enums_cap) {
        #if T1_META_ASSERTS
        assert(0); // not enough memory, init() with higher meta_enums_cap
        #endif
        return;
    }
    
    MetaEnum * new_enum = &t1rs->meta_enums[t1rs->meta_enums_size];
    t1rs->meta_enums_size += 1;
    
    new_enum->name = T1_refl_copy_str_to_store(enum_type_name, good);
    if (!*good) { return; } else { *good = 0; }
    
    new_enum->size_bytes = size_bytes;
    
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
        #if T1_META_ASSERTS
        assert(0); // invalid argument
        #endif
        return;
    }
    
    if (value_name == NULL || value_name[0] == '\0') {
        #if T1_META_ASSERTS
        assert(0); // invalid argument
        #endif
        return;
    }
    
    if (t1rs->meta_enum_vals_size + 1 >= t1rs->meta_enum_vals_cap) {
        #if T1_META_ASSERTS
        assert(0); // not enough memory, init() with higher meta_enums_cap
        #endif
        return;
    }
    
    MetaEnumValue * new_value = &t1rs->
        meta_enum_vals[t1rs->meta_enum_vals_size];
    t1rs->meta_enum_vals_size += 1;
    
    new_value->name = T1_refl_copy_str_to_store(value_name, good);
    if (!*good) { return; } else { *good = 0; }
    
    new_value->value = value;
    new_value->metaenum_id = UINT16_MAX;
    for (uint16_t i = 0; i < t1rs->meta_enums_size; i++) {
        if (
            t1rs->strcmp(
                t1rs->meta_enums[i].name,
                enum_type_name) == 0)
        {
            new_value->metaenum_id = i;
            break;
        }
    }
    
    if (new_value->metaenum_id >= UINT16_MAX) {
        #if T1_META_ASSERTS
        assert(0); // Tried to register enum value to non-existant enum
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
    
    #if T1_META_ASSERTS
    assert(target_mstruct == NULL); // name already taken
    #endif
    
    if (target_mstruct == NULL) {
        if (
            t1rs->meta_structs_size + 1 >= t1rs->meta_structs_cap)
        {
            *good = 0;
            return;
        }
        
        target_mstruct = t1rs->metastructs +
            t1rs->meta_structs_size;
        t1rs->meta_structs_size += 1;
        construct_metastruct(target_mstruct);
        target_mstruct->name = T1_refl_copy_str_to_store(
            struct_name,
            good);
        target_mstruct->size_bytes = size_bytes;
        if (!*good) {
            #if T1_META_ASSERTS
            // nout enough ascii store for a struct name should
            // probably never happen
            assert(0);
            #endif
            return;
        }
        *good = 0;
    }
    
    *good = 1;
}

void T1_meta_reg_field(
    const char * property_name,
    const uint32_t property_offset,
    const T1Type property_type,
    const char * property_struct_name,
    const uint16_t property_array_size_1,
    const uint16_t property_array_size_2,
    const uint16_t property_array_size_3,
    uint32_t * good)
{
    *good = 0;
    
    if (
        property_struct_name != NULL &&
        property_struct_name[0] == 'N' &&
        property_struct_name[1] == 'U' &&
        property_struct_name[2] == 'L' &&
        property_struct_name[3] == 'L' &&
        property_struct_name[4] == '\0')
    {
        property_struct_name = NULL;
    }
    
    #if T1_META_ASSERTS
    assert(property_name != NULL);
    // When invoking the macro versions of this function, call using
    // the actual struct type, not a string containing the struct type
    assert(property_name[0] != '"');
    if (property_struct_name != NULL) {
        // When invoking the macro versions of this function, call using
        // the actual type name, not a string containing the type name
        assert(property_struct_name[0] != '"');
    }
    #endif
    
    // There should be no '.' in the property, because substructs
    // should be registered separately
    for (uint32_t i = 0; i < UINT16_MAX; i++) {
        if (property_name[i] == '.') {
            #if T1_META_ASSERTS
            assert(0);
            #endif
            return;
        }
        if (property_name[i] == '\0') {
            break;
        }
        #if T1_META_ASSERTS
        assert(i < 250); // 250 character property is absurd
        #endif
    }
    
    if (t1rs->meta_structs_size < 1) {
        #if T1_META_ASSERTS
        assert(0); // use T1_meta_reg_struct() first
        #endif
        *good = 0;
        return;
    }
    
    // check for existing struct name
    MetaStruct * target_mstruct = &t1rs->metastructs[t1rs->meta_structs_size-1];
    
    // check for existing field name in that struct
    MetaField * target_mfield = find_field_in_struct_by_name(
        target_mstruct,
        property_name);
    
    if (target_mfield == NULL) {
        if (
            t1rs->meta_fields_size + 1 >=
                t1rs->meta_fields_store_cap)
        {
            return;
        }
        
        target_mfield = t1rs->metafields_store +
            t1rs->meta_fields_size;
        uint16_t target_mfield_i = t1rs->meta_fields_size;
        t1rs->meta_fields_size += 1;
        
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
    
    target_mfield->type = property_type;
    target_mfield->offset = property_offset;
    target_mfield->array_sizes[0] = property_array_size_1;
    target_mfield->array_sizes[1] = property_array_size_2;
    target_mfield->array_sizes[2] = property_array_size_3;
    #if T1_META_ASSERTS
    if (target_mfield->array_sizes[1] < 2) {
        assert(target_mfield->array_sizes[2] < 2);
    }
    if (target_mfield->array_sizes[0] < 2) {
        assert(target_mfield->array_sizes[1] < 2);
    }
    for (uint32_t i = 0; i < T1_REFL_MAX_ARRAY_SIZES; i++) {
        assert(target_mfield->array_sizes[i] > 0);
    }
    #endif
    
    target_mfield->name = T1_refl_copy_str_to_store(
        property_name,
        good);
    if (!*good) { return; }
    *good = 0;
    
    if (target_mfield->type == T1_TYPE_STRUCT) {
        #if T1_META_ASSERTS
        assert(property_struct_name != NULL);
        #endif
        if (property_struct_name == NULL) {
            *good = 0;
            return;
        }
        
        target_mfield->struct_type_name =
            T1_refl_copy_str_to_store(
                /* const char * to_copy: */
                    property_struct_name,
                /* uint32_t * good: */
                    good);
        if (!*good) { return; }
        *good = 0;
    }
    
    #if T1_META_ASSERTS
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
    #endif
    
    *good = 1;
}

static void strip_array_brackets_and_get_array_indices(
    char * to_strip,
    uint32_t * array_indices,
    uint32_t * array_indices_found)
{
    size_t field_name_len = t1rs->strlen(to_strip);
    uint8_t is_array = 0;
    t1rs->memset(
        array_indices,
        0,
        sizeof(uint32_t) * T1_REFL_MAX_ARRAY_SIZES);
    
    while (
        field_name_len > 3 &&
        to_strip[field_name_len-1] == ']' &&
        to_strip[field_name_len-2] >= '0' &&
        to_strip[field_name_len-2] <= '9')
    {
        for (uint32_t i = T1_REFL_MAX_ARRAY_SIZES-1; i >= 1; i--) {
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
    #if T1_META_ASSERTS
    for (uint32_t i = 0; i < T1_REFL_MAX_ARRAY_SIZES; i++) {
        assert(array_indices[i] < field->array_sizes[i]);
    }
    #endif
    
    uint32_t return_val = 0;
    
    for (int32_t i = 0; i < T1_REFL_MAX_ARRAY_SIZES; i++) {
        uint32_t offset_per_slice = 1;
        for (int32_t j = i+1; j < T1_REFL_MAX_ARRAY_SIZES; j++) {
            offset_per_slice *= field->array_sizes[j];
        }
        
        return_val += (offset_per_slice * array_indices[i]);
    }
    
    return return_val;
}

static void T1_refl_get_field_recursive(
    T1MetaField * return_value,
    const char * struct_name,
    const char * field_name,
    uint32_t * good)
{
    *good = 0;
    
    MetaStruct * metastruct = find_struct_by_name(struct_name);
    
    if (metastruct == NULL) {
        return_value->array_sizes[0] = 0;
        return_value->array_sizes[1] = 0;
        return_value->array_sizes[2] = 0;
        return_value->data_type = T1_TYPE_NOTSET;
        return_value->offset = -1;
        return;
    }
    
    // The field name may include a dot, in which case we only
    // search for the first part, then do a recursion
    char * first_part = T1_refl_copy_str_to_store(
        field_name,
        good);
    #if T1_META_ASSERTS
    assert(good);
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
        second_part = T1_refl_copy_str_to_store(
            first_part + dot_i + 1,
            good);
        #if T1_META_ASSERTS
        assert(good);
        #endif
        *good = 0;
    }
    
    
    uint32_t array_indices[T1_REFL_MAX_ARRAY_SIZES];
    uint32_t array_indices_found = 0;
    strip_array_brackets_and_get_array_indices(
        first_part,
        array_indices,
        &array_indices_found);
    
    MetaField * metafield = find_field_in_struct_by_name(
        metastruct, first_part);
    
    if (metafield == NULL) {
        return_value->array_sizes[0] = 0;
        return_value->array_sizes[1] = 0;
        return_value->array_sizes[2] = 0;
        return_value->data_type = T1_TYPE_NOTSET;
        return_value->offset = -1;
        *good = 1;
        return;
    }
    
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
                    #if T1_META_ASSERTS
                    // our struct has an unregistered struct as
                    // a property, that shouldn't be possible
                    assert(0);
                    #endif
                    *good = 0;
                    return;
                }
                offset_per_array_index = substruct->size_bytes;
                break;
            case T1_TYPE_NOTSET:
            default:
                #if T1_META_ASSERTS
                assert(0);
                #endif
                *good = 0;
                return;
        }
    }
    
    return_value->offset += (uint32_t)metafield->offset + (offset_per_array_index * flat_array_index);
    return_value->data_type = metafield->type;
    return_value->array_sizes[0] = metafield->array_sizes[0];
    return_value->array_sizes[1] = metafield->array_sizes[1];
    return_value->array_sizes[2] = metafield->array_sizes[2];
    while (array_indices_found > 0) {
        return_value->array_sizes[0] = return_value->array_sizes[1];
        return_value->array_sizes[1] = return_value->array_sizes[2];
        return_value->array_sizes[2] = return_value->array_sizes[0];
        array_indices_found -= 1;
    }
    
    if (second_part != NULL) {
        #if T1_META_ASSERTS
        assert(return_value->data_type == T1_TYPE_STRUCT);
        assert(metafield->struct_type_name != NULL);
        assert(metafield->struct_type_name[0] != '\0');
        #endif
        
        T1_refl_get_field_recursive(
            return_value,
            metafield->struct_type_name,
            second_part,
            good);
        if (!*good) { return; }
    }
    
    *good = 1;
}

T1MetaField T1_meta_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    uint32_t * good)
{
    #if T1_META_ASSERTS
    assert(struct_name != NULL);
    assert(struct_name[0] != '"');
    assert(field_name != NULL);
    assert(field_name[0] != '"');
    #endif
    
    T1MetaField return_value;
    return_value.data_type = T1_TYPE_NOTSET;
    return_value.array_sizes[0] = 0;
    return_value.array_sizes[1] = 0;
    return_value.array_sizes[2] = 0;
    return_value.offset = 0;
    
    // T1_refl_get_field_recursive() will push to the ascii store
    // as if it were stack memory
    uint16_t before_recursion_ascii_store_next_i =
        t1rs->ascii_store_next_i;
    
    T1_refl_get_field_recursive(
        &return_value,
        struct_name,
        field_name,
        good);
    
    #if T1_META_ASSERTS
    assert(good);
    #endif
    
    // discard or "pop" T1_refl_get_field_recursive()'s strings
    t1rs->ascii_store_next_i = before_recursion_ascii_store_next_i;
    
    *good = 1;
    return return_value;
}
