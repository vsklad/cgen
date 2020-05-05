//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef cnfclauses_hpp
#define cnfclauses_hpp

#include "container.hpp"
#include "variables.hpp"

namespace bal {
    
    // CLAUSE MEMORY STRUCTURE
    //   clause is a sequence of 32 bit words
    //   first word is a header; it is followed by a sequence of sorted variable/literal IDs
    //     literals are stored in ascending order
    //   lower 15 bits of the clause header define clause size (number of literals)
    //   16th bit is a flag which marks the clause as excluded when set
    //   higher 16 bits of the header are clause-size-dependent flags
    //     clauses of size less than or equal to 4 literals are "aggregated"
    //       unnegated literals are used only, defining the combination of variables
    //       all clauses with the same variables are aggregated together
    //       up to 16 flags identify which literal combinations are present
    //     longer clauses: flags not used, both negated and unnegated variables are present
    //
    //   header: 32 bit word, E - exclusion flag, 1 bit; 16 bits aggregated clause flags
    //  |0                           14| |16                            31|
    //  |==============================|=|================================|
    //  |   size (#literals) 15 bits   |E|     aggregated clause flags    |
    //  |==============================|=|================================|
    //
    // CLAUSE CONTAINER MEMORY STRUCTURE
    //   the container (memory buffer) is a sequence of 32 bit words
    //   index + header + literals
    //   index only present within the Container and is absent
    //   when the cause is operated outside of it;
    //   each element is a 32 bit word
    //  |========|========|========|========|========|========|========|
    //  | parent | left   | right  | header | literalid_t literals[]   |
    //  |========|========|========|========|========|========|========|
    //  |          index           |            clause data            |
    //  |==========================|===================================|
    //  |<- offset                 |<- p_clause                        |
    //  |<-------------------- memory size --------------------------->|
    //
    
    typedef uint16_t clause_flags_t;
    typedef uint16_t clause_size_t;
    typedef uint32_t clauses_size_t;
    
    // maximal supported length of the clause (number of literals)
    // limited to embed into 32 bit clause header
    static const clause_size_t constexpr CLAUSE_SIZE_MAX = 0x7FFF;
    // maximal theoretically supported number of clauses
    // due to clause index
    static const clauses_size_t constexpr CLAUSES_SIZE_MAX = CONTAINER_SIZE_MAX;
    static const clauses_size_t constexpr CLAUSES_END = CONTAINER_END;
    static const variables_size_t constexpr VARIABLES_SIZE_MAX = VARIABLEID_MAX;
    
