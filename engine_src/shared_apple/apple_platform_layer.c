#define SHARED_APPLE_PLATFORM
#define PLATFORM_NS_FILEMANAGER

#include "../shared/platform_layer.h"

uint8_t * platform_malloc_unaligned_block(
    const uint64_t size)
{
    uint8_t * return_value = (uint8_t *)malloc(size);
    
    return return_value;
}

#define MAX_SIMUL_SOUNDS 3
static AVAudioPlayer * sound_players[MAX_SIMUL_SOUNDS];
static uint32_t next_sound_player = 0;

static AVAudioPlayer * active_music_player = NULL;
static float sound_volume = 0.135f;
static float music_volume = 0.035f;

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    NSArray * paths = NSSearchPathForDirectoriesInDomains(
        NSApplicationSupportDirectory,
        NSUserDomainMask,
        YES);
    
    NSString * libraryDirectory = [paths objectAtIndex:0];
    
    char * library_dir =
        (char *)[libraryDirectory
            cStringUsingEncoding: NSUTF8StringEncoding];
    
    strcpy_capped(recipient, recipient_size, library_dir);
    strcat_capped(recipient, recipient_size, "/");
    strcat_capped(recipient, recipient_size, APPLICATION_NAME);
    
    platform_mkdir_if_not_exist(recipient);
}

uint32_t platform_get_directory_separator_size(void) {
    return 1;
}

void platform_get_directory_separator(char * recipient) {
    recipient[0] = '/';
    recipient[1] = '\0';
}

uint64_t __attribute__((no_instrument_function))
platform_get_current_time_microsecs(void) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    uint64_t result =
        1000000 *
            (uint64_t)tv.tv_sec +
            (uint64_t)tv.tv_usec;
    
    return result;
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
    
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSError * error_value = nil;
    
    uint64_t file_size = (uint64_t)[
        [[NSFileManager defaultManager]
        attributesOfItemAtPath:nsfilepath
        error:&error_value] fileSize];
    
    if (error_value != nil) {
        log_append("ERROR - failed to get size of file: ");
        log_append(filepath);
        log_append("\n");
        return 0;
    }
    
    // let's not use 20MB+ files in development
    assert(file_size < 20000000);

    return_value = file_size;
    
    return return_value;
}

void platform_read_resource_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer)
{
    char pathfile[500];
    resource_filename_to_pathfile(
        filename,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    platform_read_file(
        /* filepath :*/
            pathfile,
        /* out_preallocatedbuffer: */
            out_preallocatedbuffer);
}

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer)
{
    @autoreleasepool {
    NSString * nsfilepath =
        [NSString
            stringWithCString:filepath
            encoding:NSASCIIStringEncoding];
    
    NSError * error = NULL;
    NSData * file_data =
        [NSData
            dataWithContentsOfFile: nsfilepath
            options: NSDataReadingUncached
            error: &error];
    
    if (error) {
        log_append("Error - failed [NSData initWithContentsOfFile:]\n");
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    [file_data
        getBytes: out_preallocatedbuffer->contents
        length: out_preallocatedbuffer->size];
    
    out_preallocatedbuffer->good = true;
    }
}

bool32_t platform_resource_exists(const char * resource_name) {
    char pathfile[500];
    resource_filename_to_pathfile(
        resource_name,
        /* recipient: */ pathfile,
        /* capacity: */ 500);
    
    return platform_file_exists(
        /* filepath: */ pathfile);
}

bool32_t platform_file_exists(
    const char * filepath)
{
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    BOOL is_directory = false;
    if ([[NSFileManager defaultManager]
        fileExistsAtPath: nsfilepath
        isDirectory: &is_directory])
    {
        if (is_directory) {
	    log_append("warning filepath: ");
	    log_append(filepath);
	    log_append(" is a directory, returnin FALSE for existence\n");
            return false;
        }
        
        return true;
    }
    
    log_append("filepath: ");
    log_append(filepath);
    log_append(" does not exist, returning FALSE...\n");
    return false;
}

void platform_mkdir_if_not_exist(const char * dirname) {    
    
    log_append("make directory if it doesn't exist: ");
    log_append(dirname);
    log_append("\n");
    
    NSString * directory_path = [NSString
        stringWithCString:dirname
        encoding:NSASCIIStringEncoding];
    NSURL * directory_url = [NSURL
        fileURLWithPath: directory_path
        isDirectory: true];
    assert(directory_url != NULL);
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:directory_path])
    {
        NSError * error = NULL;
        
        bool success = [[NSFileManager defaultManager]
            createDirectoryAtPath:directory_path
            withIntermediateDirectories:true
            attributes:NULL 
            error:&error];
        
        if (!success) {
            log_append("ERROR - tried to create a directory and failed\n");
            if (error != NULL) {
                NSLog(@" error => %@ ", [error userInfo]);
            }
        } else {
            assert([[NSFileManager defaultManager]
                fileExistsAtPath:directory_path]);
        }
    }
    
    return;
}

void platform_delete_file(const char * filepath) {
    
    log_append("trying to delete a file with NSFileManager: ");
    log_append(filepath);
    log_append("\n");
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    [[NSFileManager defaultManager]
        removeItemAtPath: nsfilepath
        error: nil];
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    log_append("trying to copy from: ");
    log_append(filepath_source);
    log_append(", to: ");
    log_append(filepath_destination);
    log_append_char('\n');
    
    NSString * nsfilepath_source = [NSString
        stringWithCString:filepath_source
        encoding:NSASCIIStringEncoding];
    
    NSString * nsfilepath_destination = [NSString
        stringWithCString:filepath_destination
        encoding:NSASCIIStringEncoding];
    
    NSError * error = NULL;
    
    [[NSFileManager defaultManager]
        copyItemAtPath: nsfilepath_source
        toPath: nsfilepath_destination
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        assert(0);
    }
}

