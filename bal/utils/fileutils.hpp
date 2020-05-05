//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef fileutils_hpp
#define fileutils_hpp

#include <type_traits>
#include <iostream>
#include <fstream>
#include "streamable.hpp"

namespace bal {

    template<typename FORMULA_T, typename READER_T>
    bool read_from_file(FORMULA_T& formula, const char* file_name) {
        static_assert(std::is_base_of<TextStreamReader<FORMULA_T>, READER_T>::value, "READER_T must be a descendant of TextStreamReader<FORMULA_T>");
        bool result = false;
        std::ifstream file(file_name);
        if (file.is_open()) {
            try {
                READER_T reader(file);
                reader.read(formula);
                file.close();
                result = true;
            }
            catch (TextReaderException e) {
                std::cout << "Parse Error: " << e << "." << std::endl;
            }
        }
        else {
            std::cout << "Error: canot open the file \"" << file_name << "\"." << std::endl;
        };
        return result;
    };

    template<typename FORMULA_T, typename WRITER_T>
    bool write_to_file(const FORMULA_T& formula, const char* file_name) {
        static_assert(std::is_base_of<StreamWriter<FORMULA_T>, WRITER_T>::value, "WRITER_T must be a descendant of StreamWriter<FORMULA_T>");
        bool result = false;
        std::ofstream file(file_name);
        if (file.is_open()) {
            WRITER_T writer(file);
            writer.write(formula);
            file.close();
            result = true;
        }
        else {
            std::cout << "Error: canot open the file \"" << file_name << "\"." << std::endl;
        };
        return result;
    };
};

#endif /* fileutils_hpp */
