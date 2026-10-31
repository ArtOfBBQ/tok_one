#include "T1_os.h"

#include "T1_std.h"
#include "T1_assert.h"
#include "T1_audio.h"
#include "T1_win32.h"

#if T1_AUDIO_ACTIVE == T1_ACTIVE

/*
Sound through WASAPI, the API under all of Windows' sound since Vista,
loaded from ole32.dll at runtime and called through its COM vtables,
declared here instead of including <audioclient.h>.

We share the default output device with other apps: Windows' audio engine
mixes us in, converting T1's 44.1 kHz 16-bit stereo to the device's format.
It wakes our thread every period (usually 10 ms) to top up our buffer with
T1's mix, like the 2 buffers macOS's audio queue asks us to fill.

Everything COM happens on that thread: COM makes a thread choose how it
works with COM objects, and we don't choose that for the app's main thread.
*/

#define T1_WASAPI_SAMPLE_RATE 44100
#define T1_WASAPI_FRAME_BYTES 4 // 2 channels of 16 bits

// If the device is gone (e.g. unplugged), we try the new default this often
#define T1_WASAPI_REOPEN_MS 500
// and whether the default output changed (in Windows' settings) this often
#define T1_WASAPI_DEFAULT_CHECK_MS 1000
/*
Our buffer, in 100 ns units: room for T1_AUDIO_PLATFORM_FRAMES (rounded
up), what macOS's audio queue holds. Windows may give us more, we never
fill more than that (see T1_os_win_audio_fill()), so both platforms play
the same distance behind the mix. The engine wakes us every period (10 ms)
to top it up, a late wake is heard only once it's about 24 ms late
*/
#define T1_WASAPI_BUFFER_100NS \
    (((s64)T1_AUDIO_PLATFORM_FRAMES * 10000000 + \
        T1_WASAPI_SAMPLE_RATE - 1) / T1_WASAPI_SAMPLE_RATE)
// a device's id, cut to this many characters (they're about 55)
#define T1_WASAPI_DEVICE_ID_CAP 256
// T1_os_audio_init() waits this long for the audio thread to open the device
#define T1_WASAPI_OPEN_TIMEOUT_MS 3000

#define T1COINIT_MULTITHREADED 0x0u
#define T1CLSCTX_ALL 0x17u
#define T1eRender 0
#define T1eConsole 0
#define T1AUDCLNT_SHAREMODE_SHARED 0
#define T1AUDCLNT_STREAMFLAGS_EVENTCALLBACK 0x00040000u
#define T1AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000u
#define T1AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000u
#define T1WAVE_FORMAT_PCM 1

typedef struct {
    u32 data1;
    u16 data2;
    u16 data3;
    u8  data4[8];
} T1WinGUID;

static const T1WinGUID T1_CLSID_MMDeviceEnumerator = {
    0xBCDE0395, 0xE52F, 0x467C,
    {0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E}};
static const T1WinGUID T1_IID_IMMDeviceEnumerator = {
    0xA95664D2, 0x9614, 0x4F35,
    {0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6}};
static const T1WinGUID T1_IID_IAudioClient = {
    0x1CB9AD4C, 0xDBFA, 0x4C32,
    {0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2}};
static const T1WinGUID T1_IID_IAudioRenderClient = {
    0xF294ACFC, 0x3146, 0x4483,
    {0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2}};

typedef struct {
    u16 format_tag;
    u16 channels;
    u32 samples_per_sec;
    u32 avg_bytes_per_sec;
    u16 block_align;
    u16 bits_per_sample;
    u16 size;
} T1WaveFormatEx;

typedef struct T1MMDeviceEnumerator T1MMDeviceEnumerator;
typedef struct T1MMDevice T1MMDevice;
typedef struct T1AudioClient T1AudioClient;
typedef struct T1AudioRenderClient T1AudioRenderClient;

// The vtables, each up to the last method we call

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1MMDeviceEnumerator *);
    void * EnumAudioEndpoints;
    s32 (T1_WINAPI * GetDefaultAudioEndpoint)(
        T1MMDeviceEnumerator *,
        s32 data_flow,
        s32 role,
        T1MMDevice **);
} T1MMDeviceEnumeratorVtbl;

