//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfdimacs_hpp
#define cnfdimacs_hpp

#include "streamable.hpp"
#include "cnf.hpp"
#include "variablesio.hpp"

namespace bal {
    
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
            cnf.resize(variables_size, clauses_size);
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
        
    public:
        DimacsStreamReader(std::istream& stream): TextStreamReader<Cnf>(stream) {};
        
        void read(Cnf& value) override {
            value.initialize();
            
            bool is_header_read = false;
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
                } else if (is_symbol('p')) {
                    assert(!is_header_read);
                    read_header(value);
                    is_header_read = true;
                } else {
                    assert(is_header_read);
                    literals.clear();
                    while (!is_eol()) {
                        skip_space();
                        if (is_symbol('0')) {
                            break;
                        }
                        else {
                            literals.push_back(literal_t__from_sint(read_sint32()));
                        }
                    };
                    skip_space();
                    read_symbol('0');
                    skip_space();
                    read_eol();
                    
                    assert(literals.size() <= CLAUSE_SIZE_MAX);
                    value.append_clause(literals.data(), literals.size());
                }
            };
            
            read_eof();
        };
    };
    
    class DimacsStreamWriter: public StreamWriter<Cnf> {
    protected:
        void write_header(const Cnf& value) {
            stream << "p cnf " << std::dec << value.variables_size() << ' ' << value.clauses_size() << std::endl;
        };

        void write_parameter(const std::string& key, const std::string& value) {
            stream << "c var ." << key << " = {" << value << "}" << std::endl;
        };
        
        virtual void write_parameters(const Cnf& value) {
            const formula_parameters_t parameters = value.get_parameters();
            for (auto it = parameters.begin(); it != parameters.end(); it++) {
                if (it->first.compare("writer") != 0) {
                    write_parameter(it->first, it->second);
                };
            };
            if (!value.is_empty()) {
                std::string writer_parameters = "is_sorted: 1, literals_order: \"ascending\", literals_compare_order: ";
                writer_parameters += (value.is_compare_left_right() ? "\"left-right\"" : "\"right-left\"");
                write_parameter("writer", writer_parameters);
            };
        };
        
        void write_variables(const Cnf& value) {
            const formula_named_variables_t& nv = value.get_named_variables();
            for (formula_named_variables_t::const_iterator it = nv.begin(); it != nv.end(); ++it) {
                stream << "c var " << it->first << " = " << it->second << std::endl;
            };
        };
        
        virtual void write_clauses(const Cnf& value) {
            for (auto it: value.clauses()) {
                print_clause(stream, _clauses_offset_item_clause(it), " 0\n");
            };
        };
        
    public:
        DimacsStreamWriter(std::ostream& stream): StreamWriter<Cnf>(stream) {};
        
        void write(const Cnf& value) override {
            write_parameters(value);
            write_variables(value);
            write_header(value);
            write_clauses(value);
        };
    };
};

#endif /* cnfdimacs_hpp */