    // when clauses are compared literal by literal
    // defines direction: whether right to left or left to right
    static const bool COMPARE_CLAUSES_LEFT_RIGHT = false;
    
#define _clause_header(p_clause) (*(p_clause))
#define _clause_header_make(flags, size) (uint32_t)(((flags) << 16) | (size))
#define _clause_header_flags(header) (clause_flags_t)((header) >> 16)
#define _clause_header_size(header) ((header) & CLAUSE_SIZE_MAX)
#define _clause_header_memory_size(header) _clause_size_memory_size(_clause_header_size(header))
#define _clause_header_set(p_clause, flags, size) _clause_header(p_clause) = _clause_header_make((flags), (size))
#define _clause_flags(p_clause) _clause_header_flags(_clause_header(p_clause))
#define _clause_flags_include(p_clause, flags) _clause_header(p_clause) |= _clause_header_make(flags, 0)
#define _clause_flags_set(p_clause, flags) _clause_header(p_clause) = (((flags) << 16) | (_clause_header(p_clause) & 0xFFFF))
#define _clause_size(p_clause) _clause_header_size(_clause_header(p_clause))
#define _clause_size_memory_size(clause_size) ((clause_size) + 1)
#define _clause_memory_size(p_clause) _clause_size_memory_size(_clause_size(p_clause))
#define _clause_size_is_aggregated(clause_size) (clause_size <= 4)
#define _clause_is_aggregated(p_clause) _clause_size_is_aggregated(_clause_size(p_clause))
#define _clause_literals(p_clause) ((p_clause) + 1)
#define _clause_literal(p_clause, index) _clause_literals(p_clause)[(index)]
#define _clause_is_literal_unchanged(p_clause, index, variables) \
    _clause_literal((p_clause), (index)) == (variables)[literal_t__variable_id(_clause_literal((p_clause), (index)))]
#define _clause_variable(p_clause, index) literal_t__variable_id(_clause_literal(p_clause, index))
    
#define _clause_include(p_clause) (_clause_header(p_clause) &= 0xFFFF7FFF)
#define _clause_exclude(p_clause) (_clause_header(p_clause) |= 0x00008000)
#define _clause_is_included(p_clause) ((_clause_header(p_clause) & 0x00008000) == 0)
    
#define _clause_copy_size(src, dst, clause_size) std::copy((src), (src) + _clause_size_memory_size(clause_size), (dst));
#define _clause_copy(src, dst) _clause_copy_size(src, dst, _clause_size(src));

#define _clause_c1_make(l0) {_clause_header_make(literal_t__is_negation(l0) ? 1 : 2, 1), literal_t__unnegated(l0)}
#define _clause_c2_make(flags, l0, l1) {_clause_header_make((flags), 2), (l0), (l1)}
#define _clause_c3_make(flags, l0, l1, l2) {_clause_header_make((flags), 3), (l0), (l1), (l2)}
    
// for a processed clause, only the below non-0xF flag combinations are possible
// because anything else would have been processed and the clause would be excluded;
// i.e. a single binary clause may exist for a particular pair of variables;
// mapping of a single bit to its position index
// meaning only one clause is present in the aggregate
//#define _c2_combination_index(value) ((value) == 1 ? 0 : (value) == 2 ? 1 : (value) == 4 ? 2 : (value) == 8 ? 3 : 0xF)
#define _c2_combination_index(value) __c2_combination_index[value]
constexpr uint8_t __c2_combination_index[16] = {
    0xF, 0, 1, 0xF, 2, 0xF, 0xF, 0xF, 3, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF
};
    
#define _c2_is_single_clause_flags(value) ((value) == 0b0001 || (value) == 0b0010 || (value) == 0b0100 || (value) == 0b1000)
    
    extern unsigned __find_clause_found;
    extern unsigned __find_clause_unfound;
    extern unsigned __compare_clauses_;
    extern unsigned __append_clause_;
    extern unsigned __normalize_clause_;
    
    // assume the first SAME_LITERALS are the same
    template<bool compare_left_right = true>
    inline int compare_clauses(const uint32_t* lhs, const uint32_t* rhs) {
        
        __compare_clauses_++;
        
        clause_size_t lhs_size = _clause_size(lhs);
        clause_size_t rhs_size = _clause_size(rhs);
        
        if (compare_left_right) {
            const clause_size_t common_size = lhs_size > rhs_size ? rhs_size : lhs_size;
            for (auto i = 1; i <= common_size; i++) {
                if (lhs[i] != rhs[i]) {
                    return lhs[i] < rhs[i] ? -1 : 1;
                };
            };
        } else {
            while (lhs_size > 0 && rhs_size > 0) {
                if (lhs[lhs_size] != rhs[rhs_size]) {
                    return lhs[lhs_size] < rhs[rhs_size] ? -1 : 1;
                };
                lhs_size--;
                rhs_size--;
            };
        };
        
        // if all the corresponding literals match then the shorter clause is smaller
        return lhs_size < rhs_size ? -1 : (lhs_size == rhs_size ? 0 : 1);
    };
    
