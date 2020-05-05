//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnftracer_h
#define cnftracer_h

#include "referenceable.hpp"
#include "variables.hpp"
#include "cnf.hpp"

namespace bal {

    class CnfTracer: public Referenceable {
    public:
        typedef unsigned level_t;
        
    public:
        virtual ~CnfTracer() {};
        
        virtual void start(const Cnf& cnf) {};
        virtual void finish() {};
        
        virtual void level_next() {};
        virtual void level_prev() {};
        
        // invoked every time variable value changes
        // can be either a constant assignment or an equivalence assignment to a literal
        virtual void assign_variable(const variableid_t variable_id, const literalid_t value) {};
        // invoked once at the end of processing the formula
        // array is the map containing new variable value for each variable
        // these values can be either constants or new literals
        // reflecting both equivalence assignments and normalization of variable numbers
        virtual void update_variables(const VariablesArray& variables) {};
        
        // invoked when starting processing the clause
        // p_clause may not remain valid until leveling up
        // or until anoher process_clause call at the current level
        // offset is specified if available; p_clause may be a different memory instance however
        virtual void process_clause(const uint32_t* const p_clause,
                                    const container_offset_t offset,
                                    const bool is_resolvent) {};
        
        // invoked during clause normalization for the current clause
        // for each changed literal when changing it
        virtual void assign_literal(const literalid_t original_value,
                                    const literalid_t assigned_value) {};
        
        // invoked when p_clause is added to the formula
        virtual void append_clause(const uint32_t* const p_clause) {};
        
        // invoked after p_clause is merged with the formula clause identified by offset
        // p_clause is already merged
        virtual void merge_clause(const uint32_t* const p_clause,
                                  const container_offset_t offset) {};
        
        // assigned offset means the clause is removed from the formula
        // otherwise a derivative clause is ignored without adding o the formula
        virtual void remove_clause(const container_offset_t offset) {};
    };
    
};

#endif /* cnftracer_h */
