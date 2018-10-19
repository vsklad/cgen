//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfencoderbit_hpp
#define cnfencoderbit_hpp

#include <cstring>
#include "literalbit.hpp"
#include "cnf.hpp"

namespace ple {

    class CnfEncoderBitAllocator;
    
    class CnfEncoderBit: public LiteralBit<CnfEncoderBit> {
    public:
        friend class CnfEncoderBitAllocator;
        using Allocator = CnfEncoderBitAllocator;
        
    private:
        // cnf is only required when the value is a variable and needs to be encoded
        // variable initialization takes care of this
        // i.e. it is not required for constants
        Cnf* cnf_ = nullptr;
        
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
            
            literalid_t f = cnf_->variable_generator().new_variable_literal();
            for (uint32_t i = 0; i < (1 << args_size); i++) {
                // i bits can be used to determine the negation and the result
                BitValue result_flag = BitValue_0;
                for (ArgsSize j = 0; j < args_size; j++) {
                    literalid_args[j] = literal_t__negated_onlyif(args[j]->value_, i & (0x1 << j));
                    result_flag = (i & (0x1 << j)) ? not result_flag : result_flag;
                };
                literalid_args[args_size] =  literal_t__negated_onlyif(f, result_flag != BitValue_1);
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
        
    public:
        // PropositionalLogicEntity2
        
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
                        if (literal_t__is_same_variable(args[i]->value_, variable_args[j]->value_)) {
                            if ((literal_t__is_negation_of(args[i]->value_, variable_args[j]->value_))) {
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
            if (!reduce_con2(x, y)) {
                // assign new variable and encode con2
                // note that one of the operands may = this
                assert(cnf_ != nullptr);
                literalid_t value = cnf_->variable_generator().new_variable_literal();
                cnf_->append_clause_l(literal_t__negated(x->value_), literal_t__negated(y->value_), value);
                cnf_->append_clause_l(x->value_, literal_t__negated(value));
                cnf_->append_clause_l(y->value_, literal_t__negated(value));
                value_ = value;
            };
            return this;
        };
        
        virtual CnfEncoderBit* const dis2(const CnfEncoderBit* const x, const CnfEncoderBit* const y) override {
            if (!reduce_dis2(x, y)) {
                // assign new variable and encode dis2
                // note that one of the operands may = this
                assert(cnf_ != nullptr);
                literalid_t value = cnf_->variable_generator().new_variable_literal();
                cnf_->append_clause_l(x->value_, y->value_, literal_t__negated(value));
                cnf_->append_clause_l(literal_t__negated(x->value_), value);
                cnf_->append_clause_l(literal_t__negated(y->value_), value);
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
            if (!reduce_ch(x, y, z)) {
                assert(cnf_ != nullptr);
                literalid_t f = cnf_->variable_generator().new_variable_literal();
                cnf_->append_clause_l(literal_t__negated(x->value_), y->value_, literal_t__negated(f));
                cnf_->append_clause_l(x->value_, z->value_, literal_t__negated(f));
                cnf_->append_clause_l(literal_t__negated(x->value_), literal_t__negated(y->value_), f);
                cnf_->append_clause_l(x->value_, literal_t__negated(z->value_), f);
                value_ = f;
            };
            return this;
        };
        
        // xy ∨ xz ∨ yz
        // (¬f ∨ x ∨ y) ∧ (¬f ∨ x ∨ z) ∧ (¬f ∨ y ∨ z) ∧
        // (f ∨ ¬x ∨ ¬y) ∧ (f ∨ ¬x ∨ ¬z) ∧ (f ∨ ¬y ∨ ¬z)
        virtual CnfEncoderBit* const maj(const CnfEncoderBit* const x, const CnfEncoderBit* const y, const CnfEncoderBit* const z) override {
            if (!reduce_maj(x, y, z)) {
                assert(cnf_ != nullptr);
                literalid_t f = cnf_->variable_generator().new_variable_literal();
                cnf_->append_clause_l(x->value_, y->value_, literal_t__negated(f));
                cnf_->append_clause_l(x->value_, z->value_, literal_t__negated(f));
                cnf_->append_clause_l(y->value_, z->value_, literal_t__negated(f));
                cnf_->append_clause_l(literal_t__negated(x->value_), literal_t__negated(y->value_), f);
                cnf_->append_clause_l(literal_t__negated(x->value_), literal_t__negated(z->value_), f);
                cnf_->append_clause_l(literal_t__negated(y->value_), literal_t__negated(z->value_), f);
                value_ = f;
            };
            return this;
        };
        
        static void record_clauses(const char* const * const map, const size_t map_size, CnfEncoderBit* const * const args, const size_t input_size, const size_t output_size) {
            assert(map_size > 0 && input_size > 0 && output_size > 0);
            
            // locate CNF
            Cnf* cnf = args[0]->cnf_;
            assert(cnf != nullptr);
            
            // create output literals
            literalid_t output_literals[output_size];
            for (auto i = 0; i < output_size; i++) {
                output_literals[i] = cnf->variable_generator().new_variable_literal();
            };
            
            // buffer for clause literals
            literalid_t literalid_args[input_size + output_size];
            
            // record clauses one by one from the map
            for(auto i = 0; i < map_size; i++) {
                assert(std::strlen(map[i]) == input_size + output_size);
                
                size_t clause_size = 0;
                
                // record input literals
                for (auto j = 0; j < input_size; j++) {
                    if (map[i][j] == '0' || map[i][j] == '1') {
                        literalid_args[clause_size++] = literal_t__negated_onlyif(args[j]->value_, map[i][j] == '0');
                    };
                };
                
                // record output literals
                for (auto j = input_size; j < input_size + output_size; j++) {
                    if (map[i][j] == '0' || map[i][j] == '1') {
                        literalid_args[clause_size++] = literal_t__negated_onlyif(output_literals[j - input_size], map[i][j] == '0');
                    };
                };
                
                cnf->append_clause(literalid_args, clause_size);
            };
            
            // assign output variables
            for (auto i = 0; i < output_size; i++) {
                args[input_size + i]->value_ = output_literals[i];
            };
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
