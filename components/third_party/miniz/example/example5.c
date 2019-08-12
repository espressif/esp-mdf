// example5.c - Demonstrates how to use miniz.c's low-level tdefl_compress() and tinfl_inflate() API's for simple string to string compression/decompression.
// The low-level API's are the fastest, make no use of dynamic memory allocation, and are the most flexible functions exposed by miniz.c.
// Public domain, April 11 2012, Rich Geldreich, richgel99@gmail.com. See "unlicense" statement at the end of tinfl.c.
// port to esp32 by Huifeng Zhang

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "assert.h"
#include "esp_log.h"
#include "miniz.h"

/* Ensure that miniz low stack consumption is also working when it is turned off.
 * Normally 4K is enough*/
#define STACK_SIZE_OF_EXAMPLE5 (8 * 1024)

// COMP_OUT_BUF_SIZE is the size of the output buffer used during compression.
// COMP_OUT_BUF_SIZE must be >= 1 and <= OUT_BUF_SIZE
#define COMP_OUT_BUF_SIZE (1024)

// OUT_BUF_SIZE is the size of the output buffer used during decompression.
// OUT_BUF_SIZE must be a power of 2 >= TINFL_LZ_DICT_SIZE (because the low-level decompressor not only writes, but reads from the output buffer as it decompresses)
#define OUT_BUF_SIZE (1024)

// The string to compress.
static const char *s_pStr = "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
                            "I can eat glass, it doesn't hurt me."
                            "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
                            "I can eat glass, it doesn't hurt me."
                            "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
                            "I can eat glass, it doesn't hurt me."
                            "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
                            "I can eat glass, it doesn't hurt me."
                            "Good morning Dr. Chandra. This is Hal. I am ready for my first lesson."
                            "I can eat glass, it doesn't hurt me.";

/* Table of compression level */
static const mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32, 16, 32, 128, 256, 512, 768, 1500 };

