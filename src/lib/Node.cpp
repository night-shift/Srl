#include "Srl/Srl.h"
#include "Srl/Internal.h"

using namespace std;
using namespace Srl;
using namespace Lib;

namespace {

    template<class T> const string& tp_name();
    template<> const string& tp_name<Node>()  { static const string str = "Node";  return str; }
    template<> const string& tp_name<Value>() { static const string str = "Value"; return str; }

    typedef uint8_t Option;

    const Option None            = 0;
    const Option Hash            = 1 << 0; /* compare by name hash */
    const Option Address         = 1 << 1; /* compare by address */
    const Option Index           = 1 << 2; /* access  by Index */
    const Option Iterator        = 1 << 3; /* return the iterator, not the link */
    const Option Throw           = 1 << 4; /* throw exception on not found or multiple findings */
    const Option Check_Duplicate = 1 << 5; /* check for multiple results */

    constexpr bool enabled(Option options, Option option)
    {
        return (options & option) > 0;
    }

    template<class T>
    using Cont = vector<Link<T>*>;

    template<class T>
    using Itr = typename Cont<T>::const_iterator;

    template<Option Opt, class T>
    using Result = typename conditional<enabled(Opt, Iterator), Itr<T>, Link<T>*>::type;

    template<Option Opt, class T> typename enable_if<enabled(Opt, Hash), bool>::type
    compare(Itr<T>& itr, size_t hash) { return (*itr)->hash == hash; }

    template<Option Opt, class T> typename enable_if<enabled(Opt, Address), bool>::type
    compare(Itr<T>& itr, T* pointer) { return &(*itr)->field == pointer; }

    template<Option Opt, class T> typename enable_if<enabled(Opt, Iterator), Itr<T>>::type
    result(Itr<T> itr, bool) { return itr; }

    template<Option Opt, class T> typename enable_if<!enabled(Opt, Iterator), Link<T>*>::type
    result(Itr<T> itr, bool found) { return found ? *itr : nullptr; }

    template<Option Opt, class TKey, class T>
    typename enable_if<enabled(Opt, Index), Result<Opt, T>>::type
    find_link(TKey index, const Cont<T>& links)
    {
        if(index < links.size()) {
            return result<Opt, T>(links.begin() + index, true);

        } else {
            if(enabled(Opt, Throw)) {
                auto msg = "Cannot access " + tp_name<T>() + " at index " + to_string(index) + ". Index out of bounds.";
                throw Exception(msg);
            }
            return result<Opt, T>(links.end(), false);
        }
    }

    template<Option Opt, class TKey, class T>
    typename enable_if<!enabled(Opt, Index), Result<Opt, T>>::type
    find_link(TKey key, const Cont<T>& links, const String& name = String())
    {
        Itr<T> end  = links.end();
        Itr<T> rslt = end;

        for(auto itr = links.begin(); itr != end; itr++) {
            if(compare<Opt, T>(itr, key)) {

                if(enabled(Opt, Throw) && rslt != end) {
                    auto msg = tp_name<T>() + " duplication: " + name.unwrap<char>(false) + ".";
                    throw Exception(msg);
                }

                rslt = itr;

                if(!enabled(Opt, Check_Duplicate)) {
                    break;
                }
            }
        }

        bool found = rslt != end;

        if(enabled(Opt, Throw) && !found) {
            auto msg = tp_name<T>() + " " + name.unwrap<char>(false) + " not found.";
            throw Exception(msg);
        }

        return result<Opt, T>(rslt, found);
    }

    template<class T>
    void remove_link(Itr<T> itr, Cont<T>& links)
    {
        if(itr != links.end()) {
            auto pos = links.begin();
            advance(pos, distance<Itr<T>>(pos, itr));
            links.erase(pos);
        }
    }

    template<class T>
    void insert_link(Link<T>* link, Cont<T>& links)
    {
        links.push_back(link);
    }
}

void Node::insert_value(const Value& new_value, const String& name)
{
    if(this->just_parse) {
        this->tree->parse_value(new_value, name);

    } else {
        auto* link = this->tree->storage.store_value(new_value, name);
        insert_link(link, this->values);
    }
}

Node* Node::insert_node(const Node& new_node, const String& name)
{
    auto* link = this->tree->storage.store_node(new_node, *this->tree, name);
    insert_link(link, this->nodes);

    return &link->field;
}

Node* Node::insert_node(Type node_type, const String& name)
{
    return this->insert_node(Node(this->tree, node_type), name);
}

Value* Node::value(const String& name) const
{
    auto hash = this->tree->storage.hash_string(name);
    auto* link = find_link<Throw | Hash>(hash, this->values, name);

    return &link->field;
}

Value* Node::value(size_t index) const
{
    auto* link = find_link<Throw | Index>(index, this->values);
    return &link->field;
}

/* In rare cases empty document nodes can't be distinguished
 * from empty plain values, try to detect such a case, create an empty node and forward it. */
