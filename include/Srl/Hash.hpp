#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"

namespace Srl { namespace Lib {

    template<class K, class V, class H>
    size_t HTable<K, V, H>::get_bucket(size_t hash)
    {
        /* mix in the upper half bits of the hash unless table capacity is bigger 2 ^ (bits in word / 2) - 1 */
        const auto shift = sizeof(size_t) * 4;
        const auto lim   = ~(size_t)0 >> shift;

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
            if(entry->hash == hash) {
                return &entry->val;

            } else {
                entry = entry->next;
            }
        }

        return nullptr;
    }

    template<class K, class V, class H>
    typename HTable<K, V, H>::Entry* HTable<K, V ,H>::get_rm(size_t hash)
    {
        if(this->elements < 1) {
            return nullptr;
        }

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* prev  = nullptr;

        while(entry) {

            if(entry->hash == hash) {
                if(prev) {
                    prev->next = entry->next;

                } else if(table[bucket] == entry) {
                    table[bucket] = entry->next;
                }

                return entry;

            } else {
                prev = entry;
                entry = entry->next;
            }
        }

        return nullptr;
    }

    template<class K, class V, class H> template<class... Args>
    std::pair<bool, V*> HTable<K, V, H>::insert(const K& key, Args&&... args)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, std::forward<Args>(args)...);
    }

    template<class K, class V, class H> template<class... Args>
    std::pair<bool, V*> HTable<K, V, H>::insert_hash(size_t hash, Args&&... args)
    {
        if(srl_unlikely(this->elements >= this->limit)) {
            this->redistribute();
        }

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* node  = nullptr;

        while(entry) {

            if(entry->hash == hash) {
                return { true, &entry->val };
            }

            node = entry;
            entry = entry->next;
        }

        this->elements++;

        auto& slot = node ? node->next : table[bucket];
        slot = this->heap.template create<Entry>(hash, std::forward<Args>(args)...);

        return { false, &slot->val };
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::clear()
    {
        if(this->elements < 1) {
            return;
        }

        this->destroy<V>();
        this->heap.clear();
        this->elements = 0;
        this->limit = 0;
    }

    template<class K, class V, class H>
    HTable<K, V, H>& HTable<K, V, H>::operator= (HTable<K, V, H>&& m)
    {
        this->destroy<V>();

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
    void HTable<K, V, H>::foreach_entry_cont(const std::function<bool(size_t, V&)>& fnc)
    {
        if(this->elements < 1) {
            return;
        }

        auto n = 0U;
        bool abort = false;

        for(auto i = 0U; i < this->cap; i++) {
            auto* entry = table[i];

            while(entry && !abort) {
                abort = fnc(entry->hash, entry->val);
                entry = entry->next;
                n++;
            }

            if(n >= this->elements || abort) {
                break;
            }
        }
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::foreach_entry(const std::function<void(size_t, V&)>& fnc)
    {
        this->foreach_entry_cont([&fnc](size_t h, V& val)
        {
            fnc(h, val);
            return false;
        });
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::remove(const K& key)
    {
        auto hash   = hash_fnc(key);
        auto* entry = get_rm(hash);

        if(entry) {
            destroy<V>(entry->val);
            heap.put_mem((uint8_t*)entry, sizeof(Entry));
            elements--;
        }
    }

    template<class K, class V, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HTable<K, V, H>::destroy(V& val)
    {
        val.~V();
    }

    template<class K, class V, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HTable<K, V, H>::destroy()
    {
        foreach_entry([](size_t, V& val) { val.~V(); });
    }

} }

#endif