struct T1MMDeviceEnumerator {
    const T1MMDeviceEnumeratorVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1MMDevice *);
    s32 (T1_WINAPI * Activate)(
        T1MMDevice *,
        const T1WinGUID * iid,
        u32 cls_context,
        void * activation_params,
        void ** interface);
    void * OpenPropertyStore;
    // the id string is the caller's, freed with CoTaskMemFree()
    s32 (T1_WINAPI * GetId)(T1MMDevice *, T1WChar ** id);
} T1MMDeviceVtbl;

struct T1MMDevice {
    const T1MMDeviceVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1AudioClient *);
    s32 (T1_WINAPI * Initialize)(
        T1AudioClient *,
        s32 share_mode,
        u32 stream_flags,
        s64 buffer_duration, // in 100 ns units
        s64 periodicity,
        const T1WaveFormatEx * format,
        const T1WinGUID * session);
    s32 (T1_WINAPI * GetBufferSize)(T1AudioClient *, u32 * frames);
    void * GetStreamLatency;
    s32 (T1_WINAPI * GetCurrentPadding)(T1AudioClient *, u32 * frames);
    void * IsFormatSupported;
    void * GetMixFormat;
    void * GetDevicePeriod;
    s32 (T1_WINAPI * Start)(T1AudioClient *);
    void * Stop;
    void * Reset;
    s32 (T1_WINAPI * SetEventHandle)(T1AudioClient *, T1WinHandle);
    s32 (T1_WINAPI * GetService)(
        T1AudioClient *,
        const T1WinGUID * iid,
        void ** service);
} T1AudioClientVtbl;

struct T1AudioClient {
    const T1AudioClientVtbl * vtbl;
};

typedef struct {
    void * QueryInterface;
    void * AddRef;
    u32 (T1_WINAPI * Release)(T1AudioRenderClient *);
    s32 (T1_WINAPI * GetBuffer)(
        T1AudioRenderClient *,
        u32 frames,
        u8 ** data);
    s32 (T1_WINAPI * ReleaseBuffer)(
        T1AudioRenderClient *,
        u32 frames,
        u32 flags);
} T1AudioRenderClientVtbl;

struct T1AudioRenderClient {
    const T1AudioRenderClientVtbl * vtbl;
};

typedef s32 (T1_WINAPI * T1CoInitializeExFn)(void *, u32);
typedef s32 (T1_WINAPI * T1CoCreateInstanceFn)(
    const T1WinGUID * clsid,
    void * outer,
    u32 cls_context,
    const T1WinGUID * iid,
    void ** object);
typedef void (T1_WINAPI * T1CoTaskMemFreeFn)(void *);
typedef T1WinHandle (T1_WINAPI * T1AvSetMmThreadCharacteristicsWFn)(
    const T1WChar * task,
    u32 * task_index);

typedef struct {
    T1CoInitializeExFn CoInitializeEx;
    T1CoCreateInstanceFn CoCreateInstance;
    T1CoTaskMemFreeFn CoTaskMemFree;
    T1AvSetMmThreadCharacteristicsWFn AvSetMmThreadCharacteristicsW;
    T1MMDeviceEnumerator * enumerator;
    T1MMDevice * device;
    T1AudioClient * client;
    T1AudioRenderClient * render;
    T1WinHandle wake; // the audio engine sets it when it wants samples
    T1WinHandle opened; // the audio thread tried to open the device
    T1WinHandle start; // T1_os_audio_start_loop() was called
    u32 buffer_frames;
    T1WChar device_id[T1_WASAPI_DEVICE_ID_CAP]; // the open device's
    u64 default_checked_ms;
    b8 good;
} T1OSWindowsAudioState;

static T1OSWindowsAudioState T1_os_win_audio_s;

static void T1_os_win_audio_get_fn(
    void * fn,
    T1WinHandle dll,
    const char * name)
{
    T1WinFarProc found = dll != NULL ? GetProcAddress(dll, name) : NULL;
    T1_std_memcpy(fn, &found, sizeof(T1WinFarProc));
}

static void T1_os_win_audio_close(void) {
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;

    if (s->render != NULL) { s->render->vtbl->Release(s->render); }
    if (s->client != NULL) { s->client->vtbl->Release(s->client); }
    if (s->device != NULL) { s->device->vtbl->Release(s->device); }
    s->render = NULL;
    s->client = NULL;
    s->device = NULL;
    s->buffer_frames = 0;
    s->device_id[0] = 0;
}

