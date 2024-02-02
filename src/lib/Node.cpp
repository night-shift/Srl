#include "Srl/Srl.h"
#include "Srl/Lib.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    int MAX_SAFE_NESTED_SCOPE_DEPTH = 128;

    template<class T> const string& tp_name();
    template<> const string& tp_name<Node>()  { static const string str = "Node";  return str; }
    template<> const string& tp_name<Value>() { static const string str = "Value"; return str; }

    typedef uint8_t Option;

    const Option None            = 1 << 0;
    const Option Hash            = 1 << 1; /* compare by name hash */
    const Option Address         = 1 << 2; /* compare by address */
    const Option Remove          = 1 << 3; /* remove found link */
    const Option Throw           = 1 << 4; /* throw exception on not found or multiple findings */
    const Option Check_Duplicate = 1 << 5; /* check for multiple results */
    const Option Name            = 1 << 6; /* compare by name */


    constexpr bool enabled(Option options, Option option)
    {
        return (options & option) > 0;
    }

    Exception field_name_not_found_ex(const String& field_name, const String& node_name)
    {
        return Exception("Field <" + field_name.unwrap(false) + "> in <" + node_name.unwrap(false) + "> not found.");
    }

    template<class T>
    using Cont = Lib::Items<T>;

    template<class T>
    using Itr = typename Cont<T>::iterator;

    template<Option Opt, class T> typename enable_if<enabled(Opt, Hash), bool>::type
    compare(Itr<T>& itr, size_t hash) { return itr->hash == hash; }

    template<Option Opt, class T> typename enable_if<enabled(Opt, Name), bool>::type
    compare(Itr<T>& itr, const String& name) { return itr->field.name() == name; }

    template<Option Opt, class T> typename enable_if<enabled(Opt, Name) && enabled(Opt, Name), bool>::type
    compare(Itr<T>& itr, const pair<uint64_t, const String*>& hash_name)
    {
        return itr->hash == hash_name.first &&
               itr->field.name() == *hash_name.second;
    }

    template<Option Opt, class T> typename enable_if<enabled(Opt, Address), bool>::type
    compare(Itr<T>& itr, T* pointer) { return &itr->field == pointer; }

    template<Option Opt,  class T>
    Link<T>* get_link_at_index(size_t index, Cont<T>& links)
    {
        if(index < links.size()) {
            auto itr = links.begin();
            advance(itr, index);
            auto* rslt = &*itr;
            if(enabled(Opt, Remove)) {
                links.erase(itr);
                return nullptr;
            }
            return rslt;

        } else {
            if(enabled(Opt, Throw)) {
                auto msg = "Cannot access <" + tp_name<T>() + "> at index <" + to_string(index) + ">, index out of bounds.";
                throw Exception(msg);
            }
            return nullptr;
        }
    }

    template<Option Opt, class TKey, class T>
    Link<T>* find_link(TKey key, Cont<T>& links, const String& field_name = Environment::EmptyString,
                                                 const String& node_name = Environment::EmptyString)
    {
        Itr<T> end    = links.end();
        Link<T>* rslt = nullptr;

        for(auto itr = links.begin(); itr != end; itr++) {
            if(compare<Opt, T>(itr, key)) {

                if(enabled(Opt, Throw) && rslt) {
                    auto msg = tp_name<T>() + " duplication: <" + field_name.unwrap(false) + "> in <" + node_name.unwrap(false) + ">";
                    throw Exception(msg);
                }

                rslt = &*itr;

                if(enabled(Opt, Remove)) {
                    links.erase(itr);
                    rslt = nullptr;
                    break;
                }

                if(!enabled(Opt, Check_Duplicate)) {
                    break;
                }
            }
        }

        if(enabled(Opt, Throw) && !rslt) {
            throw field_name_not_found_ex(field_name, node_name);
        }

        return rslt;
    }

    uint64_t hash_string(const String& str, Environment& env)
    {
        return str.encoding() == Environment::Str_Encoding
            ? Aux::hash_fnc(str.data(), str.size())
            : env.hash_string(str);
    }

    template<class T>
    Itr<T> find_link_iterator(const String& name, Cont<T>& links, Environment& env)
    {
        auto hash = hash_string(name, env);

        for(auto itr = links.begin(), end = links.end(); itr != end; itr++) {

            if(compare<Hash, T>(itr, hash) && compare<Name, T>(itr, name)) {
                return itr;

            }
        }

        return links.end();
    }

    bool compare(const String& a, const MemBlock& b)
    {
        return a.size() == b.size && memcmp(a.data(), b.ptr, b.size) == 0;
    }
}

