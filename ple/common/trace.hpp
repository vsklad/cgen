//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef trace_hpp
#define trace_hpp

#include "ref.hpp"
#include "word.hpp"
#include "variablesarray.hpp"

//#define TRACER_LOG_COUT

namespace ple {

    // base class for tracing algorithm execution
    // descendants may implement approcach-specific optional trace functionality
    // V is the base class for implementation-specific propositional variable
    // with respect to which the calculation is performed
    template<class V>
    class Tracer {
    public:
        template<WordSize WORD_SIZE>
        inline void trace(const char* const name, const Word<WORD_SIZE, V>* const value) {
#ifdef TRACER_LOG_COUT
            std::cout << name << ": " << *value << std::endl;
#endif
        };
        
        template<WordSize WORD_SIZE>
        inline void trace(const char* const name, const Word<WORD_SIZE, V>* const value, const ArraySize index) {
#ifdef TRACER_LOG_COUT
            std::cout << name << "[" << index << "]: " << *value << std::endl;
#endif
        };
        
        virtual void trace(const char* const name, const VariablesArray& value) {
#ifdef TRACER_LOG_COUT
            std::cout << name << ": " << value << std::endl;
#endif
        };
        
        virtual void trace(const char* const name, const VariablesArray& value, const variableid_t index) {
#ifdef TRACER_LOG_COUT
            std::cout << name << "[" << index << "]: " << value << std::endl;
#endif
        };
    };
    
    template<class V, WordSize WORD_SIZE>
    inline void trace(Tracer<V>& tracer, const char* const name, const Ref<Word<WORD_SIZE, V>>& value) {
        tracer.trace(name, value.data());
    };
    
    template<class V, WordSize WORD_SIZE>
    inline void trace(Tracer<V>& tracer, const char* const name, const Ref<Word<WORD_SIZE, V>>& value, const ArraySize index) {
        tracer.trace(name, value.data(), index);
    };
    
    template<class V, WordSize WORD_SIZE>
    inline void trace(Tracer<V>& tracer, const char* const name, const RefArray<Word<WORD_SIZE, V>>& value) {
        for (ArraySize i = 0; i < value.size(); i++) {
            trace(tracer, name, value[i], i);
        };
    };
    
};

#endif /* trace_hpp */
