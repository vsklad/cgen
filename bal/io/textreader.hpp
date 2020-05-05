//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef textreader_hpp
#define textreader_hpp

#include <string>
#include <vector>
#include <ostream>
#include <stdint.h>
#include "assertlevels.hpp"

#define ERROR_BIN_INVALID_SYMBOL "Invalid symbol in binary value"
#define ERROR_HEX_INVALID_SYMBOL "Invalid symbol in hexadecimal value"
#define ERROR_DEC_INVALID_SYMBOL "Invalid symbol in decimal value"
#define ERROR_INVALID_CONSTANT_VALUE "Invalid constant value"
#define ERROR_MISSING_CLOSING_QUOTE "Missing closing quote symbol"
#define ERROR_MISSING_HEX_VALUE "Hexadecimal value prefix specified without the value"
#define ERROR_MISSING_BIN_VALUE "Binary value prefix specified without the value"

#define _is_bin_symbol(value) ((value) == '0' || (value) == '1')
#define _is_dec_symbol(value) ((value) >= '0' && (value) <= '9')
#define _is_hex_letter(value) ((value) >= 'A' && (value) <= 'F') || ((value) >= 'a' && (value) <= 'f')
#define _is_hex_symbol(value) (_is_dec_symbol(value) || _is_hex_letter(value))
#define _is_letter_capital_case(value) ((value) >= 'A' && (value) <= 'Z')
#define _is_letter_lower_case(value) ((value) >= 'a' && (value) <= 'z')
#define _is_digit(value) _is_dec_symbol(value)
#define _is_letter(value) (_is_letter_capital_case(value) || _is_letter_lower_case(value))
#define _is_literal_symbol(value) (_is_digit(value) || _is_letter(value) || (value) == '_')

// convert a char to a 4 bit hex value; assume the character has been validated
#define _hex_value(value) ((value) < 'A' ? (value) - '0' : 10 + ((value) < 'a' ? (value) - 'A' : (value) - 'a'))
#define _bin_value(value) ((value) == '0' ? 0 : 1)

class TextReaderException {
private:
    size_t line_index_;
    size_t pos_;
    std::string line_;
    std::string message_;
    
public:
    TextReaderException(size_t line_index, size_t pos, std::string line, std::string message): line_index_(line_index), pos_(pos), line_(line), message_(message) {};
    
    const size_t get_pos() { return pos_; };
    const char* const get_line() { return line_.c_str(); };
    const char* const get_message() { return message_.c_str(); };
    
    friend std::ostream& operator << (std::ostream& stream, const TextReaderException &e) {
        stream << e.message_;
        if (e.line_index_ > 0) {
            stream << ", line: " << e.line_index_ << ", pos: " << e.pos_ << ", \"" << e.line_ << "\"";
        };
        return stream;
    };
};

class TextReader {
public:
    enum TokenType {ttUnknown, ttDec, ttNegativeDec, ttHex, ttBin, ttLiteral, ttSymbol, ttQuoted};
    
private:
    size_t current_token_pos_ = SIZE_MAX;
    size_t current_token_len_ = 0; // zero means token is not loaded
    std::string current_line_ = "";
    size_t current_line_index_ = 0;
    TokenType current_token_type_ = ttUnknown;

protected:
    virtual bool is_eof_() = 0;
    virtual void getline(std::string& str) = 0;
    
    const char* const get_current_token() { return current_line_.c_str() + current_token_pos_; };
    size_t get_current_token_len() { return current_token_len_; };
    
private:
    inline void load_first_line() {
        if (current_line_index_ == 0)
            load_next_line();
    };
    
    inline void load_next_line() {
        if (!is_eof_()) {
            getline(current_line_);
            current_line_index_ += 1;
        } else {
            current_line_ = "";
        };
        current_token_pos_ = 0;
        current_token_len_ = 0;
    };
    
    inline void reset_token() {
        current_token_len_ = 0;
        current_token_type_ = ttUnknown;
    };
    
    inline void parse_hex() {
        current_token_len_++;
        current_token_len_++;
        while (current_token_pos_ + current_token_len_ < current_line_.size() &&
               _is_hex_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            current_token_len_++;
        };
        if (current_token_pos_ + current_token_len_ < current_line_.size() &&
            _is_literal_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            parse_error(ERROR_HEX_INVALID_SYMBOL);
        } else if (current_token_len_ == 2) {
            parse_error(ERROR_MISSING_HEX_VALUE);
        };
    };
    
