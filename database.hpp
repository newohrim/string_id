// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED
#define FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED

#include <cmath>
#include <memory>
#include <mutex>
#include <string>

#include "basic_database.hpp"
#include "config.hpp"

namespace foonathan { namespace string_id
{    
    /// \brief A database that doesn't store the string-values.
    /// \detail It does not detect collisions or allows retrieving,
    /// \c lookup() returns "string_id database disabled".
    template<typename STORAGE_T>
    class dummy_database : public basic_database<STORAGE_T>
    {
    public:        
        insert_status insert(STORAGE_T, const char*, std::size_t) FOONATHAN_OVERRIDE
        {
            return insert_status::new_string;
        }
        
        insert_status insert_prefix(STORAGE_T, STORAGE_T, const char*, std::size_t) FOONATHAN_OVERRIDE
        {
            return insert_status::new_string;
        }
        
        const char* lookup(STORAGE_T) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE
        {
            return "string_id database disabled";
        }
    };
    
    /// \brief A database that uses a highly optimized hash table.
    template<typename STORAGE_T>
    class map_database : public basic_database<STORAGE_T>
    {
    public:        
        /// \brief Creates a new database with given number of buckets and maximum load factor.
        explicit map_database(std::size_t size = 1024, double max_load_factor = 1.0);
        ~map_database() FOONATHAN_NOEXCEPT;
        
        insert_status insert(STORAGE_T hash, const char* str, std::size_t length) FOONATHAN_OVERRIDE;
        insert_status insert_prefix(STORAGE_T hash, STORAGE_T prefix, const char* str, std::size_t length) FOONATHAN_OVERRIDE;
        const char* lookup(STORAGE_T hash) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;
        
    private:        
        void rehash();
        
        class node_list;
        std::unique_ptr<node_list[]> buckets_;
        std::size_t no_items_, no_buckets_;
        double max_load_factor_;
        std::size_t next_resize_;
    };
    
    /// \brief A thread-safe database adapter.
    /// \detail It derives from any database type and synchronizes access via \c std::mutex.
    template <class Database>
    class thread_safe_database : public Database
    {
    public:
        /// \brief The base database.
        typedef Database base_database;
        
        // workaround of lacking inheriting constructors
		template <typename ... Args>
        explicit thread_safe_database(Args&&... args)
		: base_database(std::forward<Args>(args)...) {}
        
        insert_status insert(base_database::STORAGE_TYPE hash, const char* str, std::size_t length) FOONATHAN_OVERRIDE
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return Database::insert(hash, str, length);
        }
        
        insert_status insert_prefix(base_database::STORAGE_TYPE hash, base_database::STORAGE_TYPE prefix, const char* str, std::size_t length) FOONATHAN_OVERRIDE
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return Database::insert_prefix(hash, prefix, str, length);
        }
        
