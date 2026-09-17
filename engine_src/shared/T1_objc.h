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

void * T1_objc_get_class(
    const char * class_name);

void * T1_objc_reg_sel(
    const char * selector_name);

void * T1_objc_msg_expect_ptr(
    void * recip,
    void * selector);

void * T1_objc_msgx2_expect_ptr(
    void * recip,
    void * selector_1,
    void * selector_2);

u32 T1_objc_msg_expect_u32(
    void * recip,
    void * selector);

f32 T1_objc_msg_expect_f32(
    void * recip,
    void * selector);

#endif // T1_OJBC_H
