// Frequency Table Implementation w/ basic compression:
//     diff encoding, variable length integers, bit packing

// Stores the symbols and frequencies used for compression/decompression
// Serialization performs some basic bit packing, leveraging the sorted counts
// followed by corresponding byte symbols, no compression


#include <fstream>      // ifstream,ofstream
#include <immintrin.h>  // lzcnt

// Simple structure to contain the dictionary information.
// Array of 64 bit numbers,
// The first 7 most significant bytes store the count
// and the last byte stores the character being counted
// This allows for sorting directly on the full 64 bits
// Size is always 256*8 = 2048 bytes 
struct FreqChar {
    uint64_t data[256] = {0};
    void setChar(unsigned char i, unsigned char symbol) {
        data[i] = (data[i] & 0xFFFFFFFFFFFFFF00) | symbol;
    }
    unsigned char getChar(unsigned char i) {
        return data[i];
    }
    void setCount(unsigned char i, size_t count) {
        data[i] += count << 8;
    }
    uint64_t getCount(unsigned char i) {
        return data[i] >> 8;
    }
    void incrCount(unsigned char i) {
        data[i] += 1ull << 8;
        //warning: no overflow detection
    }
    //sort the dictionary by frequency, ascending
    void sortData() {
        std::sort(data, data+(sizeof(data)/sizeof(data[0])));
    }

    // A very basic freq table serialization
    // bit packing of sorted counts, when count==0, we also know the number of byte symobls to read
    // returns count of bytes written to file
    uint64_t serialize(std::ofstream& out_file) {
        uint64_t output_byte_count = 0;        
        // write the bit length of the largest count, max 6 bits
        uint64_t count = (data[255] >> 8);
        int8_t bit_length = (64 - _lzcnt_u64(count));
        // !!! use last bit length, not current
        int8_t last_bit_length = (64 - _lzcnt_u64(count));
        uint8_t byte_buffer = last_bit_length;
        uint8_t bit_offset = 6;
        int non_zero = 0;
        //bit pack the counts
        for(int i=255; i>=0; i--) {
            count = this->getCount(i);
            bit_length = (64 - _lzcnt_u64(count));
            int8_t bits_output = 0;
            while(last_bit_length>bits_output) {
                // if byte would fill, write and go next byte
                if((last_bit_length-bits_output) >= (8 - bit_offset)) {
                    byte_buffer |= count << bit_offset;
                    count = count >> (8 - bit_offset);
                    out_file << byte_buffer;
                    output_byte_count++;
                    bits_output += (8 - bit_offset);
                    byte_buffer = 0;
                    bit_offset = 0;
                } else {
                    // no overflow, insert and move offset
                    byte_buffer |= count << bit_offset;
                    bit_offset += last_bit_length-bits_output; //can't be >= 8 based on if
                    bits_output += last_bit_length-bits_output; //can't be >= 8 based on if
                }
            }
            if(this->getCount(i)==0) {
                //exit loop
                //todo: test with freqs that end here
                break;
            }
            non_zero++;
            last_bit_length = bit_length;
            // use the lenght of the current count to set the next one
        }
        // if unwritten bits in buffer, flush
        // this will waste at most 7 bits, could be used by encoding, skipping optimization for now
        if(bit_offset != 0) {
            out_file << byte_buffer;
            output_byte_count++;
        }

        //output non zero count symbols (bytes) in the same order as the sort (desc)
        //todo: assert non_zero >= 1
        for(int i=255; i>=(255-non_zero); i--) {
            if(this->getCount(i)) {
                out_file << this->getChar(i);
                output_byte_count++;
            } else {
                break;
            }
        }
        return output_byte_count;
    }

    // basic deserializer
    // returns bytes read
    uint64_t deserialize(std::ifstream& input_file) { 
        //for legacy reasons, keep all zero counts in place
        //todo: resize array for non-zero counts
        //todo: assert file len > 1
        uint64_t read_byte_count = 1;
        // read first 6 bits for count bit length
        uint8_t byte_buffer;
        input_file.read((char*)&byte_buffer, 1);
        uint8_t bit_length = byte_buffer & 0b00111111;

        uint8_t bit_offset = 6;
        int symbol_count = 0;
        // count loop
        do {
            int64_t count = 0;
            int8_t bits_read = 0;
            // read bytes loop
            while(bit_length>bits_read) {
                if((bit_length-bits_read) >= (8 - bit_offset)) {
                    // overflow, load then read next byte
                    //load and read next byte, loop until done
                    count |= ((uint64_t)(byte_buffer >> bit_offset)) << bits_read;
                    bits_read += (8 - bit_offset);
                    input_file.read((char*)&byte_buffer, 1);
                    read_byte_count++;
                    bit_offset = 0;
                    //update bits read
                } else {
                    // fits in current byte
                    // load count and update offset
                    count |= ((uint64_t)(byte_buffer >> bit_offset)) << bits_read;
                    //zero out bits beyond the length
                    count &= ((1ull << (bit_length))-1);
                    bit_offset += bit_length-bits_read;
                    bits_read = bit_length;
                }
            }
            //set the count, ascending order
            this->setCount(255-symbol_count, count);
            // exit loop if the count is 0 (last in sequence)
            if(count==0) {
                break;
            }
            symbol_count++;
            // update bit_length with current length
            bit_length = (64 - _lzcnt_u64(count));

        } while(symbol_count < 255); //for safety, can be removed, unreachable

        // if bit_offset == 0, it contains a symbol
        // else unset bits, read next byte
        if(bit_offset != 0) {
            input_file.read((char*)&byte_buffer, 1);
            read_byte_count++;
        }
        // assert: symbol_count > 1
        this->setChar(255, byte_buffer);
        std::vector<uint8_t> found_symbols(symbol_count);
        found_symbols.at(0) = byte_buffer;
        // set characters
        for(int i=254; i>(255-symbol_count); i--) {
            input_file.read((char*)&byte_buffer, 1);
            read_byte_count++;
            this->setChar(i, byte_buffer);
            found_symbols.at(255-i) = byte_buffer;
        }

        std::sort(found_symbols.begin(), found_symbols.end());
        // print vector
        int found_idx = 0;
        for(int i=0; i<(256); i++) {
            if(found_idx < symbol_count && i==found_symbols.at(found_idx)) {
                //skip
                found_idx++;
            } else {
                // update char
                this->setChar(i-found_idx, i);
            }

        }
        // legacy code: back fill other symbols
        // vector symbol_count


        return read_byte_count;
        //unique_symbols += 1; // 0 is never used
    }    

};


