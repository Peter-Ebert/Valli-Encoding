// Valli Decompression - Proof of concept
// clang++ -std=c++17 -O2 poc-decompress.cpp -lgmp -o poc-decompress

// This implementation is more complicated than the naive approach in the documentation.
// It uses an an approximate calculation to estimate the binomial, then adjusts it from there.
// Also some shortcuts for trivial values and other minor optimizations.
#include <iostream>     // cout
#include <fstream>      // ifstream,ofstream
#include <bitset>       // bitset
#include <immintrin.h>  // intrinsics
#include <chrono>       // timer
#include <math.h>       // log2
#include <gmp.h>        // bigint mpz_t
#include <algorithm>    // sort

#include "utility-functions.hpp"


using namespace std;

int main(int argc, char* argv[]) {

    // verify args
    if (argc != 2) {
        cout << "Specify a compressed file ending in .vli, example: " << argv[0] << " <file>" << endl;
        return 1;
    }

    // variable to set file output
    bool write_file = true;

    // validate and set filenames
    string compressed_path_file = argv[1];
    string file_ending = ".vli";
    // Ensure file ending is .vli, no other validation performed.
    // Will error out (vector out of bounds) if the encoded data value is too large for the frequency counts.
    // Since this *should* only be caused by user error/manipulation, leaving unhandled for now.
    // *could also be cause by bugs, please report any issues
    // Else, every value smaller than the max permutations will decompress to some permutation of symbols.
    if (!(compressed_path_file.length() >= file_ending.length() && compressed_path_file.compare(compressed_path_file.length() - file_ending.length(), file_ending.length(), file_ending) == 0)) {
        cout << "Invalid filename, must end with '.vli'" << endl;
        return 1;
    } 

    // assume frequencies file name & location based on source file name
    string basename = compressed_path_file.substr(0, compressed_path_file.length() - file_ending.length());
    string filename_freq_table = basename + ".freq";
    // create a new output file so that they can be compared to the source
    string filename_out = basename + ".decom";

    cout << "Compressed file: " << compressed_path_file << endl;
    cout << "Frequencies file: " << filename_freq_table << endl;
    cout << "Output file: " << filename_out << endl;

    // read file
    std::ifstream input_file(compressed_path_file, std::ios::binary);
    if (!input_file) {
        // Handle file open error
        std::cerr << "Error opening file, most likely file does not exist." << std::endl;
        return 1;
    }
    // deserialize frequency table at start of file
    FreqChar freqs;
    size_t freq_byte_count = freqs.deserialize(input_file);
    if (!input_file) {
        std::cerr << "Error reading frequency table." << std::endl;
        return 1;
    }
    cout << "Frequency table size (bytes):" << freq_byte_count << endl;
    
    cout << "Frequencies:" << endl;
    cout << "int : char : count" << endl;
    uint64_t total_symbols = 0;
    uint64_t unique_symbols = 0;
    for (int i = 0; i < 256; i++) {
        if(freqs.getCount(i)) {
            cout << (uint)freqs.getChar(i) << " : " << freqs.getChar(i) << " : " << freqs.getCount(i) << endl;
            total_symbols += freqs.getCount(i);
            unique_symbols++;
        }
    }
    
    // load encoding into input_buffer
    std::vector<char> input_buffer((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    // Check successful file read
    if (!input_file && !input_file.eof()) {
        std::cerr << "Error reading encoding." << std::endl;
        return 1;
    }
    input_file.close();
    cout << "Encoding size (bytes): " << input_buffer.size() << endl;

    mpz_t     compressed_data, symbol_combo, extracted_combo, root_result, binomial, numerator, denominator, factorial, uncombiner, est_binomial;
    mpz_inits(compressed_data, symbol_combo, extracted_combo, root_result, binomial, numerator, denominator, factorial, uncombiner, est_binomial, NULL);

    // export and import must match
    // mpz_export(output_array, NULL,                1, 1, -1, 0, data_accumulator);
    mpz_import(compressed_data, input_buffer.size(), 1, 1, -1, 0, input_buffer.data());

    // verbose info
    gmp_printf("Imported Integer: %Zd\n", compressed_data);
    if(unique_symbols < 1) {
        // todo: handle this case, not implemented
        cout << "Not enough unique symbols, 2 required in current implementation, aborting." << endl;
        return 1;
    }
    
    // This implementation fills the output message buffer with the
    // last symbol and uses it as an 'empty' location indicator.
    // After all other symbols are placed correctly the
    // last symbol is already in the correct locations.
    char last_symbol = (char)freqs.getChar(255);
    // allocate buffer for decoded output,  fill with most common character
    std::vector<char> output_buffer(total_symbols, last_symbol);
    uint64_t remaining_locations = total_symbols;

    mpz_set_ui(uncombiner, 1);
    size_t symbol_start=256; //starting out of bounds
    // start at 254 becuase last symbol isnt encoded

    // symbol index
    size_t symbol_idx = 0;
    // advance index to start of populated symbols
    while(freqs.getCount(symbol_idx) == 0) {
        symbol_idx++;
    } 

    // Loop through each symbol, except the last 
    while(symbol_idx < 255) {
        // verbose output
        cout << "------------------------------------" << endl;
        char current_symbol = (char)freqs.getChar(symbol_idx);
        cout << "Current symbol: " << current_symbol << " (" << (uint)current_symbol << ")" << endl;
        cout << "Locations remaining: " << remaining_locations << endl;
        // To extract symbol_combo from compressed_data
        // we mod the compressed_data by the number of combinations for that symbol
        // then subtract out that remainder and repeat for next symbol

        // 2nd to last value, calculation & extraction not needed
        if(symbol_idx != 254) {
            // calculate permutations of symbol="uncombiner" to extract symbol combination
            choose_reuse(remaining_locations, freqs.getCount(symbol_idx), uncombiner, numerator, denominator);
            // compressed data = quotient
            // extracted combo = remainder
            mpz_tdiv_qr(compressed_data, extracted_combo, compressed_data, uncombiner);
        } else {
            // Last encoded value
            // no more extraction needed, what remains is the 254th symbol's sum of binomials
            // because uncombiner == 1 so extracted_combo = compressed_data
            mpz_set(extracted_combo, compressed_data);
        }
        
        // verbose output
        gmp_printf("Uncombiner: %Zd \n", uncombiner);    
        gmp_printf("Extracted Binomial Sum: %Zd \n", extracted_combo);

        // setup loop to deconstruct the extracted binomial sum
        // largest index location extracted first
        size_t symbol_count = freqs.getCount(symbol_idx);
        size_t insert_offset = total_symbols - remaining_locations;
        // zero based index for locations
        size_t loc_idx;
        size_t last_loc_idx = total_symbols - 1; 
        // size_t removed_symbols = total_symbols - remaining_locations;
        // calculate factorial 
        mpz_fac_ui(factorial, symbol_count);

        // Symbol extraction innner loop
        // continue while symbol_count > extracted_combo
        // this avoids estimates that are below zero
        while(mpz_cmp_ui(extracted_combo, symbol_count) > 0) {
            // use sum of binomials property, combined with Newton's method
            // combo ~= (n-k//2)^k / k!
            // (combo*k!)^(1/k) + k//2 ~= n
            
            // multiply in k!
            mpz_mul(symbol_combo, extracted_combo, factorial);
            
            // find the root
            mpz_root(root_result, symbol_combo, symbol_count);
            // add symbol_count//2 to go from near the middle to near the top=n
            size_t loc_idx = mpz_get_ui(root_result) + symbol_count/2;
            
            // calculate estimated binomial value
            choose_reuse(loc_idx, symbol_count, est_binomial, numerator, denominator);
            // verbose output
            cout << "Estimated: " << loc_idx << " choose " << symbol_count << endl;

            // Validate estimation (1 & 2) //
            // (1) ensure estimate is less than target value, some even values of k can over estimate
            if(mpz_cmp(est_binomial, extracted_combo) > 0) {
                // if overestimated, shift N down by 1
                // N-1 choose K: multiply by N-K; divide by N
                mpz_mul_ui(est_binomial, est_binomial, loc_idx-symbol_count);
                mpz_divexact_ui(est_binomial, est_binomial, loc_idx);
                loc_idx -= 1;
                cout << "Adjusted down" << endl;
            }
            // (2) ensure estimate is not too low by looking at the delta
            // todo: opt: can skip if corrected down
            // subtract estimate from binomial
            mpz_sub(extracted_combo, extracted_combo, est_binomial);
            // calculate diff between N choose K and N+1 choose K => N choose K-1 => est_binomial *K then /(N-K+1)
            mpz_mul_ui(est_binomial, est_binomial, symbol_count);
            // todo: should be removable, checked for safety
            // avoid div by zero
            if(loc_idx-symbol_count+1 != 0) {
                // est_binomial / (N-K+1)
                mpz_divexact_ui(est_binomial, est_binomial, loc_idx-symbol_count+1); 
            }
            
            // tracking for information purposes only, not needed for calculation
            size_t adjust_up_count = 0;
            cout << "---Adjustment loop---" << endl;
            
            // Estimate adjustment loop
            // while(delta to next binomial < remaining sum of binomials)
            while(mpz_cmp(est_binomial, extracted_combo) <= 0 && mpz_cmp_ui(extracted_combo, symbol_count) > 0) {
                // estimate too small, adjust estimated N up by 1
                // subtract diff (N Choose K-1)
                mpz_sub(extracted_combo, extracted_combo, est_binomial);
                loc_idx += 1;
                // calculate next diff and confirm
                // to continue calculation up, estimate N+1 choose K = current estimate *N+1; /N-K to current estimate
                // if estimate is zero, cannot shift up
                if(loc_idx<=symbol_count) {
                    //then previous estimate is 0, set est_binomial = 1
                    mpz_set_ui(est_binomial, 1);
                    loc_idx = symbol_count;
                } else {
                    mpz_mul_ui(est_binomial, est_binomial, loc_idx);
                }
                //avoid division by zero
                if(loc_idx-symbol_count != 0) { 
                    mpz_divexact_ui(est_binomial, est_binomial, loc_idx-(symbol_count-1));
                    // mpz_divexact_ui(est_binomial, est_binomial, loc_idx-symbol_count+1)); // cleaner? VERIFY
                }
                adjust_up_count += 1;             
            }
            // verbose output
            cout << "Adjusted up: " << adjust_up_count << "x" << endl;

            // calculate offset based on previously placed symbols
            // optimization: start with the total count of placed symbols, subtract already placed symbols as we move backwards
            // for each non-last symbol found, reduce the offset
            // cout << "loop: " << last_loc_idx << " to " << loc_idx+insert_offset << endl;
            for(size_t i=last_loc_idx; i>=loc_idx+insert_offset && insert_offset!=0; i--) {
                if(output_buffer[i] != last_symbol) {
                    insert_offset--;
                }
            }
            // verbose output
            cout << "Location offset: " << insert_offset << endl;
            last_loc_idx = loc_idx+insert_offset-1; // -1 because current placement location already checked
            //update character in output buffer
            cout << "Symbol placed at: " << loc_idx+insert_offset << endl; 
            output_buffer.at(loc_idx+insert_offset) = current_symbol;

            // Setup next loop
            // calculate next lower factorial=(N-1)!=(N!)/N
            mpz_divexact_ui(factorial, factorial, symbol_count);
            symbol_count -= 1;            
        }

        // if symbol_count <= extracted combo, decoding is trivial
        if(symbol_count != 0 && mpz_cmp_ui(extracted_combo, symbol_count) <= 0) {
            cout << "symbol_count <= remaining sum of binomials" << symbol_count << endl;
            // gmp_printf("extracted_combo: %Zd\n", extracted_combo);
            // cout << "symbol_count: " << symbol_count << endl;
            size_t null_idx = 0;
            do {
                // seek to next non-null index
                while(output_buffer[null_idx] != last_symbol) {
                        null_idx += 1;
                }
                if(mpz_cmp_ui(extracted_combo, symbol_count) < 0) {
                    // if symbol count gt extracted combo place all remaining symbols in in the first location they appear
                    output_buffer.at(null_idx) = current_symbol;
                    cout << "Symbol placed at: " << null_idx << endl; 
                    symbol_count -= 1;
                } else if(mpz_cmp_ui(extracted_combo, symbol_count) == 0) {
                    // if equal skip one location
                    null_idx += 1;
                    while(output_buffer[null_idx] != last_symbol) {
                        null_idx += 1;
                    }
                    output_buffer.at(null_idx) = current_symbol;
                    cout << "Symbol placed at: " << null_idx << endl;
                    symbol_count -= 1;
                } else {
                    // if lt, continue placing
                    output_buffer.at(null_idx) = current_symbol;
                    cout << "Symbol placed at: " << null_idx << endl;
                    symbol_count -= 1;
                }
            } while(symbol_count > 0);
        }
        //used for next loop and location placement later
        remaining_locations -= freqs.getCount(symbol_idx);
        symbol_idx++;
    }

    // decompression done, final output:
    cout << "----------------------------------" << endl;
    // print decompressed data to console
    for (const auto& value : output_buffer) {
        cout << value;
    }
    cout << endl;

    cout << "Writing decompressed data to: " << filename_out << endl;
    // check setting for file output
    if (write_file) {
        //output to file
        ofstream output_file(filename_out);
        if (!output_file.is_open()) {
            std::cerr << "Failed creating output file." << std::endl;
            return 1;
        }
        for (const auto& value : output_buffer) {
            output_file << value;
        }
        output_file.close();
    }

    return 0;
}
