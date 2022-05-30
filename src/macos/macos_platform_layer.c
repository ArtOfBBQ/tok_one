#define PLATFORM_NS_FILEMANAGER

#ifndef FUINT64 
#define FUINT64 "%llu"
#endif

#include "../shared/platform_layer.h"

NSFileManager * file_manager;

/*
Get a file's size. Returns -1 if no such file
*/
int64_t platform_get_filesize(const char * filename)
{
    NSString * nsfilename = [NSString
        stringWithUTF8String:filename];
    
    NSURL * file_url = [[NSBundle mainBundle]
        URLForResource:[nsfilename stringByDeletingPathExtension]
        withExtension: [nsfilename pathExtension]];
        
    NSError * error_value = nil;
    NSNumber * file_size;
    
    [file_url
        getResourceValue:&file_size
        forKey:NSURLFileSizeKey
        error:&error_value];
    
    if (error_value != nil)
    {
        NSLog(@" error => %@ ", [error_value userInfo]);
        assert(0);
        return -1;
    }
    
    // let's not use 20MB+ files in development
    assert(file_size.intValue < 20000000);
    
    return file_size.intValue;
}

void platform_read_file(
    const char * filename,
    FileBuffer * out_preallocatedbuffer)
{
    printf(
        "platform_read_file: %s into buffer of size: " 
        FUINT64
        "\n",
        filename,
        out_preallocatedbuffer->size);
    
    NSString * nsfilename = [NSString
        stringWithUTF8String:filename];
    
    NSURL * file_url = [[NSBundle mainBundle]
        URLForResource:[nsfilename stringByDeletingPathExtension]
        withExtension: [nsfilename pathExtension]];
    
    if (file_url == nil) {
        printf(
            "couldn't find file: %s\n",
            filename);
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        return;
    }
    
    NSInputStream * input_stream = [NSInputStream
        inputStreamWithURL:file_url];
    [input_stream open];
    
    NSInteger result = [input_stream
        read: (uint8_t *)out_preallocatedbuffer->contents
        maxLength: out_preallocatedbuffer->size - 1];
    
    if (result < 1) {
        NSError * stream_error = input_stream.streamError;
        if (stream_error != NULL) {
            NSLog(@" error => %@ ", [stream_error userInfo]);
        }
        out_preallocatedbuffer->size = 0;
        out_preallocatedbuffer->good = false;
        [input_stream close];
        return;
    }
    
    out_preallocatedbuffer->size = (uint64_t)result + 1;
    out_preallocatedbuffer->
        contents[out_preallocatedbuffer->size - 1] = '\0';
    out_preallocatedbuffer->good = true;
    [input_stream close];
}

char * platform_get_cwd()
{
    if (file_manager == NULL) {
        file_manager =
            [[NSFileManager alloc] init];
    }
    
    NSString * cwd = [file_manager currentDirectoryPath];
    NSLog(@"%@", cwd);
    
    char * return_value = (char *)[cwd UTF8String];
    
    return return_value;
}

