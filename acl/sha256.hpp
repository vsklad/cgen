//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef sha256_hpp
#define sha256_hpp

#include "operators.hpp"
#include "tracer.hpp"
#include "sha.hpp"

#define SHA256_WORD_SIZE 32
#define SHA256_HASH_SIZE 8
#define SHA256_MESSAGE_BLOCK_SIZE 16
#define SHA256_ROUNDS_NUMBER 64

namespace acl {
    
    using namespace bal;

    template <class BIT>
    class SHA256: public SHA<SHA256_WORD_SIZE, SHA256_MESSAGE_BLOCK_SIZE, BIT> {
    public:
        using Word = typename SHA<SHA256_WORD_SIZE, SHA256_MESSAGE_BLOCK_SIZE, BIT>::Word;
        
        static constexpr const char* const NAME = "SHA-256";
        static constexpr uint32_t HASH_SIZE = SHA256_HASH_SIZE;
        static constexpr uint32_t ROUNDS_NUMBER = SHA256_ROUNDS_NUMBER;
        
    public:
        Ref<Word> sl0(const Ref<Word> &x) {
            Ref<Word> args[] = { rotr(x, 2), rotr(x, 13), rotr(x, 22) };
            return eor(args);
        };
        
        Ref<Word> sl1(const Ref<Word> &x) {
            Ref<Word> args[] = { rotr(x, 6), rotr(x, 11), rotr(x, 25) };
            return eor(args);
        };
        
        Ref<Word> ss0(const Ref<Word> &x) {
            Ref<Word> args[] = { rotr(x, 7), rotr(x, 18), shr(x, 3) };
            return eor(args);
        };
        
        Ref<Word> ss1(const Ref<Word> &x) {
            Ref<Word> args[] = { rotr(x, 17), rotr(x, 19), shr(x, 10) };
            return eor(args);
        };
        
    public:
        void execute(Ref<Word> (&M)[SHA256_MESSAGE_BLOCK_SIZE],
                     Ref<Word> (&H)[SHA256_HASH_SIZE],
                     Tracer<SHA256_WORD_SIZE, BIT>& tracer,
                     const uint32_t rounds = ROUNDS_NUMBER) {
            assert(rounds > 0 && rounds <= SHA256_ROUNDS_NUMBER);
            
            const Ref<Word> H0[] = {
                0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
            };
            
            const Ref<Word> K[SHA256_ROUNDS_NUMBER] = {
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
                0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
                0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
                0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
            };
            
            trace(tracer, "M", M);
            trace(tracer, "W", M);
            
            Ref<Word> W[rounds];
            
            for (unsigned int i = 0; i < _min(SHA256_MESSAGE_BLOCK_SIZE, rounds); i++) {
                W[i] = M[i];
            };
            
            for (unsigned int i = SHA256_MESSAGE_BLOCK_SIZE; i < rounds; i++) {
                Ref<Word> args[] = { ss1(W[i-2]), W[i-7], ss0(W[i-15]), W[i-16] };
                W[i] = add(args);
                tracer.trace("W", i, W[i]);
            };
            
            Ref<Word> a = H0[0];
            Ref<Word> b = H0[1];
            Ref<Word> c = H0[2];
            Ref<Word> d = H0[3];
            Ref<Word> e = H0[4];
            Ref<Word> f = H0[5];
            Ref<Word> g = H0[6];
            Ref<Word> h = H0[7];
            
            for (unsigned int i = 0; i < rounds; i++) {
                Ref<Word> args[] = { h, sl1(e), ch(e, f, g), K[i], W[i] };
                Ref<Word> t1 = add(args);
                Ref<Word> t2 = sl0(a) + maj(a, b, c);
                
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
                
                tracer.trace("A", i, a);
            };
            
            H[0] = a + H0[0];
            H[1] = b + H0[1];
            H[2] = c + H0[2];
            H[3] = d + H0[3];
            H[4] = e + H0[4];
            H[5] = f + H0[5];
            H[6] = g + H0[6];
            H[7] = h + H0[7];
            
            trace(tracer, "H", H);
        };
    };
};

#endif /* sha256_hpp */
