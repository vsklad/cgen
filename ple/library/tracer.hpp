//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef tracer_hpp
#define tracer_hpp

#include "ref.hpp"
#include "word.hpp"
#include "variablesarray.hpp"

//#define TRACER_LOG_COUT

namespace ple {

    // base class for tracing algorithm execution
    // descendants may implement approcach-specific optional trace functionality
    // BIT is the base class for implementation-specific propositional variable
    // with respect to which the calculation is performed
    template<WordSize WORD_SIZE, class BIT>
    class Tracer {
    public:
        inline void trace(const char* const name, const Word<WORD_SIZE, BIT>* const value) {
#ifdef TRACER_LOG_COUT
            std::cout << name << ": " << *value << std::endl;
#endif
        };
        
        inline void trace(const char* const name, const Word<WORD_SIZE, BIT>* const value, const ArraySize index) {
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
        
        virtual void trace(const char* const name, const Ref<Word<WORD_SIZE, BIT>>& value) {
            trace(name, value.data());
        };
        
        virtual void trace(const char* const name, const Ref<Word<WORD_SIZE, BIT>>& value, const ArraySize index) {
            trace(name, value.data(), index);
        };
        
        virtual void trace(const char* const name, const RefArray<Word<WORD_SIZE, BIT>>& value) {
            for (ArraySize i = 0; i < value.size(); i++) {
                trace(name, value[i], i);
            };
        };
    };
    
};

#endif /* tracer_hpp */