    inline void parse_bin() {
        current_token_len_++;
        current_token_len_++;
        while (current_token_pos_ + current_token_len_ < current_line_.size() &&
               _is_bin_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            current_token_len_++;
        };
        if (current_token_pos_ + current_token_len_ < current_line_.size() &&
            _is_literal_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            parse_error(ERROR_BIN_INVALID_SYMBOL);
        } else if (current_token_len_ == 2) {
            parse_error(ERROR_MISSING_BIN_VALUE);
        };
    };
    
    inline void parse_dec() {
        current_token_len_++;
        while (current_token_pos_ + current_token_len_ < current_line_.size() &&
               _is_dec_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            current_token_len_++;
        };
        if (current_token_pos_ + current_token_len_ < current_line_.size() &&
            _is_literal_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            parse_error(ERROR_DEC_INVALID_SYMBOL);
        };
    };
    
    inline void parse_literal() {
        current_token_len_++;
        while (current_token_pos_ + current_token_len_ < current_line_.size() &&
               _is_literal_symbol(current_line_[current_token_pos_ + current_token_len_])) {
            current_token_len_++;
        };
    };
    
    inline void parse_quoted(const char quote_symbol) {
        current_token_len_++;
        while (current_token_pos_ + current_token_len_ < current_line_.size() &&
            (current_line_[current_token_pos_ + current_token_len_] != quote_symbol)) {
            current_token_len_++;
        };
        if (current_token_pos_ + current_token_len_ < current_line_.size() &&
            (current_line_[current_token_pos_ + current_token_len_] == quote_symbol)) {
            current_token_len_++;
        } else {
            parse_error(ERROR_MISSING_CLOSING_QUOTE);
        };
    };
    
    void load_next_token() {
        // current_token_len_ > 0 means the token is loaded already
        if (!is_eol() && current_token_len_ == 0) {
            if (current_token_pos_ + current_token_len_ < current_line_.size()) {
                switch(current_line_[current_token_pos_ + current_token_len_]) {
                    case '0':
                        if (current_token_pos_ + current_token_len_ + 1 < current_line_.size()) {
                            switch(current_line_[current_token_pos_ + current_token_len_ + 1]) {
                                case 'x':
                                case 'X':
                                    current_token_type_ = ttHex;
                                    parse_hex();
                                    break;
                                case 'b':
                                case 'B':
                                    current_token_type_ = ttBin;
                                    parse_bin();
                                    break;
                                default:
                                    current_token_type_ = ttDec;
                                    parse_dec();
                                    break;
                            };
                        } else {
                            current_token_type_ = ttDec;
                            current_token_len_= 1;
                        };
                        break;
                    case '1'...'9':
                        current_token_type_ = ttDec;
                        parse_dec();
                        break;
                    case 'a'...'z':
                    case 'A'...'Z':
                    case '_':
                        current_token_type_ = ttLiteral;
                        parse_literal();
                        break;
                    case '"':
                        current_token_type_ = ttQuoted;
                        parse_quoted('"');
                        break;
                    case ' ':
                    case '\t':
                        current_token_type_ = ttUnknown;
                        break;
                    default:
                        current_token_type_ = ttSymbol;
                        current_token_len_++;
                        break;
                };
            } else {
                current_token_type_ = ttUnknown;
            };
        };
    };
    
public:
    void parse_error(std::string message) {
        TextReaderException exception(current_line_index_, current_token_pos_, current_line_, message);
        throw exception;
    };
    
    const char* const get_current_line() const { return current_line_.c_str(); };
    
    inline bool is_space() {
        return is_symbol(' ') || is_symbol('\t');
    };
    
    inline bool is_symbol(const char value) {
        load_next_token();
        if (current_token_len_ > 0) {
            return current_line_[current_token_pos_] == value;
        };
        return false;
    };
    
    inline bool is_token(const char* value) {
        load_next_token();
        if (current_token_len_ > 0) {
            auto index = 0;
            while (index < current_token_len_ && value[index] != 0) {
                if (current_line_[current_token_pos_ + index] != value[index])
                    return false;
                index++;
            };
            if (value[index] != 0) {
                index++;
            };
            return (index == current_token_len_);
        };
        return false;
    };

    inline bool is_digit() {
        load_next_token();
        if (current_token_len_ > 0) {
            return (current_line_[current_token_pos_] >= '0' &&
                    current_line_[current_token_pos_] <= '9');
        };
        return false;
    };
    
