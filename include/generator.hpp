// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_GENERATOR_HPP_INCLUDED
#define FOONATHAN_STRING_ID_GENERATOR_HPP_INCLUDED

#include <atomic>
#include <random>

#include "config.hpp"
#include "string_id.hpp"

namespace foonathan { namespace string_id
{
    namespace detail
    {
        template<typename STRID_T>
        bool handle_generation_error(std::size_t counter, const char* name, const STRID_T& result)
        {
            return get_generation_error_handler<typename STRID_T::STORAGE_TYPE>()(counter, name, result.hash_code(), result.string());
        }
        
        template <typename STRID_T, typename Generator>
        STRID_T try_generate(const char* name, Generator generator, const STRID_T& prefix)
        {
            insert_status status;
            auto result = STRID_T(prefix, generator(), status);
            for (std::size_t counter = 1; status != insert_status::new_string && handle_generation_error(counter, name, result); ++counter)
                result = STRID_T(prefix, generator(), status);
            return result;
        }
    }
    
    /// \brief A generator that generates string ids with a prefix followed by a number.
    /// \detail It can be used by multiple threads at the same time.
    template<typename STRID_T>
    class counter_generator
    {
    public:
        /// \brief The type of the internal state, an unsigned integer.
        typedef unsigned long long state;
        
        /// \brief Creates a new generator with given prefix.
        /// \arg \c counter is the start value for the counter.
        /// \arg \c length is the length of the number appended.If it is \c 0,
        /// there are no restrictions. Else it will either prepend zeros to the number
        /// or cut the number to \c length digits. 
        /// \note For implementation reasons, \c length can't be higher than a certain value.
        /// If it is, it behaves as if \c length has this certain value.
        explicit counter_generator(const STRID_T& prefix,
                                   state counter = 0, std::size_t length = 0)
        : prefix_(prefix), counter_(counter), length_(length) {}
        
        /// \brief Generates a new \ref string_id.
        /// \detail If it was already generated previously, the \ref generator_error_handler will be called in a loop as described there.
        STRID_T operator()();
        
        /// \brief Discards a number of states by advancing the counter.
        void discard(unsigned long long n) FOONATHAN_NOEXCEPT;
        
    private:
        STRID_T prefix_;
        std::atomic<state> counter_;
        std::size_t length_;
    };
    
    /// \brief Information about the characters used by \ref random_generator.
    struct character_table
    {
        /// \brief A pointer to an array of characters.
        const char* characters;
        /// \brief The length of it.
        std::size_t no_characters;
        
        /// \brief Creates a new table.
        character_table(const char* chars, std::size_t no) FOONATHAN_NOEXCEPT
        : characters(chars), no_characters(no) {}
        
        /// \brief A table with all English letters (both cases) and digits.
        static character_table alnum() FOONATHAN_NOEXCEPT;
        
        /// \brief A table with all English letters (both cases).
        static character_table alpha() FOONATHAN_NOEXCEPT;
    };
    
    /// \brief A generator that generates string ids by appendending random characters to a prefix.
    /// \detail This class is thread safe if the random number generator is thread safe.
    template <typename STRID_T, class RandomNumberGenerator, std::size_t Length>
    class random_generator
    {        
    public:        
        /// \brief The state of generator, a random number generator like \c std::mt19937.
        typedef RandomNumberGenerator state;
        
        /// \brief The number of characters appended.
        static FOONATHAN_CONSTEXPR_FNC std::size_t length() FOONATHAN_NOEXCEPT
        {
            return Length;
        }
        
        /// \brief Creates a new generator with given prefix, random number generator and character table.
        /// \detail By default is uses the table \ref character_table::alnum().
        explicit random_generator(const STRID_T& prefix,
                                  state s = state(),
                                  character_table table = character_table::alnum())
        : prefix_(prefix), state_(std::move(s)),
          table_(table) {}
        
        /// \brief Generates a new \ref string_id.
        /// \detail If it was already generated previously, the \ref generator_error_handler will be called in a loop as described there.
        STRID_T operator()()
        {
            std::uniform_int_distribution<std::size_t>
                dist(0, table_.no_characters - 1);
            char random[Length];
            return detail::try_generate("foonathan::string_id::random_generator",
                    [&]()
                    {
                        for (std::size_t i = 0u; i != Length; ++i)
                            random[i] = table_.characters[dist(state_)];
                        return string_info(random, Length);
                    }, prefix_);
        }
        
        /// \brief Discards a certain number of states, this forwards to the random number generator.
        void discard(unsigned long long n)
        {
            state_.discard(n);
        }
        
    private:
        STRID_T prefix_;
        state state_;
        character_table table_;
    };

    template <typename STRID_T>
    string_info to_string(typename counter_generator<STRID_T>::state s, char* begin, char* end,
                               std::size_t length)
    {
        auto cur = end;
        std::size_t i = 0;

        do
        {
            *--cur = '0' + (s % 10);
            s /= 10;
            ++i;
        } while (s != 0u);

        if (i < length)
            for (; cur - 1 != begin && i < length; ++i)
                *--cur = '0';
        else if (length && i > length)
            cur += i - length;

        return string_info(cur, end - cur);
    }

    template<typename STRID_T>
    STRID_T counter_generator<STRID_T>::operator()()
    {
        // 4 times sizeof(state) is enough for the integer representation
        static FOONATHAN_CONSTEXPR auto max_size = 4 * sizeof(state);
        char string[max_size];
        return detail::try_generate(
            "foonathan::string_id::counter_generator",
            [&]() { return to_string<STRID_T>(counter_++, string, string + max_size, length_); }, prefix_);
    }

    template<typename STRID_T>
    void counter_generator<STRID_T>::discard(unsigned long long n) FOONATHAN_NOEXCEPT
    {
        counter_ += n;
    }
    
}} // namespace foonathan::string_id

#endif // FOONATHAN_STRING_ID_GENERATOR_HPP_INCLUDED
