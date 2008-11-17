#include <stack>
#include <string>
#include <map>
#include "types.h"

class Lexer
{
    private:
    enum { state_whitespace, state_normal, state_string } state;

    public:
    Lexer() : state(state_whitespace) {};
    std::vector< std::pair<std::string, SourceLocation> > lex(std::string s);
};

class Compiler
{
    private:
    std::stack<Clause *> clauses;
    std::string active_def;
    enum { state_normal, state_comment, state_variable } state;
    Value compile_token(std::string token, SourceLocation location);

    public:
    Compiler();
    Value *compile(std::vector < std::pair<std::string, SourceLocation> > tokens);
    void register_primitive(std::string name, Value tag);
    std::string name_for_value(Value *v);
    std::map<std::string, Value> &dictionary();
};
