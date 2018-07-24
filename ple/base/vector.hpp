//
//  Propositional Logic Engine (PLE) Library
//  https://cgen.sophisticatedways.net
//  Copyright © 2018 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef vector_hpp
#define vector_hpp

#include <algorithm>
#include <assert.h>
#include <stdint.h>

namespace ple {
    
    template<typename T, typename iterator = T*>
    class vector {
    public:
        typedef uint32_t vector_size_t;
        static const constexpr uint32_t VECTOR_SIZE_MAX = UINT32_MAX;
        
    public:
        // memory buffer
        T* data_ = nullptr;
        // number of items of type T filled with data
        vector_size_t size_ = 0;
        
    private:
        // size of the allocated memory as a number of items of type T
        vector_size_t allocated_size_ = 0;
        
    private:
        inline void resize_(const vector_size_t size) {
            if (allocated_size_ != size) {
                if (data_ != nullptr) {
                    if (size == 0) {
                        free(data_);
                        data_ = nullptr;
                        size_ = 0;
                    } else {
                        void* new_data = realloc(data_, size * sizeof(T));
                        assert(new_data != nullptr);
                        data_ = (T*)new_data;
                    };
                } else if (size != 0) {
                    data_ = (T*)malloc(size * sizeof(T));
                    assert(data_ != nullptr);
                    size_ = 0;
                } else {
                    size_ = 0;
                };
                allocated_size_ = size;
            };
        };
        
    public:
        vector() = default;
        
        vector(const vector_size_t size) {
            reserve(size);
            size_ = size;
            /* append(T(0), size); */
        };
        
        inline ~vector() {
            reset(0);
        };
        
        inline const iterator begin() const { return iterator(data_); };
        inline const iterator end() const { return iterator(data_ + size_); };
        inline iterator begin() { return iterator(data_); };
        inline iterator end() { return iterator(data_ + size_); };
        
        inline void reset(const vector_size_t reserve_size) {
            resize_(reserve_size);
            size_ = 0;
        };
        
        // size is number of words to reserve
        inline void reserve(const vector_size_t reserve_size) {
            if (allocated_size_ < size_ + reserve_size) {
                // grow 1.5 times each time
                resize_(size_ + reserve_size + (allocated_size_ >> 1));
            };
        };
        
        // append the list with
        inline void append(const T value, const vector_size_t repeat_size) {
            reserve(repeat_size);
            std::fill(data_ + size_, data_ + size_ + repeat_size, value);
            size_ += repeat_size;
        };
    };
};

#endif /* vector_hpp */
