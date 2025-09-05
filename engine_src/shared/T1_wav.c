#include "T1_wav.h"

typedef struct FileHeader {
    char     riff[4];
    uint32_t file_size; // described sometimes as "integer", assuming uint
    char     wave[4];
} FileHeader;

/*
RIFF files consist entirely of "chunks". The overall format is identical to IFF, except for the endianness as previously stated, and the different meaning of the chunk names.

All chunks have the following format:

4 bytes: an ASCII identifier for this chunk (examples are "fmt " and "data"; note the space in "fmt ").
4 bytes: an unsigned, little-endian 32-bit integer with the length of this chunk (except this field itself and the chunk identifier).
variable-sized field: the chunk data itself, of the size given in the previous field.
a pad byte, if the chunk's length is not even
*/
typedef struct WavChunkHeader {
    char ascii_id[4];
    uint32_t data_size;
} WavChunkHeader;

typedef struct FormatChunkBody {
    uint16_t type; // Type of format (1 is PCM) - 2 byte integer
    uint16_t channels; // Number of Channels - 2 byte integer
    uint32_t sample_rate; // Sample Rate - u32. 44100 (CD), 48000 (DAT).
    uint32_t idontgetit; // (Sample Rate * BitsPerSample * Channels) / 8
    uint16_t must_be_four; // don't get this one either
    uint16_t bits_per_sample;
} FormatChunkBody;

#define consume_struct(from_ptr, StructName) *(StructName *)from_ptr; from_ptr += sizeof(StructName);

static uint32_t strings_are_equal(
    const char * string_1,
    const char * string_2)
{
    uint32_t i = 0;
    while (string_2[i] != '\0')
    {
        if (string_1[i] != string_2[i]) {
            return 0;
        }
        i++;
    }
    
    return string_2[i] == '\0';
}

static void check_strings_equal(
    char * actual,
    char * expected_nullterm,
    char * field_description,
    uint32_t * good)
{
    uint32_t at_i = 0;
    while (expected_nullterm[at_i] != '\0') {
        if (actual[at_i] != expected_nullterm[at_i]) {
            *good = 0;
        }
        at_i += 1;
    }
    
    if (!*good) {
        #ifndef WAV_SILENCE
        if (at_i > 50) {
            at_i = 50;
        }
        
        char actual_nullterm[50];
        for (uint32_t i = 0; i < at_i; i++) {
            actual_nullterm[i] = actual[i];
        }
        
        printf(
            "Mismatch at [%s]: expected %s, got %s\n",
            field_description,
            expected_nullterm,
            actual_nullterm);
        #endif
    }
}

void wav_samples_to_wav(
    unsigned char * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    int16_t * samples,
    const uint32_t samples_size)
{
    (void)recipient_cap; // TODO: check cap
    
    FileHeader riff_header;
    riff_header.riff[0] = 'R';
    riff_header.riff[1] = 'I';
    riff_header.riff[2] = 'F';
    riff_header.riff[3] = 'F';
    riff_header.file_size = (samples_size * sizeof(int16_t)) + 36;
    riff_header.wave[0] = 'W';
    riff_header.wave[1] = 'A';
    riff_header.wave[2] = 'V';
    riff_header.wave[3] = 'E';
    
    assert(sizeof(FileHeader) % 2 == 0); // no padding needed
    
    T1_std_memcpy(recipient, &riff_header, sizeof(FileHeader));
    recipient += sizeof(FileHeader);
    
    WavChunkHeader format_header;
    format_header.ascii_id[0] = 'f';
    format_header.ascii_id[1] = 'm';
    format_header.ascii_id[2] = 't';
    format_header.ascii_id[3] = ' ';
    format_header.data_size = sizeof(FormatChunkBody);
    assert(format_header.data_size == 16);
    
    T1_std_memcpy(recipient, &format_header, sizeof(WavChunkHeader));
    recipient += sizeof(WavChunkHeader);
    
    FormatChunkBody format_body;
    format_body.type = 1;
    format_body.channels = 2;
    format_body.sample_rate = 44100;
    format_body.idontgetit = 176400;
    format_body.must_be_four = 4;
    format_body.bits_per_sample = 16;
    assert(sizeof(FormatChunkBody) % 2 == 0); // no padding needed
    
    T1_std_memcpy(recipient, &format_body, sizeof(FormatChunkBody));
    recipient += sizeof(FormatChunkBody);
    
    WavChunkHeader data_header;
    data_header.ascii_id[0] = 'd';
    data_header.ascii_id[1] = 'a';
    data_header.ascii_id[2] = 't';
    data_header.ascii_id[3] = 'a';
    data_header.data_size = samples_size * sizeof(int16_t);
    assert(sizeof(WavChunkHeader) % 2 == 0); // no padding needed
    
    T1_std_memcpy(recipient, &data_header, sizeof(WavChunkHeader));
    recipient += sizeof(WavChunkHeader);
    
    if (samples_size % 2 == 1) {
        // add padding byte
        *recipient_size += 1;
        recipient[0] = 0;
        recipient += 1;
    }
    
    T1_std_memcpy(recipient, samples, samples_size * sizeof(int16_t));
    
    recipient += sizeof(int16_t) * samples_size;
    recipient[0] = '\0';
    
    #ifndef NDEBUG
    uint32_t before_sample_data_size_bytes =
        sizeof(WavChunkHeader) +
        sizeof(WavChunkHeader) +
        sizeof(FileHeader) +
        sizeof(FormatChunkBody);
    #endif
    assert(before_sample_data_size_bytes == 44);
    
    *recipient_size += (samples_size * sizeof(int16_t)) + 44;
}

