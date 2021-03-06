//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <string>

// size_type max_size() const;

#include <string>
#include <cassert>
#include <crossbow/string.hpp>
using namespace crossbow;

#include "min_allocator.h"

template <class S>
void
test1(const S &s) {
    S s2(s);
    const size_t sz = s2.max_size() - 1;
    try {
        s2.resize(sz, 'x');
    } catch (const std::bad_alloc &) {
        return ;
    } catch (const std::length_error &) {
        return;
    }
    assert(s2.size() ==  sz);
}

template <class S>
void
test2(const S &s) {
    S s2(s);
    const size_t sz = s2.max_size();
    try {
        s2.resize(sz, 'x');
    } catch (const std::bad_alloc &) {
        return ;
    }
    assert(s.size() ==  sz);
}

template <class S>
void
test3(const S &s) {
    S s2(s);
    const size_t sz = s2.max_size() + 1;
    try {
        s2.resize(sz, 'x');
    } catch (const std::length_error &) {
        return ;
    }
    assert(false);
}

template <class S>
void
test(const S &s) {
    assert(s.max_size() >= s.size());
    test1(s);
    test2(s);
    test3(s);
}

int main() {
    {
        typedef string S;
        test(S());
        test(S("123"));
        test(S("12345678901234567890123456789012345678901234567890"));
    }
#if __cplusplus >= 201103L
    {
        typedef basic_string<char, std::char_traits<char>, min_allocator<char>> S;
        test(S());
        test(S("123"));
        test(S("12345678901234567890123456789012345678901234567890"));
    }
#endif
}
