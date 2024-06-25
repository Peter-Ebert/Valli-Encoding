# Valli-Encoding
A compression algorithm that uses combinatorics (binomials).

**Table of Contents**  
* [Introduction](#introduction)  
* [The Algorithm](#the-algorithm)  
* [Running the Code](#running-the-code)  
* [Support](#support)  
* [Final Thoughts](#final-thoughts)  

## Introduction
This repository contains a basic *proof of concept* implementation of what I'd like to call Valli encoding, which leverages the exact count of the symbol frequencies to compress the input with combinatorics.  The input is processed one unique symbol at a time using the number of combinations (binomials) that could occur before placing each symbol, the sum of which generates a single symbol's encoding.  These symbol encodings can then be combined together based on the number of permutations of each preceeding symbol.  For a more detailed walkthrough see [how the code works](the-algorithm.md).

I'd be happy to be politely corrected if this already exists, as far as I can tell this is a novel approach, but I am just a problem solver for fun.  If I use any terminology incorrectly please correct me with references, that's how I learn best.

The size of the output will always be the size of the total number of permutations of the symbols given the symbol frequencies table, also known as [multinomials](https://en.wikipedia.org/wiki/Multinomial_theorem#Number_of_unique_permutations_of_words)  
Let A,B,C,... = symbol counts  
T = total count of symbols = A + B + C + ...  
Bit size = log2( T! / (A! * B! * C! * ...) )  

### Regarding the Shannon Limit
I had only ever seen entropy calculations at the limit, one such example is in the Wikipedia entry for [arithmetic coding](https://en.wikipedia.org/wiki/Arithmetic_coding#Sources_of_inefficiency) which states for probabilities 1/3, 1/3, 1/3 "the entropy of the message would be 4.755" or 5 bits, while the multinomial would mean a size of log2(3!) = 2.585 => 3 bits.  So I was surprised when I first saw this result, as the encoding seemed to be below the Shannon limit.  However, it seems with adaptive techniques that adjust the frequencies as you encode the same size can be achieved with other entropy encoders.

### The Frequency File
I've seen some objections to the frequency file that is output separate from the compression, this was done to make it easy to check it's contents as the vli file will look like random noise.  No more information is stored in the .freq file than is in this implementation of [arithmetic encoding](https://github.com/nayuki/Reference-arithmetic-coding/blob/fde1357935494f395b4d17ca7e9e897c226ad208/python/arithmetic-compress.py#L50), I just use 8 bytes per symbol where they use 4, a fixed 2048 vs their fixed 1024 bytes.  If you remove this fixed size table and look at the difference between this encoding and the *non-adaptive* arithmetic encoding, you'll see that this algorithm produces smaller output.  Having read that arithmetic encoding was the smallest compression possible, this also initially surprised me.  Again, with more research it seems that the adaptive versions can reach compression sizes equivalent to this compression technique.

## The Algorithm
Follow this link to see [how the code works](the-algorithm.md) in a step by step example walkthrough.

If you have more questions, check the [FAQ](FAQ.md).

## Running the Code
#### Required Dependencies:
* [GMP library](https://gmplib.org/) - Used for large integer math. Unfortunately there isn't one simple command for this, use your favorite search engine or LLM with your OS version specified.
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

## Support
Ideally knowledge would be free, but it takes time and effort to create and communicate, all the while one cannot live on ideas and dissertations alone.  With the emergence of sites that enable direct support from viewers like Patreon and Twitch/Youtube, I hope exchanging this content for financial support isn't asking too much.  If you enjoyed this as much as a drink, a movie, or more and have the ability to help out I'd greatly appreciate it.   

[Support this project](https://www.paypal.com/donate/?business=S7Q76A99VU44W&no_recurring=0&currency_code=USD)  
The receipt will have an email address if you want to send questions/requests, if you've donated I'll do my best to answer them if not in the README/[FAQ](FAQ.md).

I have more ideas I'd like to explore, but I quit my job to work on this and have spent the funds I put aside.  I'm grateful and lucky to have had the chance to set aside time to work on this and never intended to make money off it, so any donations would encourage further research or improvements.

## Final Thoughts
This approach for encoding is admittedly is not very practical, given it's polynomial / factorial nature and the fact encoding requires changing every bit along the entire length of the compressed value (bad for cpu caches), but it is fun to think about how this the absolute smallest we can possibly get for random symbol locations.  The dataset I originally set out to compress was bit sets from hash values (e.g. probablistic counts, bloom filters), so it may have some applications there since those datasets don't normally compress very well.  Memory and storage are ample these days so the few bytes it saves may not seem significant, but it's worth noting that even a 1 bit reduction means we've reduced the number of possible values in half.

Another noteworthy feature is this algorithm can be parallelized per unique symbol, as far as I know no other compression algorithm is parallelizable without sacrificing the compression ratio (not counting large lookup tables which can compress multiple symbols at once, which are still sequential in nature).  We could also consider not combining individual symbol encodings (binomial sums) together, instead storing each unique symbol separately, as combining only saves <1 bit per unique symbol and combining uses expensive large number multiplication.

I have more ideas, but those ideas also makes the code harder to read, so I've held off for now.  If there's more interest or support I might get to them.  For now I hope you enjoyed this as much as I did making it.
