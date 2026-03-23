# Valli-Encoding
A compression algorithm that uses combinatorics (binomials/multinomials).

**Table of Contents**  
* [Introduction](#introduction)  
* [Comparison to Others](#comparison-to-other-entropy-encoder-implementations)
* [The Algorithm](#the-algorithm)  
* [Running the Code](#running-the-code)  
* [Support](#support)  
* [Final Thoughts](#final-thoughts)  

## Introduction
This repository contains a basic *proof of concept* implementation of what I'd like to call Valli encoding.  It uses the exact count of the symbol frequencies to compress the input with combinatorics.  The output will be a number between 0 (inclusive) and the number of permutations of symbols in the frequency table (exclusive).  The input is processed one unique symbol at a time using the number of combinations (binomials) that could occur before placing each symbol, the sum of which generates a single symbol's encoding.  These symbol encodings can then be combined together based on the number of permutations of each preceding symbol.  For a step-by-step walkthrough of the math see [how the code works](the-algorithm.md).

I'd be happy to be politely corrected if this already exists, as far as I can tell this is a novel approach.  Feel free to raise an issue if you can find prior work or other feedback.

The size of the output will always be the size of the total number of permutations of the symbols given the symbol frequencies table, see [multinomials](https://en.wikipedia.org/wiki/Multinomial_theorem#Number_of_unique_permutations_of_words) for more info:  
Let A,B,C,... = symbol counts  
T = total count of symbols = A + B + C + ...  
Bit size = log2( T! / (A! * B! * C! * ...) )  

## Comparison to Other Entropy Encoder Implementations
This algorithm's output is smaller than other entropy encoder implementations I can find online, within 1 bit of entropy not counting the frequency table, however the computations required are substantial and the gains are small, so it's not a very practical algorithm in it's current form. Note this algorithm is just an entropy encoder, I'm aware of other techniques like LZ/LZW, PPM, Neural Nets, etc. that would produce smaller output by leveraging information about the relationship between symbols.  In theory arithmetic should be able to match the performance but I've not seen an implementation that can do so Please feel free to share a link if you have an pure entropy encoder that produces smaller output through the "[issues](https://github.com/Peter-Ebert/Valli-Encoding/issues)" option at the top of the page.  

#### Example 1: wizard_of_oz (chapter 5) - 9905 bytes
A moderately sized input that uses many different letters and symbols.

| Algorithm                                                                                                               | Freq Table | Encoding | Total Size (bytes) |
| ----------------------------------------------------------------------------------------------------------------------- | ---------- | -------- | ------------------ |
| [Static AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/arithmetic-compress.py)            | 101**      | 5423     | 5524               |
| [Adaptive AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/adaptive-arithmetic-compress.py) | N/A        | 5611     | 5611               |
| [rANS](https://github.com/rygorous/ryg_rans)                                                                            | 101**      | 5428     | 5529               |
| Valli                                                                                                                   | 101        | 5395     | **5496**           |


\*\*Their code does not compress the frequency table, so I've used my implementation's smaller bit packed table size instead.

#### Example 2: pangram - 43 bytes
Worst case scenario for frequency table size vs message size.

| Algorithm                                                                                                               | Freq Table | Encoding | Total Size (bytes) |
| ----------------------------------------------------------------------------------------------------------------------- | ---------- | -------- | ------------------ |
| [Static AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/arithmetic-compress.py)            | 34**       | 25       | 59                 |
| [Adaptive AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/adaptive-arithmetic-compress.py) | N/A        | 42       | **42**             |
| Valli                                                                                                                   | 34         | **19**   | 53                 |

#### Example 3: tongue_twister - 35 bytes
Many repeated syllables leads to a smaller frequency table.

| Algorithm                                                                                                               | Freq Table | Encoding | Total Size (bytes) |
| ----------------------------------------------------------------------------------------------------------------------- | ---------- | -------- | ------------------ |
| [Static AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/arithmetic-compress.py)            | **16       | 15       | 48                 |
| [Adaptive AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/adaptive-arithmetic-compress.py) | N/A        | 32       | 32                 |
| Valli                                                                                                                   | 16         | 12       | **28**             |

#### Example 4: sparse - 110 bytes
Data is mostly a single symbol with a few others.

| Algorithm                                                                                                               | Freq Table | Encoding | Total Size (bytes) |
| ----------------------------------------------------------------------------------------------------------------------- | ---------- | -------- | ------------------ |
| [Static AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/arithmetic-compress.py)            | 6**        | 4        | 10                 |
| [Adaptive AC](https://github.com/nayuki/Reference-arithmetic-coding/blob/master/python/adaptive-arithmetic-compress.py) | N/A        | 44       | 44                 |
| Valli                                                                                                                   | 6          | 3        | **9**              |

## The Algorithm
Follow this link to see [how the code works](the-algorithm.md) in a step by step example walkthrough.

If you have more questions, check the [FAQ](FAQ.md).

## The Frequency Table
I've combined the frequency table and encoding together into a single file, you will see the size of each at the end of the compressor's console output.  The frequency table uses only very basic bit packing for the counts, the characters are stored as raw bytes.  Some additional compression techniques could make it even smaller, but again this is just a proof of concept.

## Running the Code
#### Required Dependencies:
* [GMP library](https://gmplib.org/) - Used for large integer math. Unfortunately there isn't one simple command for this, use your favorite search engine or LLM with your OS version specified.
* A 64 bit CPU that supports LZCNT, most modern Intel/AMD 64 bit CPUs will, except it seems Apple's M series.  This is for fast bit packing of the frequency table.  If anyone wants to contribute a LZCNT equivlaent for ARM please do.
#### Recommended (optional):
* Clang - GCC should work too just haven't tested.
* C++17 - known to be working, other C++ standards should should work but are not tested.  Some shortcuts like "auto" are used which require at least C++11 but could be rewritten.

#### Linux Commands
```
git clone git@github.com:Peter-Ebert/Valli-Encoding.git
clang++ -std=c++17 -O2 poc-compress.cpp -lgmp -o poc-compress
clang++ -std=c++17 -O2 poc-decompress.cpp -lgmp -o poc-decompress
```

To compress the test data:
```
./poc-compress testfiles/input1
```

The output to console is intentionally verbose to help with understanding the calculations performed for each symbol, it slows the output some and can be commented out if desired.

Two files are created in the same directory as the compressed file:
* input1.vli - the compressed output
* input1.freq - contains frequency counts for each symbol/character, it is sorted ascending and uncompressed.  The first 7 bytes are the count and the next 1 byte indicates the symbol, repeating.  File size is the same for any input (64bits * 256=2048 bytes).  The data could be compressed easily (even zero counts are included), but this simple format shows that nothing is being hidden inside.

To decompress:
```
./poc-decompress testfiles/input1.vli
```

This will output "\[filename\].decom", so that the input and output can be compared. The decompressor will assume the associated frequency file is in the same folder with the same name but replaces ".vli" with ".freq" (created previously by the compressor).  As with the compressor, the console output will show much of the math involved to decode the compressed file.

To verify the input matches the output:
```
diff -s testfiles/input1 testfiles/input1.decom
```
Expected output:
```
Files testfiles/input1 and testfiles/input1.decom are identical
```

