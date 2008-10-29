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

Value *Compiler::compile(std::vector < std::pair<std::string, SourceLocation> > tokens) {
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
    c->push_back(ops["r>"]);
    c->push_back(ops["pc"]);
    c->push_back(ops["!"]);
    Value *cv = (Value *)malloc(sizeof(Value) * c->size());
    std::copy(c->begin(), c->end(), cv);
    delete c;

    return cv;
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
	    char *s = (char *)malloc(token.size() - 2 + 1);
	    token.copy(s, token.size() - 2, 1);
	    s[token.size() - 2] = 0;
	    return (Value)(s) | 2;
	} else if (token == "{") {
	    clauses.push(new Clause);
	    return 0;
	} else if (token == "}") {
	    Clause *c = clauses.top();
	    clauses.pop();
	    c->push_back(ops["r>"]);
	    c->push_back(ops["pc"]);
	    c->push_back(ops["!"]);
	    Value *v = (Value *)malloc(sizeof(Value) * (c->size()));
	    std::copy(c->begin(), c->end(), v);
	    delete c;

	    return (Value)v | 2;
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
    case state_comment:
	if (token == ")") {
	    state = state_normal;
	}
	return 0;
    case state_variable:
	Value *c = (Value *)malloc(sizeof(Value) * 2);
	*c++ = (Value)malloc(sizeof(Value)) | 2;
	*c++ - 0;
	ops[token] = (Value)c | 2;
	state = state_normal;
	return 0;
    }
}


std::string Compiler::name_for_value(Value *v) {
    std::pair<std::string, Value> p;
    std::map<std::string, Value>::iterator i = ops.begin();
    while (i != ops.end()) {
	p = *i++;
	if (p.second == (Value)v) return p.first;
    }
    return "";
}


std::map<std::string, Value> &Compiler::dictionary() {
    return ops;
}