void wav_parse(
    int16_t * recipient,
    uint32_t * recipient_size,
    const uint32_t recipient_cap,
    unsigned char * raw_file,
    const uint32_t data_size,
    uint32_t * good)
{
    *good = 1;
    
    unsigned char * raw_file_at = raw_file;
    FileHeader file_header = consume_struct(raw_file_at, FileHeader);
    
    check_strings_equal(
        /* char * actual: */
            file_header.riff,
        /* char * expected_nullterm: */
            "RIFF",
        /* char * field_description: */
            "file header",
        /* uint32_t * good: */
            good);
    if (!*good) { return; }
    
    if (file_header.file_size + 8 != data_size) {
        #ifndef WAV_SILENCE
        printf(
            ".wav header claims filesize %u+8 bytes, got %u byte datastream\n",
            file_header.file_size,
            data_size);
        #endif
        *good = 0;
        return;
    }
    
    check_strings_equal(
        /* char * actual: */
            file_header.wave,
        /* char * expected_nullterm: */
            "WAVE",
        /* char * field_description: */
            "file header",
        /* uint32_t * good: */
            good);
    if (!*good) { return; }
    
    FormatChunkBody format_data = {0};
    
    while (
        *good &&
        file_header.file_size > (((ptrdiff_t)raw_file_at - (ptrdiff_t)raw_file) +
            (ptrdiff_t)(sizeof(WavChunkHeader) + 2)))
    {
        // assert((ptrdiff_t)(void *)raw_file_at % 32 == 0);
        WavChunkHeader chunk_header =
            consume_struct(raw_file_at, WavChunkHeader);
        
        if (
            strings_are_equal(
                chunk_header.ascii_id,
                "JUNK"))
        {
            /*
            Structure of the 'JUNK' chunk
            To align RIFF chunks to certain boundaries (i.e. 2048bytes for CD-ROMs)
            the RIFF specification includes a JUNK chunk. Its contents are to be
            skipped when reading. When writing RIFFs, JUNK chunks should not have
            odd number as Size.
            
            Name	Size	Description
            ID	4 byte	four ASCII character identifier 'JUNK'
            Size	4 byte	size of Data
            Data	Size bytes	nothing
            unused	1 byte	present if Size is odd
            */
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "bext"))
        {
            /*
            Broadcast Wave adds a required "Broadcast Audio Extension" (bext)
            chunk to hold the minimum information considered necessary for
            broadcast applications. Additional metadata chunks have also been
            developed;
            */
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "LGWV"))
        {
            // logic wave software, ignore
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "ResU"))
        {
            // undocumented chunk, ignore
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "ID3 ") ||
            strings_are_equal(
                chunk_header.ascii_id,
                "id3 "))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "SMED"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "iXML"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "LIST"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "_PMX"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "FLLR"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "smpl"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "splp"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "sprg"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "spcl"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "spcc"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "srtn"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "spca"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "acid"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "inst"))
        {
            raw_file_at += chunk_header.data_size;
            if (chunk_header.data_size % 2 == 1) {
                raw_file_at += 1;
            }
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "fmt"))
        {
            format_data = *(FormatChunkBody *)raw_file_at;
            raw_file_at += chunk_header.data_size;
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.type == 1); // supporting only PCM for now
            #endif
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.channels == 2); // supporting only stereo for now
            #endif
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.sample_rate == 44100); // supporting only 44.1khertz
            #endif
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.bits_per_sample == 16); // supporting only 24bit
            #endif
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.must_be_four == 4);
            #endif
            
            #ifndef WAV_IGNORE_ASSERTS
            assert(format_data.idontgetit == 176400);
            #endif
        } else if (
            strings_are_equal(
                chunk_header.ascii_id,
                "data"))
        {
            if (chunk_header.data_size > (recipient_cap * 2)) {
                #ifndef WAV_SILENCE
                printf(
                    "Recipient size of %u can't contain %u bytes of sound "
                    "data\n",
                    recipient_cap,
                    chunk_header.data_size);
                #endif
                *good = 0;
                return;
            }
            
            if (chunk_header.data_size > file_header.file_size) {
                #ifndef WAV_SILENCE
                printf(
                    "Chunk size %u larger than file size %u?\n",
                    chunk_header.data_size,
                    file_header.file_size);
                #endif
                *good = 0;
                return;
            }
            
            if (
                format_data.bits_per_sample == 24)
            {
                // TODO: conver to 16-bit samples
                assert(0);
            } else if (
                format_data.bits_per_sample == 16)
            {
                T1_std_memcpy(recipient, raw_file_at, chunk_header.data_size);
                *recipient_size = chunk_header.data_size / 2;
                raw_file_at += chunk_header.data_size;
            }
        } else {
            *good = 0;
            #ifndef WAV_SILENCE
            printf(
                "Unrecognized chunk type: %s\n",
                chunk_header.ascii_id);
            #endif
            return;
        }
    }
    
    *good = 1;
    return;
}

