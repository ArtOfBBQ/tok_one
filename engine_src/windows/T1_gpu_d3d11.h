#ifndef T1_GPU_D3D11_H
#define T1_GPU_D3D11_H

#include "T1_stdint.h"

/*
Draws the T1Tex tex into the window (an HWND) as large as it fits, keeping
its aspect ratio, with black bands around it, and presents it. Blocks until
the display's next refresh. Returns false if nothing was presented.
*/
b8 T1_gpu_d3d11_present_tex(void * hwnd, const u32 tex);

#endif // T1_GPU_D3D11_H
