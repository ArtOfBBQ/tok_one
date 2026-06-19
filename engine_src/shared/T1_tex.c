#include "T1_tex.h"

void T1_tex_set_array_i(
    T1Tex * recip,
    const s16 newval)
{
    *recip &= 0x07FF;
    *recip |= (newval << 11);
}

void T1_tex_set_slice_i(
    T1Tex * recip,
    const s16 newval)
{
    *recip &= 0xF800;
    *recip |= newval;
}