Node* Node::node(size_t index) const
{
    if(this->nodes.size() > 0) {
        return &find_link<Throw | Index>(index, this->nodes)->field;

    } else {
        /* check if a value at given index exists */
        find_link<Throw | Index>(index, this->values);
        /* this might be such a case now if all values are empty  */
        for(auto e : this->values) {
            if(e->field.size() > 0) {
                auto msg = "Cannot access Node at pos " + to_string(index) + ". Index out of bounds.";
                throw Exception(msg);
            }
        }
        auto* link = this->tree->storage.create_node(*this->tree, Type::Container, String());

        return &link->field;
    }

}
/* same for named nodes */
Node* Node::node(const String& name) const
{
    auto hash = this->tree->storage.hash_string(name);
    auto* link = find_link<Hash>(hash, this->nodes, name);

    if (link != nullptr) {
        return &link->field;

    } else {
        auto* link_value = find_link<Throw | Hash>(hash, this->values, name);
        if(link_value->field.size() > 0) {
            auto msg = "Field " + name.unwrap<char>(false) + " not found.";
            throw Exception(msg);
        }
        auto* new_link = this->tree->storage.create_node(*this->tree, Type::Container, name);

        return &new_link->field;
    }
}

void Node::parse_in(In& source, Parser& parser)
{
    while(true) {

        auto seg  = parser.parse_in(source);
        auto& val = seg.value;

        if(val.type() == Type::Scope_End) {
            break;
        }

        auto name = String(seg.name, Encoding::UTF8);
        /* in direct parse mode the whole tree is discarded afterwards,
         * so storing data blocks is unnecessary */
        bool store_data = !this->just_parse || seg.data_buffered;

        if(!TpTools::is_scope(val.type())) {
            /* new value */
            auto* link = this->tree->storage.store_value(
                val, name, store_data
            );
            insert_link(link, this->values);

        } else {
            /* new node */
            auto* link = this->tree->storage.create_node(*this->tree, val.type(), name, store_data);
            link->field.just_parse = this->just_parse;
            insert_link(link, this->nodes);

            link->field.parse_in(source, parser);
        }
    }
}

void Node::to_source()
{
    Value scope_start = Value(this->scope_type);
    this->tree->parse_value(scope_start, *this->name_ptr);

    for(auto v : this->values) {
        this->tree->parse_value(v->field, *v->field.name());
    }

    for(auto n : this->nodes) {
        n->field.to_source();
    }

    Value scope_end = Value(Type::Scope_End);
    this->tree->parse_value(scope_end, *this->name_ptr);
}

void Node::forall_nodes(const function<void(Node*)>& fnc, bool recursive) const
{
    auto cpy = this->nodes;
    for(auto link : this->nodes) {

        fnc(&link->field);

        if(recursive) {
            link->field.forall_nodes(fnc, recursive);
        }
    }
}

void Node::forall_values(const function<void(Value*)>& fnc, bool recursive) const
{
    for(auto link : this->values) {
        fnc(&link->field);
    }

    if(recursive) {
        this->forall_nodes([&fnc](Node* node) {
            node->forall_values(fnc);
        }, true);
    }
}

deque<Node*> Node::find_nodes(const String& name, bool recursive) const
{
    deque<Node*> rslt;
    auto hash = this->tree->storage.hash_string(name);

    this->forall_nodes([&rslt, this, hash] (Node* node) {
        if(this->tree->storage.hash_string(*node->name_ptr) == hash) {
            rslt.push_back(node);
        }
    }, recursive);

    return move(rslt);
}

deque<Value*> Node::find_values(const String& name, bool recursive) const
{
    deque<Value*> rslt;
    auto hash = this->tree->storage.hash_string(name);

    this->forall_values([&rslt, this, hash] (Value* value) {
        if(this->tree->storage.hash_string(*value->name()) == hash) {
            rslt.push_back(value);
        }
    }, recursive);

    return move(rslt);
}

deque<Node*> Node::all_nodes(bool recursive) const
{
    deque<Node*> rslt;

    this->forall_nodes([&rslt] (Node* node) {
        rslt.push_back(node);
    }, recursive);

    return move(rslt);
}

deque<Value*> Node::all_values(bool recursive) const
{
    deque<Value*> rslt;

    this->forall_values([&rslt] (Value* value) {
        rslt.push_back(value);
    }, recursive);

    return move(rslt);
}

void Node::remove_node(const String& name)
{
    auto hash = this->tree->storage.hash_string(name);
    auto itr = find_link<Iterator | Hash>(hash, this->nodes, name);
    remove_link(itr, this->nodes);
}

void Node::remove_node(size_t index)
{
    auto itr = find_link<Iterator | Index>(index, this->nodes);
    remove_link(itr, this->nodes);
}

void Node::remove_node(Node* to_remove)
{
    auto itr = find_link<Iterator | Address>(to_remove, this->nodes);
    remove_link(itr, this->nodes);
}

void Node::remove_value(const String& name)
{
    auto hash = this->tree->storage.hash_string(name);
    auto itr = find_link<Iterator | Hash>(hash, this->values, name);
    remove_link(itr, this->values);
}

void Node::remove_value(size_t index)
{
    auto itr = find_link<Iterator | Index>(index, this->values);
    remove_link(itr, this->values);
}

void Node::remove_value(Value* to_remove)
{
    auto itr = find_link<Iterator | Address>(to_remove, this->values);
    remove_link(itr, this->values);
}

bool Node::has_node(const String& field_name) const
{
    auto hash = this->tree->storage.hash_string(field_name);
    return find_link<Hash>(hash, this->nodes, field_name) != nullptr;
}

bool Node::has_value(const String& field_name) const
{
    auto hash = this->tree->storage.hash_string(field_name);
    return find_link<Hash>(hash, this->values, field_name) != nullptr;
}

