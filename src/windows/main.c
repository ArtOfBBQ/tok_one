// functions we must implement
#include "../shared/platform_layer.h"

// functions for us to use
#include "../shared/software_renderer.h"
#include "../shared/box.h"

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

/*
This functionality must be provided by the platform because
of iOS, where reading your own app's files is a security
ordeal
*/
FileBuffer * platform_read_file(char * filename) {
    
    FileBuffer * return_value = malloc(sizeof(FileBuffer));
    
    FILE * modelfile = fopen(
        filename,
        "rb");
    
    fseek(modelfile, 0, SEEK_END);
    unsigned long fsize = (unsigned long)ftell(modelfile);
    fseek(modelfile, 0, SEEK_SET);
    
    char * buffer = malloc(fsize);
    
    size_t bytes_read = fread(
        /* ptr: */
            buffer,
        /* size of each element to be read: */
            1,
        /* nmemb (no of members) to read: */
            fsize,
        /* stream: */
            modelfile);
    
    fclose(modelfile);
    if (bytes_read != fsize) {
        printf("Error - expected bytes read equal to fsize\n");
        return NULL;
    }
    
    return_value->contents = buffer;
    return_value->size = bytes_read;
    
    return return_value;
}

int main() {
    assert(box == NULL);
    
    z_constants_init();
    renderer_init();
    
    printf("hello, windows!\n");
    printf("WINDOW_WIDTH: %f\n", WINDOW_WIDTH);
    printf("WINDOW_HEIGHT: %f\n", WINDOW_HEIGHT);

    ColoredVertex * next_workload = 
        malloc(sizeof(ColoredVertex) * 5);
    uint32_t next_workload_size = 0;
    software_render(next_workload, &next_workload_size);

    printf(
        "vertices obtained in 1st frame: %u\n",
        next_workload_size);
    
    free(next_workload);

    return 0;
}

