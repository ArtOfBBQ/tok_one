#include "T1_os_apple_audio.h"

#include <CoreFoundation/CoreFoundation.h>

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_log.h"
#include "T1_stdint.h"
#include "T1_objc.h"

#define T1kAudioFormatLinearPCM 0x6C70636D
#define T1kLinearPCMFormatFlagIsSignedInteger (1L << 2) // 0x04
#define T1kLinearPCMFormatFlagIsPacked (1L << 3) // 0x08

typedef struct {
    int16_t  mSubframes;
    int16_t  mSubframeDivisor;
    uint32_t mCounter;
    uint32_t mType;
    uint32_t mFlags;
    int16_t  mHours;
    int16_t  mMinutes;
    int16_t  mSeconds;
    int16_t  mFrames;
} T1SMPTETime;

typedef struct {
    f64         mSampleTime;
    u64         mHostTime;
    f64         mRateScalar;
    u64         mWordClockTime;
    T1SMPTETime mSMPTETime;
    u32         mFlags;
    u32         mReserved;
} T1AudioTimeStamp;

typedef struct {
    s64  mStartOffset;
    u32  mVariableFramesInPacket;
    u32  mDataByteSize;
} T1AudioStreamPacketDescription;

typedef void * T1AudioQueueRef;

typedef struct {
    const u32 mAudioDataBytesCapacity;
    void *const mAudioData;
    u32 mAudioDataByteSize;
    void *mUserData;
    const u32 mPacketDescriptionCapacity;
    T1AudioStreamPacketDescription * const
        mPacketDescriptions;
    u32 mPacketDescriptionCount;
} T1AudioQueueBuffer;

typedef T1AudioQueueBuffer * T1AudioQueueBufferRef;

typedef void (*T1AudioQueueOutputCallback)(
    void *inUserData,
    T1AudioQueueRef inAQ,
    T1AudioQueueBufferRef inBuffer
);

typedef struct {
    f64 mSampleRate;
    u32 mFormatID;
    u32 mFormatFlags;
    u32 mBytesPerPacket;
    u32 mFramesPerPacket;
    u32 mBytesPerFrame;
    u32 mChannelsPerFrame;
    u32 mBitsPerChannel;
    u32 mReserved;
} T1AudioStreamBasicDescription;

typedef void (*AudioQueueOutputCallback)(
    void *inUserData,
    T1AudioQueueRef inAQ,
    T1AudioQueueBufferRef inBuffer
);

#if T1_AUDIO_ACTIVE == T1_ACTIVE
typedef struct {
    void * (* msg_send)(void *, void *);
    u32    (* msg_send_u32)(void *, void *);
    
    s32 (* audio_queue_new_output)(
        const T1AudioStreamBasicDescription *,
        T1AudioQueueOutputCallback,
        void *,
        CFRunLoopRef,
        CFStringRef,
        u32,
        T1AudioQueueRef *);
    s32 (* audio_queue_alloc_buffer)(
        T1AudioQueueRef,
        u32,
        T1AudioQueueBufferRef *);
    s32 (* audio_queue_start)(
        T1AudioQueueRef,
        const T1AudioTimeStamp *);
    s32 (* audio_queue_enqueue_buffer)(
        T1AudioQueueRef,
        T1AudioQueueBufferRef,
        u32,
        const T1AudioStreamPacketDescription *);
    
    u8 good;
} T1AppleAudioLibObjCPointers;

static T1AppleAudioLibObjCPointers * T1_aa_s = NULL;

void T1_apple_audio_init(u8 * good) {
    *good = 0;
    
    T1_aa_s = T1_mem_malloc_unmanaged(
        sizeof(T1AppleAudioLibObjCPointers));
    if (!T1_aa_s) { return; }
    T1_std_memset(T1_aa_s, 0, sizeof(T1AppleAudioLibObjCPointers));
    
    T1_objc_open_framework("/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox");
    
    T1_aa_s->audio_queue_new_output = T1_objc_get_func("AudioQueueNewOutput");
    if (T1_aa_s->audio_queue_new_output == 0) { return; }
    T1_aa_s->audio_queue_alloc_buffer = T1_objc_get_func("AudioQueueAllocateBuffer");
    if (T1_aa_s->audio_queue_alloc_buffer == 0) { return; }
    T1_aa_s->audio_queue_start = T1_objc_get_func("AudioQueueStart");
    if (T1_aa_s->audio_queue_start == 0) { return; }
    T1_aa_s->audio_queue_enqueue_buffer = T1_objc_get_func("AudioQueueEnqueueBuffer");
    if (T1_aa_s->audio_queue_enqueue_buffer == 0) { return; }
    
    *good = 1;
}

