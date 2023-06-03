//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2023 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef sha_hpp
#define sha_hpp

#include <stdexcept>
#include "gf2n.hpp"
#include "assertlevels.hpp"

#define _min(value1, value2) (value1 < value2 ? value1 : value2)

namespace acl {
    
    using namespace bal;
    
    // prototype for a class implementing one of
    // the SHA family of hash functions
    template <std::size_t WORD_SIZE_, uint32_t MESSAGE_BLOCK_SIZE_, class BIT>
    class SHA {
    public:
        using Bit = BIT;
        using Word = bal::GF2NElement<WORD_SIZE_, BIT>;
        
        static constexpr std::size_t WORD_SIZE = WORD_SIZE_;
        static constexpr uint32_t MESSAGE_BLOCK_SIZE = MESSAGE_BLOCK_SIZE_;
        
    public:
        // assume the padded message can be represented as a sequence of words
        // i.e. the number of bits in the padded message is a multiple of word size
        // this is done for simplicity of representation as a sequence of hexadecimal constants
        //   more than any other reason
        static VariablesArray pad_message(const literalid_t* const message, const variables_size_t message_size) {
            constexpr auto block_bits_size = MESSAGE_BLOCK_SIZE * WORD_SIZE;
            variables_size_t number_of_blocks = message_size / block_bits_size + 1;
            if ((message_size % block_bits_size) > 447) {
                number_of_blocks++;
            };
            
            const variables_size_t padded_words_size = number_of_blocks * MESSAGE_BLOCK_SIZE;
            const variables_size_t padded_bits_size = padded_words_size * WORD_SIZE;
            VariablesArray padded_message(padded_words_size, WORD_SIZE);
            literalid_t* data = padded_message.data();

            // copy the message
            std::copy(message, message + message_size, data);
            // append 1 bit
            data[message_size] = literal_t__constant(1);
            // append the rest with zeros except for the last 64 bits
            std::fill(data + message_size + 1, data + padded_bits_size - 64, literal_t__constant(0));
            // the last 64 bits is the message size in bits
            uint64_t message_size_ = message_size;
            for (auto i = 0; i < 64; i++) {
                data[padded_bits_size - i - 1] = message_size_ & 1;
                message_size_ >>= 1;
            };

            return padded_message;
        };
    };
};

#endif /* sha_hpp */
