// Valli Entropy Encoder
// Quick proof of concept
// To build:
// -requires: GMP lib https://gmplib.org/
// clang++ -std=c++17 -O2 poc-compress.cpp -lgmp -o poc-compress

#include <iostream>  // cout
#include <fstream>   // ifstream,ofstream
#include <bitset> // bitset
#include <immintrin.h> // intrinsics
#include <chrono> // timer
#include <math.h>       /* log2 */
#include <gmp.h> // bigint mpz_t
#include <algorithm> // sort

#include "utility-functions.hpp"


using namespace std;

int main(int argc, char* argv[]) {

    // parse file name
    if (argc != 2) {
        cout << "Specify a single file, example: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    // variable to set file output
    bool write_file = true;

    string source_path_file = argv[1];
    string filename = source_path_file.substr(source_path_file.find_last_of("/\\") + 1);
    string filename_entropy = source_path_file + ".vli";
    string filename_freq_table = source_path_file + ".freq";

    // read file into memory
    std::vector<char> buffer;
    if(!FileToCharVector(source_path_file, buffer)) {
        cout << "File read error, most likely it does not exist." << endl;
        return 1;
    }
    cout << "File size: " << buffer.size() << " bytes" << endl;
    // Warn & exit in case someone accidentally submits a large file
    // File sizes much larger than your CPU cache can be quite slow
    // This implementation is designed as a POC and not highly optimized
    if(buffer.size() > 64000) {
        cout << "The filesize is greater than 64kb, this compressor implementation is not highly optimized, so compression times may be long once outside of L1 cache, you can comment out this if statement if you wish to proceed." << endl;
        return 0;
    }

    // count the frequencies of each symbol (char/byte)
    FreqChar freqs;
    CalcFrequencyPairs(buffer, freqs);

    //sort the frequency table, ascending by count then symbol
    sort(freqs.data, freqs.data+sizeof(freqs.data)/sizeof(freqs.data[0]));

    printf("Sorted Frequencies:\n");
    cout << "idx : chr : int : count" << endl;
    // print non-zero frequencies and count unique symbols
    uint64_t unique_symbols = 0;
    for (int i = 0; i < 256; i++) {
        // cout << freqs.getCount(i) << endl;
        if(freqs.getCount(i)) {
            unique_symbols++;
            cout << i << " : '" <<  freqs.getChar(i) << "' : " << (uint)freqs.getChar(i) << " : " << freqs.getCount(i) << endl;
        }
    }
    uint64_t total_symbols = buffer.size();

    cout << "==============================" << endl;
    cout << "Total symbols: " << total_symbols << endl;
    cout << "Unique symbols: " << unique_symbols << endl;
    cout << "------------------------------" << endl;

    if(unique_symbols < 2) {
        // todo: handle this case
        cout << "Less than 2 unique symbols, nothing to encode, aborting." << endl;
        return 1;
    }

    uint64_t remaining_loc = total_symbols;
    // select the least common character
    char null_symbol = (char)freqs.getChar(0);
    // This simple demonstration implementation select one character as a 'null' symbol to take the place of
    // symbols which have already been encoded.
    // As a result, all 256 byte values cannot be used in the input, 
    // since this is only likely with random or already compressed data, shouldn't be an issue for a POC
    if(freqs.getCount(0) != 0) {
        cout << "Unhandled: This implementation requires at least one symbol in the input to be unused ('null' symbol).  This input has all byte values used (0-255)." << endl;
        return -1;
    }
    // todo: to process inputs with all 256 characters are used:
    // if unique_symbols==256: pick lowest frequency and encode solo, then set that as the null symbol
    // alternatively (advanced), can rotate the 256 bytes w/ addition so that the max value occurs at the max byte value,
    // then use less than to test for placement or not, can be done in parallel threads this way as the array can be static

    // Making an assmumption about the gmp library (not verified)
    // Heavy reuse of mpz_t variables that are similar in size, 
    // with the assumption that there will be fewer allocatitons needed (and less inits)
    mpz_t num_product_seq, denom_fact, combo_result, symbol_accumulator;
    mpz_inits(num_product_seq, denom_fact, combo_result, symbol_accumulator, NULL);

    uint64_t symbol_count;
    uint64_t symbol_idx;
    
    mpz_t multiply_combiner, data_accumulator;
    mpz_inits(multiply_combiner, data_accumulator, NULL);
    mpz_set_ui(multiply_combiner, 1);
    mpz_set_ui(data_accumulator, 0);
    
    // Loop through each possible symbol
    // 256-1 because the last symbol (asc sort) does not need to be encoded/decoded
    for (int i = 0; i < 256-1; i++) { 
        // if character exists in message
        if(freqs.getCount(i)) {
            // reset for new symbol
            mpz_set_ui(symbol_accumulator, 0);
            mpz_set_ui(denom_fact, 1);
            // reset symbol count
            symbol_count = 1;

            //calculate location for first item
            cout << "--- " << freqs.getChar(i) << ":" << freqs.getCount(i) << " (" << (uint)freqs.getChar(i) << ")" << " ---" << endl;
            
            //find symbol location
            size_t removed_loc = 0;

            // encode current symbol by looping through buffer to find each location
            // can exit loop when last instance is found k = symbol_count
            for(size_t byte_loc = 0; byte_loc < buffer.size(); byte_loc++) {
                if(buffer[byte_loc]==(char)freqs.getChar(i)) { 
                    //found instance of symbol
                    // verbose: combination calculation for location choose symbol_count
                    cout << " + " << byte_loc-removed_loc << " choose " << symbol_count << endl;
                    
                    encode_symbol_location_reuse(byte_loc-removed_loc, symbol_count, symbol_accumulator, num_product_seq, denom_fact, combo_result);
                    buffer[byte_loc] = null_symbol;
                    if(symbol_count==freqs.getCount(i)) {
                        // all symbols have been found
                        // exit loop
                        break;
                    }
                    // increment k and multiply into denom_fact for next loop
                    symbol_count += 1;
                    mpz_mul_ui(denom_fact, denom_fact, symbol_count);
                    
                } else if(buffer[byte_loc]==null_symbol) {
                    // count the number of symbols that have been removed between the last byte location and the next one
                    removed_loc++; 
                }
            }

            // verbose output: sum of symbols and combiner multiple
            gmp_printf("Sum of Binomials: %Zd \n", symbol_accumulator);
            gmp_printf("Multiply combiner: %Zd \n", multiply_combiner);
            mpz_mul(combo_result, multiply_combiner, symbol_accumulator);
            mpz_add(data_accumulator, data_accumulator, combo_result);

            // calculation not needed for last symbol since it's not encoded
            // however it is needed for the 'max bit length' calculation at the end
            // otherwise can wrap with if(i != 254) {}
            next_multiply_combiner(multiply_combiner, remaining_loc, symbol_count, combo_result, num_product_seq, denom_fact);            

            //track how many possible locations remain without the current symbol
            remaining_loc -= freqs.getCount(i);
        }
    }

    // Verbose: output the final integer and statistics around the output
    cout << "----------Final Data----------" << endl;
    gmp_printf("%Zd \n", data_accumulator);
    cout << "------------------------------" << endl;

    size_t bit_length = mpz_sizeinbase(data_accumulator, 2);
    cout << "Current byte length: " << ceil(bit_length/8.0) << endl;
    cout << "Current bit length: " << bit_length << endl;
    // Use combiner to calc max bit len (total # of permutations of symbol frequencies)
    size_t max_bit_length = mpz_sizeinbase(multiply_combiner, 2);
    cout << "Max bit length: " << max_bit_length << endl;
    
    // Calculate the Shannon minimum bit length, non-adaptive
    // = shannon entropy per symbol * message length
    double shannon_entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        // cout << freqs.getCount(i) << endl;
        if(freqs.getCount(i)) {
            double probability = static_cast<double>(freqs.getCount(i)) / total_symbols;
            shannon_entropy -= probability * log2(probability);
        }
    }
    shannon_entropy = shannon_entropy * total_symbols;

    cout << "Static frequency Shannon limit: " << ceil(shannon_entropy) << endl;
    cout << "Bits saved: " << ceil(shannon_entropy)-max_bit_length << endl;
    cout << "Relative Size: " << 100 * (max_bit_length / ceil(shannon_entropy)) << "%" << endl;

    // Write compressed data and frequencies table
    if(write_file) {
        cout << "Writing compressed data to: " << filename_entropy << endl;
        ofstream out_file(filename_entropy);
        //write frequency table
        cout << "Frequency table size (bytes): " << freqs.serialize(out_file) << endl;
        // write encoded data
        size_t out_size = mpz_sizeinbase(data_accumulator, 256);
        cout << "Encoded data (bytes): " << out_size << endl;
        unsigned char *output_array = new unsigned char[out_size];
        //         output_array, word_count, order, size, endian, nails, data
        mpz_export(output_array, NULL,     1,    1,     -1,     0, data_accumulator);
        if(mpz_cmp_ui(data_accumulator, 0) == 0) {
            // if output == 0, gmp will not write to the array
            output_array[0] = 0;
        }
        out_file.write((char *)output_array, out_size);
        delete[] output_array;
        out_file.close();
    } else {
        cout << "Skipping data write." << endl;
    }

    return 0;
}