static void T1_apple_audio_callback(
    void * in_user_data,
    T1AudioQueueRef queue,
    T1AudioQueueBufferRef buffer)
{
    (void)in_user_data;
    
    T1_log_assert(buffer->mAudioDataByteSize ==
        buffer->mAudioDataBytesCapacity);
    
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
    
    s32 err = T1_aa_s->audio_queue_enqueue_buffer(
        queue, buffer, 0, NULL);
    if (err != 0) {
        T1_log_assert(0);
        return;
    }
}

void T1_apple_audio_start_loop(void) {
    u32 platform_buffer_size_bytes = 3000;
    
    // stereo 16-bit interleaved linear PCM audio data at 48kHz in SNORM format
    T1AudioStreamBasicDescription audio_stream_basic_description;
    // we consume 44.1k int16's per second consumed
    audio_stream_basic_description.mSampleRate = 44100.0f;
    audio_stream_basic_description.mFormatID = T1kAudioFormatLinearPCM;
    audio_stream_basic_description.mFormatFlags =
        T1kLinearPCMFormatFlagIsSignedInteger |
        T1kLinearPCMFormatFlagIsPacked;
    audio_stream_basic_description.mBytesPerPacket = 4;
    audio_stream_basic_description.mFramesPerPacket = 1;
    audio_stream_basic_description.mBytesPerFrame = 4; // 2x 16-bit ints
    audio_stream_basic_description.mChannelsPerFrame = 2;
    audio_stream_basic_description.mBitsPerChannel = 16;
    
    T1AudioQueueRef audio_queue_ref = 0;
    T1AudioQueueBufferRef audio_queue_buffer_refs[2];
    
    // most of the 0 and nullptr params here are for compressed sound
    // formats etc.
    s32 err = T1_aa_s->audio_queue_new_output(
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
        /* T1AudioQueueRef  _Nullable * _Nonnull outAQ: */
            &audio_queue_ref);
    
    if (err != 0) {
        assert(0);
        return;
    }
    
    err = T1_aa_s->audio_queue_alloc_buffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* UInt32 inBufferByteSize: */
            platform_buffer_size_bytes,
        /* T1AudioQueueBufferRef  _Nullable * _Nonnull outBuffer: */
            &audio_queue_buffer_refs[0]);
    
    if (err != 0) {
        assert(0);
        return;
    }
    
    T1AudioQueueBuffer * buf;
    buf = audio_queue_buffer_refs[0];
    buf->mAudioDataByteSize = platform_buffer_size_bytes;
    
    err = T1_aa_s->audio_queue_alloc_buffer(
        /* T1AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* UInt32 inBufferByteSize: */
            platform_buffer_size_bytes,
        /* T1AudioQueueBufferRef  _Nullable * _Nonnull outBuffer: */
            &audio_queue_buffer_refs[1]);
    
    if (err != 0) {
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
    
    T1_aa_s->audio_queue_enqueue_buffer(
        /* AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* T1AudioQueueBufferRef  _Nonnull inBuffer: */
            audio_queue_buffer_refs[0],
        /* UInt32 inNumPacketDescs (0 for constant bit rate): */
            0,
        /* const AudioStreamPacketDescription * _Nullable inPacketDescs: :*/
            NULL);
    T1_aa_s->audio_queue_enqueue_buffer(
        /* T1AudioQueueRef  _Nonnull inAQ: */
            audio_queue_ref,
        /* T1AudioQueueBufferRef  _Nonnull inBuffer: */
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
#endif // T1_AUDIO_ACTIVE
