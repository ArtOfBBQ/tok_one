#ifndef T1_OBJC_H
#define T1_OBJC_H

// Convenience functions to interface with
// objective-c frameworks

#include "T1_stdint.h"

void T1_objc_init(
    void * (* malloc_perma)(size_t));

void T1_objc_open_framework_and_link_perma_good_val(
    const char * framework_name,
    u8 * perma_good_checker);

void T1_objc_close_current_framework(void);

void * T1_objc_get_func(
    const char * func_name);

void * T1_objc_get_class(
    const char * class_name);

void * T1_objc_reg_sel(
    const char * selector_name);

void * T1_objc_msg_expect_ptr(
    void * recip,
    void * selector);

void * T1_objc_msg_with_1arg_expect_ptr(
    void * recip,
    void * selector,
    uintptr_t arg1);

void * T1_objc_msg_with_2arg_expect_ptr(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2);

void * T1_objc_msg_with_3arg_expect_ptr(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3);

void * T1_objc_msg_with_4arg_expect_ptr(
    void * recip,
    void * selector,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3,
    uintptr_t arg4);

#if 1
void * T1_objc_msg_with_2arg_sizet_expect_ptr(
    void * recip,
    void * selector,
    size_t arg1,
    size_t arg2);
#endif

void * T1_objc_msg_with_char_arg_expect_ptr(
    void * recip,
    void * selector,
    const char * arg1);

void * T1_objc_msgx2_expect_ptr(
    void * recip,
    void * selector_1,
    void * selector_2);

u32 T1_objc_msg_expect_u32(
    void * recip,
    void * selector);
u64 T1_objc_msg_expect_u64(
    void * recip,
    void * selector);

f32 T1_objc_msg_expect_f32(
    void * recip,
    void * selector);

void * T1_objc_nsstring_construct(
    const char * from);


/*
Metal-specific
*/
typedef struct {
    uintptr_t x, y, z;
} T1ObjcMTLOrigin;

typedef struct {
    uintptr_t x, y, z;
} T1ObjcSet;

T1ObjcSet T1_objc_set_make(
    uintptr_t x,
    uintptr_t y,
    uintptr_t z);

void T1_cmd_copy_texture_to_buffer(
    void *    command_encoder,
    void *    sel_copy_to_buf,
    void *    src_texture,
    uintptr_t src_slice,
    uintptr_t src_level,
    T1ObjcSet src_origin,
    T1ObjcSet src_size,
    void *    dst_buffer,
    uintptr_t dst_offset,
    uintptr_t dst_bytes_per_row,
    uintptr_t dst_bytes_per_image);

#endif // T1_OJBC_H
