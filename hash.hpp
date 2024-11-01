// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_HASH_HPP_INCLUDED
#define FOONATHAN_STRING_ID_HASH_HPP_INCLUDED

#include <cstdint>

#include "config.hpp"

namespace foonathan { namespace string_id
{
    namespace detail
    {
        constexpr uint32_t FNV_32_BASIS = 0x811c9dc5;
        constexpr uint32_t FNV_32_PRIME = 0x01000193;
        FOONATHAN_CONSTEXPR uint64_t fnv_basis = 14695981039346656037ull;
        FOONATHAN_CONSTEXPR uint64_t fnv_prime = 1099511628211ull;

        // https://ru.wikipedia.org/wiki/FNV
        template<typename T>
        FOONATHAN_CONSTEXPR_FNC T FNV1aHash(const char* buf, T hval, T prime)
        {
            while (*buf)
            {
                hval ^= (T)*buf++;
                hval *= FNV_32_PRIME;
            }

            return hval;
        }
        
        // FNV-1a 64 bit hash
        FOONATHAN_CONSTEXPR_FNC void sid_hash(const char* str, uint64_t& res, uint64_t hash = fnv_basis)
        {
            res = FNV1aHash(str, fnv_basis, fnv_prime);
        }

        // FNV-1a 32 bit hash
        FOONATHAN_CONSTEXPR_FNC void sid_hash(const char* str, uint32_t& res, uint32_t hash = FNV_32_BASIS)
        {
            res = FNV1aHash(str, FNV_32_BASIS, FNV_32_PRIME);
        }
    } // namespace detail
}} // foonathan::string_id

#endif // FOONATHAN_STRING_ID_DETAIL_HASH_HPP_INCLUDED
