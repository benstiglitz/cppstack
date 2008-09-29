#include <stack>
#include <string>
#include <map>
#include "types.h"

class Compiler
{
    private:
    std::stack<Clause *> clauses;
    std::map<std::string, Value> ops;
    std::string active_def;
    enum { state_normal, state_def , state_comment, state_variable } state;
    Value compile_token(std::string token);

    public:
    Compiler();
    Clause *compile(std::string s);
};
