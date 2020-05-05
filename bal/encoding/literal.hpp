//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef literal_hpp
#define literal_hpp

#include "variablesarray.hpp"
#include "referenceable.hpp"
#include "gf2.hpp"
#include "gf2n.hpp"

// conditional preprocessing for encoding add and xor
//   order of arguments before splitting into batches
//   if not specified, maintain order specified by the caller
#define XOR_ARGS_ORDER_ASCENDING
//#define XOR_ARGS_ORDER_DESCENDING

namespace bal {

    // Literal implements common functionality for encoders of formulas: GF(2)^N -> GF(2)
    // which introduce auxiliary variables in order to contain the complexity of the formula
    // in particular, when the resulting encoded expression cannot be expressed with a literal
    // the class impements common patterns for operations of the boolean algebra
    
    // Literal is "immutable" i.e. once assigned the instance cannot be changed;
    // note that some of the operations take non-constant arguments which may be returned as result
    // integrity is maintained however through Literal's immutability
    
    template <class FORMULA_T>
    class Literal: public GF2Element<Literal<FORMULA_T>> {
    public:
        using Formula = FORMULA_T;
        
    public:
        // the formula that the literal exists in context of
        FORMULA_T* const formula;
        // the literal that the instance represents
        const literalid_t value;
        
    private:
        static const Ref<Literal> unassigned_;
        
    public:
        Literal(const literalid_t value_): formula(nullptr), value(value_) {
            _assert_level_0(!literal_t__is_variable(value_));
        };
        
        Literal(FORMULA_T* const formula_, const literalid_t value_): formula(formula_), value(value_) {
            _assert_level_0(literal_t__is_variable(value_) == (formula_ != nullptr));
        };
        
        Literal* assign(const Literal* const value) override {
            _assert_level_1(false);
        };
        
        void assign(const bool value) override {
            _assert_level_1(false);
        };
        
        operator const bool() const override {
            _assert_level_1(literal_t__is_constant(value));
            return literal_t__is_constant_1(value);
        };
        
        bool is_constant() const override {
            return literal_t__is_constant(value);
        };
        
        template<typename value_t, typename std::enable_if<std::is_integral<value_t>::value, int>::type = 0>
        static Literal* static_assign(Literal* instance, const value_t& value) {
            if (value == LITERALID_UNASSIGNED) {
                return unassigned_;
            } else {
                return GF2Element<Literal<FORMULA_T>>::static_assign(instance, value);
            };
        };
        
        friend std::ostream& operator << (std::ostream& stream, const Literal& value) {
            return stream << literal_t(value.value);
        };
        
        friend void update_formula(FORMULA_T*& formula, const Literal* const instance) {
            _assert_level_0(instance != nullptr);
            if (instance->formula != nullptr) {
                _assert_level_1(formula == nullptr || formula == instance->formula);
                formula = instance->formula;
            };
        };
        
        static Ref<Literal> from_literal(FORMULA_T* const formula, const literalid_t value) {
            if (literal_t__is_variable(value)) {
                return new Literal(formula, value);
            } else {
                return value;
            };
        };
    };
    
    template <class FORMULA_T>
    const Ref<Literal<FORMULA_T>> Literal<FORMULA_T>::unassigned_ = new Literal<FORMULA_T>(LITERALID_UNASSIGNED);
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> inv(Literal<FORMULA_T>* const r,
                                const Literal<FORMULA_T>* const x) {
        if (literal_t__is_variable(x->value)) {
            return new Literal<FORMULA_T>(x->formula, literal_t__negated(x->value));
        } else if (literal_t__is_constant(x->value)) {
            return literal_t__negated(x->value);
        } else if (literal_t__is_unassigned(x->value)) {
            return x->value;
        } else {
            _assert_level_0(false);
        };
    };
    
