//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef word_hpp
#define word_hpp

#include <assert.h>
#include "ref.hpp"
#include "entity.hpp"
#include "operations.hpp"
#include "bit.hpp"

namespace ple {

    typedef ArraySize WordSize;
    typedef unsigned int WordValue;
    
    template<WordSize WORD_SIZE, class BIT>
    class Word:
        public virtual RefTarget<Word<WORD_SIZE, BIT>>,
        public virtual StaticRefArray<BIT, WORD_SIZE>,
        public AssignableScalarEntity<Word<WORD_SIZE, BIT>, WordValue>,
        public PropositionalLogicEntity2<Word<WORD_SIZE, BIT>>,
        public ChooseEntity<Word<WORD_SIZE, BIT>>,
        public MajorityEntity<Word<WORD_SIZE, BIT>>,
        public ParityEntity<Word<WORD_SIZE, BIT>>,
        public AriphmeticEntity2<Word<WORD_SIZE, BIT>>,
        public ShiftableEntity<Word<WORD_SIZE, BIT>> {
            
    public:
        using ScalarValue = WordValue;
        static const constexpr WordSize SIZE = WORD_SIZE;

    public:
        // AssignableScalarEntity
        
        // TODO: check if can substitute parent implementation directly
        virtual Word* const assign(const Word* const value) override {
            StaticRefArray<BIT, WORD_SIZE>::assign(value);
            return this;
        };
            
        virtual Word* const assign(const WordValue value) override {
            auto lambda = [this, value](WordSize i) {
                return Ref<BIT>((value >> i) & 0b1);
            };
            StaticRefArray<BIT, WORD_SIZE>::initialize(lambda);
            return this;
        };
            
        // value must be big endian
        // i.e. the first element is the highest bit
        virtual Word* const assign(const literalid_t* const value, const size_t value_size) override {
            assert(value_size == WORD_SIZE && value != nullptr);
            for (ArraySize i = 0; i < WORD_SIZE; i++) {
                (*this)[i].new_instance();
                (*this)[i]->assign(value + WORD_SIZE - 1 - i, 1);
            };
            return this;
        };
            
        virtual Word* const assign(VariableGenerator& generator) override {
            RefArray<BIT>::operator=(generator);
            return this;
        };
        
        virtual WordValue evaluate(const literalid_t* const value,
                                   const size_t value_size) const override {
            WordValue word_value = 0;
            for (int i = WORD_SIZE - 1; i >= 0; i--) {
                assert(this->data()[i] != nullptr);
                // would fail if the bit expression is not a constant
                word_value = (word_value << 1) | this->data()[i]->evaluate(value, value_size);
            }
            return word_value;
        };
            
        virtual const operator WordValue() const override {
            WordValue value = 0;
            for (int i = WORD_SIZE - 1; i >= 0; i--) {
                assert(this->data()[i] != nullptr);
                // would fail if the bit expression is not a constant
                value = (value << 1) | (BitValue)(*(this->data()[i]));
            }
            return value;
        };
            
        virtual const bool is_constant() const override {
            for (int i = WORD_SIZE - 1; i >= 0; i--) {
                if (!this->data()[i]->is_constant()) {
                    return false;
                };
            };
            return true;
        };
            
        // PropositionalLogicEntity
        
        virtual Word* const inv(const Word* const value) override {
            this->execute_elementwise(&BIT::inv, value);
            return this;
        };
        
        virtual Word* const con(const Word* const args[], const ArgsSize args_size) override {
            this->execute_elementwise(&BIT::con, args, args_size);
            return this;
        };
        
        virtual Word* const dis(const Word* const args[], const ArgsSize args_size) override {
            this->execute_elementwise(&BIT::dis, args, args_size);
            return this;
        };
        
        virtual Word* const eor(const Word* const args[], const ArgsSize args_size) override {
            this->execute_elementwise(&BIT::eor, args, args_size);
            return this;
        };
            
        // PropositionalLogicEntity2
            
        virtual Word* const con2(const Word* const x, const Word* const y) override {
            this->execute_elementwise(&BIT::con2, x, y);
            return this;
        };

        virtual Word* const dis2(const Word* const x, const Word* const y) override {
            this->execute_elementwise(&BIT::dis2, x, y);
            return this;
        };
            
        virtual Word* const eor2(const Word* const x, const Word* const y) override {
            this->execute_elementwise(&BIT::eor2, x, y);
            return this;
        };
            
        // ChooseEntity, MajorityEntity, ParityEntity
        
        virtual Word* const ch(const Word* const x, const Word* const y, const Word* const z) override {
            this->execute_elementwise(&BIT::ch, x, y, z);
            return this;
        };

        virtual Word* const maj(const Word* const x, const Word* const y, const Word* const z) override {
            this->execute_elementwise(&BIT::maj, x, y, z);
            return this;
        };
            
        virtual Word* const parity(const Word* const x, const Word* const y, const Word* const z) override {
            this->execute_elementwise(&BIT::parity, x, y, z);
            return this;
        };

        // AriphmeticEntity2
        
        virtual Word* const add2(const Word* const x, const Word* const y) override {
            word_add2(this, x, y);
            return this;
        };
        
        // ShiftableEntity
        
        // assigns elements from the supplied array after shifting it
        // n can be positive or negative, positive means high to low index
        virtual Word* const shr(const Word* const value, const int n) override {
            Ref<BIT> zero_bit_ref = BitValue_0;
            auto lambda = [value, n, zero_bit_ref](WordSize i) {
                if ((0 <= (i + n)) and ((i + n) < WORD_SIZE))
                    return value->data()[i + n];
                else
                    return zero_bit_ref.data();
            };
            StaticRefArray<BIT, WORD_SIZE>::initialize(lambda);
            return this;
        };
        
        virtual Word* const shl(const Word* const value, const int n) override {
            return this->shr(value, -n);
        };
        
        // assigns elements from the supplied array after rotating them
        // n can be positive or negative, positive means high to low index
        virtual Word* const rotr(const Word* const value, const int n) override {
            auto lambda = [value, n](WordSize i) {
                return value->data()[(i + n) % WORD_SIZE];
            };
            StaticRefArray<BIT, WORD_SIZE>::initialize(lambda);
            return this;
        };
        
        virtual Word* const rotl(const Word* const value, const int n) override {
            return this->rotr(value, -n);
        };
            
        // default output - iterate elements
        friend std::ostream& operator << (std::ostream& stream, const Word& value) {
            if (value.is_constant()) {
                return stream << "0x" << std::setfill('0') << std::setw(sizeof(WordValue) * 2) << std::hex << (WordValue)value;
            } else {
                return stream << (StaticRefArray<BIT, WORD_SIZE>)value;
            };
        };
    };
    