    inline uint16_t get_cardinality_uint16(const uint16_t value) {
        // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
        const uint16_t map[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
        return map[value & 0xF] + map[value >> 4 & 0xF] + map[value >> 8 & 0xF] + map[value >> 12 & 0xF];
    };
    
    template<clause_size_t size1, clause_size_t size2, clause_size_t l0_index, clause_size_t l1_index, clause_size_t l2_index = size2>
    inline void caca_expand_flags(clause_flags_t& value) {
        static_assert(2 <= size1 && size1 < size2 && size2 <= 4, "expansion is possible to c3 or c4");
        static_assert(0 <= l0_index && l0_index < l1_index && l1_index < l2_index, "invalid indexes");
        static_assert(size1 != 3 || l2_index < size2, "the third index is only used for c3->c4");
        static_assert(size1 == 3 || l2_index == size2, "the third index is only used for c3->c4");
        if (size1 == 2) {
            if (l1_index < 3) {
                if (l0_index == 0 && l1_index == 1) {
                    value |= (value << 4);
                } else if (l0_index == 0 && l1_index == 2) {
                    value = (value & 0x0003) | ((value & 0x000C) << 2);
                    value |= value << 2;
                } else if (l0_index == 1 && l1_index == 2) {
                    value = (value & 0x0001) | ((value & 0x0002) << 1) |
                    ((value & 0x0004) << 2) | ((value & 0x0008) << 3);
                    value |= value << 1;
                };
                if (size2 == 4) {
                    value |= (value << 8);
                };
            } else {
                _assert_level_0(_c2_is_single_clause_flags(value));
                if (l0_index == 0) {
                    value = (value == 1 ? 0x0055 : value == 2 ? 0x00AA : value == 4 ? 0x5500 : 0xAA00);
                } else if (l0_index == 1) {
                    value = (value == 1 ? 0x0033 : value == 2 ? 0x00CC : value == 4 ? 0x3300 : 0xCC00);
                } else {
                    value = (value == 1 ? 0x000F : value == 2 ? 0x00F0 : value == 4 ? 0x0F00 : 0xF000);
                };
            };
        } else {
            if (l0_index == 0 && l1_index == 1 && l2_index == 2) {
                value |= (value << 8);
            } else if (l0_index == 0 && l1_index == 1 && l2_index == 3) {
                value = (value & 0x000F) | ((value & 0x00FF) << 4) | ((value & 0x00F0) << 8);
            } else if (l0_index == 0 && l1_index == 2 && l2_index == 3) {
                value = (value & 0x0003) | ((value & 0x000C) << 2) | ((value & 0x0030) << 4) | ((value & 0x00C0) << 6);
                value |= value << 2;
            } else if (l0_index == 1 && l1_index == 2 && l2_index == 3) {
                value = (value & 0x0001) | ((value & 0x0002) << 1) | ((value & 0x0004) << 2) | ((value & 0x0008) << 3) |
                            ((value & 0x0010) << 4) | ((value & 0x0020) << 5) | ((value & 0x0040) << 6) | ((value & 0x0080) << 7);
                value |= value << 1;
            };
        };
    };
    
    // size1 - length of the reduced clause, 2 or 3
    // size2 - length of the original clause, 3 or 4
    // index0, index1, index2 - indexes of the literals of the reduced clause within the original clause
    // index2 is only used when reducing c4 to c3
    template<clause_size_t size1, clause_size_t size2, clause_size_t index0, clause_size_t index1, clause_size_t index2 = size2>
    inline clause_flags_t caca_reduced_flags(const clause_flags_t value) {
        static_assert(2 <= size1 && size1 < size2 && size2 <= 4, "expansion is possible to c3 or c4");
        static_assert(0 <= index0 && index0 < index1 && index1 < index2, "invalid indexes");
        static_assert(size1 != 3 || index2 < size2, "the third index is only used for c3->c4");
        static_assert(size1 == 3 || index2 == size2, "the third index is only used for c3->c4");
        if (size1 == 2) {
            if (size2 == 3) {
                if (index0 == 0 && index1 == 1) {
                    return (value >> 4) & value;
                } else if (index0 == 0 && index1 == 2) {
                    clause_flags_t flags = (value >> 2) & value;
                    return (flags & 0x3) | ((flags >> 2) & 0xC);
                } else {
                    // l0_index == 1 && l1_index == 2
                    clause_flags_t flags = (value >> 1) & value;
                    return (flags & 0x1) | ((flags >> 1) & 0x2) | ((flags >> 2) & 0x4) | ((flags >> 3) & 0x8);
                };
            };
            if (size2 == 4) {
                if (index0 == 0 && index1 == 1) {
                    return value & (value >> 4) & (value >> 8) & (value >> 12) & 0x000F;
                } else if (index0 == 0 && index1 == 2) {
                    clause_flags_t f = value & (value >> 2) & (value >> 8) & (value >> 10) & 0x0033;
                    return (f | (f >> 2)) & 0x000F;
                } else if (index0 == 1 && index1 == 2) {
                    clause_flags_t f = value & (value >> 1) & (value >> 8) & (value >> 9) & 0x0055;
                    f = (f | (f >> 1)) & 0x0033;
                    return (f | (f >> 2)) & 0x000F;
                } else if (index0 == 0 && index1 == 3) {
                    clause_flags_t f = value & (value >> 2) & (value >> 4) & (value >> 6) & 0x0303;
                    return (f | (f >> 6)) & 0x000F;
                } else if (index0 == 1 && index1 == 3) {
                    clause_flags_t f = value & (value >> 1) & (value >> 4) & (value >> 5) & 0x0505;
                    f = (f | (f >> 1)) & 0x0303;
                    return (f | (f >> 6)) & 0x000F;
                } else {
                    // index0 == 2 && index1 == 3
                    clause_flags_t f = value & (value >> 1) & (value >> 2) & (value >> 3) & 0x1111;
                    return (f | (f >> 3) | (f >> 6) | (f >> 9)) & 0x000F;
                };
            };
        } else {
            // c4 -> c3, due to parameter constraints
            if (index0 == 0 && index1 == 1 && index2 == 2) {
                return value & (value >> 8) & 0x00FF;
            } else if (index0 == 0 && index1 == 1 && index2 == 3) {
                const clause_flags_t f = value & (value >> 4) & 0x0F0F;
                return (f | (f >> 4)) & 0x00FF;
            } else if (index0 == 0 && index1 == 2 && index2 == 3) {
                const clause_flags_t f = value & (value >> 2) & 0x3333;
                return ((f | (f >> 2)) & 0x000F) | (((f | (f >> 2)) & 0x0F00) >> 4);
            } else {
                // index0 == 1 && index1 == 2 && index2 == 3
                const clause_flags_t f = value & (value >> 1) & 0x5555;
                return (f & 0x0001) | ((f >> 1) & 0x0002) | ((f >> 2) & 0x0004) | ((f >> 3) & 0x0008) |
                    ((f >> 4) & 0x0010) | ((f >> 5) & 0x0020) | ((f >> 6) & 0x0040) | ((f >> 7) & 0x0080);
            }
        };
    };
    
    inline clause_flags_t c2ca_expanded_flags(const clause_size_t size,
                                              const clause_size_t l0_index,
                                              const clause_size_t l1_index,
                                              clause_flags_t value) {
        switch(size << 8 | l0_index << 4 | l1_index) {
            case 0x301:
                caca_expand_flags<2, 3, 0, 1>(value);
                break;
            case 0x302:
                caca_expand_flags<2, 3, 0, 2>(value);
                break;
            case 0x312:
                caca_expand_flags<2, 3, 1, 2>(value);
                break;
            case 0x401:
                caca_expand_flags<2, 4, 0, 1>(value);
                break;
            case 0x402:
                caca_expand_flags<2, 4, 0, 2>(value);
                break;
            case 0x403:
                caca_expand_flags<2, 4, 0, 3>(value);
                break;
            case 0x412:
                caca_expand_flags<2, 4, 1, 2>(value);
                break;
            case 0x413:
                caca_expand_flags<2, 4, 1, 3>(value);
                break;
            case 0x423:
                caca_expand_flags<2, 4, 2, 3>(value);
                break;
            default:
                _assert_level_0(false);
        };
        return value;
    };
    
    template<clause_size_t size>
    inline clause_flags_t ca_residual_flags(const clause_flags_t value) {
        static_assert(_clause_size_is_aggregated(size), "must be an aggregated clause");
        if (size == 2) {
            return _c2_is_single_clause_flags(value) ? value : 0;
        } else if (size == 3) {
            return value &
                ~(value & ((value >> 4) | (value << 4))) &
                ~(value & ((value & 0x33) << 2 | (value & 0xCC) >> 2)) &
                ~(value & ((value & 0x55) << 1 | (value & 0xAA) >> 1));
        } else {
            return value &
                ~(value & ((value >> 8) | (value << 8))) &
                ~(value & ((value & 0x0F0F) << 4 | (value & 0xF0F0) >> 4)) &
                ~(value & ((value & 0x3333) << 2 | (value & 0xCCCC) >> 2)) &
                ~(value & ((value & 0x5555) << 1 | (value & 0xAAAA) >> 1));
        };
    };
    
    inline clause_flags_t ca_residual_flags(const clause_flags_t value, const clause_size_t size) {
        _assert_level_0(_clause_size_is_aggregated(size));
        return size == 2 ? ca_residual_flags<2>(value) :
            (size == 3 ? ca_residual_flags<3>(value) : (size == 4 ? ca_residual_flags<4>(value) : value));
    };
    
    inline void ca_flags_negate(uint16_t& flags, const clause_size_t index) {
        constexpr uint16_t map0[4] = { 0x5555, 0x3333, 0x0F0F, 0x00FF };
        constexpr uint16_t map1[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
        const uint16_t shift = 1 << index;
        flags = ((flags & map1[index]) >> shift) | ((flags & map0[index]) << shift);
    };
    
    // calculate aggregated clause flags assuming
    // literal <index> is matched with literal <c2_index>
    // c2_flags are required to be a single clause
    inline void resolve_ca_c2_flags(uint16_t& flags, const clause_size_t index, const clause_size_t c2_index, const uint16_t c2_flags) {
        
        // filters out clauses with specified literal negated (or not)
        // first dimention is whether negated or not, 0 means negated 1 direct
        // second dimention is the literal index, 0..3
        constexpr uint16_t clause_flags_negation_mask[2][4] = {
            {0x5555, 0x3333, 0x0F0F, 0x00FF}, // negated
            {0xAAAA, 0xCCCC, 0xF0F0, 0xFF00}  // direct
        };
        
        _assert_level_0(c2_flags <= 0xF);
        const uint8_t c2_combination = _c2_combination_index(c2_flags);
        _assert_level_1(c2_combination != 0xF);
        
        // if implicant is complemented, only get uncompemented clauses and vise versa
        if (c2_index == 0) {
            // (... lo ...) (lo, lx)
            flags &= clause_flags_negation_mask[!(c2_combination & 0b01)][index];
        } else {
            // (... lo ...) (lx, l0)
            flags &= clause_flags_negation_mask[!(c2_combination & 0b10)][index];
        };
        
        // if the resolvent literal is complemented, uncomplement the flags
        if (c2_combination == 0b00 || c2_combination == 0b11) {
            ca_flags_negate(flags, index);
        };
    };
    
    // insert into sorted position, merge flags for duplicates
    inline void ca_insert_last_literal_sorted(uint16_t& flags, clause_size_t& literals_size, literalid_t* const literals) {
        clause_size_t index = 0;
        while (index < literals_size) {
            if (literals[literals_size] <= literals[index]) {
                break;
            };
            index++;
        };
        if (index == literals_size) {
            // new literal is the last one, nothing else to do
            literals_size++;
        } else if (literals[literals_size] == literals[index]) {
            // nothing to insert, transform flags
            switch((index << 4) | literals_size) {
                case 0x01: // l0 = l1
                    flags = (flags & 0x0001) | (flags & 0x0018) >> 2 |
                    (flags & 0x0180) >> 4 | (flags & 0x1800) >> 6 | (flags & 0x8000) >> 8;
                    break;
                case 0x02: // l0 = l2
                    flags = (flags & 0x0005) | (flags & 0x05A0) >> 4 | (flags & 0xA000) >> 8;
                    break;
                case 0x03: // l0 = l3
                    flags = (flags & 0x0055) | (flags & 0xAA00) >> 8;
                    break;
                case 0x12: // l1 = l2
                    flags = (flags & 0x0003) | (flags & 0x03C0) >> 4 | (flags & 0xC000) >> 8;
                    break;
                case 0x13: // l1 = l3
                    flags = (flags & 0x0033) | (flags & 0xCC00) >> 8;
                    break;
                case 0x23: // l2 = l3
                    flags = (flags & 0x000F) | (flags & 0xF000) >> 8;
                    break;
            };
        } else {
            // insert into index position and update flags
            switch((index << 4) | literals_size) {
                case 0x01: // l1 = l0
                    flags = (flags & 0x9999) | (flags & 0x2222) << 1 | (flags & 0x4444) >> 1; \
                    std::swap(literals[0], literals[1]);
                    break;
                case 0x02: // l2 = l0
                    flags = (flags & 0x8181) |
                    (flags & 0x0202) << 1 | (flags & 0x0404) << 2 | (flags & 0x0808) << 3 |
                    (flags & 0x1010) >> 3 | (flags & 0x2020) >> 2 | (flags & 0x4040) >> 1;
                    std::swap(literals[1], literals[2]);
                    std::swap(literals[0], literals[1]);
                    break;
                case 0x03: // l3 = l0
                    flags = (flags & 0x8001) |
                    (flags & 0x0002) << 1 | (flags & 0x0004) << 2 |
                    (flags & 0x0008) << 3 | (flags & 0x0010) << 4 |
                    (flags & 0x0020) << 5 | (flags & 0x0040) << 6 |
                    (flags & 0x0080) << 7 | (flags & 0x0100) >> 7 |
                    (flags & 0x0200) >> 6 | (flags & 0x0400) >> 5 |
                    (flags & 0x0800) >> 4 | (flags & 0x1000) >> 3 |
                    (flags & 0x2000) >> 2 | (flags & 0x4000) >> 1;
                    
                    std::swap(literals[2], literals[3]);
                    std::swap(literals[1], literals[2]);
                    std::swap(literals[0], literals[1]);
                    break;
                case 0x12: // l2 = l1
                    flags = (flags & 0xC3C3) | (flags & 0x0C0C) << 2 | (flags & 0x3030) >> 2;
                    std::swap(literals[1], literals[2]);
                    break;
                case 0x13: // l3 = l1
                    flags = (flags & 0xC003) |
                    (flags & 0x000C) << 2 | (flags & 0x0030) << 4 | (flags & 0x00C0) << 6 |
                    (flags & 0x0300) >> 6 | (flags & 0x0C00) >> 4 | (flags & 0x3000) >> 2;
                    std::swap(literals[2], literals[3]);
                    std::swap(literals[1], literals[2]);
                    break;
                case 0x23: // l3 -> l2
                    flags = (flags & 0xF00F) | (flags & 0x00F0) << 4 | (flags & 0x0F00) >> 4;
                    std::swap(literals[2], literals[3]);
                    break;
            };
            literals_size++;
        };
    };
     
    inline void print_clause(std::ostream& stream, const uint32_t* const p_clause, const char* final_token = nullptr) {
        uint16_t clauses_bitmap = _clause_flags(p_clause);
        const clause_size_t clause_size = _clause_size(p_clause);
        
        if (clause_size <= 4 && clauses_bitmap != 0) {
            uint16_t clause_bitmap = 0;
            while (clauses_bitmap > 0) {
                if (clauses_bitmap & 1) {
                    for (auto i = 0; i < clause_size; i++) {
                        literal_t literal(literal_t__negated_onlyif(_clause_literal(p_clause, i), (clause_bitmap & (1 << i)) == 0));
                        stream << ((i > 0) ? " " : "") << literal;
                    };
                    
                    if (final_token != nullptr) {
                        stream << final_token;
                    };
                };
                clause_bitmap++;
                clauses_bitmap >>= 1;
            };
        } else {
            for (auto i = 0; i < clause_size; i++) {
                stream << ((i > 0) ? " " : "") << literal_t(*(p_clause + i + 1));
            };
            
            if (final_token != nullptr) {
                stream << final_token;
            };
        };
    };
    
    // prints in human readable format mainly for debugging
    // use _clauses_offset_clause to print from offset
    bool __print_clause(const uint32_t* const p_clause);
    
    void __statistics_reset();
    void __statistics_print();
    
    void __print_conflict(const literalid_t variables[], const uint32_t* const p_clause);
};

#endif /* cnfclauses_hpp */
