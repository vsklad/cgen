//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
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
        // support 1 block only
        static VariablesArray pad_message(const literalid_t* const message, const variableid_t message_size) {
            constexpr auto padded_size = MESSAGE_BLOCK_SIZE * WORD_SIZE;
            assert(padded_size >= 512);
            
            if (message_size == 0 || (message_size >> 3) > 55) {
                throw std::invalid_argument("Message must be between 1 and 55 bytes long");
            };
            
            VariablesArray padded_message(MESSAGE_BLOCK_SIZE, WORD_SIZE);
            literalid_t* data = padded_message.data();

            // copy the message
            std::copy(message, message + message_size, data);
            // append 1 bit
            data[message_size] = literal_t__constant(1);
            // append the rest with zeros except the last two bytes
            std::fill(data + message_size + 1, data + padded_size - 16, literal_t__constant(0));
            // the last 16 bits is the message size in bits
            uint16_t message_size_ = message_size;
            for (auto i = 0; i < 16; i++) {
                data[padded_size - i - 1] = message_size_ & 1;
                message_size_ >>= 1;
            };

            return padded_message;
        };
    };
};

#endif /* sha_hpp */
