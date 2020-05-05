//
//  Boolean Algebra Library (BAL)
//  https://cgen.sophisticatedways.net
//  Copyright © 2018-2020 Volodymyr Skladanivskyy. All rights reserved.
//  Published under terms of MIT license.
//

#ifndef assertlevels_hpp
#define assertlevels_hpp

#include <assert.h>

// ASSERTIONS
// categorize validation using assertion levels by criticality, complexity and meaning
// level 0:
//    basic critical check such as parameters validation
//    would cause undefined behaviour and/or program abortion if violated
// level 1:
//    algorithm limitations; would lead to unpredictable or incorrect results if violated
// level 2:
//    scenarios that may be correct but were never verified because never occured
// level 3:
//    high level computationally complex/heavy checks to validate expectations
//    do not cause immediate error but may signify a problem somewhere else
// level 4:
//    high impact computationally complex/heavy checks that signify an error
//    however should be switched off usually due to significant impact on performance

#define ASSERT_LEVEL 3
#if ASSERT_LEVEL >= 0
#define _assert_level_0(expression) assert(expression)
#else
#define _assert_level_0(expression)
#endif
#if ASSERT_LEVEL >= 1
#define _assert_level_1(expression) assert(expression)
#else
#define _assert_level_1(expression)
#endif
#if ASSERT_LEVEL >= 2
#define _assert_level_2(expression) assert(expression)
#else
#define _assert_level_2(expression)
#endif
#if ASSERT_LEVEL >= 3
#define _assert_level_3(expression) assert(expression)
#else
#define _assert_level_3(expression)
#endif
#if ASSERT_LEVEL >= 4
#define _assert_level_4(expression) assert(expression)
#else
#define _assert_level_4(expression)
#endif

#endif /* assertlevels_hpp */
