#include "compiler.h"
#include <iostream>
#include <stdlib.h>

Compiler::Compiler() : state(state_normal)
{
}

void Compiler::register_primitive(std::string name, Value tag)
{
    ops[name] = (tag << 2) | 3;
}

Clause *Compiler::compile(std::string s) {
    int offset = 0;
    clauses.push(new Clause);
    while(offset < s.size()) {
	int pos = s.find(" ", offset);
	if (pos == std::string::npos) { break; }
	std::string token = s.substr(offset, pos - offset);
	Value v = compile_token(token);
	if (v != 0) {
	    clauses.top()->push_back(v);
	}
	offset = pos + 1;
    }

    if (offset < s.size()) {
	std::string token = s.substr(offset, s.size() - offset);
	Value v = compile_token(token);
	if (v != 0) {
	    clauses.top()->push_back(v);
	}
    }
    Clause *c = clauses.top();
    clauses.pop();
    return c;
}

Value Compiler::compile_token(std::string token) {
    const char *token_str = token.c_str();
    char *token_end;
    long number = strtol(token_str, &token_end, 10);
    switch (state) {
    case state_normal:
	if (token_end[0] == '\0') {
	    return (number << 2) | 1;
	} else if (token == "{") {
	    clauses.push(new Clause);
	    return 0;
	} else if (token == "}") {
	    Clause *c = clauses.top();
	    clauses.pop();
	    return (Value)c | 2;
	} else if (token == ":") {
	    clauses.push(new Clause);
	    state = state_def;
	    return 0;
	} else if (token == ";") {
	    clauses.pop();
	    return 0;
	} else if (token == "(") {
	    state = state_comment;
	    return 0;
	} else if (token == "variable") {
	    state = state_variable;
	    return 0;
	} else if (ops.find(token) != ops.end()) {
	    return ops[token];
	} else {
	    std::cout << "unknown token " << token << std::endl;
	    return 1;
	}
    case state_def:
	active_def = token;
	ops[active_def] = (Value)clauses.top();
	state = state_normal;
	return 0;
    case state_comment:
	if (token == ")") {
	    state = state_normal;
	}
	return 0;
    case state_variable:
	Clause *c = new Clause;
	c->push_back((Value)malloc(sizeof(Value)) | 2);
	ops[token] = (Value)c | 2;
	state = state_normal;
	return 0;
    }
}
