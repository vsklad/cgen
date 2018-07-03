//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnftracer_hpp
#define cnftracer_hpp

#include "trace.hpp"
#include "cnfencoderbit.hpp"
#include "cnfword.hpp"

namespace ple {
    
    // records named variables
    class CnfTracer: public Tracer<CnfEncoderBit> {
    private:
        Cnf& cnf_;
        
    public:
        CnfTracer(Cnf& cnf): cnf_(cnf) {};
        
        Cnf& cnf() { return cnf_; }
        
        
        virtual void trace(const char* const name, const VariablesArray& value) {
            Tracer<CnfEncoderBit>::trace(name, value);
            cnf_.add_named_variable(name, value);
        };
        
        virtual void trace(const char* const name, const VariablesArray& value, const variableid_t index) {
            Tracer<CnfEncoderBit>::trace(name, value, index);
            cnf_.add_named_variable(name, value, index);
        };
    };
    
    // specialized templates for CnfEncoderBit
    // to achieve optimised representation & adding of named variables
    
    template<WordSize WORD_SIZE>
    inline void trace(Tracer<CnfEncoderBit>& tracer, const char* const name, const Ref<Word<WORD_SIZE, CnfEncoderBit>>& value) {
        VariablesArray array(WORD_SIZE);
        Word2VariablesArray(value, array);
        tracer.trace(name, array);
    };
    
    template<WordSize WORD_SIZE>
    inline void trace(Tracer<CnfEncoderBit>& tracer, const char* const name, const Ref<Word<WORD_SIZE, CnfEncoderBit>>& value, const ArraySize index) {
        VariablesArray array(WORD_SIZE);
        Word2VariablesArray(value, array);
        tracer.trace(name, array, (variableid_t)index);
    };
};

#endif /* cnftracer_hpp */
