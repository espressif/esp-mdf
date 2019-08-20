# Miniz

Miniz is a lossless, high performance data compression library in a single source file that implements the zlib (RFC 1950) and Deflate (RFC 1951) compressed data format specification standards. It supports the most commonly used functions exported by the zlib library, but is a completely independent implementation so zlib's licensing requirements do not apply. Miniz also contains simple to use functions for writing .PNG format image files and reading/writing/appending .ZIP format archives. Miniz's compression speed has been tuned to be comparable to zlib's, and it also has a specialized real-time compressor function designed to compare well against fastlz/minilzo.

## Modifications

Because the original miniz consumes too much memory and stack space, it is not suitable for esp32, so I made the following modifications. You can find original miniz here [richgel999/miniz](https://github.com/richgel999/miniz).

* large arrays in tdefl_start_dynamic_block(...), tdefl_radix_sort_syms(...) and tdefl_optimize_huffman_table(...) are replaced with pointers and applied when needed to reduce stack consumption. This function can be turned on or off in `component config > MINIZ > Low stack usage`, which is enabled by default.
* The array in tdefl_compressor can be replaced with a pointer. It is used separately for memory at initialization time. It is released after compression and reduces the memory size of a single application. This function can be turned on or off in `component > MINIZ > Allocate memory multiple times in compress`, which is enabled by default.
* Large arrays in struct tinfl_decompressor_tag and tinfl_huff_table can be replaced with pointers and applied separately at initialization time for the same reason. This function can be turned on or off in `component > MINIZ > Allocate memory multiple times in decompress`, which is enabled by default.
* TDEFL_LZ_DICT_SIZE is reduced from 32768 to 512
* TDEFL_LZ_CODE_BUF_SIZE is reduced from 24*1024 to 512
* TDEFL_LZ_DICT_SIZE is reduced from 32768 to 512
* MZ_MALLOC(x) can print message when it failed
* MZ_MALLOC_RETRY(x) can retry if malloc(x) return 0, it also can print message when it failed. Called in tdefl_start_dynamic_block(), tdefl_radix_sort_syms() and tdefl_optimize_huffman_table() 
* MZ_FREE(x) can determine if x is NULL, if not set x to NULL after free()
* Add Kconfig support

## Usage

You can read example1.c and example2.c in miniz/example/

1. example1.c shows how to use compress() and uncompress(), both of them are APIs that encapsulate the low layer API.
1. example5.c shows how to use tdefl_compress() and tinfl_decompress() , both of them are the lowest layer compression decompression API.

## Performance

### Stack usage

1. Stack usage of tdefl_compress() is 2936 bytes if`Low stack usage`is enable
1. Stack usage of tinfl_decompress() is 1556 bytes
1. Stack usage of High layer API of compress and decompress is little more than low layer API

### Speed

1. tdefl_compress() consumes about 2095 us
1. tinfl_decompress() consumes about 1238 us

> Compressed objects can be seen in example5.c

### Heap usage

1. example1.c shows how to use compress() and uncompress(), both of them are APIs that encapsulate the underlying layer.
1. example5.c shows how to use tdefl_compress() and tinfl_decompress() , both of them are the lowest level compression decompression API.

## Features

* MIT licensed
* A portable, single source and header file library written in plain C. Tested with GCC, clang and Visual Studio.
* Easily tuned and trimmed down by defines
* A drop-in replacement for zlib's most used API's (tested in several open source projects that use zlib, such as libpng and libzip).
* Fills a single threaded performance vs. compression ratio gap between several popular real-time compressors and zlib. For example, at level 1, miniz.c compresses around 5-9% better than minilzo, but is approx. 35% slower. At levels 2-9, miniz.c is designed to compare favorably against zlib's ratio and speed. See the miniz performance comparison page for example timings.
* Not a block based compressor: miniz.c fully supports stream based processing using a coroutine-style implementation. The zlib-style API functions can be called a single byte at a time if that's all you've got.
* Easy to use. The low-level compressor (tdefl) and decompressor (tinfl) have simple state structs which can be saved/restored as needed with simple memcpy's. The low-level codec API's don't use the heap in any way.
* Entire inflater (including optional zlib header parsing and Adler-32 checking) is implemented in a single function as a coroutine, which is separately available in a small (~550 line) source file: miniz_tinfl.c
* A fairly complete (but totally optional) set of .ZIP archive manipulation and extraction API's. The archive functionality is intended to solve common problems encountered in embedded, mobile, or game development situations. (The archive API's are purposely just powerful enough to write an entire archiver given a bit of additional higher-level logic.)

## Known Problems

* No support for encrypted archives. Not sure how useful this stuff is in practice.
* Minimal documentation. The assumption is that the user is already familiar with the basic zlib API. I need to write an API wiki - for now I've tried to place key comments before each enum/API, and I've included 6 examples that demonstrate how to use the module's major features.

## Special Thanks

Thanks to Alex Evans for the PNG writer function. Also, thanks to Paul Holden and Thorsten Scheuermann for feedback and testing, Matt Pritchard for all his encouragement, and Sean Barrett's various public domain libraries for inspiration (and encouraging me to write miniz.c in C, which was much more enjoyable and less painful than I thought it would be considering I've been programming in C++ for so long).

Thanks to Bruce Dawson for reporting a problem with the level_and_flags archive API parameter (which is fixed in v1.12) and general feedback, and Janez Zemva for indirectly encouraging me into writing more examples.

## Patents

I was recently asked if miniz avoids patent issues. miniz purposely uses the same core algorithms as the ones used by zlib. The compressor uses vanilla hash chaining as described [here](http://www.gzip.org/zlib/rfc-deflate.html#algorithm). Also see the [gzip FAQ](http://www.gzip.org/#faq11). In my opinion, if miniz falls prey to a patent attack then zlib/gzip are likely to be at serious risk too.

[![Build Status](https://travis-ci.org/uroni/miniz.svg?branch=master)](https://travis-ci.org/uroni/miniz)
