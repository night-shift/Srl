#include "Srl/Srl.h"
#include "Srl/Lib.h"

using namespace std;
using namespace Srl;
using namespace Lib;

Tree::Tree(const String& name)
{
    this->env = unique_ptr<Environment>(new Environment(*this));
    this->root_node = &env->create_node(Type::Object, name)->field;
}

Tree::Tree(Tree&& g)
{
    *this = forward<Tree>(g);
}

Tree& Tree::operator= (Tree&& g)
{
    this->env       = move(g.env);
    this->root_node = g.root_node;
    this->env->tree = this;

    return *this;
}

void Tree::to_source(Type type, Parser& parser, Lib::Out::Source source, const function<void()>& store_switch)
{
    this->env->set_output(parser, source);
    this->env->parsing = true;

    auto& name = this->root_node->name();
    this->env->write(Value(type), name);

    store_switch();

    this->env->write(Value(Type::Scope_End), name);

    this->env->out.flush();
}

void Tree::read_source(Parser& parser, In::Source source)
{
    this->clear();
    
    this->env->set_input(parser, source);

    MemBlock name; Value val;
    tie(name, val) = parser.read(this->env->in);

    if(!TpTools::is_scope(val.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    auto* link = this->env->create_node(val.type(), name);
    this->root_node = &link->field;

    this->root_node->read_source();
}

void Tree::read_source(Parser& parser, In::Source source, const function<void()>& restore_switch)
{
    this->env->set_input(parser, source);

    MemBlock name; Value val;
    tie(name, val) = parser.read(this->env->in);

    if(!TpTools::is_scope(val.type())) {
        throw Exception("Unable to parse source. Data malformed.");
    }

    this->root_node = &this->env->create_node(val.type(), name)->field;
    this->root_node->parsed = false;

    restore_switch();
}

Node& Tree::root()
{
    return *this->root_node;
}

void Tree::clear()
{
    this->env->clear();
    this->root_node = &this->env->create_node(Type::Object, "")->field;
}
