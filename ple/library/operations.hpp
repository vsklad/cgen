//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef entities_hpp
#define entities_hpp

#include "reducible.hpp"
#include "variables.hpp"

namespace ple {

    // Propositional Logic Entity is an abstract class defining propositional logic calculus
    // Parameters:
    //   T - final descendant class
    // Notes:
    //   the result is awlays stored in the instance (this)
    //   methods returning T* result result must always return "this"
    template <class T>
    class PropositionalLogicEntity {
    /*
    private:
        template <typename F, typename... Args>
        inline T* const FunctionA(Args... args) { return this->F(this, args...); };
     
        template <typename F, typename... Args>
        inline T* const FunctionN(Args... args) {
            T_REF result(new T());
            this->F(result.ref(), args...);
            return result;
        };
    */
    public:
        // LOGICAL OPERATIONS
        // inversion
        virtual T* const inv(const T* const value) = 0;
        // conjunction, logical product (and)
        virtual T* const con(const T* const args[], const ArgsSize args_size) = 0;
        // disjunction (or)
        virtual T* const dis(const T* const args[], const ArgsSize args_size) = 0;
        // exclusive or (xor)
        virtual T* const eor(const T* const args[], const ArgsSize args_size) = 0;
    };

    template <class T>
    class AriphmeticEntity {
    public:
        virtual T* const add(const T* const args[], const ArgsSize args_size) = 0;
    };

    template <class T>
    class ShiftableEntity {
    public:
        // SHIFT OPERATIONS
        virtual T* const shr(const T* const value, const int n) = 0;
        virtual T* const shl(const T* const value, const int n) = 0;
        virtual T* const rotr(const T* const value, const int n) = 0;
        virtual T* const rotl(const T* const value, const int n) = 0;
    };

    template <class T>
    class PropositionalLogicEntity2: public virtual PropositionalLogicEntity<T>, protected virtual Reducible<T> {
    public:

        virtual T* const con2(const T* const x, const T* const y) = 0;
        virtual T* const dis2(const T* const x, const T* const y) = 0;
        virtual T* const eor2(const T* const x, const T* const y) = 0;
        
        // default implementation
        
        virtual T* const con(const T* const args[], const ArgsSize args_size) override {
            this->reduce((T*)this, &T::con2, args, args_size);
            return (T*)this;
        };
        virtual T* const dis(const T* const args[], const ArgsSize args_size) override {
            this->reduce((T*)this, &T::dis2, args, args_size);
            return (T*)this;
        };
        virtual T* const eor(const T* const args[], const ArgsSize args_size) override {
            this->reduce((T*)this, &T::eor2, args, args_size);
            return (T*)this;
        };
    };

    template <class T>
    class AriphmeticEntity2: public virtual AriphmeticEntity<T>, protected virtual Reducible<T> {
    public:
        virtual T* const add2(const T* const x, const T* const y) = 0;
        
        virtual T* const add(const T* const args[], const ArgsSize args_size) override {
            this->reduce((T*)this, &T::add2, args, args_size);
            return (T*)this;
        };
    };
    
    template <class T>
    class ChooseEntity {
    public:
        // x&y ^ ~x&z
        // = x&y^(1^x)&z = x&y^x&z^z = x&(y^z)^z
        virtual T* const ch(const T* const x, const T* const y, const T* const z) = 0;
    };
    
    template <class T>
    class MajorityEntity {
    public:
        // x&y ^ x&z ^ y&z
        // = x&(y^z) ^ y&z
        virtual T* const maj(const T* const x, const T* const y, const T* const z) = 0;
    };
    
    template <class T>
    class ParityEntity {
    public:
        // x ^ y ^ z
        virtual T* const parity(const T* const x, const T* const y, const T* const z) = 0;
    };
    
};

#endif /* entities_hpp */
