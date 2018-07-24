//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variablesrandom_hpp
#define variablesrandom_hpp

#include "variables.hpp"

namespace ple {
    
    // set random_count variables to random binary constant values
    // variables set to <variable> and <unassigned> can be set to random values
    // variables set to constant values cannot be changed and are preserved
    // assume variables array is already pre-set with the template or by other means
    // returns number of the assigned variables
    // if the total number of variable values within the array is less
    // than random_count, the returned value will be less than random_count
    
    const variables_size_t assign_random(literalid_t* const variables,
                                         const variables_size_t variables_size,
                                         const variables_size_t random_count);
    
};

#endif /* variablesrandom_hpp */
