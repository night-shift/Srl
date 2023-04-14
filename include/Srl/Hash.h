#ifndef SRL_HASH_H
#define SRL_HASH_H

#include "Common.h"
#include "Aux.h"
#include "Srl/Exception.h"

#include <cmath>
#include <deque>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace Srl { namespace Lib {

    namespace Aux {

        uint64_t hash_fnc(const uint8_t* bytes, size_t nbytes);

    }

    template<class T, class = void> struct HashSrl;

    template<> struct HashSrl<std::string> {
        uint64_t operator() (const std::string& s) const { return Aux::hash_fnc((const uint8_t*)s.data(), s.size()); }
    };


    template<class T>
    struct HashSrl<T, typename std::enable_if<std::is_convertible<T, uint64_t>::value>::type> {
        uint64_t operator() (const uint64_t& s) const { return s; }
    };

#ifdef __APPLE__
    template<> struct HashSrl<size_t> {
        size_t operator()(size_t s) const { return s; }
    };
#endif

    template <class Key, class Val, class HashFnc = HashSrl<Key>>
    class HTable {

        struct Entry;

    public:
        HTable(size_t buckets = 64, double load_factor_ = 1.0)
            : load_factor(fmax(load_factor_, 0.1)), cap(buckets) { }

        ~HTable() { destroy_all<Key, Val>(); }

        HTable(const HTable& m)             = delete;
        HTable& operator= (const HTable& m) = delete;

        HTable(HTable&& m) { *this = std::forward<HTable>(m); }

        HTable& operator= (HTable&& m);

        Val* get (const Key& key);
        /* fst -> exists? snd -> entry */
        template<class KV, class... Args>
        std::pair<bool, Val*> insert (KV&& key, Args&&... args);

        /* fst -> updated? snd -> entry */
        template<class KV, class... Args>
        std::pair<bool, Val*> upsert (KV&& key, Args&&... args);

        void foreach(const std::function<void(const Key&, Val&)>& fnc) const;

        void foreach_break(const std::function<bool(const Key&, Val&)>& fnc) const;

        void remove_if(const std::function<bool(const Key&, Val&)>& fnc);

        void remove(const Key& key);

        std::optional<Val> extract(const Key& key);

        size_t num_entries() const { return this->elements; }

        void clear();

        struct Iterator;

        Iterator begin() noexcept
        {
            return Iterator(this);
        }

        Iterator end() noexcept
        {
            return Iterator();

        }

    private:
        struct Entry {

            template<class KV, class... Args> Entry (uint64_t hash_, KV&& key_, Args&&... args)
                : hash(hash_), key(std::forward<KV>(key_)), val(std::forward<Args>(args)...) { }

            uint64_t hash;
            Entry*   next = nullptr;
            Key      key;
            Val      val;
        };

        double load_factor;
        size_t cap;

        size_t limit    = 0;
        size_t elements = 0;

        Entry** table = nullptr;
        std::vector<Entry*> mem_cache;

        HashFnc hash_fnc;
        Heap    heap;

        void     redistribute();
        uint64_t get_bucket(uint64_t hash);
        void     set_capacity(size_t new_cap);

        Entry** alloc_table(size_t sz);

        Entry* get_rm (const Key& k, uint64_t hash);

        template<class KV, class... Args>
        std::pair<bool, Val*> insert_hash (uint64_t hash, bool update_on_dup, KV&& key, Args&&... args);

        template<class T>
        typename std::enable_if<std::is_trivially_destructible<T>::value, void>::type
        destroy_item(T&) { }

        template<class T>
        typename std::enable_if<!std::is_trivially_destructible<T>::value, void>::type
        destroy_item(T&);

        template<class K, class V>
        typename std::enable_if<std::is_trivially_destructible<K>::value &&
                                std::is_trivially_destructible<V>::value, void>::type
        destroy_all() { }

        template<class K, class V>
        typename std::enable_if<!std::is_trivially_destructible<K>::value ||
                                !std::is_trivially_destructible<V>::value, void>::type
        destroy_all();
    };


} }

#endif