    // build optimized list of xor operands
    //   2 instances of the same literal is always 0 and should be removed
    //   a variable and its complement is always 1
    // return the reduced args list and the constant;
    // assume the constant has the initial value;
    // assume optimized_args has length of at least args_size
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> eor(Literal<FORMULA_T>* const r,
                                const Literal<FORMULA_T>* const args[], const std::size_t args_size) {
        literalid_t constant = 0;
        literalid_t optimized_args[args_size];
        std::size_t optimized_args_size = 0;
        FORMULA_T* formula = nullptr;
        
        for (auto i = 0; i < args_size; i++) {
            if (literal_t__is_constant(args[i]->value)) {
                constant ^= args[i]->value;
            } else {
                // eliminate if the variables are negated or duplicated
                // build a sorted list because:
                // INCONCLUSIVE: lower indexes first - order matters
                // grouping of lower indexes reduces length of variables chain (?)
                for (auto j = 0; j <= optimized_args_size; j++) {
                    if (j == optimized_args_size) {
                        // at the end, no duplicates, append to lagrs
                        update_formula(formula, args[i]);
                        optimized_args[j] = args[i]->value;
                        optimized_args_size++;
                        break;
                    } else if (literal_t__is_same_variable(args[i]->value, optimized_args[j])) {
                        if ((literal_t__is_negation_of(args[i]->value, optimized_args[j]))) {
                            // negated means always 1
                            constant ^= 1;
                        };
                        // remove from variable_args
                        for (auto k = j; k < optimized_args_size - 1; k++) {
                            optimized_args[k] = optimized_args[k + 1];
                        };
                        optimized_args_size--;
                        break;
#if defined(XOR_ARGS_ORDER_ASCENDING) || defined(XOR_ARGS_ORDER_DESCENDING)
                    } else if (literal_t__variable_id(args[i]->value)
#if defined(XOR_ARGS_ORDER_ASCENDING)
                               <
#elif defined(XOR_ARGS_ORDER_DESCENDING)
                               >
#endif
                               literal_t__variable_id(optimized_args[j])) {
                        // insertion point found
                        for (auto k = optimized_args_size; k > j; k--) {
                            optimized_args[k] = optimized_args[k - 1];
                        };
                        update_formula(formula, args[i]);
                        optimized_args[j] = args[i]->value;
                        optimized_args_size++;
                        break;
#endif
                    };
                };
            };
        };
        
        return Literal<FORMULA_T>::from_literal(formula, eor(formula, optimized_args, optimized_args_size, constant));
    };
    
    // assume args are optimised: constants aggregated, no duplicates or negations, sorted
    // splits eor into batches
    // note: this implementation overwrites args
    template<class FORMULA_T>
    literalid_t eor(FORMULA_T* const formula, literalid_t args[], const std::size_t args_size, const literalid_t constant) {
        _assert_level_0(literal_t__is_constant(constant));
        if (args_size == 0) {
            return constant;
        } else {
            literalid_t result = LITERALID_UNASSIGNED;
            _assert_level_0(formula != nullptr);
            
            // split a single xor into smaller ones if requested
            const std::size_t batch_size = formula->get_xor_max_args();
            std::size_t vargs_start_idx = 0;
            
            while (true) {
                const std::size_t vargs_size = batch_size > args_size - vargs_start_idx ? args_size - vargs_start_idx : batch_size;
                
                if (vargs_size > 1) {
                    _assert_level_0(formula != nullptr);
                    result = formula->new_variable_literal();
                    eor(formula, result, args + vargs_start_idx, vargs_size);
                } else if (vargs_size == 1) {
                    result = args[vargs_start_idx];
                };
                
                vargs_start_idx += vargs_size;
                if (vargs_start_idx < args_size) {
                    vargs_start_idx--;
                    args[vargs_start_idx] = result;
                } else {
                    break;
                };
            };
            
            // invert the result if the sum of the constants is 1, i.e. xor(x, 1) <-> not(x)
            return literal_t__negated_onlyif(result, literal_t__is_constant_1(constant));;
        };
    };
    
