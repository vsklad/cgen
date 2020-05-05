//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef polybori_hpp
#define polybori_hpp

#include "streamable.hpp"
#include "anf.hpp"
#include "variablesio.hpp"

namespace bal {
    
    // reads a text file produced by PolyBoRiStreamWriter
    
    class PolyBoRiStreamReader:
    private virtual TextStreamReader<Anf>,
    private virtual VariableTextReader {
        
    private:
        void read_parameter(Anf& anf, const std::string key) {
            const std::string name = read_literal();
            skip_space();
            read_symbol(':');
            skip_space();
            if (is_token(ttDec)) {
                anf.add_parameter(key, name, read_uint32());
            } else {
                anf.add_parameter(key, name, read_quoted());
            };
        };
        
        void read_parameters(Anf& anf) {
            const std::string key = read_literal();
            skip_space();
            read_symbol('=');
            skip_space();
            read_symbol('{');
            skip_space();
            
            read_parameter(anf, key);
            skip_space();
            while (is_symbol(',')) {
                read_symbol(',');
                skip_space();
                read_parameter(anf, key);
                skip_space();
            };
            
            read_symbol('}');
            skip_space();
            read_eol();
        };
        
        void read_named_variable(Anf& anf) {
            std::string name = read_literal();
            skip_space();
            read_symbol('=');
            skip_space();
            VariablesArray value = read_variable_value();
            skip_space();
            read_eol();
            anf.add_named_variable(name.data(), value);
        };
        
    public:
        PolyBoRiStreamReader(std::istream& stream): TextStreamReader<Anf>(stream) {};
        
        void read(Anf& value) override {
            value.initialize();
            
            std::vector<literalid_t> literals;
            
            while (!is_eof()) {
                if (is_symbol('c')) {
                    skip_symbol();
                    skip_space();
                    if (is_token("var")) {
                        skip_token();
                        skip_space();
                        if (is_symbol('.')) {
                            skip_symbol();
                            read_parameters(value);
                        } else if (is_token(ttLiteral)) {
                            read_named_variable(value);
                        } else {
                            skip_line();
                        };
                    } else {
                        skip_line();
                    };
                } else {
                    value.append_equation();
                    while (!is_eol()) {
                        // read term by term
                        literals.clear();
                        while (!is_eol()) {
                            skip_space();
                            if (is_symbol('1')) {
                                read_symbol();
                                literals.push_back(1);
                            } else if (is_symbol('0')) {
                                read_symbol();
                                literals.push_back(0);
                            } else {
                                read_symbol('x');
                                literals.push_back(variable_t__literal_id(variable_t__from_uint(read_uint32())));
                            };
                            skip_space();
                            if (is_symbol('*')) {
                                skip_symbol();
                            } else {
                                break;
                            };
                        };
                        _assert_level_1(literals.size() > 0);
                        value.append_equation_term(literals.data(), literals.size());
                        if (is_symbol('+')) {
                            skip_symbol();
                        } else {
                            break;
                        };
                    };
                    value.complete_equation(0, false);
                    read_eol();
                };
            };
            read_eof();
        };
    };
    
    // writes a text file which resemples PolyBoRi output
    
    class PolyBoRiStreamWriter: public StreamWriter<Anf> {
    protected:
        void write_header(const Anf& value) {
            stream << "c variables: " << value.variables_size() << ", equations: " << value.equations_size() << std::endl;
        };
        
        void write_parameters(const Anf& value) {
            const formula_parameters_t parameters = value.get_parameters();
            for (auto it = parameters.begin(); it != parameters.end(); it++) {
                stream << "c var ." << it->first << " = {" << it->second << "}" << std::endl;
            };
        };
        
        void write_variables(const Anf& value) {
            const formula_named_variables_t& nv = value.get_named_variables();
            for (formula_named_variables_t::const_iterator it = nv.begin(); it != nv.end(); ++it) {
                stream << "c var " << it->first << " = " << it->second << std::endl;
            };
        };
        
        void write_equations(const Anf& value) {
            for (auto i = 0; i < value.equations_size(); i++) {
                value.print_equation(stream, i);
            };
        }
        
    public:
        PolyBoRiStreamWriter(std::ostream& stream): StreamWriter<Anf>(stream) {};
        
        void write(const Anf& value) override {
            write_header(value);
            write_parameters(value);
            write_variables(value);
            write_equations(value);
        };
    };
    
}

#endif /* polybori_hpp */
