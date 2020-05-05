//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef container_hpp
#define container_hpp

#include <algorithm>
#include <stdint.h>
#include "assertlevels.hpp"

namespace bal {
    
    typedef uint32_t container_offset_t;
    typedef container_offset_t container_size_t;

    static constexpr container_size_t CONTAINER_SIZE_MAX = UINT32_MAX - 1;
    static constexpr container_size_t CONTAINER_END = UINT32_MAX;
    
    template<typename T>
    class Container {
    public:
        // must return 0 if the objects are equal, < 0 if lhs < rhs, > 0 if lhs > rhs
        typedef int (comparator_t)(const T* const lhs, const T* const rhs);
        typedef comparator_t* comparator_p;
        
    public:
        // memory buffer
        T* data_ = nullptr;
        // number of items of type T filled with data
        container_size_t size_ = 0;
        const container_size_t& allocated_size = allocated_size_;
        
    private:
        // size of the allocated memory as a number of items of type T
        container_size_t allocated_size_ = 0;
        
    private:
        inline void resize_(const container_size_t size) {
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
        
    protected:
        // note: no initialization
        inline void resize(const container_size_t size) {
            if (size_ != size) {
                resize_(size);
                size_ = size;
            };
        };
        
    public:
        Container() = default;
        
        // note: optional initialization
        Container(const container_size_t size) {
            resize(size);
        };

        Container(const Container& other) {
            this->operator=(other);
        };
        
        Container(Container&& other) {
            this->operator=(std::move(other));
        };
        
        inline ~Container() {
            reset(0);
        };
        
        inline Container& operator = (const Container& other) {
            resize(other.size_);
            std::copy(other.data_, other.data_ + size_, data_);
            return *this;
        };
        
        inline Container& operator = (Container&& other) noexcept {
            resize_(0);
            data_ = other.data_;
            allocated_size_ = other.allocated_size_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.allocated_size_ = 0;
            other.size_ = 0;
            return *this;
        };
        
        // number of bytes used to store actual data; used not allocated
        inline size_t memory_size() const { return size_ * sizeof(T); };
        
        inline void reset(const container_size_t reserve_size) {
            resize_(reserve_size);
            size_ = 0;
        };
        
        // size is number of words to reserve
        inline void reserve(const container_size_t reserve_size) {
            if (allocated_size_ < size_ + reserve_size) {
                // grow 1.5 times each time
                resize_(size_ + reserve_size + (allocated_size_ >> 1));
            };
        };
        
        // append the list with
        inline void append(const T value, const container_size_t repeat_size) {
            reserve(repeat_size);
            std::fill(data_ + size_, data_ + size_ + repeat_size, value);
            size_ += repeat_size;
        };
    };
    
    template<class CONTAINER_T, template<class> class ITERATOR_T>
    class ContainerIterable {
    private:
        const CONTAINER_T& container_;
        
    public:
        ContainerIterable(const CONTAINER_T& container): container_(container) {};
        
        using iterator_t = ITERATOR_T<CONTAINER_T>;
        
        inline const iterator_t begin() const { return (container_, true); };
        inline const iterator_t end() const { return iterator_t(container_, false); };
        inline iterator_t begin() { return iterator_t(container_, true); };
        inline iterator_t end() { return iterator_t(container_, false); };
    };
};

#endif /* container_hpp */