void example5(void *arg)
{
    int level = 9;
    size_t src_len = strlen(s_pStr);
    /* Allocate buffers to hold compressed and uncompressed data. */
    uint8_t *s_combuf = (uint8_t *)MZ_MALLOC(sizeof(uint8_t) * COMP_OUT_BUF_SIZE);
    uint8_t *s_outbuf = (uint8_t *)MZ_MALLOC(sizeof(uint8_t) * OUT_BUF_SIZE);

    if (s_combuf == NULL || s_outbuf == NULL) {
        printf("Out of memory!\n");
        assert(s_combuf != NULL && s_outbuf != NULL);
    }

    /* tdefl_compressor contains all the state needed
     * by the low-level compressor so it's a pretty big struct (~9k total). */
    tdefl_compressor *g_deflator = (tdefl_compressor *)MZ_MALLOC(sizeof(tdefl_compressor));

    if (g_deflator == NULL) {
        printf("malloc %zu bytes memory for g_deflator failed\n", sizeof(tdefl_compressor));
        assert(g_deflator != NULL);
    }

#ifdef CONFIG_MINIZ_SPLIT_TDEFL_COMPRESSOR
    /* allocate memory for member of g_deflator */
    g_deflator->m_dict = (mz_uint8 *)MZ_MALLOC(
                             sizeof(mz_uint8) * (TDEFL_LZ_DICT_SIZE + TDEFL_MAX_MATCH_LEN - 1));
    assert(g_deflator != NULL);

    for (uint8_t i = 0; i < TDEFL_MAX_HUFF_TABLES; i++) {
        g_deflator->m_huff_count[i] = (mz_uint16 *)MZ_MALLOC(sizeof(mz_uint16) * TDEFL_MAX_HUFF_SYMBOLS);
        assert(g_deflator->m_huff_count[i] != NULL);
        g_deflator->m_huff_codes[i] = (mz_uint16 *)MZ_MALLOC(sizeof(mz_uint16) * TDEFL_MAX_HUFF_SYMBOLS);
        assert(g_deflator->m_huff_codes[i] != NULL);
        g_deflator->m_huff_code_sizes[i] = (mz_uint8 *)MZ_MALLOC(sizeof(mz_uint8) * TDEFL_MAX_HUFF_SYMBOLS);
        assert(g_deflator->m_huff_code_sizes[i] != NULL);
    }

    g_deflator->m_lz_code_buf = (mz_uint8 *)MZ_MALLOC(sizeof(mz_uint8) * TDEFL_LZ_CODE_BUF_SIZE);
    assert(g_deflator->m_lz_code_buf != NULL);
    g_deflator->m_next = (mz_uint16 *)MZ_MALLOC(sizeof(mz_uint16) * TDEFL_LZ_DICT_SIZE);
    assert(g_deflator->m_next != NULL);
    g_deflator->m_hash = (mz_uint16 *)MZ_MALLOC(sizeof(mz_uint16) * TDEFL_LZ_HASH_SIZE);
    assert(g_deflator->m_hash != NULL);
    g_deflator->m_output_buf = (mz_uint8 *)MZ_MALLOC(sizeof(mz_uint8) * TDEFL_OUT_BUF_SIZE);
    assert(g_deflator->m_output_buf != NULL);
#endif
    assert(COMP_OUT_BUF_SIZE <= OUT_BUF_SIZE);
    printf("Input string size: %zu\n", src_len);

    // Compression.
    do {
        size_t in_bytes, out_bytes;
        tdefl_status status;

        /* flags for tdefl_compress(...) */
        mz_uint comp_flags = TDEFL_WRITE_ZLIB_HEADER | s_tdefl_num_probes[MZ_MIN(10, level)]
                             | ((level <= 3) ? TDEFL_GREEDY_PARSING_FLAG : 0);

        if (!level) {
            comp_flags |= TDEFL_FORCE_ALL_RAW_BLOCKS;
        }

        /* Initialize the low-level compressor. */
        status = tdefl_init(g_deflator, NULL, NULL, comp_flags);

        if (status != TDEFL_STATUS_OKAY) {
            printf("tdefl_init() failed!\n");
            assert(status != TDEFL_STATUS_OKAY);
        }

        in_bytes = src_len; // size of compressed data
        out_bytes = COMP_OUT_BUF_SIZE;  //size of out buf
        status = tdefl_compress(g_deflator, s_pStr, &in_bytes, s_combuf, &out_bytes, TDEFL_FINISH);

        if (status == TDEFL_STATUS_DONE) {
            /* Compression completed successfully. */
            printf("Compress from %zu to %zu bytes\n", in_bytes, out_bytes);
            break;
        } else {
            /* Compression somehow failed. */
            MZ_FREE(s_combuf);
            MZ_FREE(s_outbuf);
            printf("tdefl_compress() failed with status %i!\n", status);
#ifdef CONFIG_MINIZ_SPLIT_TDEFL_COMPRESSOR
            MZ_FREE(g_deflator->m_dict);

            for (uint8_t i = 0; i < TDEFL_MAX_HUFF_TABLES; i++) {
                MZ_FREE(g_deflator->m_huff_count[i]);
                MZ_FREE(g_deflator->m_huff_codes[i]);
                MZ_FREE(g_deflator->m_huff_code_sizes[i]);
            }

            MZ_FREE(g_deflator->m_lz_code_buf);
            MZ_FREE(g_deflator->m_next);
            MZ_FREE(g_deflator->m_hash);
            MZ_FREE(g_deflator->m_output_buf);
#endif
            MZ_FREE(g_deflator);
            assert(status != TDEFL_STATUS_DONE);
        }
    } while (0);

#ifdef CONFIG_MINIZ_SPLIT_TDEFL_COMPRESSOR
    MZ_FREE(g_deflator->m_dict);

    for (uint8_t i = 0; i < TDEFL_MAX_HUFF_TABLES; i++) {
        MZ_FREE(g_deflator->m_huff_count[i]);
        MZ_FREE(g_deflator->m_huff_codes[i]);
        MZ_FREE(g_deflator->m_huff_code_sizes[i]);
    }

    MZ_FREE(g_deflator->m_lz_code_buf);
    MZ_FREE(g_deflator->m_next);
    MZ_FREE(g_deflator->m_hash);
    MZ_FREE(g_deflator->m_output_buf);
#endif
    MZ_FREE(g_deflator);

    // Decompression.
    /* tdefl_compressor contains all the state needed
     * by the low-level decompressor so it's a pretty big struct (~11k total). */
    tinfl_decompressor *inflator = (tinfl_decompressor *)MZ_MALLOC(sizeof(tinfl_decompressor));

    if (inflator == NULL) {
        printf("Malloc %zu bytes memory for inflator failed\n", sizeof(tinfl_decompressor));
        assert(inflator != NULL);
    }

#ifdef CONFIG_MINIZ_SPLIT_TINFL_DECOMPRESSOR_TAG

    /* allocate memory for member of inflator */
    for (uint32_t i = 0; i < TINFL_MAX_HUFF_TABLES; i++) {
        inflator->m_tables[i] = (tinfl_huff_table *)MZ_MALLOC(sizeof(tinfl_huff_table));
        assert(inflator->m_tables[i] != NULL);
        inflator->m_tables[i]->m_look_up = (mz_int16 *)MZ_MALLOC(sizeof(mz_int16) * TINFL_FAST_LOOKUP_SIZE);
        assert(inflator->m_tables[i]->m_look_up != NULL);
        inflator->m_tables[i]->m_tree = (mz_int16 *)MZ_MALLOC(sizeof(mz_int16) * TINFL_MAX_HUFF_SYMBOLS_0 * 2);
        assert(inflator->m_tables[i]->m_tree != NULL);
    }

#endif

    do {
        size_t in_bytes, out_bytes;
        tinfl_status status;

        tinfl_init(inflator);
        in_bytes = COMP_OUT_BUF_SIZE;
        out_bytes = OUT_BUF_SIZE;
        /* TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF flag must be give if you want low heap usage,
         * more detial can be find in miniz_tinfl.h: 12 */
        status = tinfl_decompress(inflator, (const mz_uint8 *)s_combuf, &in_bytes, s_outbuf, s_outbuf, &out_bytes,
                                  TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

        if (status == TINFL_STATUS_DONE) {
            /* Decompression completed successfully. */
            printf("Decompress from %zu to %zu bytes\n", in_bytes, out_bytes);
            break;
        } else {
            /* Compression somehow failed. */
            printf("tinfl_decompress() failed with status %i!\n", status);
            MZ_FREE(s_combuf);
            MZ_FREE(s_outbuf);
#ifdef CONFIG_MINIZ_SPLIT_TINFL_DECOMPRESSOR_TAG

            for (int i = 0; i < TINFL_MAX_HUFF_TABLES; i++) {
                MZ_FREE(inflator->m_tables[i]->m_look_up);
                MZ_FREE(inflator->m_tables[i]->m_tree);
                MZ_FREE(inflator->m_tables[i]);
            }

#endif
            free(inflator);
            assert(status == TINFL_STATUS_DONE);
        }
    } while (0);

    printf("Finish!!!\n");
    printf("Result of memcmp(s_pStr, s_outbuf) is %d\n", memcmp(s_pStr, s_outbuf, src_len));
    MZ_FREE(s_combuf);
    MZ_FREE(s_outbuf);

#ifdef CONFIG_MINIZ_SPLIT_TINFL_DECOMPRESSOR_TAG

    for (int i = 0; i < TINFL_MAX_HUFF_TABLES; i++) {
        MZ_FREE(inflator->m_tables[i]->m_look_up);
        MZ_FREE(inflator->m_tables[i]->m_tree);
        MZ_FREE(inflator->m_tables[i]);
    }

#endif
    MZ_FREE(inflator);

    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(example5, "example5",
                STACK_SIZE_OF_EXAMPLE5, NULL, 3, NULL);
}