// A device's id in recip, empty if it has none
static void T1_os_win_audio_device_id(T1MMDevice * device, T1WChar * recip) {
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;
    recip[0] = 0;
    T1WChar * id = NULL;
    if (device->vtbl->GetId(device, &id) < 0 || id == NULL) { return; }
    u32 i = 0;
    while (id[i] != 0 && i + 1 < T1_WASAPI_DEVICE_ID_CAP) {
        recip[i] = id[i];
        i++;
    }
    recip[i] = 0;
    s->CoTaskMemFree(id);
}

/*
Whether Windows' default output is another device than ours now (the
user picked another one), which isn't an error of our stream: it keeps
playing on the old device as long as that's there
*/
static b8 T1_os_win_audio_default_changed(void) {
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;
    T1MMDevice * device = NULL;
    if (
        s->enumerator->vtbl->GetDefaultAudioEndpoint(
            s->enumerator, T1eRender, T1eConsole, &device) < 0 ||
        device == NULL)
    {
        return 0;
    }
    T1WChar id[T1_WASAPI_DEVICE_ID_CAP];
    T1_os_win_audio_device_id(device, id);
    device->vtbl->Release(device);
    if (id[0] == 0 || s->device_id[0] == 0) { return 0; }
    for (u32 i = 0; i < T1_WASAPI_DEVICE_ID_CAP; i++) {
        if (id[i] != s->device_id[i]) { return 1; }
        if (id[i] == 0) { break; }
    }
    return 0;
}

/*
Hands the audio engine T1's mix up to T1_AUDIO_PLATFORM_FRAMES ahead, the
part it hasn't played yet ("padding") stays. Not all the room in our
buffer: Windows may have made it bigger than we asked
*/
static s32 T1_os_win_audio_fill(void) {
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;

    u32 padding = 0;
    s32 result = s->client->vtbl->GetCurrentPadding(s->client, &padding);
    if (result < 0) { return result; }

    u32 target = s->buffer_frames < T1_AUDIO_PLATFORM_FRAMES ?
        s->buffer_frames : T1_AUDIO_PLATFORM_FRAMES;
    if (padding >= target) { return 0; }
    u32 frames = target - padding;

    u8 * data = NULL;
    result = s->render->vtbl->GetBuffer(s->render, frames, &data);
    if (result < 0) { return result; }

    T1_audio_consume_int16_samples((s16 *)data, frames * 2);

    return s->render->vtbl->ReleaseBuffer(s->render, frames, 0);
}

/*
Opens the default output device for T1's format, with our buffer full,
ready to Start()
*/
static b8 T1_os_win_audio_open(void) {
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;

    if (
        s->enumerator->vtbl->GetDefaultAudioEndpoint(
            s->enumerator,
            T1eRender,
            T1eConsole,
            &s->device) < 0 ||
        s->device->vtbl->Activate(
            s->device,
            &T1_IID_IAudioClient,
            T1CLSCTX_ALL,
            NULL,
            (void **)&s->client) < 0)
    {
        T1_os_win_audio_close();
        return 0;
    }
    T1_os_win_audio_device_id(s->device, s->device_id);

    T1WaveFormatEx format;
    T1_std_memset(&format, 0, sizeof(T1WaveFormatEx));
    format.format_tag = T1WAVE_FORMAT_PCM;
    format.channels = 2;
    format.samples_per_sec = T1_WASAPI_SAMPLE_RATE;
    format.bits_per_sample = 16;
    format.block_align = T1_WASAPI_FRAME_BYTES;
    format.avg_bytes_per_sec = T1_WASAPI_SAMPLE_RATE * T1_WASAPI_FRAME_BYTES;

    if (
        s->client->vtbl->Initialize(
            s->client,
            T1AUDCLNT_SHAREMODE_SHARED,
            T1AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                T1AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                T1AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
            T1_WASAPI_BUFFER_100NS,
            0,
            &format,
            NULL) < 0 ||
        s->client->vtbl->SetEventHandle(s->client, s->wake) < 0 ||
        s->client->vtbl->GetBufferSize(s->client, &s->buffer_frames) < 0 ||
        s->client->vtbl->GetService(
            s->client,
            &T1_IID_IAudioRenderClient,
            (void **)&s->render) < 0 ||
        T1_os_win_audio_fill() < 0)
    {
        T1_os_win_audio_close();
        return 0;
    }

    return 1;
}

