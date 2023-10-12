#include "../shared/platform_layer.h"

float platform_sound_volume = 0.15f;
float platform_music_volume = 0.15f;

void platform_get_cwd(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        "./");
}

uint8_t * platform_malloc_unaligned_block(
    const uint64_t size)
{
    uint8_t * return_value = (uint8_t *)malloc(size);
    
    return return_value;
}

uint32_t platform_get_directory_separator_size(void) {
    return 1;
}

void platform_get_directory_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t
platform_get_current_time_microsecs(void) {
    // TODO: we need time to any rendering
    // struct timeval tv;
    // gettimeofday(&tv,NULL);
    // uint64_t result =
    //     1000000 *
    //         (uint64_t)tv.tv_sec +
    //         (uint64_t)tv.tv_usec;
    
    return 1;
}

/*
Get a file's size. Returns -1 if no such file

same as platform_get_filesize() except it assumes
the resources directory
*/
uint64_t platform_get_resource_size(const char * filename) {
    char pathfile[500];
    resource_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* assert_capacity: */ 500);
    
    return platform_get_filesize(pathfile);
}

/*
Get a file's size. Returns 0 if no such file
*/
uint64_t platform_get_filesize(const char * filepath) {
    
    uint64_t return_value;
    
    FILE * f = fopen(filepath, "rb");
    
    if (!f) { return 0; }
    
    fseek(f, 0, SEEK_END);
    return_value = ftell(f);
    fseek(f, 0, SEEK_SET);
    fclose(f);
    
    return return_value;
}

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer)
{
    FILE * f = fopen(filepath, "rb");
    
    if (!f) { return; }
    
    fseek(f, 0, SEEK_END);
    out_preallocatedbuffer->size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    fread(
        out_preallocatedbuffer->contents,
        out_preallocatedbuffer->size,
        1,
        f);
    fclose(f);
    
    out_preallocatedbuffer->contents[out_preallocatedbuffer->size] = '\0';
}

bool32_t platform_file_exists(
    const char * filepath)
{
    FILE *file;
    if ((file = fopen(filepath, "rb")))
    {
        fclose(file);
        return true;
    }
    
    return false;
}

void platform_mkdir_if_not_exist(const char * dirname) {    
    
    // TODO: implement
}

void platform_delete_file(const char * filepath) {
    // TODO: implement
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    // TODO: implement
}

void
platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    // TODO: implement
}

void platform_write_file_to_writables(
    const char * filepath_inside_writables,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    char recipient[500];
    writable_filename_to_pathfile(
        /* filename: */
            filepath_inside_writables,
        /* recipient: */
            recipient,
        /* recipient_capacity: */
            500);
    
    platform_write_file(
        /* const char * filepath: */
            recipient,
        /* const char * output: */
            output,
        /* const uint32_t output_size: */
            output_size,
        /* bool32_t * good: */
            good);
}

void platform_get_filenames_in(
    const char * directory,
    char ** filenames,
    const uint32_t recipient_capacity,
    uint32_t * recipient_size)
{
    // TODO: implement
}

void
platform_get_application_path(
    char * recipient,
    const uint32_t recipient_size)
{
    // strcpy_capped(
    //     recipient,
    //     recipient_size,
    //     (char *)[[[NSBundle mainBundle] bundlePath]
    //         cStringUsingEncoding: NSASCIIStringEncoding]);
}

char application_path[128];
void platform_get_resources_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        application_path);
    strcat_capped(
        recipient,
        recipient_size,
        "/resources");
}

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        application_path);
    strcat_capped(
        recipient,
        recipient_size,
        "/saveddata");
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    // TODO: maybe we should just use pthread for threads instead of
    // dispatch_async, since we need pthreads for mutex locks anyway
    // Let's revisit this when we port to other platforms
    
    // pthread_t thread;
    // uint32_t result = pthread_create(
    //     &thread,
    //     NULL,
    //     function_to_run,
    //     argument);
    // log_assert(result == 0);
}

void platform_play_sound_resource(
    const char * resource_filename)
{
    
}

void platform_update_sound_volume(void) {
    
}

void platform_update_music_volume(void) {
    
}

void platform_play_music_resource(
    const char * resource_filename)
{
    
}

#define MUTEXES_SIZE 30
static pthread_mutex_t mutexes[MUTEXES_SIZE];
static uint32_t next_mutex_id = 0;
/*
creates a mutex and return the ID of said mutex for you to store
*/
uint32_t platform_init_mutex_and_return_id(void) {
    #ifndef LOGGER_IGNORE_ASSERTS
    log_assert(next_mutex_id + 1 < MUTEXES_SIZE);
    int32_t success = pthread_mutex_init(&(mutexes[next_mutex_id]), NULL);
    log_assert(success == 0);
    #endif
    
    uint32_t return_value = next_mutex_id;
    next_mutex_id++;
    return return_value;
}

/*
returns whether or not a mutex was locked, and locks the mutex if it
was unlocked
*/
void platform_mutex_lock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    #ifndef LOGGER_IGNORE_ASSERTS
    int return_value =
    #endif
        pthread_mutex_lock(&(mutexes[mutex_id]));
    log_assert(return_value == 0);
    return;
}

int32_t platform_mutex_unlock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    int return_value = pthread_mutex_unlock(&(mutexes[mutex_id]));
    log_assert(return_value == 0);
    
    return return_value;
}

void platform_gpu_init_texture_array(
    const int32_t texture_array_i,
    const uint32_t num_images,
    const uint32_t single_image_width,
    const uint32_t single_image_height)
{
}

void platform_gpu_push_texture_slice(
    const int32_t texture_array_i,
    const int32_t texture_i,
    const uint32_t parent_texture_array_images_size,
    const uint32_t image_width,
    const uint32_t image_height,
    const uint8_t * rgba_values)
{
}

void platform_close_application(void) {
    
}

