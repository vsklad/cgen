//
//  Propositional Logic Engine (PLE) Library
//  https://cnfgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef variablesio_hpp
#define variablesio_hpp

#include <ostream>
#include "textreader.hpp"
#include "variablesarray.hpp"

namespace ple {

    // output a compact representation, aggregating constants and sequences
    // [-]<variable_id>[/<sequence_count>[/<sequence_increment>]] | 0x<hex_value> | 0b<bin_value>
    // sequential hexadecimal constants formed up to 64 bit long, split into several values if longer
    // binary constants formed from 1 to 3 symbols to aling the sequence to the closest hex symbol
    // constants assume big endian format, i.e. the most significant bit first
    std::ostream& operator << (std::ostream& stream, const VariablesArray& array);

    // read text representation produced by the above function
    // returns the variable as a sequence of literals in <value>
    class VariableTextReader: public virtual TextReader {
    private:
        inline void read_variable_sequence_(unsigned int& sequence_size, signed int& step_size);
        inline void read_variable_element_hex_(vector<literalid_t>& value);
        inline void read_variable_element_bin_(vector<literalid_t>& value);
        inline void read_variable_element_var_(vector<literalid_t>& value);
        inline void read_variable_element_item_(vector<literalid_t>& value);
        inline void read_variable_elements_sequence_(vector<literalid_t>& value, const variableid_t element_size);
        inline void read_variable_element_(vector<literalid_t>& value, variableid_t& element_size);
        
    protected:
        VariablesArray read_variable_value();
    };
};

#endif /* variablesio_hpp */
