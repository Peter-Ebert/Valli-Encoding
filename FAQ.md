# FAQ

## Why isn't the output smaller than gzip/zstd/etc...?
This compressor is solely an [entropy encoder](https://en.wikipedia.org/wiki/Entropy_coding) while many commonly used compression tools have 2 stages of compression.  The first stage is usually an LZ77/78 ([Lempel-Ziv](https://en.wikipedia.org/wiki/LZ77_and_LZ78)) type and the second stage is an entropy encoder.  If you used the output of an LZ style compressor as an input to this encoder, then you could see a smaller size than the aforementioned tools.  The goal of this code is to illustrate the new entropy coder, not a replacement for other common compression tools.

## Why is the frequency file (.freq) so large?
Mainly for transparency and simplicity, you can run xxd (or another parser) on the file and see it's only 7 bytes of count, 1 byte to indicate the symbol and then the next count with symbol and so on, including symbols with zero counts for a total fixed size of 2KiB (8\*256).  It would be easy to compress the frequency count file, but that's a solved problem and not the point of the proof of concept.  Since the frequency counts are not included when measuring the compressed size of other entropy encoders I didn't see a need to implement it.

## Why does the program exit on file sizes larger than 64KB?
It's just a precaution to prevent long runtimes.  First of all this is just a proof of concept that isn't highly optimized and secondly the encoding math requires changing bits along the entire length of an arbitrarily large integer, once this size exceeds the CPU cache it can become quite slow.  If you want to run on bigger inputs simply comment out the line that does this check.

## Is there any input this implementation cannot compress?
The compression algorithm can work on any dataset, however there are 2 datasets this specific implementation will abort on: (1) If there is only 1 symbol in the input, e.g. 'aaaaa' there is nothing to compress, the frequency table is essentially RLE which is not new, so this is not implemented. (2) The input cannot contain all 256 byte values since this implementation picks one byte value to act as a 'null' vale, code could be added to handle this case.



