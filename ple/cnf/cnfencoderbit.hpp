//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfencoderbit_hpp
#define cnfencoderbit_hpp

#include "bit.hpp"
#include "cnf.hpp"

namespace ple {

    class CnfEncoderBitAllocator;
    
    class CnfEncoderBit: public Bit<CnfEncoderBit> {
    public:
        friend class CnfEncoderBitAllocator;
        using Allocator = CnfEncoderBitAllocator;
        
    private:
        // cnf is only required when the value is a variable and needs to be encoded
        // variable initialization takes care of this
        // i.e. it is not required for constants
        Cnf* cnf_ = nullptr;
        literal_t value_ = literal_t::constant(false);
        
    private:
        static const constexpr uint32_t MAX_XOR_SIZE = 32;
        
        inline const ArgsSize get_eor_batch_size(ArgsSize args_size) const {
            assert(cnf_ != nullptr);
            return cnf_->get_xor_max_args() < 2 ? 2 :
                cnf_->get_xor_max_args() < MAX_XOR_SIZE ? cnf_->get_xor_max_args() : MAX_XOR_SIZE;
        };
        
        // assume all arguments are variables
        // assume size at least two arguments
        // assume this can be one of the arguments
        inline void eor_variables(const CnfEncoderBit* const args[], const ArgsSize args_size) {
            // more than one variable, generate 2^(N-1) clauses
            assert(cnf_ != nullptr);
            assert(args_size > 1 && args_size <= MAX_XOR_SIZE);
            literalid_t literalid_args[args_size + 1];
            
            literal_t f = cnf_->new_variable_literal();
            for (uint32_t i = 0; i < (1 << args_size); i++) {
                // i bits can be used to determine the negation and the result
                BitValue result_flag = BitValue_0;
                for (ArgsSize j = 0; j < args_size; j++) {
                    literalid_args[j] = args[j]->value_.negated(i & (0x1 << j));
                    result_flag = (i & (0x1 << j)) ? not result_flag : result_flag;
                };
                
                literalid_args[args_size] =  f.negated(result_flag != BitValue_1);
                cnf_->append_clause(literalid_args, args_size + 1);
            };
            value_ = f;
        };
        
    public:
        CnfEncoderBit(): CnfEncoderBit(false) {};
        CnfEncoderBit(BitValue value) { assign(value); };
        CnfEncoderBit(const CnfEncoderBit* const value) { assign(value); };
        
        // the below methods are for read-only access only
        // all manipulations must happen within the class implementation
        const Cnf* cnf() const { return cnf_; };
        const literal_t& literal() const { return value_; };
        
        // AssignableScalarEntity
        
        virtual CnfEncoderBit* const assign(const CnfEncoderBit* const value) override {
            value_ = value->value_;
            return this;
        };
        
        virtual CnfEncoderBit* const assign(const BitValue value) override {
            value_ = literal_t::constant(value);
            return this;
        };
        
        virtual CnfEncoderBit* const assign(const literalid_t* const value,
                                            const size_t value_size) override {
            assert(value_size == 1 && value != nullptr);
            value_ = *value;
            return this;
        };

        virtual CnfEncoderBit* const assign(VariableGenerator& generator) override {
            value_ = literal_t(variable_t(generator.new_variable()));
            return this;
        };
        
        virtual BitValue evaluate(const literalid_t* const value,
                                  const size_t value_size) const override {
            if (value_.is_constant()) {
                return value_.is_constant_1();
            } else {
                assert(value != nullptr);
                assert(value_.variable_id() < value_size);
                assert(literal_t__is_constant(value[value_.variable_id()]));
                return literal_t__is_constant_1(value[value_.variable_id()]);
            };
        };
        
        virtual operator BitValue() const override {
            assert(is_constant());
            return value_.is_constant_1();
        };
        
        virtual const bool is_constant() const override {
            return value_.is_constant();
        };
        
        // PropositionalLogicEntity
        