    inline bool is_token(const TokenType token_type) {
        load_next_token();
        if (current_token_len_ > 0) {
            return (current_token_type_ == token_type);
        }
        return false;
    };
    
    inline bool is_sint32(const int32_t value) {
        std::string s = std::to_string(value);
        return is_token(s.c_str());
    };
    
    inline bool is_uint32(const uint32_t value) {
        std::string s = std::to_string(value);
        return is_token(s.c_str());
    };
    
    inline bool is_eol() {
        load_first_line();
        // skip \r symbol at the end if exists
        if (current_token_pos_ == current_line_.size() - 1 && current_line_[current_token_pos_] == '\r') {
            current_token_pos_++;
        };
        return current_token_pos_ >= current_line_.size();
    };
    
    inline bool is_eof() { return is_eof_() && is_eol(); };
    
    inline char read_symbol() {
        load_next_token();
        if (current_token_len_ > 0) {
            const char value = current_line_[current_token_pos_];
            skip_symbol();
            return value;
        } else {
            parse_error("Cannot read symbol");
            return 0;
        };
    };
    
    inline void read_symbol(const char value) {
        if (is_symbol(value))
            skip_symbol();
        else
            parse_error(std::string("Expect \"") + std::string(1, value) + std::string("\" symbol"));
    };
    
    inline void read_token(const char* value) {
        if (is_token(value))
            skip_token();
        else
            parse_error(std::string("Expect \"") + std::string(value) + std::string("\" token"));
    };
    
    inline const std::string read_until_eol() {
        std::string result = get_current_token();
        current_token_pos_ = current_line_.size();
        reset_token();
        return result;
    };
    
    inline std::string read_literal() {
        load_next_token();
        if (current_token_type_ != ttLiteral) {
            parse_error("Expect literal");
        };
        std::string result = current_line_.substr(current_token_pos_, current_token_len_);
        skip_token();
        return result;
    };
    
    inline int32_t read_sint32() {
        bool is_negative = is_symbol('-');
        if (is_negative) {
            skip_symbol();
        };
        int64_t result = read_uint32();
        if (is_negative) result = -result;
        
        if (result > INT32_MAX || result < INT32_MIN) {
            parse_error("The signed int 32 bit value is out of bounds");
        };
        return (int32_t)result;
    };
    
    inline void read_sint32(const int32_t value) {
        if (is_sint32(value))
            skip_token();
        else
            parse_error(std::string("Expect \"") + std::to_string(value) + std::string("\" value"));
    };
    
    inline uint32_t read_uint32(const uint32_t min_value = 0, const uint32_t max_value = UINT32_MAX) {
        load_next_token();
        if (current_token_type_ == ttDec) {
            uint64_t result = 0;
            auto index = 0;
            while (index < current_token_len_) {
                // this is checked when parsing the token already
                assert(_is_digit(current_line_[current_token_pos_ + index]));
                result = result * 10 + (current_line_[current_token_pos_ + index] - '0');
                if (result < min_value || result > max_value) {
                    parse_error(std::string("The unsigned int 32 bit value is out of bounds: ") +
                                std::to_string(min_value) + std::string("..") + std::to_string(max_value));
                };
                index++;
            };
            skip_token();
            return (uint32_t)result;
        };
        parse_error("Expect an unsigned int 32 bit value");
        return 0;
    };
    
    std::string read_quoted() {
        load_next_token();
        if (current_token_type_ == ttQuoted) {
            std::string result = current_line_.substr(current_token_pos_ + 1, current_token_len_ - 2);
            skip_token();
            return result;
        } else {
            parse_error("Expect a quoted value");
            return std::string();
        };
    };
    
    inline void read_eol() { if (!is_eol()) parse_error("Expect end of the line"); else skip_line(); };
    inline void read_eof() { if (!is_eof_()) parse_error("Expect end of the file"); };
    
    inline void skip_space() {
        while (current_token_pos_ < current_line_.size()) {
            if (current_line_[current_token_pos_] != ' ')
                break;
            current_token_pos_++;
            if (current_token_len_ > 0) {
                reset_token();
            };
        };
    };
    
    inline void skip_symbol() {
        if (current_token_len_ > 0) {
            current_token_pos_++;
            reset_token();
        };
    };
    
    inline void skip_token() {
        current_token_pos_ += current_token_len_;
        reset_token();
    };
    
    inline void skip_line() {
        load_first_line();
        load_next_line();
    };
};

#endif /* textreader_hpp */
