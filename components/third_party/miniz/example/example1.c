/* This is example1 port from miniz example1 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "miniz.h"
#include "sdkconfig.h"

#define STACK_SIZE_OF_MINIZ_THREAD (8 * 1024)

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

void miniz_test_thread()
{
    int cmp_status;
    ulong src_len = (ulong)strlen(s_pStr);
    ulong cmp_len = compressBound(src_len);
    ulong uncomp_len = src_len;
    uint8_t *pCmp, *pUncomp;
    uint total_succeeded = 0;

    do {
        //Allocate buffers to hold compressed and uncompressed data.
        pCmp = (mz_uint8 *)malloc((size_t)cmp_len);
        pUncomp = (mz_uint8 *)malloc((size_t)src_len);

        if ((!pCmp) || (!pUncomp)) {
            printf("Out of memory!\n");
            assert(pCmp != NULL && pUncomp != NULL);
        }

        // Compress the string.
        cmp_status = compress(pCmp, &cmp_len, (const unsigned char *)s_pStr, src_len);

        if (cmp_status != Z_OK) {
            printf("compress() failed! return value is %d\n", cmp_status);
            free(pCmp);
            free(pUncomp);
            assert(false);
        }

        printf("Compressed from %u to %u bytes\n", (mz_uint32)src_len, (mz_uint32)cmp_len);

        // Decompress.
        cmp_status = uncompress(pUncomp, &uncomp_len, pCmp, cmp_len);
        total_succeeded += (cmp_status == Z_OK);

        if (cmp_status != Z_OK) {
            printf("uncompress failed!\n");
            free(pCmp);
            free(pUncomp);
            assert(false);
        }

        printf("Decompressed from %u to %u bytes\n", (mz_uint32)cmp_len, (mz_uint32)uncomp_len);

        // Ensure uncompress() returned the expected data.
        if ((uncomp_len != src_len) || (memcmp(pUncomp, s_pStr, (size_t)src_len))) {
            printf("Decompression failed!\n");
            free(pCmp);
            free(pUncomp);
            assert(false);
        }

        printf("memcmp return %d\n", memcmp((const char *)s_pStr, (const char *)pUncomp, uncomp_len));
        free(pCmp);
        free(pUncomp);
    } while (false);

    printf("Success.\n");
    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(miniz_test_thread, "miniz test thread",
                STACK_SIZE_OF_MINIZ_THREAD, NULL, 3, NULL);
}
