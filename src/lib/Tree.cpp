#include "Srl/Srl.h"
#include "Srl/Lib.h"

using namespace std;
using namespace Srl;
using namespace Lib;

Tree::Tree(Tree&& g)
{
    *this = forward<Tree>(g);
}

Tree& Tree::operator= (Tree&& g)
{
    this->root_node = g.root_node;
    this->env       = move(g.env);
    if(this->env) {
        this->env->tree = this;
    }
    g.root_node = nullptr;

    return *this;
}

Tree::Tree(Type tp)
{
    this->create_env(tp);
}

void Tree::prologue_in(Parser& parser, Lib::In::Source& source)
{
    if(this->env) {
        this->clear();

    } else {
        this->create_env();
    }

    this->env->set_input(parser, source);

    MemBlock name; Value val;
    tie(name, val) = parser.read(this->env->in);

    if(!TpTools::is_scope(val.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    auto* link = this->env->create_node(val.type(), name);
    this->root_node = &link->field;
}

void Tree::to_source(Type type, Parser& parser, Lib::Out::Source source, const function<void()>& store_switch)
{
    if(!this->env) {
        this->create_env();
    }
    this->env->set_output(parser, source);
    this->env->parsing = true;

    auto& name = this->root_node->name();
    this->env->write(Value(type), name);

    store_switch();

    this->env->write(Value(Type::Scope_End), name);

    this->env->parsing = false;
    this->env->out.flush();
}

void Tree::read_source(Parser& parser, In::Source source)
{
    prologue_in(parser, source);
    this->root_node->read_source();
}

void Tree::read_source(Parser& parser, In::Source source, const function<void()>& restore_switch)
{
    prologue_in(parser, source);
    this->root_node->parsed = false;

    restore_switch();
}

Node& Tree::root()
{
    if(!this->env) {
        this->create_env();
    }
    return *this->root_node;
}

Union Tree::get(const String& field_name)
{
    auto& rn = this->root();
    return rn.get(field_name);
}

optional<Union> Tree::try_get(const String& field_name)
{
    auto& rn = this->root();
    return rn.try_get(field_name);
}

void Tree::clear()
{
    if(!this->env) {
        return;
    }

    auto rtp = this->root().scope_type;

    this->env->clear();
    this->root_node = &env->create_node(rtp, "")->field;
}

void Tree::create_env(Type tp)
{
    assert(!this->env);

    if(tp != Srl::Type::Object && tp != Srl::Type::Array) {
        tp = Srl::Type::Object;
    }

    this->env = unique_ptr<Environment>(new Environment(*this));
    this->root_node = &env->create_node(tp, "")->field;
}

Environment& Tree::get_env()
{
    if(!this->env) {
        this->create_env();
    }

    return *this->env.get();
}
