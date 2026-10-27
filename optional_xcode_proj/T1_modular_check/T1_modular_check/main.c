//
//  main.c
//  T1_modular_check
//
//  Created by Jelle Vandeneynde on 2026/10/27.
//

#include <stdlib.h>
#include <stdio.h>

#include "T1.h"

int main(int argc, const char * argv[]) {
    
    char * fatal_error = 0;
    
    T1_init(
        /* void *(*alloc_forever)(size_t): */
            malloc,
        /* void *(*alloc_temp)(size_t): */
            malloc,
        /* void (*free_temp)(void *): */
            free,
        /* char **const fatal_error: */
            &fatal_error);
    
    T1_window_open(&fatal_error);
    
    while (!T1_window_close_requested()) {
        T1_window_poll_events();
        if (!T1_window_visible()) { continue; }
        T1_window_present();
    }
    
    return EXIT_SUCCESS;
}
