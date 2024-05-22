#import "apple_audio.h"

static void audio_callback(
    void * in_user_data,
    AudioQueueRef queue,
    AudioQueueBufferRef buffer)
{
    (void)in_user_data;
    
    assert(
        buffer->mAudioDataByteSize == buffer->mAudioDataBytesCapacity);
    
    // we're just filling the entire buffer here
    // In a real game we might only fill part of the buffer and set the
    // mAudioDataBytes accordingly.
    uint32_t samples_to_copy = buffer->mAudioDataBytesCapacity / 2;
    // uint32_t frames_to_copy_both_runs = bytes_to_copy_both_runs / 4;
    // buffer->mAudioDataByteSize = bytes_to_copy_both_runs;
    
    int16_t * platform_buffer_at = (int16_t *)buffer->mAudioData;
    
    for (uint32_t _ = 0; _ < samples_to_copy; _++) {
        int32_t new_val = (int32_t)(
            (float)sound_settings->samples_buffer[
                (sound_settings->play_cursor) %
                    sound_settings->global_samples_size] *
                        sound_settings->volume);
        new_val = new_val > INT16_MAX ? INT16_MAX : new_val;
        new_val = new_val < INT16_MIN ? INT16_MIN : new_val;
        *platform_buffer_at++ = (int16_t)new_val;
        sound_settings->play_cursor += 1;
    }
    
    log_assert(sound_settings->play_cursor < UINT32_MAX * 10);
    // sound_settings->play_cursor %= sound_settings->global_samples_size;
    
    OSStatus err = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    assert(!err);
}

void start_audio_loop(void) {
    uint32_t platform_buffer_size_bytes = 12000; // (44100 / 60) * 4;
    
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
    assert(!err);
    
    err = AudioQueueAllocateBuffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* UInt32 inBufferByteSize: */
            platform_buffer_size_bytes,
        /* AudioQueueBufferRef  _Nullable * _Nonnull outBuffer: */
            &audio_queue_buffer_refs[0]);
    assert(!err);
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
    assert(!err);
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
    
    // go!
    AudioQueueStart(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* const AudioTimeStamp * _Nullable inStartTime: */
            0);
}
