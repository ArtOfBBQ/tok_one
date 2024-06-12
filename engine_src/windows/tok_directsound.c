#include "tok_directsound.h"

static LPDIRECTSOUNDBUFFER secondary_buffer;

long (* extptr_DirectSoundCreate)(
    LPGUID lpGuid,
    LPDIRECTSOUND * ppDS,
    LPUNKNOWN pUnkOuter) = NULL;

void init_directsound(
    HWND top_window_handle,
    const uint32_t audio_buffer_size,
    unsigned int * success)
{
    // DirectSound
    HMODULE module = LoadLibraryA("dsound.dll");
    if (!module) {
        MessageBox(
            /* HWND hWnd: */
                0,
            /* LPCSTR lpText: */
                "Couldn't find dsound.dll",
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        
        *success = 0;
        return;
    }
    
    extptr_DirectSoundCreate = (void *)GetProcAddress(
        module,
        "DirectSoundCreate");
    if (extptr_DirectSoundCreate == NULL) {
        MessageBox(
            /* HWND hWnd: */
                0,
            /* LPCSTR lpText: */
                "Failed to get the address for DirectSoundCreate()",
            /* LPCSTR lpCaption: */
                "Error",
            /* UINT uType: */
                MB_OK);
        
        *success = 0;
        return;
    }
    
    LPDIRECTSOUND direct_sound_handle;
    HRESULT dsound_result = 
        extptr_DirectSoundCreate(0, &direct_sound_handle, 0);
   
    switch (dsound_result) {
        case DS_OK:
            
            break;
        case DSERR_OUTOFMEMORY:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to load DirectSound (Out of memory)",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        case DSERR_NODRIVER:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to load DirectSound (No driver)",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to load DirectSound",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
    }
    
    HRESULT coopset_result =
        direct_sound_handle->lpVtbl->SetCooperativeLevel(
            direct_sound_handle,
            top_window_handle,
            DSSCL_PRIORITY);
    
    switch (coopset_result) {
        case DS_OK:
            break;
        case DSERR_INVALIDPARAM:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to set DirectSound cooperative level"
                    " (DSERR_INVALIDPARAM)",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to set DirectSound cooperative level",
                /* LPCSTR lpCaption: */
                    "Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
    }
    
    DSBUFFERDESC buffer_description;
    memset(&buffer_description, 0, sizeof(DSBUFFERDESC));
    buffer_description.dwSize = sizeof(DSBUFFERDESC); // lol msft
    buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
    
    LPDIRECTSOUNDBUFFER primary_buffer;
    HRESULT created_result = IDirectSound_CreateSoundBuffer(
            direct_sound_handle,
        /* LPCDSBUFFERDESC lpcDSBufferDesc: */
            &buffer_description,
        /* LPLPDIRECTSOUNDBUFFER lplpDirectSoundBuffer: */
            &primary_buffer,
        /* IUnknown FAR* pUnkOuter: */
            NULL);
    
    switch (created_result) {
        case DS_OK:
            break;
        case DSERR_INVALIDPARAM:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Found DirectSound, but got DSERR_INVALIDPARAM when "
                    "trying to create primary buffer",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Found DirectSound, but failed to create primary buffer",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
    }
    
    // Primary buffer was created, now we can do the other obligatory
    // dance steps
    WAVEFORMATEX wave_format;
    memset(&wave_format, 0, sizeof(WAVEFORMATEX));
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = 44100;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = 4;
    wave_format.nAvgBytesPerSec =
        wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    
    HRESULT set_format_result = IDirectSoundBuffer_SetFormat(
        primary_buffer,
        &wave_format);
    
    switch (set_format_result) {
        case DS_OK:
            break;
        case DSERR_BADFORMAT:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Initialized DirectSound, but failed to set "
                    "primary buffer's format (DSERR_BADFORMAT)",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        case DSERR_INVALIDCALL:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Initialized DirectSound, but failed to set "
                    "primary buffer's format (DSERR_INVALIDCALL)",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        case DSERR_INVALIDPARAM:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Initialized DirectSound, but failed to set "
                    "primary buffer's format (DSERR_INVALIDPARAM)",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        case DSERR_PRIOLEVELNEEDED:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Initialized DirectSound, but failed to set "
                    "primary buffer's format (DSERR_PRIOLEVELNEEDED)",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Initialized DirectSound, but failed to set "
                    "primary buffer's format",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
    }
       
    // We can finally create a second buffer
    buffer_description.dwBufferBytes = audio_buffer_size;
    buffer_description.dwFlags = 0;
    buffer_description.lpwfxFormat = &wave_format;
    
    created_result = IDirectSound_CreateSoundBuffer(
            direct_sound_handle,
        /* LPCDSBUFFERDESC lpcDSBufferDesc: */
            &buffer_description,
        /* LPLPDIRECTSOUNDBUFFER lplpDirectSoundBuffer: */
            &secondary_buffer,
        /* IUnknown FAR* pUnkOuter: */
            NULL);
    
    switch (created_result) {
        case DS_OK:
            break;
        case DSERR_INVALIDPARAM:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Found DirectSound, but got DSERR_INVALIDPARAM when "
                    "trying to create secondary buffer",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Found DirectSound, but failed to create "
                    "secondary buffer",
                /* LPCSTR lpCaption: */
                    "Unhandled Error",
                /* UINT uType: */
                    MB_OK);
            *success = 0;
            return;
    }
    
    *success = 1;
}

