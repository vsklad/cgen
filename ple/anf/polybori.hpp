//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef polybori_hpp
#define polybori_hpp

#include "streamable.hpp"
#include "anf.hpp"
#include "variablesio.hpp"

namespace ple {
    
    // writes a text file which resemples PolyBoRi output
    
    class PolyBoRiStreamWriter: public StreamWriter<Anf> {
    protected:
        void write_header(const Anf& value) {
            stream() << "c variables: " << value.variables_size() << ", equations: " << value.equations_size() << std::endl;
        };
        
        void write_parameters(const Anf& value) {
            const formula_parameters_t parameters = value.get_parameters();
            for (auto it = parameters.begin(); it != parameters.end(); it++) {
                stream() << "c var ." << it->first << " = {" << it->second << "}" << std::endl;
            };
        };
        
        void write_variables(const Anf& value) {
            const formula_named_variables_t& nv = value.get_named_variables();
            for (formula_named_variables_t::const_iterator it = nv.begin(); it != nv.end(); ++it) {
                stream() << "c var " << it->first << " = " << it->second << std::endl;
            };
        };
        
        void write_equations(const Anf& value) {
            for (auto i = 0; i < value.equations_size(); i++) {
                value.print_equation(stream(), i);
            };
        }
        
    public:
        PolyBoRiStreamWriter(std::ostream& stream): StreamWriter<Anf>(stream) {};
        
        virtual void write(const Anf& value) override {
            write_header(value);
            write_parameters(value);
            write_variables(value);
            write_equations(value);
        };
    };
    
}

#endif /* polybori_hpp */
