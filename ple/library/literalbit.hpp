//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef literalbit_hpp
#define literalbit_hpp

#include "bit.hpp"

namespace ple {
  
    // LiteralBit implements common functionality for encoders
    // that introduce auxiliary variables in order to contain the complexity of the target algorithm
    // in particular, when the resulting encoded expression cannot be expressed with a literal
    // the class impements common simplifications for operations of the boolean algebra
    
    template <class T>
    class LiteralBit: public Bit<T> {
    protected:
        // the literal that the instance represents
        literalid_t value_ = literal_t__constant(false);
        
    public:
        const literalid_t literal_id() const { return value_; };
        
    protected:
        // identify and equivalences within arguments and simplify if possible
        
        inline bool reduce_con2(const T* const x, const T* const y) {
            if (x->value_ == y->value_) {
                assign(x);
            } else if (literal_t__is_negation_of(x->value_, y->value_)) {
                assign(BitValue_0);
            } else if (literal_t__is_constant_0(x->value_) || literal_t__is_constant_0(y->value_)) {
                assign(BitValue_0);
            } else if (literal_t__is_constant_1(x->value_)) {
                assign(y);
            } else if (literal_t__is_constant_1(y->value_)) {
                assign(x);
            } else {
                return false;
            };
            return true;
        };
        
        inline bool reduce_dis2(const T* const x, const T* const y) {
            if (x->value_ == y->value_) {
                assign(x);
            } else if (literal_t__is_negation_of(x->value_, y->value_)) {
                assign(BitValue_1);
            } else if (literal_t__is_constant_0(x->value_)) {
                assign(y);
            } else if (literal_t__is_constant_0(y->value_)) {
                assign(x);
            } else if (literal_t__is_constant_1(x->value_) || literal_t__is_constant_1(y->value_)) {
                assign(BitValue_1);
            } else {
                return false;
            };
            return true;
        };
        
        inline bool reduce_maj(const T* const x, const T* const y, const T* const z) {
            if (literal_t__is_constant_0(x->value_)) {
                this->con2(y, z);
            } else if (literal_t__is_constant_0(y->value_)) {
                this->con2(x, z);
            } else if (literal_t__is_constant_0(z->value_)) {
                this->con2(x, y);
            } else if (literal_t__is_constant_1(x->value_)) {
                this->dis2(y, z);
            } else if (literal_t__is_constant_1(y->value_)) {
                this->dis2(x, z);
            } else if (literal_t__is_constant_1(z->value_)) {
                this->dis2(x, y);
            } else if (x->value_ == y->value_ || x->value_ == z->value_) {
                assign(x);
            } else if (y->value_ == z->value_) {
                assign(y);
            } else {
                return false;
            };
            return true;
        };
        
        inline bool reduce_ch(const T* const x, const T* const y, const T* const z) {
            if (literal_t__is_constant_0(x->value_)) {
                assign(z);
            } else if (literal_t__is_constant_1(x->value_) || y->value_ == z->value_) {
                assign(y);
            } else if (literal_t__is_constant(y->value_) && literal_t__is_constant(z->value_)) {
                if (y->value_ == z->value_) {
                    assign(y);
                } else if (literal_t__is_constant_0(y->value_)) {
                    inv(x);
                } else {
                    assign(x);
                };
            } else if (literal_t__is_constant_0(y->value_) || literal_t__is_negation_of(y->value_, x->value_)) {
                this->con2(inv(x), z);
            } else if (y->value_ == x->value_ && literal_t__is_constant_0(z->value_)) {
                assign(x);
            } else if (y->value_ == x->value_ && literal_t__is_constant_1(z->value_)) {
                assign(BitValue_1);
            } else if (literal_t__is_constant_1(y->value_) && literal_t__is_negation_of(z->value_, x->value_)) {
                assign(BitValue_1);
            } else if (literal_t__is_constant_1(y->value_) || y->value_ == x->value_) {
                // x^!x*z = x V z
                this->dis2(x, z);
            } else if (literal_t__is_constant_0(z->value_) || z->value_ == x->value_) {
                this->con2(x, y);
            } else if (literal_t__is_constant_1(z->value_) || literal_t__is_negation_of(z->value_, x->value_)) {
                // x&y ^ !x = !(x&!y) = !x V y
                this->dis2(inv(x), y);
            } else if (literal_t__is_negation_of(z->value_, y->value_)) {
                // !(x^y)
                inv(this->eor2(x, y));
            } else {
                return false;
            };
            return true;
        };
        
    public:
        // AssignableScalarEntity
        
        virtual T* const assign(const T* const value) override {
            value_ = value->value_;
            return (T*)this;
        };
        
        virtual T* const assign(const BitValue value) override {
            value_ = literal_t__constant(value);
            return (T*)this;
        };
        
        virtual T* const assign(const literalid_t* const value, const size_t value_size) override {
            assert(value_size == 1 && value != nullptr);
            value_ = *value;
            return (T*)this;
        };
        
        virtual T* const assign(VariableGenerator& generator) override {
            value_ = generator.new_variable_literal();
            return (T*)this;
        };
        
        virtual const BitValue evaluate(const literalid_t* const value, const size_t value_size) const override {
            if (literal_t__is_constant(value_)) {
                return literal_t__is_constant_1(value_);
            } else {
                assert(value != nullptr);
                assert(literal_t__variable_id(value_) < value_size);
                assert(literal_t__is_constant(value[literal_t__variable_id(value_)]));
                return literal_t__is_constant_1(value[literal_t__variable_id(value_)]);
            };
        };
        
        virtual operator const BitValue() const override {
            assert(is_constant());
            return literal_t__is_constant_1(value_);
        };
        
        virtual const bool is_constant() const override {
            return literal_t__is_constant(value_);
        };
        
        // PropositionalLogicEntity
        
        virtual T* const inv(const T* const value) override {
            // the variable is simply passed through
            value_ = literal_t__negated(value->value_);
            return (T*)this;
        };
        
    public:
        friend bool operator == (const Ref<T>& lhs, const Ref<T>& rhs) {
            return lhs->value_ == rhs->value_;
        };
        
        friend std::ostream& operator << (std::ostream& stream, const T& value) {
            return stream << literal_t(value.value_);
        };
    };
};

#endif /* literalbit_hpp */
