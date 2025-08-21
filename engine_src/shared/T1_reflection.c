#include "T1_reflection.h"

#define METASTRUCT_FIELDS_MAX 64
typedef struct MetaField {
    struct MetaField * next;
    char * name;
    T1DataType type;
    uint16_t array_size;
    uint16_t offset;
} MetaField;

typedef struct {
    char * name;
    MetaField * fields_head;
} MetaStruct;

typedef struct T1ReflectionState {
    void *(* memcpy)(void *, const void *, size_t);
    void *(* memset)(void *, int, size_t);
    int (* strcmp)(const char *, const char *);
    size_t (* strlen)(const char *);
    char * ascii_store;
    MetaField * metafields_store;
    MetaStruct * metastructs;
    uint32_t ascii_store_cap;
    uint32_t metafields_store_cap;
    uint32_t metastructs_cap;
    uint32_t metafields_size;
    uint32_t metastructs_size; // first memset target
    uint32_t ascii_store_next_i;
} T1ReflectionState;

static T1ReflectionState * t1rs = NULL;

static void construct_metastruct(MetaStruct * to_construct) {
    to_construct->name = NULL;
    to_construct->fields_head = NULL;
}

static void construct_metafield(MetaField * to_construct) {
    #if T1_REFLECTION_ASSERTS
    assert(to_construct != NULL);
    #endif
    
    to_construct->next = NULL;
    to_construct->type = T1_DATATYPE_NOTSET;
    to_construct->name = NULL;
    to_construct->offset = UINT16_MAX;
    to_construct->array_size = 0;
}

static void T1_reflection_reset(void) {
    offsetof(T1ReflectionState, metastructs);
    
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
    
    t1rs->ascii_store_cap = 1000;
    t1rs->ascii_store = T1_reflection_malloc_func(
        t1rs->ascii_store_cap);
    
    t1rs->metastructs_cap = 20;
    t1rs->metastructs = T1_reflection_malloc_func(
        sizeof(MetaStruct) * t1rs->metastructs_cap);
    
    t1rs->metafields_store_cap = 1000;
    t1rs->metafields_store = T1_reflection_malloc_func(
        sizeof(MetaField) * t1rs->metafields_store_cap);
    
    T1_reflection_reset();
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

void T1_reflection_reg(
    const char * struct_name,
    const char * property_name,
    const uint16_t property_offset,
    const T1DataType property_type,
    const uint16_t property_array_size,
    const uint32_t offset_i,
    uint32_t * good)
{
    *good = 0;
    
    // check for existing struct name
    MetaStruct * target_mstruct = NULL;
    for (int32_t i = 0; i < (int32_t)t1rs->metastructs_size; i++) {
        if (
            t1rs->strcmp(
                t1rs->metastructs[i].name,
                struct_name) == 0)
        {
            #if T1_REFLECTION_ASSERTS
            assert(target_mstruct == 0);
            #endif
            
            target_mstruct = t1rs->metastructs + i;
        }
    }
    
    if (target_mstruct == NULL) {
        if (
            t1rs->metastructs_size + 1 >= t1rs->metastructs_cap)
        {
            return;
        }
        
        target_mstruct = t1rs->metastructs +
            t1rs->metastructs_size;
        t1rs->metastructs_size += 1;
        construct_metastruct(target_mstruct);
        target_mstruct->name = T1_refl_copy_str_to_store(
            struct_name,
            good);
        if (!*good) { return; }
        *good = 0;
    }
    
    // check for existing field name in that struct
    MetaField * target_mfield = NULL;
    MetaField * i = target_mstruct->fields_head;
    while (i != NULL)
    {
        if (
            t1rs->strcmp(
                i->name,
                property_name) == 0)
        {
            target_mfield = i;
            break;
        }
        
        i = i->next;
    }
    
    if (target_mfield == NULL) {
        if (
            t1rs->metafields_size + 1 >=
                t1rs->metafields_store_cap)
        {
            return;
        }
        
        target_mfield = t1rs->metafields_store +
            t1rs->metafields_size;
        t1rs->metafields_size += 1;
        
        construct_metafield(target_mfield);
        
        MetaField * previous_link = target_mstruct->fields_head;
        if (previous_link == NULL) {
            target_mstruct->fields_head = target_mfield;
        } else {
            while (previous_link->next != NULL) {
                previous_link = previous_link->next;
            }
            previous_link->next = target_mfield;
        }
    }
    
    target_mfield->type = property_type;
    target_mfield->offset = property_offset;
    target_mfield->array_size = property_array_size;
    target_mfield->name = T1_refl_copy_str_to_store(
        property_name,
        good);
    if (*good) { return; }
    *good = 0;
    
    // do more stuff?
    
    *good = 1;
}
