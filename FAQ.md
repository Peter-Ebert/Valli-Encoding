# FAQ

## Why isn't the output smaller than gzip/zstd/etc...?
This compressor is solely an [entropy encoder](https://en.wikipedia.org/wiki/Entropy_coding) while many commonly used compression tools have 2 stages of compression.  The first stage is usually an LZ77/78 ([Lempel-Ziv](https://en.wikipedia.org/wiki/LZ77_and_LZ78)) type and the second stage is an entropy encoder.  If you used the output of an LZ style compressor as an input to this encoder, then you could see a smaller size than the aforementioned tools.  The goal of this code is to illustrate the new entropy coder, not a replacement for other common compression tools.

## Why is the frequency file (.freq) so large?
Mainly for transparency and simplicity, you can run xxd (or another parser) on the file and see it's only 7 bytes of count, 1 byte to indicate the symbol and then the next count with symbol and so on, including symbols with zero counts for a total fixed size of 2KiB (8\*256).  It would be easy to compress the frequency count file, but that's a solved problem and not the point of the proof of concept.  Since the frequency counts are not included when measuing the compressed size of other entropy encoders I didn't see a need to implement it.

## Why is the input file size limited to 64KB?
There's nothing preventing larger files but, first of all this is just a proof of concept that isn't highly optimized and secondly the encoding math touches every bit in the GMP large integer, once this exceeds the CPU cache it can become quite slow.  If you want to run on bigger inputs simply comment out the line that does this check.