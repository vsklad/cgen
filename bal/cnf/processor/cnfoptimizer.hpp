//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfoptimizer_hpp
#define cnfoptimizer_hpp

#include "variablesio.hpp"
#include "cnfsubsumption.hpp"

#ifdef CNF_TRACE
#include "cnftracer.hpp"
#endif

namespace bal {
    
    // goes through all clauses sequentially
    // each clause is validated and assigned a flag whether its included into result, or not
    // satisfied clauses are excluded
    // changed clauses are appended to the end with originals excluded as well
    // therefore, optimization proceeds like a wave and stops
    // when all clauses (inclusive of newly generated ones) are validated
    class CnfOptimizer: public CnfSubsumptionOptimizer {
    protected:
        VariablesArray& variables_;
        
        uint64_t evaluations_;
        uint64_t evaluations_aggregated_;
        uint64_t variables_assigned_;
         
    private:
        inline void exclude_clause(const container_offset_t offset);
        
        // normalization
        template<bool track_changes = false>
        inline processor_result_t normalize_ca(const uint32_t* const p_src, uint32_t* const p_dst) const;
        inline processor_result_t normalize_cu(const uint32_t* const p_src, uint32_t* const p_dst) const;
        
        // evaluation
        inline processor_result_t evaluate_clause_a(uint32_t* const p_clause,
                                                     container_offset_t offset = CONTAINER_END,
                                                     const container_offset_t original_offset = CONTAINER_END);
        inline processor_result_t evaluate_clause(const uint32_t offset);
        
        // unit propagation / unary clauses - assignment
        inline processor_result_t assign_literal_value(const literalid_t literal_id, const literalid_t value);
        
        // resolution
        template<bool b_ca_master, clause_size_t ca_size, clause_size_t ca_index, clause_size_t c2_index>
        inline processor_result_t resolve_ca_c2(uint32_t* const p_ca, uint32_t* const p_c2, const clause_flags_t ca_flags);
        // subsume ca2 by ca1
        template<clause_size_t size1, clause_size_t size2, clause_size_t index1, clause_size_t index2, clause_size_t index3 = size2>
        inline processor_result_t subsume_ca(uint32_t* const p_ca1, const container_offset_t ca2_offset);
        
        // merge ca with shorter clauses; subsume and update/insert all shorter ones if possible
        template<clause_size_t size1, clause_size_t size2, clause_size_t index1, clause_size_t index2, clause_size_t index3 = size2, bool b_lookup = true>
        inline processor_result_t merge_ca(uint32_t* const p_ca1, container_offset_t& ca1_offset, uint32_t* const p_ca2, clause_flags_t& ca2_flags);
        template<clause_size_t size>
        inline processor_result_t merge_ca(uint32_t* const p_clause);
        
        // transitive closure
        inline processor_result_t resolve_c2(uint32_t* const p_clause);
        
    private:
        // for all assigned variables_, append CNF unit clauses for constants
        // and equality clauses for variables
        inline bool assign_variable_values_unoptimized();
        
    protected:
        // reindex variables such that there are no gaps in variable numbers
        // update CNF variables count
        // returns new variables_size
        inline variableid_t update_variables(const bool b_reindex_variables);
        
        processor_result_t evaluate_clauses();
        inline bool base_execute(const bool b_reindex_variables, const FormulaProcessingMode mode);

    public:
        // processing
        // these methods are executed using process_clauses template
        inline processor_result_t process_clause_evaluate(uint32_t* const p_clause);
        
        inline bool _normalize_clause(uint32_t* const p_clause) const;
        inline bool _update_clause_variables(uint32_t* const p_clause) const;
        
    public:
        CnfOptimizer(Cnf& cnf, VariablesArray& variables): CnfSubsumptionOptimizer(cnf), variables_(variables) {
            _assert_level_0(variables.size() == cnf.variables_size());
        };
        
        // optimizes the given Cnf without any parameters
        bool execute() override;
        virtual bool execute(const bool b_reindex_variables, const FormulaProcessingMode mode);
    };
    
    // evaluates the formula given some of the variable values
    // those unknown must be set to self
    // variables array must match the formula
    class CnfVariableEvaluator: public CnfOptimizer {
    public:
        CnfVariableEvaluator(Cnf& cnf, VariablesArray& variables): CnfOptimizer(cnf, variables) {};
        bool execute() override;
    };
    
    class CnfVariableNormalizer: public CnfOptimizer {
    public:
        CnfVariableNormalizer(Cnf& cnf, VariablesArray& variables): CnfOptimizer(cnf, variables) {};
        
        virtual bool execute(const bool b_reindex_variables);
    };
    
    inline bool evaluate(Cnf& cnf, VariablesArray& variables) {
        return CnfVariableEvaluator(cnf, variables).execute();
    };
    
    inline bool process(Cnf& cnf, VariablesArray& variables,
                        const bool b_reindex_variables, const FormulaProcessingMode mode) {
        return bal::CnfOptimizer(cnf, variables).execute(b_reindex_variables, mode);
    };
    
    inline bool normalize_variables(Cnf& cnf, const bool b_reindex_variables) {
        VariablesArray variables(cnf.variables_size(), 1);
        variables.assign_sequence();
        return CnfVariableNormalizer(cnf, variables).execute(b_reindex_variables);
    };
    
#ifdef CNF_TRACE
    
    void set_cnf_tracer(Ref<CnfTracer> p_tracer);
    
#endif
    
};

#endif /* cnfoptimizer_hpp */