void Node::insert_value(const Value& new_value, const String& name_)
{
    if(this->env->parsing) {
        this->env->write(new_value, name_);

    } else {
        this->env->store_value(*this, new_value, name_);
    }
}

Node& Node::insert_node(const Node& new_node, const String& name_)
{
    return this->env->store_node(*this, new_node, name_)->field;
}

Node& Node::insert_node(Type node_type, const String& name_)
{
    return this->insert_node(Node(this->env->tree, node_type), name_);
}

Value& Node::value(const String& name_)
{
    auto hash  = hash_string(name_, *this->env);
    auto hash_name = make_pair(hash, &name_);
    auto* link = find_link<Throw | Hash | Name>(hash_name, this->values, name_, this->name());

    return link->field;
}

Value& Node::value(size_t index)
{
    auto* link = get_link_at_index<Throw>(index, this->values);
    return link->field;
}

Node& Node::node(size_t index)
{
    auto* link = get_link_at_index<Throw>(index, this->nodes);
    return link->field;
}

Node& Node::node(const String& name_)
{
    auto hash = hash_string(name_, *this->env);
    auto hash_name = make_pair(hash, &name_);
    auto* link = find_link<Throw | Hash | Name>(hash_name, this->nodes, name_, this->name());

    return link->field;
}


optional<Union> Node::try_get(const String& name_)
{
    try {
        auto res = this->get(name_);
        return { res };

    } catch(...) {
        return { };
    }
}

optional<Union> Node::try_get(size_t index)
{
    try {
        auto res = this->get(index);
        return { res };

    } catch(...) {
        return { };
    }
}

Union Node::get(const String& name_)
{
    auto* resvalue = find_link<Name>(name_, this->values, name_, this->name());

    if(resvalue) {
        return Union(resvalue->field);
    }

    auto* resnode = find_link<Throw | Name>(name_, this->nodes, name_, this->name());

    return Union(resnode->field);
}

Union Node::get(size_t index)
{
    auto* resnode = get_link_at_index<None>(index, this->nodes);

    if(resnode) {
        return Union(resnode->field);
    }

    auto* resvalue = get_link_at_index<None>(index, this->values);

    return Union(resvalue->field);
}

void Node::read_source(int scope_depth)
{
    auto& parser = *this->env->parser;
    auto& source = this->env->in;

    while(true) {

        MemBlock seg_name; Value val;
        tie(seg_name, val) = parser.read(source);

        if(val.type() == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        auto field_name = String(seg_name, Encoding::UTF8);

        if(!TpTools::is_scope(val.type())) {
            this->env->store_value(*this, val, field_name);

        } else {

            if(scope_depth + 1 > MAX_SAFE_NESTED_SCOPE_DEPTH) {
                throw Exception("Abort parsing data MAX_SAFE_NESTED_SCOPE_DEPTH [" + to_string(MAX_SAFE_NESTED_SCOPE_DEPTH) + "] exceeded");
            }

            auto* link = this->env->store_node(*this, Node(this->env->tree, val.type()), field_name);

            link->field.read_source(scope_depth + 1);
        }
    }
}

Union Node::consume_item(const String& id, bool throw_err)
{
    auto* storednode = find_link<Name>(id, this->nodes);

    if(storednode) {
        return Union(storednode->field);
    }

    auto* storedvalue = find_link<Name>(id, this->values);

    if(storedvalue) {
        return Union(storedvalue->field);
    }

    auto hash = hash_string(id, *this->env);

    while(!this->parsed) {

        MemBlock seg_name; Value val;
        tie(seg_name, val) = this->env->parser->read(this->env->in);

        auto tp = val.type();

        if(tp == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        if(TpTools::is_scope(tp)) {
            auto* link = this->env->store_node(*this, Node(this->env->tree, tp), seg_name);
            link->field.read_source();
            if(link->hash == hash && link->field.name() == id) {
                return Union(link->field);
            }

        } else {
            auto* link = this->env->store_value(*this, val, seg_name);
            if(link->hash == hash && link->field.name() == id) {
                return Union(link->field);
            }
        }
    }

    if(throw_err) {
        throw field_name_not_found_ex(id, this->name());
    }

    return Union();
}

Node Node::consume_node(bool throw_ex, const String& id)
{
    auto stored_itr = find_link_iterator(id, this->nodes, *this->env);

    if(stored_itr != this->nodes.end()) {
        auto stored_field = move(stored_itr->field);
        this->nodes.erase(stored_itr);
        return stored_field;
    }

    auto id_conv = this->env->conv_string(id);

    while(!this->parsed) {

        MemBlock seg_name; Value val;
        tie(seg_name, val) = this->env->parser->read(this->env->in);

        auto tp = val.type();

        if(tp == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        if(TpTools::is_scope(tp)) {
            if(compare(id_conv, seg_name)) {
                return Node(this->env->tree, tp, false);
            }

            auto* link = this->env->store_node(*this, Node(this->env->tree, tp), seg_name);
            link->field.read_source();

        } else {
            this->env->store_value(*this, val, seg_name);
        }
    }

    if(throw_ex) {
        throw field_name_not_found_ex(id, this->name());
    }

    return Node(this->env->tree);
}

Value Node::consume_value(bool throw_ex, const String& id)
{
    auto stored_itr = find_link_iterator(id, this->values, *this->env);

    if(stored_itr != this->values.end()) {
        auto stored_field = move(stored_itr->field);
        this->values.erase(stored_itr);
        return stored_field;
    }

    auto name_conv = this->env->conv_string(id);

    while(!this->parsed) {

        MemBlock seg_name; Value val;
        tie(seg_name, val) = this->env->parser->read(this->env->in);

        auto tp = val.type();

        if(tp == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        if(!TpTools::is_scope(tp)) {
            if(compare(name_conv, seg_name)) {
                return val;
            }

            this->env->store_value(*this, val, seg_name);

        } else {
            auto* link = this->env->store_node(*this, Node(this->env->tree, tp), seg_name);
            link->field.read_source();
        }
    }

    if(throw_ex) {
        throw field_name_not_found_ex(id, this->name());
    }

    return Value(Type::Null);
}


Node Node::consume_node(bool throw_ex, size_t)
{
    if(this->nodes.size() > 0) {
        auto itr = this->nodes.begin();
        auto stored_field = move(itr->field);
        this->nodes.erase(itr);

        return stored_field;
    }


    while(!this->parsed) {
        MemBlock seg_name; Value val;
        tie(seg_name, val) = this->env->parser->read(this->env->in);

        auto tp = val.type();

        if(tp == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        if(TpTools::is_scope(tp)) {
            Node node(this->env->tree, tp, false);
            node.name_ptr = this->env->store_string(seg_name).first;

            return node;

        } else {
            this->env->store_value(*this, val, seg_name);
        }
    }

    if(throw_ex) {
        throw Exception("Cannot access Node. Index out of bounds");
    }

    return Node(*this->env->tree);
}

Value Node::consume_value(bool throw_ex, size_t)
{
    if(this->values.size() > 0) {
        auto itr = this->values.begin();
        auto stored_field = move(itr->field);
        this->values.erase(itr);

        return stored_field;
    }

    while(!this->parsed) {
        MemBlock seg_name; Value val;
        tie(seg_name, val) = this->env->parser->read(this->env->in);

        auto tp = val.type();

        if(tp == Type::Scope_End) {
            this->parsed = true;
            break;
        }

        if(!TpTools::is_scope(tp)) {
            val.name_ptr = this->env->store_string(seg_name).first;
            return val;

        } else {
            auto* link = this->env->store_node(*this, Node(this->env->tree, tp), seg_name);
            link->field.read_source();
        }
    }

    if(throw_ex) {
        throw Exception("Cannot access Value. Index out of bounds");
    }

    return Value(Type::Null);
}

void Node::to_source()
{
    Value scope_start = Value(this->scope_type);

    this->env->write(scope_start, this->name());

    for(auto& v : this->values) {
        this->env->write(v.field, v.field.name());
    }

    for(auto& n : this->nodes) {
        n.field.to_source();
    }

    Value scope_end = Value(Type::Scope_End);
    this->env->write(scope_end, *this->name_ptr);
}

void Node::consume_scope()
{
    int depth = 0;

    auto& parser = *this->env->parser;
    auto& source = this->env->in;

    while(!this->parsed) {
        auto tp = parser.read(source).second.type();
        if(TpTools::is_scope(tp)) {
            depth++;
            continue;
        }

        if(tp == Type::Scope_End) {
            depth--;
            if(depth <= 0) {
                this->parsed = true;
            }
        }
    }
}

pair<bool, uint64_t> Node::insert_shared (const void* obj)
{
    bool exists; size_t* key;
    auto& table = this->env->shared_table_store;

    tie(exists, key) = table.insert(obj, table.num_entries());

    return { exists, *key };
}

pair<bool, shared_ptr<void>*> Node::find_shared (uint64_t key, const function<shared_ptr<void>(void)>& create)
{
    auto& table = this->env->shared_table_restore;
    auto* sptr = table.get(key);
    bool inserted_new = false;

    if(!sptr) {
        sptr = table.insert(key, create()).second;
        inserted_new = true;
    }

    return { inserted_new, sptr };
}

void Node::foreach_node(const function<void(Node&)>& fnc, bool recursive)
{
    for(auto& link : this->nodes) {

        fnc(link.field);

        if(recursive) {
            link.field.foreach_node(fnc, recursive);
        }
    }
}

void Node::foreach_value(const function<void(Value&)>& fnc, bool recursive)
{
    for(auto& link : this->values) {
        fnc(link.field);
    }

    if(recursive) {
        this->foreach_node([&fnc](Node& node) {
            node.foreach_value(fnc);
        }, true);
    }
}

list<Node*> Node::find_nodes(const String& name_, bool recursive)
{
    list<Node*> rslt;

    this->foreach_node([&rslt, name_] (Node& node) {
        if(node.name() == name_) {
            rslt.push_back(&node);
        }
    }, recursive);

    return rslt;
}

list<Value*> Node::find_values(const String& name_, bool recursive)
{
    list<Value*> rslt;

    this->foreach_value([&rslt, name_] (Value& value) {
        if(value.name() == name_) {
            rslt.push_back(&value);
        }
    }, recursive);

    return rslt;
}

list<Node*> Node::all_nodes(bool recursive)
{
    list<Node*> rslt;

    this->foreach_node([&rslt] (Node& node) {
        rslt.push_back(&node);
    }, recursive);

    return rslt;
}

list<Value*> Node::all_values(bool recursive)
{
    list<Value*> rslt;

    this->foreach_value([&rslt] (Value& value) {
        rslt.push_back(&value);
    }, recursive);

    return rslt;
}

void Node::remove_node(const String& name_)
{
    find_link<Remove | Name>(name_, this->nodes, name_);
}

void Node::remove_node(size_t index)
{
    get_link_at_index<Remove>(index, this->nodes);
}

void Node::remove_node(Node* to_remove)
{
    find_link<Remove | Address>(to_remove, this->nodes);
}

void Node::remove_value(const String& name_)
{
    find_link<Remove | Name>(name_, this->values, name_);
}

void Node::remove_value(size_t index)
{
    get_link_at_index<Remove>(index, this->values);
}

void Node::remove_value(Value* to_remove)
{
    find_link<Remove | Address>(to_remove, this->values);
}

bool Node::has_node(const String& field_name)
{
    if(this->parsed) {
        return find_link<Name>(field_name, this->nodes, field_name) != nullptr;
    }

    auto unn = this->consume_item(field_name, false);

    return unn.node() != nullptr;
}

bool Node::has_value(const String& field_name)
{
    if(this->parsed) {
        return find_link<Name>(field_name, this->values, field_name) != nullptr;
    }

    auto unn = this->consume_item(field_name, false);

    return unn.value() != nullptr;
}
