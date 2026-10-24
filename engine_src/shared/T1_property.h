#ifndef T1_PROPERTY_H
#define T1_PROPERTY_H

#include "T1_stdint.h"

/*
String properties: the client sets object and animation values by name,
e.g. T1_set_f32(id, "xyz[0]", 0.5f). A name can exist in several stores
(zsprites, texquads and zlights all have "xyz[0]"), every store that has it
is written.
*/

typedef enum : u8 {
    T1_PROP_STORE_ZS_CPU_F32 = 0,
    T1_PROP_STORE_ZS_GPU_F32,
    T1_PROP_STORE_ZS_GPU_U32,
    T1_PROP_STORE_TQ_CPU_F32, // not animatable
    T1_PROP_STORE_TQ_GPU_F32,
    T1_PROP_STORE_TQ_GPU_U32,
    T1_PROP_STORE_ZL_F32,
    T1_PROP_STORE_OUTOFBOUNDS
} T1PropertyStore;

typedef struct {
    const c8 * name;
    u32 mask;   // 0xFFFFFFFF unless several values share 1 u32
    u16 offset; // in bytes, from the start of the store's struct
    u8  shift;
    u8  store;  // T1PropertyStore
    u8  is_u32;
} T1Property;

#define T1_PROPERTY_MATCHES_CAP 4

/*
Finds every store that has a property with this name. Crashes if the name
is unknown, or if it exists but not with the requested type (f32 or u32).
*/
u32 T1_property_find(
    const c8 * name,
    const u8 is_u32,
    const T1Property ** recip,
    const u32 recip_cap);

// write bits (an f32's bits or a u32) into a store struct
void T1_property_write(
    void * store_struct,
    const T1Property * property,
    const u32 bits);

b8 T1_property_is_packed(const T1Property * property);

void T1_set_f32(
    u32 T1_id,
    const c8 * property,
    f32 value);
void T1_set_u32(
    u32 T1_id,
    const c8 * property,
    u32 value);

/*
Every object (and animation) with T1_id_to_add now has T1_id_to_keep, so it
moves, fades and gets deleted together with the others
*/
void T1_merge_T1_ids(
    u32 T1_id_to_add,
    u32 T1_id_to_keep);

void T1_delete(u32 T1_id);

#endif // T1_PROPERTY_H