        const char* lookup(base_database::STORAGE_TYPE hash) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return Database::lookup(hash);
        }
        
    private:
        mutable std::mutex mutex_;
    };

    /// \cond impl
    template <typename STORAGE_T>
    class map_database<STORAGE_T>::node_list
    {
        struct node
        {
            std::size_t length; // length of string
            STORAGE_T   hash;
            node*       next;

            node(const char* str, std::size_t length, STORAGE_T h, node* next) FOONATHAN_NOEXCEPT
            : length(length),
              hash(h),
              next(next)
            {
                void* mem  = this;
                auto  dest = static_cast<char*>(mem) + sizeof(node);
                strncpy(dest, str, length);
                dest[length] = 0;
            }

            node(const char* prefix, std::size_t length_prefix, const char* str,
                 std::size_t length_string, STORAGE_T h, node* next) FOONATHAN_NOEXCEPT
            : length(length_prefix + length_string),
              hash(h),
              next(next)
            {
                void* mem  = this;
                auto  dest = static_cast<char*>(mem) + sizeof(node);
                strncpy(dest, prefix, length_prefix);
                dest += length_prefix;
                strncpy(dest, str, length_string);
                dest[length_string] = 0;
            }

            const char* get_str() const FOONATHAN_NOEXCEPT
            {
                const void* mem = this;
                return static_cast<const char*>(mem) + sizeof(node);
            }
        };

        struct insert_pos_t
        {
            bool exists; // if true: cur set, else: prev and next set

            node*& prev;
            node*  next;

            node* cur;

            insert_pos_t(node*& prev, node* next) FOONATHAN_NOEXCEPT 
                : exists(false),
                  prev(prev),
                  next(next),
                  cur(nullptr)
            {}

            insert_pos_t(node* cur) FOONATHAN_NOEXCEPT 
                : exists(true),
                  prev(this->cur),
                  next(nullptr),
                  cur(cur)
            {}
        };

    public:
        node_list() FOONATHAN_NOEXCEPT : head_(nullptr) {}

        ~node_list() FOONATHAN_NOEXCEPT
        {
            auto cur = head_;
            while (cur)
            {
                auto next = cur->next;
                cur->~node();
                // researched: no memory leak is expected here
                // https://stackoverflow.com/questions/39648970/does-operator-delete-void-know-the-size-of-memory-allocated-with-operat
                // https://stackoverflow.com/questions/6783993/placement-new-and-delete
                ::operator delete(cur);
                cur = next;
            }
        }

        insert_status insert(STORAGE_T hash, const char* str, std::size_t length)
        {
            auto pos = insert_pos(hash);
            if (pos.exists)
                return std::strncmp(str, pos.cur->get_str(), length) == 0 ? insert_status::old_string
                                                                          : insert_status::collision;
            auto mem = ::operator new(sizeof(node) + length + 1);
            auto n   = ::new (mem) node(str, length, hash, pos.next);
            pos.prev = n;
            return insert_status::new_string;
        }

        insert_status insert_prefix(node_list& prefix_bucket, STORAGE_T prefix, STORAGE_T hash,
                                         const char* str, std::size_t length)
        {
            auto prefix_node = prefix_bucket.find_node(prefix);
            auto pos         = insert_pos(hash);
            if (pos.exists)
                return strequal(prefix_node->get_str(), str, length, pos.cur->get_str())
                           ? insert_status::old_string
                           : insert_status::collision;
            auto mem = ::operator new(sizeof(node) + prefix_node->length + length + 1);
            auto n   = ::new (mem) node(prefix_node->get_str(), prefix_node->length, str, length, hash, pos.next);
            pos.prev = n;
            return insert_status::new_string;
        }

        // inserts all nodes into new buckets, this list is empty afterwards
        void rehash(node_list* buckets, std::size_t size) FOONATHAN_NOEXCEPT
        {
            auto cur = head_;
            while (cur)
            {
                auto next = cur->next;
                auto pos  = buckets[cur->hash % size].insert_pos(cur->hash);
                assert(!pos.exists && "element can't be there already");
                pos.prev  = cur;
                cur->next = pos.next;
                cur       = next;
            }
            head_ = nullptr;
        }

        // returns element with hash, there must be one
        const char* lookup(STORAGE_T h) const FOONATHAN_NOEXCEPT
        {
            return find_node(h)->get_str();
        }

    private:
        node* find_node(STORAGE_T h) const FOONATHAN_NOEXCEPT
        {
            assert(head_ && "hash not inserted");
            auto cur = head_;
            while (cur->hash < h)
            {
                cur = cur->next;
                assert(cur && "hash not inserted");
            }
            assert(cur->hash == h && "hash not inserted");
            return cur;
        }

        insert_pos_t insert_pos(STORAGE_T hash) FOONATHAN_NOEXCEPT
        {
            node *cur = head_, *prev = nullptr;
            while (cur && cur->hash <= hash)
            {
                if (cur->hash < hash)
                {
                    prev = cur;
                    cur  = cur->next;
                }
                else if (cur->hash == hash)
                    return {cur};
            }
            return {prev ? prev->next : head_, cur};
        }

        node* head_;
    };
    /// \endcond

    template<typename STORAGE_T>
    map_database<STORAGE_T>::map_database(std::size_t size, double max_load_factor)
    : buckets_(new node_list[size]), no_items_(0u), no_buckets_(size),
      max_load_factor_(max_load_factor),
      next_resize_(static_cast<std::size_t>(std::floor(no_buckets_ * max_load_factor_)))
    {}

    template <typename STORAGE_T>
    map_database<STORAGE_T>::~map_database() FOONATHAN_NOEXCEPT {}

    template <typename STORAGE_T>
    insert_status map_database<STORAGE_T>::insert(STORAGE_T hash, const char* str, std::size_t length)
    {
        if (no_items_ + 1 >= next_resize_)
            rehash();
        auto status = buckets_[hash % no_buckets_].insert(hash, str, length);
        if (status == insert_status::new_string)
            ++no_items_;
        return status;
    }

    template <typename STORAGE_T>
    insert_status map_database<STORAGE_T>::insert_prefix(STORAGE_T hash, STORAGE_T prefix, const char* str, std::size_t length)
    {
        if (no_items_ + 1 >= next_resize_)
            rehash();
        auto status = buckets_[hash % no_buckets_].insert_prefix(buckets_[prefix % no_buckets_], prefix, hash, str, length);
        if (status == insert_status::new_string)
            ++no_items_;
        return status;
    }

    template <typename STORAGE_T>
    const char* map_database<STORAGE_T>::lookup(STORAGE_T hash) const FOONATHAN_NOEXCEPT
    {
        return buckets_[hash % no_buckets_].lookup(hash);
    }

    template <typename STORAGE_T>
    void map_database<STORAGE_T>::rehash()
    {
        static FOONATHAN_CONSTEXPR auto growth_factor = 2;
        auto new_size = growth_factor * no_buckets_;
        auto buckets = new node_list[new_size]();
        auto end = buckets_.get() + no_buckets_;
        for (auto list = buckets_.get(); list != end; ++list)
            list->rehash(buckets, new_size);
        buckets_.reset(buckets);
        no_buckets_  = new_size;
        next_resize_ = static_cast<std::size_t>(std::floor(no_buckets_ * max_load_factor_));
    }
    
    /// \brief The default database where the strings are stored.
    /// \detail Its exact type is one of the previous listed databases.
    /// You can control its selection via the macros listed in config.hpp.in.
#if FOONATHAN_STRING_ID_DATABASE && FOONATHAN_STRING_ID_MULTITHREADED
    typedef thread_safe_database<map_database<uint32_t>> default_database;
#elif FOONATHAN_STRING_ID_DATABASE
    typedef map_database<uint32_t> default_database;
#else
    typedef dummy_database<uint32_t> default_database;
#endif
}} // namespace foonathan::string_id

#endif // FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED
