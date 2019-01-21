Third Party API
===============

The third-party items:

- `Driver <https://github.com/espressif/esp-iot-solution>`_: drivers for different devices, such as frequently used buttons and LEDs
- `Miniz <https://github.com/richgel999/miniz>`_: lossless, high performance data compression library

Driver
-------
We can regard IoT solution project as a platform that contains different device drivers and features.

Miniz
------

Miniz is a lossless, high performance data compression library in a single source file that implements the zlib (RFC 1950) and Deflate (RFC 1951) compressed data format specification standards. It supports the most commonly used functions exported by the zlib library, but is a completely independent implementation so zlib's licensing requirements do not apply. Miniz also contains simple to use functions for writing .PNG format image files and reading/writing/appending .ZIP format archives. Miniz's compression speed has been tuned to be comparable to zlib's, and it also has a specialized real-time compressor function designed to compare well against fastlz/minilzo.