void
platform_write_file(
    const char * filepath,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    log_append("write file data to: ");
    log_append(filepath);
    log_append("\n");
    NSString * nsfilepath = [NSString
        stringWithCString:filepath
        encoding:NSASCIIStringEncoding];
    
    NSData * nsdata = [NSData
        dataWithBytes:output
        length:output_size];
    
    if (![[NSFileManager defaultManager]
        createFileAtPath: 
            nsfilepath
        contents:
            nsdata
        attributes:
            nil])
    {
        log_append("Failed to write to file: ");
        log_append(filepath);
        *good = false;
        return;
    }
    
    *good = true;
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
    log_assert(recipient_capacity > 0);
    *recipient_size = 0;
    
    NSString * path = [NSString
        stringWithCString:directory
        encoding:NSASCIIStringEncoding];
    // NSURL * url = [NSURL URLWithString: path];
    
    // log_assert(url != NULL);
    // NSLog(@" url => %@ ", url);
    NSError * error = NULL;
    
    NSArray * results = [[NSFileManager defaultManager]
        contentsOfDirectoryAtPath:path
        error: &error];
    
    if (error != NULL) {
        NSLog(@" error => %@ ", [error userInfo]);
        return;
    }
    
    uint32_t storable_results =
        (uint32_t)[results count] > recipient_capacity ?
            recipient_capacity
            : (uint32_t)[results count];
    
    for (
        uint32_t i = 0;
        i < storable_results;
        i++)
    {
        NSString * current_result = [results[i] lastPathComponent];
        
        filenames[i] = (char *)[current_result
            cStringUsingEncoding:NSASCIIStringEncoding];
        *recipient_size += 1;
    }
}

void
platform_get_application_path(char * recipient, const uint32_t recipient_size) {
    strcpy_capped(
        recipient,
        recipient_size,
        (char *)[[[NSBundle mainBundle] bundlePath]
            cStringUsingEncoding: NSASCIIStringEncoding]);
}

void platform_get_resources_path(char * recipient, const uint32_t recipient_size) {
    strcpy_capped(
        recipient,
        recipient_size,
        (char *)[[[NSBundle mainBundle] resourcePath] cStringUsingEncoding: NSASCIIStringEncoding]);
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
    
    dispatch_async(
        dispatch_get_global_queue(
            DISPATCH_QUEUE_PRIORITY_BACKGROUND,
            0),
        ^{
            function_to_run(argument);
        });
}

void platform_play_sound_resource(const char * resource_filename) {
    return;
    
    log_assert(resource_filename != NULL);
    
    char sound_pathfile[750];
    resource_filename_to_pathfile(
        /* filename: */
            resource_filename,
        /* recipient: */
            sound_pathfile,
        /* recipient_capacity: */
            750);
    
    log_assert(platform_resource_exists(resource_filename));
    
    NSString * soundPathFile = [NSString
        stringWithUTF8String: sound_pathfile];
    
    NSURL * soundFileURL = [NSURL
        fileURLWithPath: soundPathFile];
    
    NSError * error_value = nil;
    
    sound_players[next_sound_player] =
        [[AVAudioPlayer alloc]
            initWithContentsOfURL:soundFileURL
            error:&error_value];
    
    if (error_value != NULL) {
        log_append("break here");
        log_assert(0);
    }
    
    [sound_players[next_sound_player] setVolume: sound_volume];
    sound_players[next_sound_player].numberOfLoops = 0;
    // [sound_players[next_sound_player] setDelegate: NULL];
    [sound_players[next_sound_player] play];
    
    next_sound_player++;
    if (next_sound_player >= MAX_SIMUL_SOUNDS) {
        next_sound_player = 0;
    }
}

void platform_play_music_resource(const char * resource_filename) {
    return;
    if (active_music_player != NULL) {
        [active_music_player setVolume: 0.0f fadeDuration: 1];
    }
    
    char sound_pathfile[512];
    resource_filename_to_pathfile(
        /* filename: */
            resource_filename,
        /* recipient: */
            sound_pathfile,
        /* recipient_capacity: */
            512);
    
    NSString * soundPathFile = [NSString
        stringWithUTF8String: sound_pathfile];
    NSURL * soundFileURL = [NSURL
        fileURLWithPath: soundPathFile];
    
    AVAudioPlayer * player =
        [[AVAudioPlayer alloc]
            initWithContentsOfURL:soundFileURL
            error:nil];
    player.numberOfLoops = 0;
    
    [player setVolume: 0.001f];
    [player setVolume: music_volume fadeDuration: 5];
    [player play];
    
    active_music_player = player;
}

#define MUTEXES_SIZE 10
static pthread_mutex_t mutexes[MUTEXES_SIZE];
static uint32_t next_mutex_id = 0;
/*
creates a mutex and return the ID of said mutex for you to store
*/
uint32_t platform_init_mutex_and_return_id(void) {
    log_assert(next_mutex_id + 1 < MUTEXES_SIZE);
    int success = pthread_mutex_init(&(mutexes[next_mutex_id]), NULL);
    log_assert(success == 0);
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
    int return_value = pthread_mutex_lock(&(mutexes[mutex_id]));
    log_assert(return_value == 0);
    return;
}

void platform_mutex_unlock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    int return_value = pthread_mutex_unlock(&(mutexes[mutex_id]));
    log_assert(return_value == 0);
    return;
}