void playsquarewave(void) {
    
    DWORD play_cursor = 0;
    DWORD write_cursor = 0;
    
    HRESULT got_cursors = secondary_buffer->lpVtbl->GetCurrentPosition(
            secondary_buffer,
        /* LPDWORD lpdwCurrentPlayCursor: */
            &play_cursor,
        /* LPDWORD lpdwCurrentWriteCursor: */
            &write_cursor);
    assert(got_cursors == DS_OK);
    
    uint32_t samples_per_second = 48000;
    uint32_t tone_hz = 256;
    uint32_t tone_volume = 1000;
    uint32_t wave_period = samples_per_second / tone_hz;
    uint32_t bytes_per_sample = sizeof(int16_t) * 2;
    uint32_t secondary_buffer_size = samples_per_second * bytes_per_sample;
    uint32_t latency_sample_count = samples_per_second / 2;
    
    DWORD byte_to_lock = 0;
    DWORD bytes_to_write = latency_sample_count * bytes_per_sample;
    
    VOID * region_1 = NULL;
    DWORD region_1_size = 0;
    VOID * region_2 = NULL;
    DWORD region_2_size = 0;
    
    HRESULT lock_result = secondary_buffer->lpVtbl->Lock(
            secondary_buffer,
        /* DWORD dwWriteCursor: */
            byte_to_lock,
        /* DWORD dwWriteBytes: */
            bytes_to_write,
        /* LPVOID lplpvAudioPtr1: */
            &region_1,
        /* LPDWORD lpdwAudioBytes1: */
            &region_1_size,
        /* LPVOID lplpvAudioPtr2: */
            &region_2,
        /* LPDWORD lpdwAudioBytes2: */
            &region_2_size,
        /* DWORD dwFlags: */
            0);
    
    switch (lock_result) {
        case DS_OK:
            break;
        case DSERR_BUFFERLOST:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to lock the secondary sound buffer "
                    "(DSERR_BUFFERLOST)",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            break;
        case DSERR_INVALIDCALL:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to lock the secondary sound buffer "
                    "(DSERR_INVALIDCALL)",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            break;
        case DSERR_INVALIDPARAM:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to lock the secondary sound buffer "
                    "(DSERR_INVALIDPARAM)",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            break;
        case DSERR_PRIOLEVELNEEDED:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to lock the secondary sound buffer "
                    "(DSERR_PRIOLEVELNEEDED)",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            break;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to lock the secondary sound buffer",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            return;
    }
    
    uint32_t hertz = 256;
    uint32_t square_wave_i = 0;
    uint32_t square_wave_len = 41000 / hertz;
    for (uint32_t i = 0; i < region_1_size; i++) {
        uint16_t sample = 0;
        if (square_wave_i % square_wave_len > (square_wave_len / 2)) {
            sample = 1000;
        }
        square_wave_i += 1;
        ((int16_t *)region_1)[i  ] = sample;
        ((int16_t *)region_1)[i+1] = sample;
    }
    for (uint32_t i = 0; i < region_2_size; i++) {
        uint16_t sample = 0;
        if (square_wave_i % square_wave_len > (square_wave_len / 2)) {
            sample = 1000;
        }
        square_wave_i += 1;
        ((int16_t *)region_2)[i  ] = sample;
        ((int16_t *)region_2)[i+1] = sample;
    }
    
    HRESULT unlock_result = secondary_buffer->lpVtbl->Unlock(
            secondary_buffer,
            region_1,
            region_1_size,
            region_2,
            region_2_size);
    switch (unlock_result) {
        case DS_OK:
            break;
        default:
            MessageBox(
                /* HWND hWnd: */
                    0,
                /* LPCSTR lpText: */
                    "Failed to unlock the secondary buffer",
                /* LPCSTR lpCaption: */
                    "DirectSound Error",
                /* UINT uType: */
                    MB_OK);
            return;
    }
    
    secondary_buffer->lpVtbl->Play(
        secondary_buffer,
        0,
        0,
        DSBPLAY_LOOPING);
}