    // encode a batch of variables into the formula
    // r is the variable literal for the result
    // assume args are optimised: no constants, duplicates or negations, sorted
    template<class FORMULA_T>
    void eor(FORMULA_T* const formula, const literalid_t r, const literalid_t args[], const std::size_t args_size) {
        static_assert(not_implemented<FORMULA_T>::value, "not implemented");
    };
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> con2(Literal<FORMULA_T>* const r,
                                 Literal<FORMULA_T>* const x, Literal<FORMULA_T>* const y) {
        if (x->value == y->value) {
            return x;
        } else if (literal_t__is_negation_of(x->value, y->value)) {
            return 0;
        } else if (literal_t__is_constant_0(x->value) || literal_t__is_constant_0(y->value)) {
            return 0;
        } else if (literal_t__is_constant_1(x->value)) {
            return y;
        } else if (literal_t__is_constant_1(y->value)) {
            return x;
        } else {
            _assert_level_1(x->formula != nullptr && x->formula == y->formula);
            const literalid_t r_value = x->formula->new_variable_literal();
            con2(x->formula, r_value, x->value, y->value);
            return new Literal<FORMULA_T>(x->formula, r_value);
        };
    };
    
    template<class FORMULA_T>
    void con2(FORMULA_T* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        static_assert(not_implemented<FORMULA_T>::value, "not implemented");
    };
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> dis2(Literal<FORMULA_T>* const r,
                                 Literal<FORMULA_T>* const x, Literal<FORMULA_T>* const y) {
        if (x->value == y->value) {
            return x;
        } else if (literal_t__is_negation_of(x->value, y->value)) {
            return 1;
        } else if (literal_t__is_constant_0(x->value)) {
            return y;
        } else if (literal_t__is_constant_0(y->value)) {
            return x;
        } else if (literal_t__is_constant_1(x->value) || literal_t__is_constant_1(y->value)) {
            return 1;
        } else {
            _assert_level_1(x->formula != nullptr && x->formula == y->formula);
            const literalid_t r_value = x->formula->new_variable_literal();
            dis2(x->formula, r_value, x->value, y->value);
            return new Literal<FORMULA_T>(x->formula, r_value);
        };
    };
    
    template<class FORMULA_T>
    void dis2(FORMULA_T* const formula, const literalid_t r, const literalid_t x, const literalid_t y) {
        static_assert(not_implemented<FORMULA_T>::value, "not implemented");
    };
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> eor2(Literal<FORMULA_T>* const r,
                                 const Literal<FORMULA_T>* const x,
                                 const Literal<FORMULA_T>* const y) {
        const Literal<FORMULA_T>* const args[2] = {x, y};
        return eor(r, args, 2);
    };
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> maj(Literal<FORMULA_T>* const r,
                                Literal<FORMULA_T>* const x, Literal<FORMULA_T>* const y, Literal<FORMULA_T>* const z) {
        if (literal_t__is_constant_0(x->value)) {
            return con2(r, y, z);
        } else if (literal_t__is_constant_0(y->value)) {
            return con2(r, x, z);
        } else if (literal_t__is_constant_0(z->value)) {
            return con2(r, x, y);
        } else if (literal_t__is_constant_1(x->value)) {
            return dis2(r, y, z);
        } else if (literal_t__is_constant_1(y->value)) {
            return dis2(r, x, z);
        } else if (literal_t__is_constant_1(z->value)) {
            return dis2(r, x, y);
        } else if (x->value == y->value || x->value == z->value) {
            return x;
        } else if (y->value == z->value) {
            return y;
        } else {
            _assert_level_1(x->formula != nullptr && x->formula == y->formula && x->formula == z->formula);
            const literalid_t r_value = x->formula->new_variable_literal();
            maj(x->formula, r_value, x->value, y->value, z->value);
            return new Literal<FORMULA_T>(x->formula, r_value);
        };
    };
    
    template<class FORMULA_T>
    void maj(FORMULA_T* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        static_assert(not_implemented<FORMULA_T>::value, "not implemented");
    };
    
