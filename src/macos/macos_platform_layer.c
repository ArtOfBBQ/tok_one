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
        printf("file_url nil!\n");
        assert(0);
    }
    
    NSError * error = NULL; 
    NSData * file_data =
        [NSData
            dataWithContentsOfURL:file_url
            options:NSDataReadingUncached
            error:&error];
    
    NSString * debug_string = [
        [NSString alloc]
            initWithData:file_data
            encoding:NSASCIIStringEncoding];
    NSLog(@"read NSData: %@", debug_string);
    
    if (file_data == nil) {
        NSLog(
            @"error => %@ ",
            [error userInfo]);
        assert(0);
    } else {
        NSLog(@"Succesfully read data");
    }
    
    if (out_preallocatedbuffer->size >
        [file_data length])
    {
        printf(
            "adjusting buffer size to:"
            FUINT64
            "\n",
            (uint64_t)[file_data length]);
        out_preallocatedbuffer->size = [file_data length];
    }
    
    [file_data
        getBytes:out_preallocatedbuffer->contents
        range:NSMakeRange(0, out_preallocatedbuffer->size-1)];
        // length:out_preallocatedbuffer->size];
    
    out_preallocatedbuffer->contents[
    out_preallocatedbuffer->size - 1] = '\0';
    
    for (uint32_t i = 0; i < out_preallocatedbuffer->size - 1; i++) {
        if (
            out_preallocatedbuffer->contents[i] !=
                ((char *)[file_data bytes])[i])
        {
            printf(
                "out_preallocatedbuffer->contents[%u]: %c\n",
                i,
                out_preallocatedbuffer->contents[i]);
            printf(
                "(char *)[file_data bytes])[%u]: %c\n",
                i,
                ((char *)[file_data bytes])[i]);
            assert(0);
        }
    }
    
    printf(
        "out_preallocatedbuffer->contents:\n%s\n",
        out_preallocatedbuffer->contents);
    printf("\n");
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

