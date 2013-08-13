#ifndef SRL_HASH_HPP
#define SRL_HASH_HPP

#include "Hash.h"
#include "Aux.h"

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
    std::pair<bool, V*> HTable<K, V, H>::insert(const K& key, const V& val)
    {
        auto hash = hash_fnc(key);
        return insert_hash(hash, val);
    }

    template<class K, class V, class H>
    std::pair<bool, V*> HTable<K, V, H>::insert_hash(size_t hash, const V& val)
    {
        if(srl_unlikely(this->elements >= this->limit)) {
            this->redistribute();
        }

        auto bucket  = get_bucket(hash);
        Entry* entry = table[bucket];
        Entry* node  = nullptr;

        while(entry) {

            if(entry->hash == hash)
                return { true, &entry->val };

            node = entry;
            entry = entry->next;
        }

        this->elements++;

        auto& slot = node ? node->next : table[bucket];
        slot = this->heap.template create<Entry>(hash, val);

        return { false, &slot->val };
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::clear()
    {
        this->destroy<V>();
        this->heap.clear();
        this->elements = 0;
        this->table = alloc_table();
    }

    template<class K, class V, class H>
    typename HTable<K, V, H>::Entry**  HTable<K, V, H>::alloc_table()
    {
        Entry** tbl = this->heap.template get_mem<Entry*>(this->cap);
        memset(tbl, 0, this->cap * sizeof(Entry*));
        return tbl;
    }

    template<class K, class V, class H>
    void HTable<K, V, H>::foreach_entry(const std::function<void(size_t, V&)>& fnc)
    {
        if(this->elements < 1) {
            return;
        }

        for(auto i = 0U; i < this->cap; i++) {
            auto* entry = table[i];
            while(entry) {
                fnc(entry->hash, entry->val);
                entry = entry->next;
            }
        }
    }

    template<class K, class V, class H> template<class T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
    HTable<K, V, H>::destroy()
    {
        foreach_entry([](size_t, V& val) { val.~V(); });
    }

} }

#endif