        virtual CnfEncoderBit* const inv(const CnfEncoderBit* const value) override {
            // nothing to encode, the variable is passed through
            value_ = value->value_.negated();
            return this;
        };
        
        // optimize to exclude all additional variables for any number of parameters
        // i.e. expand to 2^(N-1) clauses, N literals each (N = args_size+1)
        // optimize out constants if any
        // TODO: check args[i] == args[j] (appears not to occur normally)
        virtual CnfEncoderBit* const eor(const CnfEncoderBit* const args[], const ArgsSize args_size) override {
            // eliminate constants
            BitValue constant = BitValue_0;
            ArgsSize variable_args_size = 0;
            const CnfEncoderBit* variable_args[args_size];
            
            for (ArgsSize i = 0; i < args_size; i++) {
                if (args[i]->is_constant()) {
                    constant ^= (BitValue)(*args[i]);
                } else {
                    bool is_optimized_away = false;
                    // eliminate if the variables are negated or duplicated
                    for (ArgsSize j = 0; j < variable_args_size; j++) {
                        if (args[i]->value_.variable_id() == variable_args[j]->value_.variable_id()) {
                            if ((args[i]->value_.is_negation_of(variable_args[j]->value_))) {
                                // negated means always 1
                                constant ^= BitValue(1);
                            };
                            // remove from variable_args
                            for (ArgsSize k = j; k < variable_args_size - 1; k++) {
                                variable_args[k] = variable_args[k + 1];
                            };
                            variable_args_size--;
                            is_optimized_away = true;
                            break;
                        };
                    };
                    if (!is_optimized_away) {
                        variable_args[variable_args_size] = args[i];
                        variable_args_size++;
                    };
                };
            };
            
            // proceed with clauses
            if (variable_args_size == 0) {
                assign(BitValue_0);
            } else {
                // split a single xor into smaller ones if necessary or requested
                // max batch size can be increased but unnecessary in practice
                const ArgsSize batch_size = get_eor_batch_size(variable_args_size);
                ArgsSize vargs_start_idx = 0;
                
                while (true) {
                    const ArgsSize vargs_size = batch_size > variable_args_size - vargs_start_idx ? variable_args_size - vargs_start_idx : batch_size;
                    
                    if (vargs_size > 1) {
                        eor_variables(&variable_args[vargs_start_idx], vargs_size);
                    } else if (vargs_size == 1) {
                        assign(variable_args[vargs_start_idx]);
                    };
                    
                    vargs_start_idx += vargs_size;
                    if (vargs_start_idx < variable_args_size) {
                        vargs_start_idx--;
                        variable_args[vargs_start_idx] = this;
                    } else {
                        break;
                    };
                };
            };
            
            // invert the result if the sum of the constants is 1, i.e. xor(x, 1) <-> not(x)
            if (constant == BitValue_1) {
                inv(this);
            };
            
            return this;
        };
        
        // PropositionalLogicEntity2
        
        virtual CnfEncoderBit* const con2(const CnfEncoderBit* const x, const CnfEncoderBit* const y) override {
            if (x->value_ == y->value_) {
                assign(x);
            } else if (x->value_.is_negation_of(y->value_)) {
                assign(BitValue_0);
            } else if (x->value_.is_constant_0() || y->value_.is_constant_0()) {
                assign(BitValue_0);
            } else if (x->value_.is_constant_1()) {
                assign(y);
            } else if (y->value_.is_constant_1()) {
                assign(x);
            } else {
                // assign new variable and encode con2
                // note that one of the operands may = this
                assert(cnf_ != nullptr);
                literal_t value = cnf_->new_variable_literal();
                cnf_->append_clause_l(x->value_.negated(), y->value_.negated(), value.id());
                cnf_->append_clause_l(x->value_.id(), value.negated());
                cnf_->append_clause_l(y->value_.id(), value.negated());
                value_ = value;
            };
            return this;
        };
        
