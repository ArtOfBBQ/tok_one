#include "../shared/platform_layer.h"

float platform_sound_volume = 0.15f;
float platform_music_volume = 0.15f;

void platform_gpu_update_viewport(void)
{
    opengl_set_projection_constants(&window_globals->projection_constants);
}

void platform_get_cwd(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        "./");
}

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    void * return_value = mmap(
        /* void *addr: */
            NULL,
        /* size_t len: */
            size,
        /* int prot: */
            PROT_READ | PROT_WRITE,
        /* int flags: */
            MAP_SHARED | MAP_ANONYMOUS,
        /* int fildes: */
            -1,
        /* off_t offset: */
            0);
    
    if (return_value == MAP_FAILED) {
        return_value = NULL;
    }
    
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
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return 
        ((uint64_t)currentTime.tv_sec * (uint64_t)1e6) +
        (uint64_t)currentTime.tv_usec;
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
    out_preallocatedbuffer->good = false;
    FILE * f = fopen(filepath, "rb");
    
    if (!f) {
        return;
    }
    
    fseek(f, 0, SEEK_END);
    uint32_t total_filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint32_t bytes_to_read = total_filesize;
    if (out_preallocatedbuffer->size < total_filesize) {
        bytes_to_read = out_preallocatedbuffer->size;
    }
    
    fread(
        out_preallocatedbuffer->contents,
        bytes_to_read,
        1,
        f);
    fclose(f);
    
    out_preallocatedbuffer->contents[bytes_to_read] = '\0';
    out_preallocatedbuffer->good = bytes_to_read > 0;
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
    // returns 0 on success
    
    uint32_t i = 0;
    char intermed_dir[256];
    while (dirname[i] != '\0') {
        while (dirname[i] != '/' && dirname[i] != '\0') {
            i++;
        }
        
        uint32_t j = 0;
        for (; j < i; j++) {
            intermed_dir[j] = dirname[j];
        }
        intermed_dir[j] = '\0';
        
        log_append("platform_mkdir_if_not_exist(");
        log_append(intermed_dir);
        log_append(")\n");
        
        int return_value = mkdir(intermed_dir, 0700);
        if (return_value != 0) {
            switch (errno) {
                case EACCES: {
                    log_append("EACCES\n");
                    break;
                }
                case EBADF: {
                    log_append("EBADF\n");
                    break;
                }
                case EDQUOT:
                    log_append("EDQUOT\n");
                    break;
                case EEXIST:
                    log_append("EEXIST\n");
                    break;
                case EFAULT:
                    log_append("EFAULT\n");
                    break;
                case EINVAL:
                    log_append("EINVAL\n");
                    break;
                case ELOOP:
                    log_append("ELOOP\n");
                    break;
                case EMLINK:
                    log_append("EMLINK\n");
                    break;
                case ENAMETOOLONG:
                    log_append("ENAMETOOLONG\n");
                    break;
                case ENOENT:
                    log_append("ENOENT\n");
                    break;
                case ENOMEM:
                    log_append("ENOMEM\n");
                    break;
                case ENOSPC:
                    log_append("ENOSPC\n");
                    break;
                case ENOTDIR:
                    log_append("ENOTDIR\n");
                    break;
                case EPERM:
                    log_append("EPERM\n");
                    break;
                case EROFS:
                    log_append("EROFS\n");
                    break;
                default: {
                    log_append("unhandled error\n");
                }
                assert(0);
            }
        }
        
        i++;
    }
}

