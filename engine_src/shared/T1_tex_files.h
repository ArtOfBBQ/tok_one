#ifndef T1_TEX_FILES_H
#define T1_TEX_FILES_H

#include "T1_stdint.h"

/*
T1's own images, embedded in T1 itself. The client's images come in as rgba
through T1_tex_new()
*/
void T1_tex_files_load_font_images(
    char ** const fatal_error);

#endif // T1_TEX_FILES_H
