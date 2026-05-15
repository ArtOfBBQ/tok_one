#include "T1_tex.h"

void T1_tex_set_array_i(
    T1Tex * recip,
    const int16_t newval)
{
    *recip &= 0x007F;
    *recip |= (newval << 11);
}

void T1_tex_set_slice_i(
    T1Tex * recip,
    const int16_t newval)
{
    *recip &= 0xF800;
    *recip |= newval;
}
