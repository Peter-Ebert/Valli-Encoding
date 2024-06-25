# FAQ

## Why isn't the output smaller than gzip/zstd/etc...?
Many commonly used compression tools have 2 stages of compression, first stage is usually an LZ77/78 ([Lempel-Ziv](https://en.wikipedia.org/wiki/LZ77_and_LZ78)) style and the second stage is an entropy encoder. This compressor is most like an adaptive [entropy encoder](https://en.wikipedia.org/wiki/Entropy_coding).   If you used the output of an LZ style compressor as an input to this encoder, then you would see similar sizes to other common tools.  The goal of this code is to illustrate what I believe is a new entropy coder, not a replacement for other common compression tools.

## Why is the frequency file (.freq) so large?
Mainly for transparency and simplicity, you can run xxd (or another parser) on the file and see it's only 7 bytes of count, 1 byte to indicate the symbol and then the next count with symbol and so on, including all 256 possible character/byte symbols (even zero counts) for a total fixed size of 2KiB (8\*256).  Other [example compressors](https://github.com/nayuki/Reference-arithmetic-coding/blob/fde1357935494f395b4d17ca7e9e897c226ad208/python/arithmetic-compress.py#L50) use the exact same method.  It would be straightforward to compress the frequency count file, but that's a solved problem and not the point of the proof of concept.  If this causes too much confusion I may compress and combine them to a single file.

## Why does the program exit on file sizes larger than 64KB?
It's just a precaution to prevent long runtimes.  First of all, this is just a proof of concept that isn't highly optimized and secondly the encoding math requires changing bits along the entire length of an arbitrarily large integer, once this size exceeds the CPU cache it can become quite slow.  If you want to experiment with bigger inputs simply comment out the line that does this check.

## Is there any input this implementation cannot compress?
The compression algorithm can work on any dataset, however there are 2 datasets this specific implementation will abort on: (1) If there is only 1 symbol in the input, e.g. 'aaaaa' there is nothing to compress, the frequency table is essentially RLE which is not new, so this is not implemented. (2) The input cannot contain all 256 byte values since this implementation picks one byte value to act as a 'null' value, code could be added to handle this case.



