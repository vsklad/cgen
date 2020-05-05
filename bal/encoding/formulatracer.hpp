//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef wordformulatracer_hpp
#define wordformulatracer_hpp

#include "tracer.hpp"
#include "formula.hpp"

namespace bal {
    
    // records named variables
    template<std::size_t N, class GF2E>
    class FormulaTracer: public Tracer<N, GF2E> {
    private:
        Formula& formula_;
        
    public:
        FormulaTracer(Formula& formula): formula_(formula) {};
        
        Formula& formula() { return formula_; }
        
        void trace(const char* const name, const Ref<GF2NElement<N, GF2E>>& value) override {
            Tracer<N, GF2E>::trace(name, value);
            VariablesArray array = variables_array(value);
#ifdef TRACER_LOG_COUT
            std::cout << name << ": " << array << std::endl;
#endif
            formula_.add_named_variable(name, array);
        };
        
        void trace(const char* const name, const size_t index, const Ref<GF2NElement<N, GF2E>>& value) override {
            Tracer<N, GF2E>::trace(name, index, value);
            VariablesArray array = variables_array(value);
#ifdef TRACER_LOG_COUT
            std::cout << name << "[" << index << "]: " << array << std::endl;
#endif
            formula_.add_named_variable(name, array, (variables_size_t)index);
        };
    };
};

#endif /* wordformulatracer_hpp */
