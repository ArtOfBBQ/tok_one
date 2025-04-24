#import "apple_audio.h"


static void audio_callback(
    void * in_user_data,
    AudioQueueRef queue,
    AudioQueueBufferRef buffer)
{
    (void)in_user_data;
    
    assert(
        buffer->mAudioDataByteSize == buffer->mAudioDataBytesCapacity);
    
    uint32_t audio_data_cap = buffer->mAudioDataBytesCapacity;
    uint32_t samples_to_copy = audio_data_cap / 2;
    // uint32_t frames_to_copy_both_runs = bytes_to_copy_both_runs / 4;
    // buffer->mAudioDataByteSize = bytes_to_copy_both_runs;
    
    int16_t * platform_buffer_at = (int16_t *)buffer->mAudioData;
    
    audio_consume_int16_samples(
        /* int16_t * recipient: */
            platform_buffer_at,
        /* const uint32_t samples_to_copy: */
            samples_to_copy);
    
    OSStatus err = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    if (err != noErr) {
        assert(0);
        return;
    }
}

void start_audio_loop(void) {
    uint32_t platform_buffer_size_bytes = 3000;
    
    // stereo 16-bit interleaved linear PCM audio data at 48kHz in SNORM format
    AudioStreamBasicDescription audio_stream_basic_description;
    // we consume 44.1k int16's per second consumed
    audio_stream_basic_description.mSampleRate = 44100.0f;
    audio_stream_basic_description.mFormatID = kAudioFormatLinearPCM;
    audio_stream_basic_description.mFormatFlags =
        kLinearPCMFormatFlagIsSignedInteger |
        kLinearPCMFormatFlagIsPacked;
    audio_stream_basic_description.mBytesPerPacket = 4;
    audio_stream_basic_description.mFramesPerPacket = 1;
    audio_stream_basic_description.mBytesPerFrame = 4; // 2x 16-bit ints
    audio_stream_basic_description.mChannelsPerFrame = 2;
    audio_stream_basic_description.mBitsPerChannel = 16;
    
    AudioQueueRef audio_queue_ref = 0;
    AudioQueueBufferRef audio_queue_buffer_refs[2];
    
    // most of the 0 and nullptr params here are for compressed sound
    // formats etc.
    OSStatus err = AudioQueueNewOutput(
        /* const AudioStreamBasicDescription * _Nonnull inFormat: */
            &audio_stream_basic_description,
        /* AudioQueueOutputCallback  _Nonnull inCallbackProc: */
            &audio_callback,
        /* void * _Nullable inUserData: */
            NULL,
        /* CFRunLoopRef  _Nullable inCallbackRunLoop: */
            0,
        /* CFStringRef  _Nullable inCallbackRunLoopMode: */
            0,
        /* UInt32 inFlags:*/
            0,
        /* AudioQueueRef  _Nullable * _Nonnull outAQ: */
            &audio_queue_ref);
    
    if (err != noErr) {
        assert(0);
        return;
    }
    
    err = AudioQueueAllocateBuffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* UInt32 inBufferByteSize: */
            platform_buffer_size_bytes,
        /* AudioQueueBufferRef  _Nullable * _Nonnull outBuffer: */
            &audio_queue_buffer_refs[0]);
    
    if (err != noErr) {
        assert(0);
        return;
    }
    
    AudioQueueBuffer * buf;
    buf = audio_queue_buffer_refs[0];
    buf->mAudioDataByteSize = platform_buffer_size_bytes;
    
    err = AudioQueueAllocateBuffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* UInt32 inBufferByteSize: */
            platform_buffer_size_bytes,
        /* AudioQueueBufferRef  _Nullable * _Nonnull outBuffer: */
            &audio_queue_buffer_refs[1]);
    
    if (err != noErr) {
        assert(0);
        return;
    }
    
    buf = audio_queue_buffer_refs[1];
    buf->mAudioDataByteSize = platform_buffer_size_bytes;
    
    audio_callback(NULL, audio_queue_ref, audio_queue_buffer_refs[0]);
    audio_callback(NULL, audio_queue_ref, audio_queue_buffer_refs[1]);
    
    // enqueue for playing
    AudioQueueEnqueueBuffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* AudioQueueBufferRef  _Nonnull inBuffer: */
            audio_queue_buffer_refs[0],
        /* UInt32 inNumPacketDescs (0 for constant bit rate): */
            0,
        /* const AudioStreamPacketDescription * _Nullable inPacketDescs: :*/
            NULL);
    AudioQueueEnqueueBuffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* AudioQueueBufferRef  _Nonnull inBuffer: */
            audio_queue_buffer_refs[1],
        /* UInt32 inNumPacketDescs (0 for constant bit rate): */
            0,
        /* const AudioStreamPacketDescription * _Nullable inPacketDescs: :*/
            NULL);
    
    AudioQueueStart(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* const AudioTimeStamp * _Nullable inStartTime: */
            0);
}
