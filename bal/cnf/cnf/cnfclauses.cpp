//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <iostream>
#include <chrono>
#include "cnfclauses.hpp"

namespace bal {
    
    unsigned __find_clause_found = 0;
    unsigned __find_clause_unfound = 0;
    unsigned __compare_clauses_ = 0;
    unsigned __append_clause_ = 0;
    std::chrono::time_point<std::chrono::system_clock> __time_start_;
    
    // Cnf
    
    void __statistics_reset() {
        __find_clause_found = 0;
        __find_clause_unfound = 0;
        __compare_clauses_ = 0;
        __append_clause_ = 0;
        __time_start_ = std::chrono::system_clock::now();
    };
    
    void __statistics_print() {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - __time_start_);
        std::cout << "Statistics: append: " << __append_clause_ << ", find: " << __find_clause_found << "/" << __find_clause_unfound << ", compare: " << __compare_clauses_ << ", " << duration.count() << " ms" << std::endl;
    };
    
    bool __print_clause(const uint32_t* const p_clause) {
        std::cout << (_clause_is_included(p_clause) ? "i" : "e") << ": ";
        print_clause(std::cout, p_clause, "; ");
        std::cout << std::endl;
        return true;
    };
    
    void __print_conflict(const literalid_t variables[], const uint32_t* const p_clause) {
        std::cout << "CONFLICT" << std::endl;
        std::cout << "Clause(s):" << std::endl;
        __print_clause(p_clause);
        std::cout << "Variable(s):" << std::endl;
        for (auto i = 0; i < _clause_size(p_clause); i++) {
            const literalid_t literal_id = _clause_literal(p_clause, i);
            const variableid_t variable_id = literal_t__variable_id(literal_id);
            const literalid_t value_id = literal_t::resolve(variables, literal_id);
            std::cout << literal_t(variable_t__literal_id(variable_id)) << " = " << literal_t(value_id) << std::endl;
        };
    };

};