        virtual CnfEncoderBit* const dis2(const CnfEncoderBit* const x, const CnfEncoderBit* const y) override {
            if (x->value_ == y->value_) {
                assign(x);
            } else if (x->value_.is_negation_of(y->value_)) {
                assign(BitValue_1);
            } else if (x->value_.is_constant_0()) {
                assign(y);
            } else if (y->value_.is_constant_0()) {
                assign(x);
            } else if (x->value_.is_constant_1() || y->value_.is_constant_1()) {
                assign(BitValue_1);
            } else {
                // assign new variable and encode dis2
                // note that one of the operands may = this
                assert(cnf_ != nullptr);
                literal_t value = cnf_->new_variable_literal();
                cnf_->append_clause_l(x->value_.id(), y->value_.id(), value.negated());
                cnf_->append_clause_l(x->value_.negated(), value.id());
                cnf_->append_clause_l(y->value_.negated(), value.id());
                value_ = value;
            };
            return this;
        };
        
        virtual CnfEncoderBit* const eor2(const CnfEncoderBit* const x, const CnfEncoderBit* const y) override {
            const CnfEncoderBit* const args[] = {x, y};
            return eor(args, 2);
        };
        
        // x&y ^ ~x&z
        // (¬f ∨ ¬x ∨ y) ∧ (¬f ∨ x ∨ z) ∧ (¬f ∨ y ∨ z) ∧ (f ∨ ¬x ∨ ¬y) ∧
        // (f ∨ x ∨ ¬z) ∧ (f ∨ ¬y ∨ ¬z)
        virtual CnfEncoderBit* const ch(const CnfEncoderBit* const x, const CnfEncoderBit* const y, const CnfEncoderBit* const z) override {
            if (x->value_.is_constant_0()) {
                assign(z);
            } else if (x->value_.is_constant_1() || y->value_ == z->value_) {
                assign(y);
            } else if (y->value_.is_constant() && z->value_.is_constant()) {
                if (y->value_ == z->value_) {
                    assign(y);
                } else if (y->value_.is_constant_0()) {
                    inv(x);
                } else {
                    assign(x);
                }
            } else if (y->value_.is_constant_0() || y->value_.is_negation_of(x->value_)) {
                con2(inv(x), z);
            } else if (y->value_ == x->value_ && z->value_.is_constant_0()) {
                assign(x);
            } else if (y->value_ == x->value_ && z->value_.is_constant_1()) {
                assign(BitValue_1);
            } else if (y->value_.is_constant_1() && z->value_.is_negation_of(x->value_)) {
                assign(BitValue_1);
            } else if (y->value_.is_constant_1() || y->value_ == x->value_) {
                // x^!x*z = x V z
                dis2(x, z);
            } else if (z->value_.is_constant_0() || z->value_ == x->value_) {
                con2(x, y);
            } else if (z->value_.is_constant_1() || z->value_.is_negation_of(x->value_)) {
                // x&y ^ !x = !(x&!y) = !x V y
                dis2(inv(x), y);
            } else if (z->value_.is_negation_of(y->value_)) {
                // !(x^y)
                inv(eor2(x, y));
            } else {
                assert(cnf_ != nullptr);
                literal_t f = cnf_->new_variable_literal();
                cnf_->append_clause_l(x->value_.negated(), y->value_.id(), f.negated());
                cnf_->append_clause_l(x->value_.id(), z->value_.id(), f.negated());
                cnf_->append_clause_l(y->value_.id(), z->value_.id(), f.negated());
                cnf_->append_clause_l(x->value_.negated(), y->value_.negated(), f.id());
                cnf_->append_clause_l(x->value_.id(), z->value_.negated(), f.id());
                cnf_->append_clause_l(y->value_.negated(), z->value_.negated(), f.id());
                value_ = f;
            };
            return this;
        };
        
