//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef anfencoderbit_hpp
#define anfencoderbit_hpp

#include "literalbit.hpp"
#include "word.hpp"
#include "anf.hpp"

namespace ple {

    class AnfEncoderBitAllocator;

    class AnfEncoderBit: public LiteralBit<AnfEncoderBit> {
    public:
        friend class AnfEncoderBitAllocator;
        using Allocator = AnfEncoderBitAllocator;
      
    private:
        Anf* anf_ = nullptr;

    public:
        AnfEncoderBit(): AnfEncoderBit(false) {};
        AnfEncoderBit(BitValue value) { assign(value); };
        AnfEncoderBit(const AnfEncoderBit* const value) { assign(value); };
        
        // PropositionalLogicEntity
        
        virtual AnfEncoderBit* const eor(const AnfEncoderBit* const args[], const ArgsSize args_size) override;
        
        // PropositionalLogicEntity2
        
        virtual AnfEncoderBit* const con2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) override;
        virtual AnfEncoderBit* const dis2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) override;
        virtual AnfEncoderBit* const eor2(const AnfEncoderBit* const x, const AnfEncoderBit* const y) override;
        
        // MajorityEntity
        virtual AnfEncoderBit* const maj(const AnfEncoderBit* const x,
                                         const AnfEncoderBit* const y, const AnfEncoderBit* const z) override;
        
        // ChooseEntity
        virtual AnfEncoderBit* const ch(const AnfEncoderBit* const x,
                                        const AnfEncoderBit* const y, const AnfEncoderBit* const z) override;

    public:
        void add2_no_carry(const AnfEncoderBit* const x, const AnfEncoderBit* const y,
                           const AnfEncoderBit* const x_prev, const AnfEncoderBit* const y_prev,
                           const AnfEncoderBit* const r_prev);
    };


    class AnfEncoderBitAllocator: public BitAllocator<AnfEncoderBit> {
    private:
        Anf* anf_;
        
    public:
        virtual AnfEncoderBit* new_instance(Ref<AnfEncoderBit>* ref) override {
            assert(anf_ != nullptr);
            AnfEncoderBit* instance = BitAllocator<AnfEncoderBit>::new_instance(ref);
            assert(instance != nullptr);
            instance->anf_ = anf_;
            return instance;
        };
        
        void set_anf(Anf* value) { anf_ = value; };
    };
    
}

#endif /* anfencoderbit_hpp */
