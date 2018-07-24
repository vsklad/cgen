//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef dimacs_hpp
#define dimacs_hpp

#include "streamable.hpp"
#include "cnf.hpp"
#include "variablesio.hpp"

namespace ple {
    
    class DimacsStreamReader:
        private virtual TextStreamReader<Cnf>,
        private virtual VariableTextReader {
            
    private:
        void read_header(Cnf& cnf) {
            read_token("p");
            skip_space();
            read_token("cnf");
            skip_space();
            variables_size_t variables_size = read_uint32();
            skip_space();
            clauses_size_t clauses_size = read_uint32();
            skip_space();
            read_eol();
            cnf.initialize(variables_size, clauses_size);
        };
            
        void read_parameter(Cnf& cnf, const std::string key) {
            const std::string name = read_literal();
            skip_space();
            read_symbol(':');
            skip_space();
            if (is_token(ttDec)) {
                cnf.add_parameter(key, name, read_uint32());
            } else {
                cnf.add_parameter(key, name, read_quoted());
            };
        };
        
        void read_parameters(Cnf& cnf) {
            const std::string key = read_literal();
            skip_space();
            read_symbol('=');
            skip_space();
            read_symbol('{');
            skip_space();
            
            read_parameter(cnf, key);
            skip_space();
            while (is_symbol(',')) {
                read_symbol(',');
                skip_space();
                read_parameter(cnf, key);
                skip_space();
            };

            read_symbol('}');
            skip_space();
            read_eol();
        };
        
        void read_named_variable(Cnf& cnf) {
            std::string name = read_literal();
            skip_space();
            read_symbol('=');
            skip_space();
            VariablesArray value = read_variable_value();
            skip_space();
            read_eol();
            cnf.add_named_variable(name.data(), value);
        };
        
        void read_clauses(Cnf& cnf) {
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
                            read_parameters(cnf);
                        } else if (is_token(ttLiteral)) {
                            read_named_variable(cnf);
                        } else {
                            skip_line();
                        };
                    } else {
                        skip_line();
                    };
                } else {
                    literals.clear();
                    while (!is_eol()) {
                        skip_space();
                        if (is_symbol('0')) {
                            break;
                        }
                        else {
                            literals.push_back(literal_t::signed_encode(read_sint32()));
                        }
                    };
                    skip_space();
                    read_symbol('0');
                    skip_space();
                    read_eol();
                    
                    assert(literals.size() <= CLAUSE_SIZE_MAX);
                    cnf.append_clause(literals.data(), literals.size());
                }
            };
        };
        
    public:
        DimacsStreamReader(std::istream& stream): TextStreamReader<Cnf>(stream) {};
        
        virtual void read(Cnf& value) override {
            read_header(value);
            read_clauses(value);
            read_eof();
        };
    };
    
    class DimacsStreamWriter: public StreamWriter<Cnf> {
    protected:
        void write_header(const Cnf& value) {
            stream() << "p cnf " << value.variables_size() << ' ' << value.clauses_size() << std::endl;
        };
        
        virtual void write_parameters(const Cnf& value) {
            const formula_parameters_t parameters = value.get_parameters();
            for (auto it = parameters.begin(); it != parameters.end(); it++) {
                stream() << "c var ." << it->first << " = {" << it->second << "}" << std::endl;
            };
        };
        
        void write_variables(const Cnf& value) {
            const formula_named_variables_t& nv = value.get_named_variables();
            for (formula_named_variables_t::const_iterator it = nv.begin(); it != nv.end(); ++it) {
                stream() << "c var " << it->first << " = " << it->second << std::endl;
            };
        };
        
        virtual void write_clauses(const Cnf& value) {
            const uint32_t* data = value.data();
            const uint32_t* data_end = data + value.data_size();
            while (data < data_end) {
                Cnf::print_clause(stream(), data, " 0\n");
                data += (*data & 0xFFFF) + 1;
            };
        };
        
    public:
        DimacsStreamWriter(std::ostream& stream): StreamWriter<Cnf>(stream) {};
        
        virtual void write(const Cnf& value) override {
            write_header(value);
            write_parameters(value);
            write_variables(value);
            write_clauses(value);
        };
    };
    
    class DimacsSortedStreamWriter: public DimacsStreamWriter {
    protected:
        virtual void write_parameters(const Cnf& value) override {
            DimacsStreamWriter::write_parameters(value);
        };
        
        virtual void write_clauses(const Cnf& value) override {
            for (auto it: value.sorted_clauses()) {
                Cnf::print_clause(stream(), it, " 0\n");
            };
        };
        
    public:
        DimacsSortedStreamWriter(std::ostream& stream): DimacsStreamWriter(stream) {};
    };
};

#endif /* dimacs_hpp */
