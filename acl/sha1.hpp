//
//  Algebraic Cryptanalysis Library (ACL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
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

namespace acl {;
    
    using namespace ple;
  
    template <class BIT>
    class SHA1: public SHA<SHA1_WORD_SIZE, SHA1_MESSAGE_BLOCK_SIZE, BIT> {
    public:
        using Word = typename SHA<SHA1_WORD_SIZE, SHA1_MESSAGE_BLOCK_SIZE, BIT>::Word;
        
        static const constexpr char* const NAME = "SHA-1";
        static const constexpr uint32_t HASH_SIZE = SHA1_HASH_SIZE;
        static const constexpr uint32_t ROUNDS_NUMBER = SHA1_ROUNDS_NUMBER;
        
    private:
        RefArray<Word> H0 = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
        RefArray<Word> K = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
        
    private:
        const Ref<Word> k(unsigned int t) { return K[t / 20]; };
        
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
        void execute(RefArray<Word> &M, RefArray<Word> &H, Tracer<SHA1_WORD_SIZE, BIT>& tracer,
                     const uint32_t rounds = SHA1_ROUNDS_NUMBER) {
            assert(M.size() == SHA1_MESSAGE_BLOCK_SIZE);
            assert(H.size() == SHA1_HASH_SIZE);
            assert(rounds > 0 && rounds <= SHA1_ROUNDS_NUMBER);
            
            tracer.trace("M", M);
            
            RefArray<Word> W(rounds);
            
            for (unsigned int t = 0; t < _min(SHA1_MESSAGE_BLOCK_SIZE, rounds); t++) {
                W[t] = M[t];
            };
            
            for (unsigned int t = SHA1_MESSAGE_BLOCK_SIZE; t < rounds; t++) {
                RefArray<Word> arg = { W[t-3], W[t-8], W[t-14], W[t-16] };
                W[t] = rotl(eor(arg), 1);
            };
            
            tracer.trace("W", W);
            
            Ref<Word> a = H0[0];
            Ref<Word> b = H0[1];
            Ref<Word> c = H0[2];
            Ref<Word> d = H0[3];
            Ref<Word> e = H0[4];
            
            for (unsigned int t = 0; t < rounds; t++) {
                Ref<Word> ft = f(t, b, c, d);
                tracer.trace("F", ft, t);
                
                RefArray<Word> temp_args = { rotl(a, 5), ft, e, W[t], k(t) };
                Ref<Word> temp = add(temp_args);
                
                e = d;
                d = c;
                c = rotl(b, 30);
                b = a;
                a = temp;
                
                tracer.trace("A", a, t);
            };
            
            H = { H0[0] + a, H0[1] + b, H0[2] + c, H0[3] + d, H0[4] + e };
            tracer.trace("H", H);
        };
    };
};

#endif /* sha1_hpp */

