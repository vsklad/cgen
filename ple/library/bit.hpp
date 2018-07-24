//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef bit_hpp
#define bit_hpp

#include "referenceable.hpp"
#include "entity.hpp"
#include "operations.hpp"
#include "ref.hpp"

namespace ple {

    typedef bool BitValue;
    constexpr BitValue BitValue_0 = 0b0;
    constexpr BitValue BitValue_1 = 0b1;

    template<class T> class BitAllocator;

    template <class T>
    class Bit:
        public virtual RefTarget<T>,
        public AssignableScalarEntity<T, BitValue>,
        public PropositionalLogicEntity2<T>,
        public ChooseEntity<T>, public MajorityEntity<T>, public ParityEntity<T> {

    public:
        using ScalarValue = BitValue;
        using Allocator = BitAllocator<T>;
            
    public:
        // default implementation
            
        // x&(y^z)^z
        virtual T* const ch(const T* const x, const T* const y, const T* const z) override {
            Ref<T> yz;
            yz.new_instance();
            yz->eor2(y, z);
            this->con2(x, yz);
            this->eor2((T*)this, z);
            return (T*)this;
        };

        // x&(y^z) ^ y&z
        virtual T* const maj(const T* const x, const T* const y, const T* const z) override {
            Ref<T> yz;
            yz.new_instance();
            yz->eor2(y, z);
            this->con2(x, yz);
            yz->con2(y, z);
            this->eor2((T*)this, yz);
            return (T*)this;
        };
        
        // x ^ y ^ z
        virtual T* const parity(const T* const x, const T* const y, const T* const z) override {
            const T* eor_args[3] = { x, y, z };
            this->eor(eor_args, 3);
            return (T*)this;
        };
    };
    
    template<class T>
    class BitAllocator: public RefTargetAllocator<T> {
    private:
        Ref<T> const_[2];
    public:
        BitAllocator() {
            this->new_instance(&const_[0])->assign(BitValue_0);
            this->new_instance(&const_[1])->assign(BitValue_1);
        };
        
        virtual T* new_constant(Ref<T>* ref, const typename T::ScalarValue value) override {
            *ref = const_[value];
            return ref->data();
        };
    };
};

#endif /* bit_hpp */
