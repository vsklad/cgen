//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#include <string>
#include "textreader.hpp"

#ifndef commandlinereader_hpp
#define commandlinereader_hpp

namespace bal {
    
    class CommandLineReader: public virtual TextReader {
    private:
        int current_arg_index_;
        const int argc_;
        const char* const* const argv_;
        
    protected:
        void getline(std::string& str) override {
            if (current_arg_index_ < argc_) {
                str = argv_[current_arg_index_++];
            };
        };
        
        bool is_eof_() override { return current_arg_index_ >= argc_; };
        
    public:
        CommandLineReader(int argc, const char* const argv[]): argc_(argc), argv_(argv), current_arg_index_(0) {};
        
        // determines if the next token is option sign
        inline bool is_option() { return is_symbol('-'); };
        
        // given an sorted array of option names, short and long combined
        // return index of the array which corresponds to the read option
        // returns options_size if the option name is not in the array
        // positions to the first symbol after end of the option name
        inline int read_option(const char* const options[], const unsigned int options_size) {
            int result = options_size;
            std::string option_name;
            read_symbol('-');
            if (!is_symbol('-')) {
                // short
                if (is_token(ttLiteral)) {
                    const char option_name = read_symbol();
                    for (auto i = 0; i < options_size; i++) {
                        if (options[i][0] == option_name && options[i][1] == 0) {
                            result = i;
                            break;
                        };
                    };
                } else {
                    parse_error("Invalid option name");
                };
            } else {
                // long
                read_symbol('-');
                if (is_token(ttLiteral)) {
                    for (auto i = 0; i < options_size; i++) {
                        if (is_token(options[i])) {
                            skip_token();
                            result = i;
                            break;
                        };
                    };
                } else {
                    parse_error("Invalid option name");
                };
            };
            return result;
        };
    };
};

#endif /* commandlinereader_hpp */
