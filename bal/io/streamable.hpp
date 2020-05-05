//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef streamable_hpp
#define streamable_hpp

#include <sstream>
#include "textreader.hpp"

namespace bal {

    template <typename T>
    class StreamReader {
    private:
        std::istream& stream_;
    
    protected:
        std::istream& stream() { return stream_; };
        virtual bool is_eof() { return stream_.eof(); };
        
    public:
        StreamReader(std::istream& stream): stream_(stream) {};
        virtual void read(T& value) = 0;
    };
    
    template <typename T>
    class StreamWriter {
    protected:
        std::ostream& stream;
        
    public:
        StreamWriter(std::ostream& _stream): stream(_stream) {};
        virtual void write(const T& value) = 0;
    };
    
    template <typename T>
    class TextStreamReader: public StreamReader<T>, public virtual TextReader {
    protected:
        using TextReader::is_eof;
        bool is_eof_() override { return StreamReader<T>::is_eof(); };
        void getline(std::string& str) override { std::getline(StreamReader<T>::stream(), str); };
        
    public:
        TextStreamReader(std::istream& stream): StreamReader<T>(stream) {};
        
    };
    
};

#endif /* streamable_hpp */
