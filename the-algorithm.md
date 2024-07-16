# The Algorithm
Note:  
    // indicates integer division (truncation)  
    N *choose* K is the [binomial coefficient](https://en.wikipedia.org/wiki/Binomial_coefficient) (N,K) 
## Encoding

#### Input
1. A message composed of some number of symbols (characters/bytes in the input file).
#### Steps
1. Create a frequency table by counting occurrences of each symbol in the message.
2. Set **encoding_accumulator** = 0; this value accumulates the encoding data for each symbol and contains the final encoding value.
3. Iterate through each symbol in the frequency table except for the last one[^1].
    1. Set **symbol_binomial_sum** = 0; holds the binomial sums for the current symbol being encoded
    2. Set **combiner** = 1; the number of permutations of all prior symbols before to the current symbol (see examples for more info)
    3. Iterate through the message until the current symbol is encountered.
        1. Calculate the binomial N *choose* K where:
            N= the zero based index of the symbol location and
            K = the current symbol count (1,2,3,...)
        1. Continue iterating through the message, summing all binomials calculated in the previous step together into **symbol_binomial_sum**.
    4. When the end of the message is reached for a symbol:
        1. Set **encoding_accumulator = encoding_accumulator + symbol_binomial_sum * combiner**
        2. Set **combiner = combiner * (message_size *choose* symbol_count)**; which will be used in the next symbol iteration.
        3. Remove the encoded symbols from the message.  (This is optimized away in the code as array resizing is expensive, instead a 'null' value is used to indicate the location should no longer be counted.)
    5. Continue symbol until you reach the last symbol in the set.  The last value is not encoded since it can be inferred by the location of all the other symbols.
3. The encoding_accumulator is now the final encoded data.

[^1]: The order of iteration through the frequency table must be consistent and repeatable for decoding.  The provided code iterates in order of ascending symbol count followed by symbol itself to resolve duplicate counts.  This sort is used since more symbol occurrences means more operations required to encode.
##  Encoding Example 

| Message | h   | i   | d   | e   | h   | o   | h   | e   | d   | e   | h   | e   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  | 11  |

Organize the data into the following frequency table, sorted by count and symbol:

| Count | Symbol |
| ----- | ------ |
| 1     | i      |
| 1     | o      |
| 2     | d      |
| 4     | e      |
| 4     | h      |

**Initial Values**
```
message_length = 12
combiner = 1
encoding_accumulator = 0
```
**Symbol 'i'**
```
symbol_binomial_sum = 0
# First 'i' in message at index 1
symbol_binomial_sum = symbol_binomial_sum + (location choose count) = 0 + (1 choose 1) = 1
no more 'i' locations
encoding_accumulator = encoding_accumulator + combiner * symbol_binomial_sum
encoding_accumulator = 0 + 1 * 1 = 1
combiner = combiner * (message_length choose symbol_count)
combiner = 1 * (12 choose 1) = 12
message_length = message_length - symbol count 
message_length = 12 - 1 = 11
# Remove 'i' from message
```

| Message | h   | d   | e   | h   | o   | h   | e   | d   | e   | h   | e   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  |

**Symbol 'o'** 
```
symbol_binomial_sum = 0
# first 'o' in message at index 4
symbol_binomial_sum = 0 + 4 choose 1 = 4
# no more 'o' locations
encoding_accumulator = 1 + 12 * 4 = 49
combiner = 12 * (11 choose 1) = 132
message_length = 11 - 1 = 10
# remove 'o' from message
```

| Message | h   | d   | e   | h   | h   | e   | d   | e   | h   | e   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   |

**Symbol 'd'**
```
# First 'd' at index 1
symbol_binomial_sum = 0 + 1 choose 1 = 1
# Second 'd' at index 6
symbol_binomial_sum = 1 + 6 choose 2 = 16
# no more 'd' locations
encoding_accumulator = 49 + 132 * 16 = 2161
combiner = 132 * (10 choose 2) = 5940
message_length = 10 - 2 = 8
# Remove 'd' from message
```

| Message | h   | e   | h   | h   | e   | e   | h   | e   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   |

**Symbol 'e'**
```
First 'e' at index 1
symbol_binomial_sum = 0 + 1 choose 1 = 1
Second 'e' at index 4
symbol_binomial_sum = 1 + 4 choose 2 = 7
Third  'e' at index 5
symbol_binomial_sum = 7 + 5 choose 3 = 17
Fourth 'e' at index 7
symbol_binomial_sum = 17 + 7 choose 4 = 52
no more 'e' locations
encoding_accumulator = 2161 + 5940 * 52 = 311041
```

| Message | h   | h   | h   | h   |
| ------- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   |

The remaining locations are all 'h' which does not need to be encoded since there is only 1 permutation possible.  This would also be the case if the message was all a single character, in that case the algorithm is essentially run length encoding using the frequency table.

Thus encoding is complete, the final value to store = 311041.  This requires 3 bytes / 19 bits to store, while the original message was 12 bytes / 96 bits.
## Decoding
Strike that, reverse it.
#### Inputs 
1. Encoded output.
2. Frequency table containing exact counts of each symbol/character.

#### Steps
1. Load the encoded data into an integer and the frequency table.
2. Create an array/buffer to hold the message that is the sum of the frequency counts.
    1. Fill this array with the last symbol to be decoded.  In the provided code this is the most common symbol, which means fewer locations to place in total.
3. Iterate through the symbols in the same order as the encoding, not including the last symbol as those locations are inferred.
    1. If the current symbol is the second to last symbol ('e' in the example message)
        1. The symbol_binomial_sum = encoded data
    2. Else
        1. Calculate the uncombiner = (total positions - decoded positions) *choose* current symbol count.
        2. Divide with remainder the encoded data by the uncombiner.
            1. The remainder is the sum of binomials for the current symbol.
            2. The quotient is the encoded data without the current symbol included, which will be used in the next iteration.
    3. For each symbol count, starting with K=total_symbol_count and working backwards to 1
        1. Find the binomial N *choose* K where K= the current symbol count that satisfies
            1. (N *choose* K) < symbol_binomial_sums and 
            2. symbol_binomial_sums < ((N+1) *choose* K)
        2. Count the offset caused by previously placed symbols. This is similar to encoding removing symbols from the message after encoding them, we must calculate the final location of the symbol as though those symbols were not there. A naïve[^2] but straightforward approach would be:
            1. `offset = 0
            2. `for i = 0 to N+offset: if message[i]!='h': offset++
        3. Place the K'th symbol in the message array in the N+offset zero based index location
    4. Continue placing symbols until symbol count == 1, then no estimate is needed and N = symbol_binomial_sum
4. When the symbol iteration completes, the message is decoded in the array contents.
[^2]: In the provided code this is optimized, the offset is set to the total number of symbols placed and working backwards from the end of the message we subtract every symbol placed from that offset, to arrive at the same result as the naive solution
### Decoding Example

**Initial values**
Frequency table

| Count | Symbol |
| ----- | ------ |
| 1     | i      |
| 1     | o      |
| 2     | d      |
| 4     | e      |
| 4     | h      |

```
encoded_data = 311041
remaining_positions = message_length = 12
```
Message Buffer/Array, filled with last symbol

| Message | h   | h   | h   | h   | h   | h   | h   | h   | h   | h   | h   | h   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  | 11  |

**Symbol 'i'**
```
uncombiner = remaining_positions choose count = 12 choose 1 = 12
symbol_binomial_sum = combined_data % uncombiner
symbol_binomial_sum = 311041 % 12 = 1 
encoded_data = encoded_data // uncombiner 
encoded_data = 311041 // 12 = 25920
decode first 'i'
# location estimate & verification not needed for count==1
# for an example go to symbol 'd'
location_estimate = symbol_binomial_sum = 1
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# no non-'h' symbols found, offset = 0
message[location_estimate + offset => 1 + 0 => 1] = 'i'
no more 'i' locations
remaining_positions = remaining_positions - count = 12 - 1 = 11
```

| Message | h   | **i**   | h   | h   | h   | h   | h   | h   | h   | h   | h   | h   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | **1**   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  | 11  |

**Symbol 'o'**
```
uncombiner = 11 choose 1 = 11
symbol_binomial_sum = 25920 % 11 = 4 
encoded_data = 25920 // 11 = 2356
# decode first 'o'
# location estimate & verification not needed for count==1
# for an example go to symbol 'd'
location_estimate = symbol_binomial_sum = 4
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i; offset = 1
message[4+1=>5] = 'o'
# no more 'o' locations
remaining_positions = 11 - 1 = 10
```

| Message | h   | i   | h   | h   | h   | **o** | h   | h   | h   | h   | h   | h   |
| ------- | --- | --- | --- | --- | --- | ----- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | **5** | 6   | 7   | 8   | 9   | 10  | 11  |

**Symbol 'd'**
```
uncombiner = 10 choose 2 = 45
symbol_binomial_sum = 2356 % 45 = 16 
encoded_data = 2356 // 45 = 52
# decode first 'd'
# symbol count > 1, estimate location
location_estimate = (symbol_binomial_sum * count!)^(1/count) + count//2
location_estimate = (16*2!)^(1/2)+2//2 = 6
# verify estimate
# check overestimate, can only overestimate by 1 with this approach
current_binomial = location_estimate choose count = 6 choose 2 = 15
if current_binomial > symbol_binomial_sum:
    # 15 > 16 => false, no underestimation
    location_estimate -= 1
else:
    # if the next location's (N+1) binomial is larger than symbol_binomial_sum
    # then the estimate is correct
    # else, loop until larger is found
    next_binomial = location_estimate+1 choose count = 7 choose 2 = 21
    while(next_binomial < symbol_binomial_sum):
        # 21 < 16 => false, no correction needed, no underestimation
        location_estimate += 1
        next_binomial = location_estimate choose count
# location_estimate = 7
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i,d; offset = 2
message[location_estimate+offset=6+2=>8] = 'd'
symbol_binomial_sum -= location_estimate choose count = 15
symbol_binomial_sum = 1
# decode second 'd'
# location estimate & verification not needed for count==1
location_estimate = symbol_binomial_sum = 1
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i; offset = 1
message[1+1=>2] = 'd'
# no more 'd' locations
remaining_positions = 10 - 2 = 8
```

| Message | h   | i   | **d** | h   | h   | o   | h   | h   | **d** | h   | h   | h   |
| ------- | --- | --- | ----- | --- | --- | --- | --- | --- | ----- | --- | --- | --- |
| Index   | 0   | 1   | **2** | 3   | 4   | 5   | 6   | 7   | **8** | 9   | 10  | 11  |

**Symbol 'e'**
```
# for the second to last symbol (last to be decoded)
# the uncombiner is not needed and symbol_binomial_sum = encoded_data
symbol_binomial_sum = encoded_data = 52

# decode first 'e'
location_estimate = (52*4!)^(1/4)+4//2 = 7
# check overestimate
current_binomial = 7 choose 4 = 35
# 35 > 52 => false, no overestimation
if current_binomial > symbol_binomial_sum:
    location_estimate -= 1
else:
    # check underestimate
    next_binomial = location_estimate+1 choose count = 8 choose 4 = 70
    # 70 < 52 => false, no correction needed, no underestimation
    while(next_binomial < symbol_binomial_sum):
        location_estimate += 1
        next_binomial = location_estimate choose count
# location_estimate = 7
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i,d,h,o; offset = 4     
message[7+4=>11] = 'e'
symbol_binomial_sum -= location_estimate choose count => 7 choose 4 => 35
symbol_binomial_sum = 17

# decode second 'e'
location_estimate = (17*3!)^(1/3)+3//2 = 5
# check overestimate
current_binomial = 5 choose 3 = 10
#   10 > 17 => false, no overestimation
# check underestimate
next_binomial = 6 choose 3 = 20
#   20 < 17 => false, no underestimation
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i,d,h,o; offset = 4     
message[5+4=>9] = 'e'
symbol_binomial_sum -= 5 choose 3 => 10
symbol_binomial_sum = 7

# decode third 'e'
location_estimate = (7*2!)^(1/2)+2//2 = 4
# check overestimate
    current_binomial = 4 choose 2 = 6
#   6 > 7 => false, no overestimation
# check underestimate
    next_binomial = 5 choose 2 = 10
#   10 < 7 => false, no underestimation
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i,d,o; offset = 3
message[4+3=>7] = 'e'
symbol_binomial_sum -= 4 choose 2 => 6
symbol_binomial_sum = 1

# decode fouth 'e'
# location estimate & verification not needed for count==1
location_estimate = symbol_binomial_sum = 1
# calculate offset
offset = 0
for i = 0 to location_estimate+offset: if message[i]!='h': offset++
# found i,d; offset = 2
message[1+2=>3] = 'e'

# no more 'e' locations
# remaining_positions no longer needed
# decoding complete
```

| Message | h   | i   | d   | e   | h   | o   | h   | e   | d   | e   | h   | e   |
| ------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Index   | 0   | 1   | 2   | 3   | 4   | 5   | 6   | 7   | 8   | 9   | 10  | 11  |

There and back again, decoding is complete.


If you enjoyed that and would like to help me make more you could consider [Supporting](https://github.com/Peter-Ebert/Valli-Encoding#support).  
More info in the [README](README.md).
