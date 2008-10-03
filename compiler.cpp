#include "compiler.h"
#include <iostream>
#include <stdlib.h>

std::vector< std::pair<std::string, SourceLocation> > Lexer::lex(std::string s) {
    unsigned offset = 0, len = s.size();
    std::vector < std::pair<std::string, SourceLocation> > tokens;

    while (offset < len) {
	char next = s[offset];
	switch (state) {
	case state_whitespace:
	    if (next == ' ') {
		offset++;
	    } else if (next == '"') {
		state = state_string;
	    } else {
		state = state_normal;
	    }
	    break;
	case state_normal:
	{
	    int token_len = 0;
	    while (next != ' ' && (offset + token_len < len)) {
		token_len++;
		next = s[offset + token_len];
	    }
	    tokens.push_back(std::make_pair(s.substr(offset, token_len), std::make_pair(offset, token_len)));
	    offset += token_len;
	    state = state_whitespace;
	}
	    break;
	case state_string:
	{
	    int token_len = 1;
	    next = s[offset + 1];
	    while (next != '"' && (offset + token_len < len)) {
		token_len++;
		next = s[offset + token_len];
	    }
	    token_len++;
	    if (offset + token_len > len) {
		throw UnterminatedStringError("\"", std::make_pair(offset, token_len - 1));
	    }

	    tokens.push_back(std::make_pair(s.substr(offset, token_len), std::make_pair(offset, token_len)));
	    offset += token_len;
	    state = state_whitespace;
	}
	    break;
	}
    }

    return tokens;
}

Compiler::Compiler() : state(state_normal)
{
}

void Compiler::register_primitive(std::string name, Value tag)
{
    ops[name] = (tag << 2) | 3;
}

Clause *Compiler::compile(std::vector < std::pair<std::string, SourceLocation> > tokens) {
    std::vector<std::pair<std::string, SourceLocation> >::iterator it;
    Value v;

    clauses.push(new Clause);
    for(it = tokens.begin(); it != tokens.end(); ++it) {
	if ((v = compile_token((*it).first, (*it).second)) != 0) {
	    clauses.top()->push_back(v);
	}
    }

    Clause *c = clauses.top();
    clauses.pop();
    return c;
}

Value Compiler::compile_token(std::string token, SourceLocation location) {
    const char *token_str = token.c_str();
    char *token_end;
    long number = strtol(token_str, &token_end, 10);
    switch (state) {
    case state_normal:
	if (token_end[0] == '\0') {
	    return (number << 2) | 1;
	} else if (token[0] == '"') {
	    return (Value)(new std::string(token.substr(1, token.size() - 2))) | 2;
	} else if (token == "{") {
	    clauses.push(new Clause);
	    return 0;
	} else if (token == "}") {
	    Clause *c = clauses.top();
	    clauses.pop();
	    return (Value)c | 2;
	} else if (token == ":") {
	    if (clauses.size() > 1) {
		while (!clauses.empty()) {
		    clauses.pop();
		}
                throw NestedDefinitionError(token, location);
            }
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
	    while (!clauses.empty()) {
		clauses.pop();
	    }
            throw UnknownTokenError(token, location);
	    return 0;
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
