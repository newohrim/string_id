// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "generator.hpp"

#include <cassert>
#include <cstring>
#include <utility>

#include "error.hpp"

namespace sid = foonathan::string_id;

namespace
{
static FOONATHAN_CONSTEXPR char table[]
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxzy0123456789";
}

sid::character_table sid::character_table::alnum() FOONATHAN_NOEXCEPT
{
    return {table, sizeof(table) - 1};
}

sid::character_table sid::character_table::alpha() FOONATHAN_NOEXCEPT
{
    return {table, sizeof(table) - 1 - 10};
}
