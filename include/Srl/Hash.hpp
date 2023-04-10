#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"

namespace Srl { namespace Lib {

    template<class K, class V, class H>
    uint64_t HTable<K, V, H>::get_bucket(uint64_t hash)
    {
        /* mix in the upper half bits of the hash unless table capacity is bigger 2 ^ (bits in word / 2) - 1 */
        const auto shift = sizeof(uint64_t) * 4;
        const auto lim   = ~(uint64_t)0 >> shift;

        return (this->cap <= lim ? hash ^ hash >> shift : hash) & (this->cap - 1);
    }

    template<class K, class V, class H>
    srl_noinline void HTable<K, V, H>::redistribute()
    {
        if(this->limit == 0) {
            /* initial allocation */
            this->table = this->alloc_table();
            this->limit = cap * load_factor;

            return;
        }

        auto old_dim = this->cap;
        this->cap *= 2;
        this->limit = cap * load_factor;

        Entry** ntable = this->alloc_table();

        for(auto i = 0U; i < old_dim; i++) {
            auto* entry = table[i];
            while(entry) {
                auto bucket = get_bucket(entry->hash);
                auto* slot = ntable[bucket];

                if(!slot) {
                    ntable[bucket] = entry;

                } else while(true) {
                    if(!slot->next) {
                        slot->next = entry;
                        break;
                    }
                    slot = slot->next;
                }

                auto* tmp = entry;
                entry = entry->next;
                tmp->next = nullptr;
            }
        }

        this->heap.put_mem((uint8_t*)this->table, old_dim * sizeof(Entry*));
        this->table = ntable;
    }

    template<class K, class V, class H>
    V* HTable<K, V, H>::get(const K& key)
    {
        if(srl_unlikely(elements < 1)) {
            return nullptr;
        }

        auto hash    = hash_fnc(key);
        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];

        while(entry) {
            if(entry->hash == hash && entry->key == key) {
                return &entry->val;

            } else {
                entry = entry->next;
            }
        }

        return nullptr;
    }

    template<class K, class V, class H>
    typename HTable<K, V, H>::Entry* HTable<K, V ,H>::get_rm(const K& key, uint64_t hash)
    {
        if(this->elements < 1) {
            return nullptr;
        }

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* prev  = nullptr;

        while(entry) {

            if(entry->hash == hash && entry->key == key) {
                if(prev) {
                    prev->next = entry->next;

                } else if(table[bucket] == entry) {
                    table[bucket] = entry->next;
                }

                return entry;

            }

            prev  = entry;
            entry = entry->next;
        }

        return nullptr;
    }

    template<class K, class V, class H> template<class KV, class... Args>
    std::pair<bool, V*> HTable<K, V, H>::insert(KV&& key, Args&&... args)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, std::forward<KV>(key), std::forward<Args>(args)...);
    }

    template<class K, class V, class H> template<class KV, class... Args>
    std::pair<bool, V*> HTable<K, V, H>::insert_hash(uint64_t hash, KV&& key, Args&&... args)
    {
        if(srl_unlikely(this->elements >= this->limit)) {
            this->redistribute();
        }

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* node  = nullptr;

        while(entry) {

            if(entry->hash == hash && entry->key == key) {
                return { true, &entry->val };
            }

            node = entry;
            entry = entry->next;
        }

        this->elements++;
        auto* new_entry = this->heap.template create<Entry>(hash, std::forward<KV>(key), std::forward<Args>(args)...);

        if(node) {
            node->next = new_entry;
        } else {
            table[bucket] = new_entry;
        }


        return { false, &(new_entry->val) };
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::clear()
    {
        if(this->elements < 1) {
            return;
        }

        this->destroy_all<K, V>();
        this->heap.clear();
        this->elements = 0;
        this->limit = 0;
    }

    template<class K, class V, class H>
    HTable<K, V, H>& HTable<K, V, H>::operator= (HTable<K, V, H>&& m)
    {
        this->destroy_all<K, V>();

        this->limit    = m.limit;
        this->elements = m.elements;

        m.limit    = 0;
        m.elements = 0;

        this->cap         = m.cap;
        this->load_factor = m.load_factor;

        this->heap  = std::move(m.heap);
        this->table = m.table;

        return *this;
    }

    template<class K, class V, class H>
    typename HTable<K, V, H>::Entry**  HTable<K, V, H>::alloc_table()
    {
        Entry** tbl = this->heap.template get_mem<Entry*>(this->cap);
        memset(tbl, 0, this->cap * sizeof(Entry*));
        return tbl;
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::foreach_break(const std::function<bool(const K&, V&)>& fnc) const
    {
        if(this->elements < 1) {
            return;
        }

        auto n = 0U;
        bool abort = false;

        for(auto i = 0U; i < this->cap; i++) {
            auto* entry = table[i];

            while(entry && !abort) {
                abort = fnc(entry->key, entry->val);
                entry = entry->next;
                n++;
            }

            if(n >= this->elements || abort) {
                break;
            }
        }
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::foreach(const std::function<void(const K&, V&)>& fnc) const
    {
        this->foreach_break([&fnc](const K& key, V& val)
        {
            fnc(key, val);
            return false;
        });
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::remove(const K& key)
    {
        auto hash   = hash_fnc(key);
        auto* entry = get_rm(key, hash);

        if(entry) {
            destroy_item<K>(entry->key);
            destroy_item<V>(entry->val);
            heap.put_mem((uint8_t*)entry, sizeof(Entry));
            elements--;
        }
    }

    template<class K, class V, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HTable<K, V, H>::destroy_item(T& itm)
    {
        itm.~T();
    }

    template<class K, class V, class H> template<class Key, class Val>
    typename std::enable_if<!std::is_trivially_destructible<Key>::value ||
                            !std::is_trivially_destructible<Val>::value, void>::type

    HTable<K, V, H>::destroy_all()
    {
        foreach([this](const Key& key, Val& val)
        {
            destroy_item(key);
            destroy_item(val);
        });
    }


    template<class K, class V, class H>
    struct HTable<K, V, H>::Iterator {

        Iterator() { }

        Iterator(HTable<K, V, H>* htbl_) noexcept : htbl(htbl_)
        {
            advance();
        }

        bool operator!=(const Iterator& o) const noexcept
        {
            return this->entry != o.entry;
        }

        Iterator& operator++() noexcept
        {
            if(entry) {
                advance();
            }

            return *this;
        }

        std::pair<const K&, V&> operator*() const
        {
            if(!entry) {
                throw Srl::Exception("access iterator out of bounds");
            }

            return { entry->key, entry->val };
        }

    private:
        HTable* htbl         = nullptr;
        HTable::Entry* entry = nullptr;
        size_t  bucket       = 0;

        void advance() noexcept
        {
            if(!htbl || !htbl->table) {
                return;
            }

            if(entry && entry->next) {
                entry = entry->next;
                return;
            }

            if(entry) {
                bucket += 1;
                entry = nullptr;
            }

            for(; bucket < htbl->cap; bucket++) {

                auto* ptr = htbl->table[bucket];

                if(ptr) {
                    this->entry = ptr;
                    break;
                }
            }
        }
    };

} }

#endif