        // xy ∨ xz ∨ yz
        // (¬f ∨ x ∨ y) ∧ (¬f ∨ x ∨ z) ∧ (¬f ∨ y ∨ z) ∧
        // (f ∨ ¬x ∨ ¬y) ∧ (f ∨ ¬x ∨ ¬z) ∧ (f ∨ ¬y ∨ ¬z)
        virtual CnfEncoderBit* const maj(const CnfEncoderBit* const x, const CnfEncoderBit* const y, const CnfEncoderBit* const z) override {
            
            if (x->value_.is_constant_0()) {
                return con2(y, z);
            } else if (y->value_.is_constant_0()) {
                return con2(x, z);
            } else if (z->value_.is_constant_0()) {
                return con2(x, y);
            } else if (x->value_.is_constant_1()) {
                return dis2(y, z);
            } else if (y->value_.is_constant_1()) {
                return dis2(x, z);
            } else if (z->value_.is_constant_1()) {
                return dis2(x, y);
            } else if (x->value_ == y->value_ || x->value_ == z->value_) {
                return assign(x);
            } else if (y->value_ == z->value_) {
                return assign(y);
            } else {
                assert(cnf_ != nullptr);
                literal_t f = cnf_->new_variable_literal();
                cnf_->append_clause_l(x->value_.id(), y->value_.id(), f.negated());
                cnf_->append_clause_l(x->value_.id(), z->value_.id(), f.negated());
                cnf_->append_clause_l(y->value_.id(), z->value_.id(), f.negated());
                cnf_->append_clause_l(x->value_.negated(), y->value_.negated(), f.id());
                cnf_->append_clause_l(x->value_.negated(), z->value_.negated(), f.id());
                cnf_->append_clause_l(y->value_.negated(), z->value_.negated(), f.id());
                value_ = f;
                return this;
            };
        };
        
        static void record_clauses(const char* const * const map, const size_t map_size, CnfEncoderBit* const * const args, const size_t input_size, const size_t output_size) {
            assert(map_size > 0 && input_size > 0 && output_size > 0);
            
            // locate CNF
            Cnf* cnf = args[0]->cnf_;
            assert(cnf != nullptr);
            
            // create output literals
            literal_t output_literals[output_size];
            for (auto i = 0; i < output_size; i++) {
                output_literals[i] = cnf->new_variable_literal();
            };
            
            // buffer for clause literals
            literalid_t literalid_args[input_size + output_size];
            
            // record clauses one by one from the map
            for(auto i = 0; i < map_size; i++) {
                assert(strlen(map[i]) == input_size + output_size);
                
                size_t clause_size = 0;
                
                // record input literals
                for (auto j = 0; j < input_size; j++) {
                    if (map[i][j] == '0' || map[i][j] == '1') {
                        literalid_args[clause_size++] = args[j]->value_.negated(map[i][j] == '0');
                    };
                };
                
                // record output literals
                for (auto j = input_size; j < input_size + output_size; j++) {
                    if (map[i][j] == '0' || map[i][j] == '1') {
                        literalid_args[clause_size++] = output_literals[j - input_size].negated(map[i][j] == '0');
                    };
                };
                
                cnf->append_clause(literalid_args, clause_size);
            };
            
            // assign output variables
            for (auto i = 0; i < output_size; i++) {
                args[input_size + i]->value_ = output_literals[i];
            };
        };
        
        friend bool operator == (const Ref<CnfEncoderBit>& lhs, const Ref<CnfEncoderBit>& rhs) {
            return lhs->value_ == rhs->value_;
        };
        
        friend std::ostream& operator << (std::ostream& stream, const CnfEncoderBit& value) {
            return stream << value.value_;
        };
    };
    
    class CnfEncoderBitAllocator: public BitAllocator<CnfEncoderBit> {
    private:
        Cnf* cnf_;
        
    public:
        virtual CnfEncoderBit* new_instance(Ref<CnfEncoderBit>* ref) override {
            assert(cnf_ != nullptr);
            CnfEncoderBit* instance = BitAllocator<CnfEncoderBit>::new_instance(ref);
            assert(instance != nullptr);
            instance->cnf_ = cnf_;
            return instance;
        };
        
        void set_cnf(Cnf* value) { cnf_ = value; }
    };
    
};

#endif /* cnfencoderbit_hpp */
