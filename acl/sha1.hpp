//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2023 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef sha1_hpp
#define sha1_hpp

#include "operators.hpp"
#include "tracer.hpp"
#include "sha.hpp"

#define SHA1_WORD_SIZE 32
#define SHA1_HASH_SIZE 5
#define SHA1_MESSAGE_BLOCK_SIZE 16
#define SHA1_ROUNDS_NUMBER 80

namespace acl {
    
    using namespace bal;
  
    template <class BIT>
    class SHA1: public SHA<SHA1_WORD_SIZE, SHA1_MESSAGE_BLOCK_SIZE, BIT> {
    public:
        using Word = typename SHA<SHA1_WORD_SIZE, SHA1_MESSAGE_BLOCK_SIZE, BIT>::Word;
        
        static constexpr const char* const NAME = "SHA-1";
        static constexpr uint32_t HASH_SIZE = SHA1_HASH_SIZE;
        static constexpr uint32_t ROUNDS_NUMBER = SHA1_ROUNDS_NUMBER;
        
    private:
        const Ref<Word> f(unsigned int t, Ref<Word> &b, Ref<Word> &c, Ref<Word> &d) {
            switch (t / 20) {
                case 0: return ch(b, c, d);
                case 1: return parity(b, c, d);
                case 2: return maj(b, c, d);
                case 3: return parity(b, c, d);
                default:
                    assert(false);
            };
        };
        
    public:
        void execute(Ref<Word> *M,
                     const std::size_t M_size, // size in words
                     Ref<Word> (&H)[SHA1_HASH_SIZE],
                     Tracer<SHA1_WORD_SIZE, BIT>& tracer,
                     const uint32_t rounds = SHA1_ROUNDS_NUMBER) {
            
            assert(M_size > 0 && (M_size % SHA1_MESSAGE_BLOCK_SIZE) == 0);
            assert(rounds > 0 && rounds <= SHA1_ROUNDS_NUMBER);
            
            const Ref<Word> K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
            
            H[0] = 0x67452301;
            H[1] = 0xEFCDAB89;
            H[2] = 0x98BADCFE;
            H[3] = 0x10325476;
            H[4] = 0xC3D2E1F0;
            
            for (std::size_t block_index = 0; block_index < M_size / SHA1_MESSAGE_BLOCK_SIZE; block_index++) {
                Ref<Word> W[rounds];
                
                for (unsigned int t = 0; t < _min(SHA1_MESSAGE_BLOCK_SIZE, rounds); t++) {
                    W[t] = M[t];
                    tracer.trace("M", block_index * SHA1_MESSAGE_BLOCK_SIZE + t, M[t]);
                    tracer.trace("W", block_index * SHA1_MESSAGE_BLOCK_SIZE + t, M[t]);
                };
                
                for (unsigned int t = SHA1_MESSAGE_BLOCK_SIZE; t < rounds; t++) {
                    Ref<Word> eor_args[] = { W[t-3], W[t-8], W[t-14], W[t-16] };
                    W[t] = rotl(eor(eor_args), 1);
                    tracer.trace("W", t, W[t]);
                };
                
                Ref<Word> a = H[0];
                Ref<Word> b = H[1];
                Ref<Word> c = H[2];
                Ref<Word> d = H[3];
                Ref<Word> e = H[4];
                
                for (unsigned int t = 0; t < rounds; t++) {
                    Ref<Word> ft = f(t, b, c, d);
                    tracer.trace("F", block_index * SHA1_MESSAGE_BLOCK_SIZE + t, ft);
                    
                    Ref<Word> temp_args[] = { rotl(a, 5), ft, e, W[t], K[t / 20] };
                    Ref<Word> temp = add(temp_args);
                    
                    e = d;
                    d = c;
                    c = rotl(b, 30);
                    b = a;
                    a = temp;
                    
                    tracer.trace("A", block_index * SHA1_MESSAGE_BLOCK_SIZE + t, a);
                };
                
                H[0] = H[0] + a;
                H[1] = H[1] + b;
                H[2] = H[2] + c;
                H[3] = H[3] + d;
                H[4] = H[4] + e;
                
                M += SHA1_MESSAGE_BLOCK_SIZE;
            };
            
            trace(tracer, "H", H);
        };
    };
};

#endif /* sha1_hpp */