void platform_delete_file(const char * filepath) {
    unlink(filepath);
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    log_append("platform_copy_file() copying from file: ");
    log_append(filepath_source);
    log_append(" to file: ");
    log_append(filepath_destination);
    log_append_char('\n');
    
    platform_delete_file(filepath_destination);
    
    int fd_source      = open(filepath_source     , O_RDONLY); 
    if (fd_source == -1) {
        log_append("failed to open file as a copy source: ");
        log_append(filepath_source);
        log_append_char('\n');
        assert(0);
    }
    
    char destination_pathonly[256];
    strcpy_capped(destination_pathonly, 256, filepath_destination);
    uint32_t i = 0;
    while (destination_pathonly[i] != '\0') { i++; }
    while (i >= 0 && destination_pathonly[i] != '/') { i--; }
    destination_pathonly[i] = '\0';
    platform_mkdir_if_not_exist(
        destination_pathonly);
    
    int fd_destination = open(
        filepath_destination,
        O_CREAT | O_WRONLY | O_TRUNC,
        0644); 
    
    if (fd_destination == -1) {
        log_append("failed to open file as a copy destination: ");
        log_append(filepath_destination);
        log_append_char('\n');
        assert(0);
    }
    
    struct stat source_stats;
    fstat(fd_source, &source_stats);
    log_append("source file size in bytes: ");
    log_append_uint(source_stats.st_size);
    log_append_char('\n');
    
    int bytes_copied = copy_file_range(
        /* int fd_in: */
            fd_source,
        /* off64_t * off_in: */
            0,
        /* int fd_out: */
            fd_destination,
        /* off64_t * off_out: */
            0,
        /* size_t len: */
            source_stats.st_size,
        /* unsigned int flags: */
            0);
    
    if (bytes_copied == -1) {
        switch (errno) {
            case EBADF: {
                log_append("EBADF\n");
                break;
            }
            case EFBIG: {
                log_append("EFBIG\n");
                break;
            }
            case EINVAL: {
                log_append("EINVAL\n");
                break;
            }
            case EIO: {
                log_append("EIO\n");
                break;
            }
            case EISDIR: {
                log_append("EISDIR\n");
                break;
            }
            case ENOMEM: {
                log_append("ENOMEM\n");
                break;
            }
            case ENOSPC: {
                log_append("ENOSPC\n");
                break;
            }
            case EOPNOTSUPP: {
                log_append("EOPNOTSUPP\n");
                break;
            }
            case EOVERFLOW: {
                log_append("EOVERFLOW\n");
                break;
            }
            case EPERM: {
                log_append("EPERM\n");
                break;
            }
            case ETXTBSY: {
                log_append("ETXBSY\n");
                break;
            }
            case EXDEV: {
                log_append("EXDEV\n");
                break;
            }
            default: {
                log_append("unhandled error\n");
            }
            assert(0);
        }
    }
    
    assert(bytes_copied == source_stats.st_size);
}

void
platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    log_append("platform_write_file(");
    log_append(filepath);
    log_append(")\n");
    
    int fd = open(filepath, 0, 0); 
    
    // write is basically the equivalent of the syscall
    // it requires unistd.h
    ssize_t bytes_written = write(
        /* int fd: */
            fd,
        /* const void buf: */
            output,
        /* size_t count: */
            output_size);
    
    if (output_size ==  bytes_written) {
        *good = true;
    }
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
    DIR * d = NULL;
    struct dirent * dir = NULL;
    
    d = opendir(directory);
    
    *recipient_size = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            strcpy_capped(
                filenames[*recipient_size],
                128,
                dir->d_name);
            *recipient_size += 1;
        }
        closedir(d);
    }
}

char application_path[128];

void
platform_get_application_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        application_path);
}


void platform_get_resources_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        application_path);
}

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    strcpy_capped(
        recipient,
        recipient_size,
        "/var/lib/");
    strcat_capped(
        recipient,
        recipient_size,
        APPLICATION_NAME);
    
    uint32_t i = 0;
    while (recipient[i] != '\0') {
        if (recipient[i] == ' ') { recipient[i] = '_'; }
        i++;
    }
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    // TODO: maybe we should just use pthread for threads instead of
    // dispatch_async, since we need pthreads for mutex locks anyway
    // Let's revisit this when we port to other platforms
    
    pthread_t thread;
    uint32_t result = pthread_create(
        &thread,
        NULL,
        function_to_run,
        argument);
    log_assert(result == 0);
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

void platform_close_application(void) {
    exit(0);    
}

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    // TODO: implement me
}