    template<class FORMULA_T>
    Ref<Literal<FORMULA_T>> ch(Literal<FORMULA_T>* const r,
                               Literal<FORMULA_T>* const x, Literal<FORMULA_T>* const y, Literal<FORMULA_T>* const z) {
        if (literal_t__is_constant_0(x->value)) {
            return z;
        } else if (literal_t__is_constant_1(x->value) || y->value == z->value) {
            return y;
        } else if (literal_t__is_constant(y->value) && literal_t__is_constant(z->value)) {
            if (literal_t__is_constant_0(y->value)) {
                return inv(r, x);
            } else {
                return x;
            };
        } else if (literal_t__is_constant_0(y->value) || literal_t__is_negation_of(y->value, x->value)) {
            // x ? 0b0 : z -> -x & z
            // x ? -x : z -> -x & z
            Ref<Literal<FORMULA_T>> inv_x = inv(r, x);
            return con2(inv_x.data(), inv_x.data(), z);
        } else if (y->value == x->value && literal_t__is_constant_0(z->value)) {
            return x;
        } else if (y->value == x->value && literal_t__is_constant_1(z->value)) {
            return 1;
        } else if (literal_t__is_constant_1(y->value) && literal_t__is_negation_of(z->value, x->value)) {
            return 1;
        } else if (literal_t__is_constant_1(y->value) || y->value == x->value) {
            // x ? 0b1 : z -> x | z
            // x ? x : z -> x | z
            return dis2(r, x, z);
        } else if (literal_t__is_constant_0(z->value) || z->value == x->value) {
            return con2(r, x, y);
        } else if (literal_t__is_constant_1(z->value) || literal_t__is_negation_of(z->value, x->value)) {
            // x&y ^ !x = !(x&!y) = !x V y
            Ref<Literal<FORMULA_T>> inv_x = inv(r, x);
            return dis2(inv_x.data(), inv_x.data(), y);
        } else if (literal_t__is_negation_of(z->value, y->value)) {
            // x ? y : -y -> xy | -x-y
            // !(x^y)
            Ref<Literal<FORMULA_T>> eor_xy = eor2(r, x, y);
            return inv(eor_xy.data(), eor_xy.data());
        } else {
            _assert_level_1(x->formula != nullptr && x->formula == y->formula && x->formula == z->formula);
            const literalid_t r_value = x->formula->new_variable_literal();
            ch(x->formula, r_value, x->value, y->value, z->value);
            return new Literal<FORMULA_T>(x->formula, r_value);
        };
    };
    
    template<class FORMULA_T>
    void ch(FORMULA_T* const formula, const literalid_t r, const literalid_t x, const literalid_t y, const literalid_t z) {
        static_assert(not_implemented<FORMULA_T>::value, "not implemented");
    };
    
    // specialized templates for Word<Literal>
    // conversion into literals array
    // words stored to the array in big endian format, i.e. with most significant bit first
    // at the same time, other arrays come sequentially, with aray index going from low to high
    
    // the templates are activated for classes with ::Formula type defined, i.e. Literal and its descendants
    
    template<std::size_t N, class GF2E,
            typename std::enable_if<std::is_base_of<Literal<typename GF2E::Formula>, GF2E>::value, int>::type = 0>
    inline VariablesArray variables_array(const Ref<GF2NElement<N, GF2E>>& value) {
        VariablesArray result(1, N);
        literalid_t* const literals = result.data();
        const GF2E* const * const p_word_bits = value->data();
        for (auto j = 0; j < N; j++) {
            _assert_level_1(p_word_bits[N - j - 1] != nullptr);
            literals[j] = p_word_bits[N - j - 1]->value;
        };
        
        return result;
    };
    
    // note the assignment of bits in reverse order
    template<std::size_t N, class GF2E, size_t LHS_SIZE,
            typename std::enable_if<std::is_base_of<Literal<typename GF2E::Formula>, GF2E>::value, int>::type = 0>
    inline void assign(Ref<GF2NElement<N, GF2E>> (&lhs) [LHS_SIZE], typename GF2E::Formula& formula, const VariablesArray& rhs) {
        _assert_level_0(LHS_SIZE * N == rhs.size());
        const literalid_t* literals = rhs.data();
        for (auto i = 0; i < LHS_SIZE; i++) {
            lhs[i] = new GF2NElement<N, GF2E>();
            for (auto j = 0; j < N; j++) {
                (*lhs[i])[j] = GF2E::from_literal(&formula, literals[N - 1 - j]);
            };
            literals += N;
        };
    };
};

#endif /* literal_hpp */
