//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <random>
#include "variablesrandom.hpp"

namespace ple {

    
    const variables_size_t assign_random(literalid_t* const variables,
                                         const variables_size_t variables_size,
                                         const variables_size_t random_count) {

        std::random_device rd;
        std::uniform_int_distribution<ple::variableid_t> index_distribution(0, variables_size - 1);
        std::uniform_int_distribution<unsigned int> value_distribution(0, 1);

        variables_size_t assigned_count = 0;
        while (assigned_count < random_count) {
            ple::variableid_t random_index = index_distribution(rd);
            unsigned int random_value = value_distribution(rd);
            
            // determine the index to set since the random_index may be a constant already
            // choose the first index greater than the random_index if so;
            // continue from 0 if reach the end of the array
            
            if (literal_t__is_constant(variables[random_index])) {
                ple::variableid_t original_index = random_index - 1;
                while (literal_t__is_constant(variables[random_index]) && random_index < variables_size) {
                    random_index++;
                };
                
                if (random_index == variables_size) {
                    random_index = 0;
                    while (literal_t__is_constant(variables[random_index]) && random_index < original_index) {
                        random_index++;
                    };
                    
                    if (literal_t__is_constant(variables[random_index])) {
                        // not enough variables to set
                        return assigned_count;
                    };
                };
            };
            
            variables[random_index] = literal_t__constant(random_value);
            assigned_count++;
        };
        
        return assigned_count;
    };
    
};
