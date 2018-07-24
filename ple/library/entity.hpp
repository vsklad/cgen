//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef entity_hpp
#define entity_hpp

#include "variables.hpp"
#include "variablesarray.hpp"

namespace ple {
    
    // Primitive is an abstract base class for PLE primitives e.g. bits and words
    // defines basic assignment, encoding and deconing methods
    // Parameters:
    //   T - final descendant class
    //   T_SCALAR - scalar value type, e.g. bool for bits
    template <class T, typename T_SCALAR>
    class AssignableScalarEntity {
    public:
        virtual T* const assign(const T* const value) = 0;
        virtual T* const assign(const T& value) { return this->assign(&value); };
        virtual T* const assign(const T_SCALAR value) = 0;
        virtual T* const assign(const literalid_t* const value, const size_t value_size) = 0;
        virtual T* const assign(VariableGenerator& generator) = 0;
        
        // evaluates T with given parameters
        virtual const T_SCALAR evaluate(const literalid_t* const value, const size_t value_size) const = 0;
        // decodes T into a scalar value
        // must work if and only if is_constant() is true
        virtual operator const T_SCALAR() const = 0;
        // indicates a cnstant, i.e. a scalar value
        virtual const bool is_constant() const = 0;
    };
    
};

#endif /* entity_h */
