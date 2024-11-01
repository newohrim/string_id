// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED
#define FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED

#include <atomic>
#include <exception>
#include <string>

#include "config.hpp"
#include "hash.hpp"

namespace foonathan { namespace string_id
{
    /// \brief The base class for all custom exception classes of this library.
    class error : public std::exception
    {
    protected:
        error() = default;
    };
    
    /// \brief The type of the collision handler.
    /// \detail It will be called when a string hashing results in a collision giving it the two strings collided.
    /// The default handler throws an exception of type \ref collision_error.
    template<typename STORAGE_T>
    using collision_handler = void(*)(STORAGE_T hash, const char *a, const char *b);
    //typedef void(*collision_handler)(hash_type hash, const char *a, const char *b);
    
    /// \brief Exchanges the \ref collision_handler.
    /// \detail This function is thread safe if \ref FOONATHAN_STRING_ID_ATOMIC_HANDLER is \c true.
    template<typename STORAGE_T>
    collision_handler<STORAGE_T> set_collision_handler(collision_handler<STORAGE_T> h);
    
    /// \brief Returns the current \ref collision_handler.
    template <typename STORAGE_T>
    collision_handler<STORAGE_T> get_collision_handler();
    
    /// \brief The exception class thrown by the default \ref collision_handler.
    template<typename STORAGE_T>
    class collision_error : public error
    {
    public:
        //=== constructor/destructor ===//
        /// \brief Creates a new exception, same parameter as \ref collision_handler.
        collision_error(STORAGE_T hash, const char* a, const char* b)
        : a_(a), b_(b),
          what_(R"(foonathan::string_id::collision_error: strings ")" + a_ + R"(" and ")" + b_ +
                R"(") are both producing the value )" + std::to_string(hash)), hash_(hash) {}
        
        ~collision_error() FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE {}
        
        //=== accessors ===//
        const char* what() const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;
        
        /// @{
        /// \brief Returns one of the two strings that colllided.
        const char* first_string() const FOONATHAN_NOEXCEPT
        {
            return a_.c_str();
        }
        
        const char* second_string() const FOONATHAN_NOEXCEPT
        {
            return b_.c_str();
        }
        /// @}
        
        /// \brief Returns the hash code of the collided strings.
        STORAGE_T hash_code() const FOONATHAN_NOEXCEPT
        {
            return hash_;
        }
        
    private:
        std::string a_, b_, what_;
        STORAGE_T hash_;
    };
    
    /// \brief The type of the generator error handler.
    /// \detail It will be called when a generator would generate a \ref string_id that already was generated.
    /// The generator will try again until the handler returns \c false in which case it just returns the old \c string_id.
    /// It passes the number of tries, the name of the generator and the hash and string of the generated \c string_id.<br>
    /// The default handler allows 8 tries and then throws an exception of type \ref generation_error.
    template<typename STORAGE_T>
    using generation_error_handler = bool (*)(std::size_t no, const char* generator_name, STORAGE_T hash, const char* str);
    
    /// \brief Exchanges the \ref generation_error_handler.
    /// \detail This function is thread safe if \ref FOONATHAN_STRING_ID_ATOMIC_HANDLER is \c true.
    template<typename STORAGE_T>
    inline generation_error_handler<STORAGE_T> set_generation_error_handler(generation_error_handler<STORAGE_T> h);
    
    /// \brief Returns the current \ref generation_error_handler.
    template<typename STORAGE_T>
    inline generation_error_handler<STORAGE_T> get_generation_error_handler();
    
    /// \brief The exception class thrown by the default \ref generation_error_handler.
    class generation_error : public error
    {
    public:
        //=== constructor/destructor ===//
        /// \brief Creates it by giving it the name of the generator.
        generation_error(const char *generator_name)
        : name_(generator_name), what_("foonathan::string_id::generation_error: Generator \"" + name_ +
                                       "\" was unable to generate new string id.") {}
        
        ~generation_error() FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE {}
        
        //=== accessors ===//
        const char* what() const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;
        
        /// \brief Returns the name of the generator.
        const char* generator_name() const FOONATHAN_NOEXCEPT
        {
            return name_.c_str();
        }
        
    private:
        std::string name_, what_;
    };

    template <typename STORAGE_T>
    inline void default_collision_handler(STORAGE_T hash, const char* a, const char* b)
    {
        throw collision_error(hash, a, b);
    }

#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    template <typename STORAGE_T>
    inline std::atomic<collision_handler<STORAGE_T>> collision_h(default_collision_handler<STORAGE_T>);
#else
    template <typename STORAGE_T>
    inline collision_handler<STORAGE_T> collision_h(default_collision_handler<STORAGE_T>);
#endif

    template<typename STORAGE_T>
    inline collision_handler<STORAGE_T> set_collision_handler(collision_handler<STORAGE_T> h)
    {
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
        return collision_h.exchange(h);
#else
        auto val    = collision_h;
        collision_h = h;
        return val;
#endif
    }

    template<typename STORAGE_T>
    inline collision_handler<STORAGE_T> get_collision_handler()
    {
        return collision_h<STORAGE_T>;
    }

    template<typename STORAGE_T>
    const char* collision_error<STORAGE_T>::what() const FOONATHAN_NOEXCEPT
    {
        try
        {
            return what_.c_str();
        }
        catch (...)
        {
            return "foonathan::string_id::collision_error: two different strings are producing the same value";
        }
    }

    FOONATHAN_CONSTEXPR inline auto no_tries_generation = 8u;

    template<typename STORAGE_T>
    inline bool default_generation_error_handler(std::size_t no, const char* generator_name, STORAGE_T, const char*)
    {
        if (no >= no_tries_generation)
            throw generation_error(generator_name);
        return true;
    }

#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
    template<typename STORAGE_T>
    inline std::atomic<generation_error_handler<STORAGE_T>> generation_error_h(default_generation_error_handler<STORAGE_T>);
#else
    template <typename STORAGE_T>
    inline sid::generation_error_handler<STORAGE_T> generation_error_h(default_generation_error_handler<STORAGE_T>);
#endif


    template<typename STORAGE_T>
    inline generation_error_handler<STORAGE_T> set_generation_error_handler(generation_error_handler<STORAGE_T> h)
    {
#if FOONATHAN_STRING_ID_ATOMIC_HANDLER
        return generation_error_h.exchange(h);
#else
        auto val = generation_error_h;
        generation_error_h = h;
        return val;
#endif
    }

    template<typename STORAGE_T>
    inline generation_error_handler<STORAGE_T> get_generation_error_handler()
    {
        return generation_error_h<STORAGE_T>;
    }
}} // namespace foonathan::string_id

#endif // FOONATHAN_STRING_ID_ERROR_HPP_INCLUDED
