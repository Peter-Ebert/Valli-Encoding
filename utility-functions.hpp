// useful functions for binomials and other calculations

#include <fstream>
#include <vector>
#include <gmp.h>  //mpz_t

#include "frequency-table.hpp"


// returns if read is valid: true = valid
bool FileToCharVector(const std::string& filename, std::vector<char>& buffer) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        // Handle file open error
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(fileSize);
    if (file.read(buffer.data(), fileSize)) {
        return true;
    } else {
        return false;
    }

}

void CalcFrequencyPairs(std::vector<char> &buffer, FreqChar &freqs) {
    // initialize values
    for(int i=0; i<256; i++) {
        freqs.data[i] = i;
    }
    // loop through buffer
    for (size_t i=0; i < buffer.size(); i++) {
        freqs.incrCount(buffer[i]);
        if(buffer[i] < 0) {
            std::cout << "index: " << i << " value:" << buffer[i] << std::endl;
        }
    }
}

void choose_reuse(uint64_t n, uint64_t k, mpz_t &c, mpz_t &c1, mpz_t c2) {
    // mpz_t's must already be initialized
    // mpz_t c1,c2;
    // mpz_inits(c1,c2, NULL); //opt: move inline to avoid cost of initialization every time
    //assert k > 0
    //if n<k return 0
    if(n<k) {
        mpz_set_ui(c, 0);
        return;
    }
    mpz_set_ui(c1, n);
    for(uint64_t i=n-1;i>n-k;--i) {
        mpz_mul_ui(c1, c1, i);
    }
    //opt: if decrementing k by 1 every time, can reuse and divide, as in encode_symbol_location_reuse
    mpz_fac_ui(c2, k);
    mpz_cdiv_q(c, c1, c2);
}

//clean-ver
void encode_symbol_location_reuse(uint64_t n, uint64_t k, mpz_t symbol_accumulator, mpz_t &numerator, mpz_t &denom_fact, mpz_t &combo_result) {
    //moved
    // // increment k and multiply into denom_fact
    // k += 1;
    // mpz_mul_ui(denom_fact, denom_fact, k);
    
    //if n<k return 0
    if(n<k) {
        mpz_set_ui(combo_result, 0);
        return;
    }

    mpz_set_ui(numerator, n);
    for(uint64_t i=n-1;i>n-k;--i) {
        mpz_mul_ui(numerator, numerator, i);
    }
    mpz_divexact(combo_result, numerator, denom_fact);
    mpz_add(symbol_accumulator, symbol_accumulator, combo_result);
}

//warning: overwrites combo_result
void next_multiply_combiner(mpz_t &multiply_combiner, uint64_t n, uint64_t k, mpz_t &combo_result, mpz_t &numerator, mpz_t &denom_fact) {
    //remaining_locations choose symbol count
    mpz_set_ui(numerator, n);
    for(uint64_t i=n-1;i>n-k;--i) {
        mpz_mul_ui(numerator, numerator, i);
    }
    mpz_divexact(combo_result, numerator, denom_fact);
    mpz_mul(multiply_combiner, multiply_combiner, combo_result);
}           
