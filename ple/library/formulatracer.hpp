//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef wordformulatracer_hpp
#define wordformulatracer_hpp

#include "tracer.hpp"
#include "formula.hpp"

namespace ple {
    
    // records named variables
    template<WordSize WORD_SIZE, class BIT>
    class FormulaTracer: public Tracer<WORD_SIZE, BIT> {
    private:
        Formula& formula_;
        
    public:
        FormulaTracer(Formula& formula): formula_(formula) {};
        
        Formula& formula() { return formula_; }
        
        virtual void trace(const char* const name, const VariablesArray& value) override {
            Tracer<WORD_SIZE, BIT>::trace(name, value);
            formula_.add_named_variable(name, value);
        };
        
        virtual void trace(const char* const name, const VariablesArray& value, const variableid_t index) override {
            Tracer<WORD_SIZE, BIT>::trace(name, value, index);
            formula_.add_named_variable(name, value, index);
        };
        
        virtual void trace(const char* const name, const Ref<Word<WORD_SIZE, BIT>>& value) override {
            Tracer<WORD_SIZE, BIT>::trace(name, value);
            VariablesArray array(WORD_SIZE);
            Word2VariablesArray(value, array);
            trace(name, array);
        };
        
        virtual void trace(const char* const name, const Ref<Word<WORD_SIZE, BIT>>& value, const ArraySize index) override {
            Tracer<WORD_SIZE, BIT>::trace(name, value, index);
            VariablesArray array(WORD_SIZE);
            Word2VariablesArray(value, array);
            trace(name, array, (variableid_t)index);
        };
        
        virtual void trace(const char* const name, const RefArray<Word<WORD_SIZE, BIT>>& value) override {
            Tracer<WORD_SIZE, BIT>::trace(name, value);
            VariablesArray array((variableid_t)value.size(), WORD_SIZE);
            Words2VariablesArray(value, array);
            trace(name, array);
        };
    };
};

#endif /* wordformulatracer_hpp */
