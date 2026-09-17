#import "T1_apple_audio.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_objc.h"

#include <AudioToolbox/AudioToolbox.h>

#if T1_AUDIO_ACTIVE == T1_ACTIVE
typedef struct {
    void * (* msg_send)(void *, void *);
    u32    (* msg_send_u32)(void *, void *);
    
    OSStatus (* audio_queue_new_output)(
        const AudioStreamBasicDescription *,
        AudioQueueOutputCallback,
        void *,
        CFRunLoopRef,
        CFStringRef,
        UInt32,
        AudioQueueRef *);
    OSStatus (* audio_queue_alloc_buffer)(
        AudioQueueRef,
        UInt32,
        AudioQueueBufferRef *);
    OSStatus (* audio_queue_start)(
        AudioQueueRef,
        const AudioTimeStamp *);
    OSStatus (* audio_queue_enqueue_buffer)(
        AudioQueueRef,
        AudioQueueBufferRef,
        UInt32,
        const AudioStreamPacketDescription *);
    
    void * class_GCController;
    void * sel_someproperty;
    u8 good;
} T1AppleAudioLibObjCPointers;

static T1AppleAudioLibObjCPointers * T1_aa_s = NULL;

void T1_apple_audio_init(void) {
    T1_aa_s = T1_mem_malloc_unmanaged(
        sizeof(T1AppleAudioLibObjCPointers));
    if (!T1_aa_s) { return; }
    T1_std_memset(T1_aa_s, 0, sizeof(T1AppleAudioLibObjCPointers));
    
    T1_objc_open_framework_and_link_perma_good_val(
        "/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox",
        &T1_aa_s->good);
    
    T1_aa_s->audio_queue_new_output = T1_objc_get_func("AudioQueueNewOutput");
    T1_aa_s->audio_queue_alloc_buffer = T1_objc_get_func("AudioQueueAllocateBuffer");
    T1_aa_s->audio_queue_start = T1_objc_get_func("AudioQueueStart");
    T1_aa_s->audio_queue_enqueue_buffer = T1_objc_get_func("AudioQueueEnqueueBuffer");
    
    assert(T1_aa_s->audio_queue_new_output);
    // T1_aa_s->class_GCController = T1_objc_get_class("GCController");
    // T1_aa_s->sel_x = T1_objc_reg_sel("current");
    
    T1_objc_close_current_framework();
}

static void T1_apple_audio_callback(
    void * in_user_data,
    AudioQueueRef queue,
    AudioQueueBufferRef buffer)
{
    (void)in_user_data;
    
    assert(
        buffer->mAudioDataByteSize == buffer->mAudioDataBytesCapacity);
    
    u32 audio_data_cap = buffer->mAudioDataBytesCapacity;
    u32 samples_to_copy = audio_data_cap / 2;
    // u32 frames_to_copy_both_runs = bytes_to_copy_both_runs / 4;
    // buffer->mAudioDataByteSize = bytes_to_copy_both_runs;
    
    int16_t * platform_buffer_at = (int16_t *)buffer->mAudioData;
    
    T1_audio_consume_int16_samples(
        /* int16_t * recipient: */
            platform_buffer_at,
        /* const u32 samples_to_copy: */
            samples_to_copy);
    
    OSStatus err = T1_aa_s->audio_queue_enqueue_buffer(queue, buffer, 0, NULL);
    if (err != noErr) {
        assert(0);
        return;
    }
}

void T1_apple_audio_start_loop(void) {
    u32 platform_buffer_size_bytes = 3000;
    
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
    OSStatus err = T1_aa_s->audio_queue_new_output(
        /* const AudioStreamBasicDescription * _Nonnull inFormat: */
            &audio_stream_basic_description,
        /* AudioQueueOutputCallback  _Nonnull inCallbackProc: */
            &T1_apple_audio_callback,
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
    
    err = T1_aa_s->audio_queue_alloc_buffer(
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
    
    err = T1_aa_s->audio_queue_alloc_buffer(
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
    
    T1_apple_audio_callback(
        NULL,
        audio_queue_ref,
        audio_queue_buffer_refs[0]);
    T1_apple_audio_callback(
        NULL,
        audio_queue_ref,
        audio_queue_buffer_refs[1]);
    
    // enqueue for playing
    T1_aa_s->audio_queue_enqueue_buffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* AudioQueueBufferRef  _Nonnull inBuffer: */
            audio_queue_buffer_refs[0],
        /* UInt32 inNumPacketDescs (0 for constant bit rate): */
            0,
        /* const AudioStreamPacketDescription * _Nullable inPacketDescs: :*/
            NULL);
    T1_aa_s->audio_queue_enqueue_buffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* AudioQueueBufferRef  _Nonnull inBuffer: */
            audio_queue_buffer_refs[1],
        /* UInt32 inNumPacketDescs (0 for constant bit rate): */
            0,
        /* const AudioStreamPacketDescription * _Nullable inPacketDescs: :*/
            NULL);
    
    T1_aa_s->audio_queue_start(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* const AudioTimeStamp * _Nullable inStartTime: */
            0);
}
#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // AUDIO_ACTIVE
