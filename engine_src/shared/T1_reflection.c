#include "T1_reflection.h"

#define METASTRUCT_FIELDS_MAX 64
typedef struct MetaField {
    char * name;
    char * struct_type_name;
    T1Type type;
    uint32_t offset;
    uint16_t next_i;
    uint16_t array_sizes[T1_REFL_MAX_ARRAY_SIZES];
} MetaField;

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
    MetaField * metafields_store;
    MetaStruct * metastructs;
    uint16_t ascii_store_cap;
    uint16_t metafields_store_cap;
    uint16_t metastructs_cap;
    uint16_t metafields_size;
    uint16_t metastructs_size; // first memset target
    uint16_t ascii_store_next_i;
} T1ReflectionState;

static T1ReflectionState * t1rs = NULL;

static void construct_metastruct(MetaStruct * to_construct) {
    t1rs->memset(to_construct, 0, sizeof(MetaStruct));
    to_construct->head_fields_i = UINT16_MAX;
}

static void construct_metafield(MetaField * to_construct) {
    #if T1_REFLECTION_ASSERTS
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
            offsetof(T1ReflectionState, metastructs_size),
            0,
            sizeof(T1ReflectionState) -
                offsetof(T1ReflectionState, metastructs_size));
    
    t1rs->memset(
        t1rs->metastructs, 0, t1rs->metastructs_cap);
    t1rs->memset(
        t1rs->ascii_store, 0, t1rs->ascii_store_cap);
    
    for (uint32_t i = 0; i < t1rs->metafields_store_cap; i++) {
        construct_metafield(&t1rs->metafields_store[i]);
    }
}

void T1_reflection_init(
    void *(* T1_reflection_memcpy)(void *, const void *, size_t),
    void *(* T1_reflection_malloc_func)(size_t),
    void *(* T1_reflection_memset_func)(void *, int, size_t),
    int (* T1_reflection_strcmp_func)(const char *, const char *),
    size_t (* T1_reflection_strlen_func)(const char *))
{
    t1rs = T1_reflection_malloc_func(sizeof(T1ReflectionState));
    
    t1rs->memcpy = T1_reflection_memcpy;
    t1rs->memset =  T1_reflection_memset_func;
    t1rs->strcmp = T1_reflection_strcmp_func;
    t1rs->strlen = T1_reflection_strlen_func;
    
    t1rs->ascii_store_cap = 2500;
    t1rs->ascii_store = T1_reflection_malloc_func(
        t1rs->ascii_store_cap);
    
    t1rs->metastructs_cap = 30;
    t1rs->metastructs = T1_reflection_malloc_func(
        sizeof(MetaStruct) * t1rs->metastructs_cap);
    
    t1rs->metafields_store_cap = 500;
    t1rs->metafields_store = T1_reflection_malloc_func(
        sizeof(MetaField) * t1rs->metafields_store_cap);
    
    T1_refl_reset();
}

static char * T1_refl_copy_str_to_store(
    const char * to_copy,
    uint32_t * good)
{
    *good = 0;
    
    size_t len = t1rs->strlen(to_copy);
    
    #ifndef T1_REFLECTION_NO_ASSERTS
    assert(len > 0);
    #endif
    
    if (t1rs->ascii_store_next_i + len >= t1rs->ascii_store_cap)
    {
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
    
    for (int32_t i = 0; i < (int32_t)t1rs->metastructs_size; i++) {
        if (
            t1rs->strcmp(
                t1rs->metastructs[i].name,
                struct_name) == 0)
        {
            #if T1_REFLECTION_ASSERTS
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

void T1_reflection_reg_struct(
    const char * struct_name,
    const uint32_t size_bytes,
    uint32_t * good)
{
    #if T1_REFLECTION_ASSERTS
    MetaStruct * target_mstruct = find_struct_by_name(struct_name);
    assert(target_mstruct == NULL); // name already taken
    #endif
    
    if (target_mstruct == NULL) {
        if (
            t1rs->metastructs_size + 1 >= t1rs->metastructs_cap)
        {
            *good = 0;
            return;
        }
        
        target_mstruct = t1rs->metastructs +
            t1rs->metastructs_size;
        t1rs->metastructs_size += 1;
        construct_metastruct(target_mstruct);
        target_mstruct->name = T1_refl_copy_str_to_store(
            struct_name,
            good);
        target_mstruct->size_bytes = size_bytes;
        if (!*good) {
            #if T1_REFLECTION_ASSERTS
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

void T1_reflection_reg_field(
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
    
    #if T1_REFLECTION_ASSERTS
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
            #if T1_REFLECTION_ASSERTS
            assert(0);
            #endif
            return;
        }
        if (property_name[i] == '\0') {
            break;
        }
        #if T1_REFLECTION_ASSERTS
        assert(i < 250); // 250 character property is absurd
        #endif
    }
    
    if (t1rs->metastructs_size < 1) {
        #if T1_REFLECTION_ASSERTS
        assert(0); // use T1_reflection_reg_struct() first
        #endif
        *good = 0;
        return;
    }
    
    // check for existing struct name
    MetaStruct * target_mstruct = &t1rs->metastructs[t1rs->metastructs_size-1];
    
    // check for existing field name in that struct
    MetaField * target_mfield = find_field_in_struct_by_name(
        target_mstruct,
        property_name);
    
    if (target_mfield == NULL) {
        if (
            t1rs->metafields_size + 1 >=
                t1rs->metafields_store_cap)
        {
            return;
        }
        
        target_mfield = t1rs->metafields_store +
            t1rs->metafields_size;
        uint16_t target_mfield_i = t1rs->metafields_size;
        t1rs->metafields_size += 1;
        
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
    #if T1_REFLECTION_ASSERTS
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
        #if T1_REFLECTION_ASSERTS
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
    
    #if T1_REFLECTION_ASSERTS
    MetaField * previous_link = metafield_i_to_ptr(
        target_mstruct->head_fields_i);
    while (previous_link != NULL) {
        MetaField * next_link = metafield_i_to_ptr(previous_link->next_i);
        // This forces the caller of T1_reflection_reg_field()
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
    uint32_t * array_indices)
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
    #if T1_REFLECTION_ASSERTS
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
    T1ReflectedField * return_value,
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
    #if T1_REFLECTION_ASSERTS
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
        #if T1_REFLECTION_ASSERTS
        assert(good);
        #endif
        *good = 0;
    }
    
    
    uint32_t array_indices[T1_REFL_MAX_ARRAY_SIZES];
    strip_array_brackets_and_get_array_indices(
        first_part,
        array_indices);
    
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
                    #if T1_REFLECTION_ASSERTS
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
                #if T1_REFLECTION_ASSERTS
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
    
    if (second_part != NULL) {
        #if T1_REFLECTION_ASSERTS
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

T1ReflectedField T1_reflection_get_field_from_strings(
    const char * struct_name,
    const char * field_name,
    uint32_t * good)
{
    #if T1_REFLECTION_ASSERTS
    assert(struct_name != NULL);
    assert(struct_name[0] != '"');
    assert(field_name != NULL);
    assert(field_name[0] != '"');
    #endif
    
    T1ReflectedField return_value;
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
    
    #if T1_REFLECTION_ASSERTS
    assert(good);
    #endif
    
    // discard or "pop" T1_refl_get_field_recursive()'s strings
    t1rs->ascii_store_next_i = before_recursion_ascii_store_next_i;
    
    *good = 1;
    return return_value;
}
