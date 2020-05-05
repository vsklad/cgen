//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef tracer_hpp
#define tracer_hpp

#include "referenceable.hpp"
#include "variablesarray.hpp"
#include "gf2n.hpp"

//#define TRACER_LOG_COUT

namespace bal {

    // base class for tracing algorithm execution
    // descendants may implement approcach-specific optional trace functionality
    // GF2E is the base class for implementation-specific propositional variable
    // with respect to which the calculation is performed
    template<std::size_t N, class GF2E>
    class Tracer {
    public:
        inline void trace(const char* const name, const GF2NElement<N, GF2E>* const value) {
#ifdef TRACER_LOG_COUT
            std::cout << name << ": " << *value << std::endl;
#endif
        };
        
        inline void trace(const char* const name, const size_t index, const GF2NElement<N, GF2E>* const value) {
#ifdef TRACER_LOG_COUT
            std::cout << name << "[" << index << "]: " << *value << std::endl;
#endif
        };
        
        virtual void trace(const char* const name, const Ref<GF2NElement<N, GF2E>>& value) {
            trace(name, value.data());
        };
        
        virtual void trace(const char* const name, const size_t index, const Ref<GF2NElement<N, GF2E>>& value) {
            trace(name, index, value.data());
        };
        
        template<size_t SIZE>
        friend void trace(Tracer& tracer, const char* const name, Ref<GF2NElement<N, GF2E>>(&value)[SIZE]) {
            for (auto i = 0; i < SIZE; i++) {
                tracer.trace(name, i, value[i]);
            };
        };
    };
    
};

#endif /* tracer_hpp */
