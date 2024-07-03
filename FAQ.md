# FAQ

## Why isn't the output smaller than gzip/zstd/etc...?
Most commonly used compression tools have 2 types of compression (there are more of course).  This compressor proof of concept only implements one that is most like an [entropy encoder](https://en.wikipedia.org/wiki/Entropy_coding) which focuses on the frequency of characters, so its performace will be close to those.  The other type is [dictionary/substitution coders](https://en.wikipedia.org/wiki/Dictionary_coder) which focuses on the structure/relationship between symbols, like [LZ77/78](https://en.wikipedia.org/wiki/LZ77_and_LZ78).  If you used the output of an LZ style compressor as an input to this encoder, then you would see similar sizes to other common tools.  The goal of this code is to provide a proof of concept illustration, not a replacement for other common compression tools, as the code is not highly optimized.

## Is there any input this implementation cannot compress?
The compression algorithm can work on any dataset, however there are 2 datasets this specific implementation will abort on: (1) If there is only 1 or 0 symbols in the input, e.g. 'aaaaa' or '' there is nothing to compress, the frequency table is essentially RLE which is not new, so this is not implemented. (2) The input cannot contain all 256 byte values since this implementation picks one byte value to act as a 'null' value.  The code could be altered to handle both cases.

## Why does the program exit on file sizes larger than 64KB?
It's just a precaution to prevent long runtimes.  First of all, this is just a proof of concept that isn't highly optimized and secondly the encoding math requires changing bits along the entire length of an arbitrarily large integer, once this size exceeds the CPU cache it can become quite slow.  If you want to experiment with bigger inputs simply comment out the line that does this check.



