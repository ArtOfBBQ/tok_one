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
    T1_PROP_STORE_ZS_MAT_F32, // not animatable
    T1_PROP_STORE_ZS_MAT_U32, // not animatable
    T1_PROP_STORE_TQ_CPU_F32, // not animatable
    T1_PROP_STORE_TQ_GPU_F32,
    T1_PROP_STORE_TQ_GPU_U32,
    T1_PROP_STORE_ZL_F32,
    T1_PROP_STORE_CAM_F32,
    T1_PROP_STORE_CAM_U32,    // not animatable
    T1_PROP_STORE_TEXT,       // the settings for the next text label
    T1_PROP_STORE_POSTPROC,   // post-processing settings
    T1_PROP_STORE_OUTOFBOUNDS
} T1PropertyStore;

typedef struct {
    const c8 * name;
    u32 mask;   // 0xFFFFFFFF unless several values share 1 u32
    u16 offset; // in bytes, from the start of the store's struct
    u8  shift;
    u8  store;  // T1PropertyStore
    u8  is_u32;
    u8  cam_ref;  // T1PropertyCamRef: the client passes a camera's T1_id
} T1Property;

/*
Some zsprite values hold cameras by index, the client sets them by T1_id
*/
typedef enum : u8 {
    T1_PROP_CAM_REF_NONE = 0,
    T1_PROP_CAM_REF_INDEX,       // the camera's index, -1 for T1_ID_NONE
    T1_PROP_CAM_REF_HIDE_MASK,   // every camera's bit but this one's
} T1PropertyCamRef;

#define T1_PROPERTY_MATCHES_CAP 8

/*
Finds every store that has a property with this name, either among the
object stores (settings_store T1_PROP_STORE_OUTOFBOUNDS) or in one settings
store (T1_PROP_STORE_TEXT or T1_PROP_STORE_POSTPROC). Crashes if the name is
unknown there, or if it exists but not with the requested type (f32 or u32).
*/
u32 T1_property_find(
    const c8 * name,
    const u8 is_u32,
    const u8 settings_store,
    const T1Property ** recip,
    const u32 recip_cap);

// the bits to store for a value the client passed (see cam_ref)
u32 T1_property_convert_bits(
    const T1Property * property,
    const u32 bits);

// read bits (an f32's bits or a u32) from a store struct
u32 T1_property_read(
    const void * store_struct,
    const T1Property * property);

// write bits (an f32's bits or a u32) into a store struct
void T1_property_write(
    void * store_struct,
    const T1Property * property,
    const u32 bits);

b8 T1_property_is_packed(const T1Property * property);

b8 T1_property_is_animatable(const T1Property * property);

void T1_set_f32(
    u32 T1_id,
    const c8 * property,
    f32 value);
void T1_set_u32(
    u32 T1_id,
    const c8 * property,
    u32 value);
// the value of the first object with T1_id that has this property
f32 T1_get_f32(
    u32 T1_id,
    const c8 * property);

/*
Every object (and animation) with T1_id_to_add now has T1_id_to_keep, so it
moves, fades and gets deleted together with the others
*/
/*
Text settings, used by every text label requested after setting them
*/
void T1_text_set_f32(
    const c8 * property,
    f32 value);
void T1_text_set_u32(
    const c8 * property,
    u32 value);

/*
Post-processing settings, applied to the whole rendered image
*/
void T1_postproc_set_f32(
    const c8 * property,
    f32 value);
f32 T1_postproc_get_f32(
    const c8 * property);


void T1_merge_T1_ids(
    u32 T1_id_to_add,
    u32 T1_id_to_keep);

void T1_delete(u32 T1_id);
void T1_delete_all(void);

#endif // T1_PROPERTY_H