    // generic template to avoid static override by specializing
    template<WordSize WORD_SIZE, class BIT>
    inline void word_add2(Word<WORD_SIZE, BIT>* const result, const Word<WORD_SIZE, BIT>* const x, const Word<WORD_SIZE, BIT>* const y) {
        BIT* const * const xref = x->data();
        BIT* const * const yref = y->data();
        const BIT* args[3];
        Ref<BIT> r;
        Ref<BIT> c;
        c.new_instance();
        
        for (auto i = 0; i < WORD_SIZE; i++) {
            swap(r, c);
            
            if (i < WORD_SIZE - 1) {
                if (c.data() == nullptr) { c.new_instance(); };
                c->maj(xref[i], yref[i], r);
            };
            
            args[0] = r;
            args[1] = xref[i];
            args[2] = yref[i];
            r->eor(args, 3);
            // assign at the end because this may reset one of the arguments
            // if the same as x, reuse x[i] for x[i+1]
            if (result == x) {
                swap((*result)[i], r);
            } else {
                (*result)[i] = r;
                r = nullptr;
            };
        };
    };
    
    // value = array
    template<WordSize WORD_SIZE, class BIT>
    inline void assign(RefArray<Word<WORD_SIZE, BIT>>& value, const VariablesArray& array) {
        assert(value.size() * WORD_SIZE == array.size());
        const literalid_t* literals = array.data();
        for (variableid_t i = 0; i < value.size(); i++) {
            value[i].new_instance();
            value[i]->assign(literals, WORD_SIZE);
            literals += WORD_SIZE;
        };
    };

};

#endif /* word_hpp */