static void * T1_os_win_audio_thread(void * unused) {
    (void)unused;
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;

    s->good =
        s->CoInitializeEx(NULL, T1COINIT_MULTITHREADED) >= 0 &&
        s->CoCreateInstance(
            &T1_CLSID_MMDeviceEnumerator,
            NULL,
            T1CLSCTX_ALL,
            &T1_IID_IMMDeviceEnumerator,
            (void **)&s->enumerator) >= 0 &&
        T1_os_win_audio_open();
    SetEvent(s->opened);
    if (!s->good) { return NULL; }

    WaitForSingleObject(s->start, T1INFINITE);

    /*
    Windows' scheduler for audio threads (MMCSS) runs us right away when
    the engine wakes us. Without it, the highest priority comes close
    */
    u32 task_index = 0;
    if (
        s->AvSetMmThreadCharacteristicsW == NULL ||
        s->AvSetMmThreadCharacteristicsW(L"Pro Audio", &task_index) == NULL)
    {
        SetThreadPriority(GetCurrentThread(), T1THREAD_PRIORITY_TIME_CRITICAL);
    }

    s->client->vtbl->Start(s->client);

    // until the process ends
    while (1) {
        WaitForSingleObject(s->wake, T1_WASAPI_REOPEN_MS);

        if (s->client == NULL) {
            // the device went away, the new default may be there now
            if (T1_os_win_audio_open()) {
                s->client->vtbl->Start(s->client);
            }
            continue;
        }

        /*
        Any error ends the stream (e.g. headphones unplugged, the audio
        service restarted): we open the default device again
        */
        if (T1_os_win_audio_fill() < 0) {
            T1_os_win_audio_close();
            continue;
        }

        u64 now_ms = GetTickCount64();
        if (now_ms - s->default_checked_ms >= T1_WASAPI_DEFAULT_CHECK_MS) {
            s->default_checked_ms = now_ms;
            if (T1_os_win_audio_default_changed()) {
                T1_os_win_audio_close();
            }
        }
    }

    return NULL;
}

void T1_os_audio_init(u8 * good) {
    *good = 0;
    T1OSWindowsAudioState * s = &T1_os_win_audio_s;
    T1_std_memset(s, 0, sizeof(T1OSWindowsAudioState));

    T1WinHandle ole32 = LoadLibraryW(L"ole32.dll");
    T1_os_win_audio_get_fn(&s->CoInitializeEx, ole32, "CoInitializeEx");
    T1_os_win_audio_get_fn(&s->CoCreateInstance, ole32, "CoCreateInstance");
    T1_os_win_audio_get_fn(&s->CoTaskMemFree, ole32, "CoTaskMemFree");
    if (
        s->CoInitializeEx == NULL ||
        s->CoCreateInstance == NULL ||
        s->CoTaskMemFree == NULL)
    {
        return;
    }
    // optional
    T1_os_win_audio_get_fn(
        &s->AvSetMmThreadCharacteristicsW,
        LoadLibraryW(L"avrt.dll"),
        "AvSetMmThreadCharacteristicsW");

    s->wake = CreateEventW(NULL, 0, 0, NULL);
    s->opened = CreateEventW(NULL, 0, 0, NULL);
    s->start = CreateEventW(NULL, 0, 0, NULL);
    if (s->wake == NULL || s->opened == NULL || s->start == NULL) {
        return;
    }

    T1_os_start_thread(T1_os_win_audio_thread, NULL);

    if (
        WaitForSingleObject(s->opened, T1_WASAPI_OPEN_TIMEOUT_MS) ==
            T1WAIT_OBJECT_0)
    {
        *good = s->good;
    }
}

void T1_os_audio_start_loop(void) {
    SetEvent(T1_os_win_audio_s.start);
}

#elif T1_AUDIO_ACTIVE == T1_INACTIVE
// Pass
#else
#error "T1_AUDIO_ACTIVE undefined"
#endif // T1_AUDIO_ACTIVE
