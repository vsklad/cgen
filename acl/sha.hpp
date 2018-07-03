//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef sha_hpp
#define sha_hpp

#include "word.hpp"

#define _min(value1, value2) (value1 < value2 ? value1 : value2)

namespace acl {

    using namespace ple;
    
    // prototype for a class implementing one of
    // the SHA family of hash functions
    template <WordSize WORD_SIZE_, uint32_t MESSAGE_BLOCK_SIZE_, class BIT>
    class SHA {
    public:
        using Word = Word<WORD_SIZE_, BIT>;
        
        static const constexpr WordSize WORD_SIZE = WORD_SIZE_;
        static const constexpr uint32_t MESSAGE_BLOCK_SIZE = MESSAGE_BLOCK_SIZE_;
        
    public:
        /*
        // support 1 block only
        static void pad_message(std::vector<uint8_t>& data) {
            auto message_size = data.size();
            
            if (message_size == 0 && message_size > 55) {
                throw std::invalid_argument("Message must be between 1 and 55 bytes long");
            };
            
            data.resize(MESSAGE_BLOCK_SIZE * (WORD_SIZE >> 3), 0);
            data[message_size] = 0x80;
            
            // assume message size is two bytes, big endian
            data[data.size() - 2] = (message_size << 3) >> 8;
            data[data.size() - 1] = (message_size << 3) & 0xFF;
        };
        */
        
        // support 1 block only
        static VariablesArray pad_message(const literalid_t* const message, const variableid_t message_size) {
            const constexpr auto padded_size = MESSAGE_BLOCK_SIZE * WORD_SIZE;
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
            // append the rest with zeros except the last two byses
            std::fill(data + message_size + 1, data + padded_size - 16, literal_t__constant(0));
            // the last 16 bits is the message size in bytes
            uint16_t message_size_ = message_size;
            for (auto i = 0; i < 16; i++) {
                data[padded_size - i - 1] = message_size_ & 1;
                message_size_ >>= 1;
            };

            return padded_message;
        };
        
        /*
        // support 1 block only
        static void encode_message(RefArray<Word> &M, const std::vector<uint8_t>& data) {
            assert(M.size() == MESSAGE_BLOCK_SIZE);
            // 1 byte is needed for the suffix 1 bit, 8 bytes for message size
            auto message_size = data.size();
            assert(message_size > 0 && message_size <= 55);
            
            // set word by word, acounting for big endianness
            WordValue value = 0;
            auto data_index = 0;
            auto byte_index = 0;
            auto word_index = 0;
            while (data_index < message_size) {
                value |= data[data_index++] << ((3 - byte_index) << 3);
                if (byte_index == 3) {
                    M[word_index++] = value;
                    value = 0;
                    byte_index = 0;
                } else {
                    byte_index++;
                };
            };
            
            // append 1 and set the last word
            value |= 0x80 << ((3 - byte_index) << 3);
            M[word_index++] = value;
            
            // set the rest words to 0
            while (word_index < MESSAGE_BLOCK_SIZE - 2) {
                M[word_index++] = WordValue(0);
            };
            
            // set the final 8 bytes to big-endian size in bits
            M[MESSAGE_BLOCK_SIZE - 2] = WordValue(0);
            M[MESSAGE_BLOCK_SIZE - 1] = WordValue(message_size * 8);
        };
        */
    };
}

#endif /* sha_hpp */
