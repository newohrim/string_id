// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "error.hpp"

#include <sstream>

namespace sid = foonathan::string_id;

const char* sid::generation_error::what() const FOONATHAN_NOEXCEPT try
{
    return what_.c_str();
}
catch (...)
{
    return "foonathan::string_id::generation_error: unable to generate new string id.";
}
