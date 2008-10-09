#include <stdio.h>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include "types.h"
#include "Compiler.h"

const int STACK_SIZE = 1024;
Value *stack_bottom, *stack_top;
Value *rstack_bottom, *rstack_top;
Value stack_always_print = 0;
std::vector<void (*)(void)> prims;
std::deque<std::string> callstack;
Value *pc;

Lexer l;
Compiler compiler;

#define PRIM(c, f, n) { int _prim_tag = prims.size(); prims.push_back(f); c.register_primitive(n, _prim_tag); }
void HighlightSourceLocation(SourceLocation loc);

void die(char *msg) {
    std::cout << "--" << std::endl << msg << std::endl;

    std::cout << "Stack:";
    while (stack_bottom < stack_top) {
	std::cout << " " << *stack_bottom++;
    }
    std::cout << std::endl;

    std::cout << "Return stack:";
    while (rstack_bottom < rstack_top) {
	std::cout << " " << (Value *)*rstack_bottom++;
    }
    std::cout << std::endl;

    std::cout << "PC: " << pc << std::endl;

    std::cout << "Call stack: " << std::endl;
    for(std::deque<std::string>::reverse_iterator i = callstack.rbegin(); i != callstack.rend(); i++) {
	std::cout << "  " << *i << std::endl;
    }

    exit(1);
}

void push(Value v) {
    *stack_top++ = v;
}

Value pop() {
    if (stack_top == stack_bottom) {
	die("Stack underflow");
    }
    Value v = *--stack_top;
    return v;
}

Value call(Value *c) {
    std::string s;
    if ((s = compiler.name_for_value(c)).empty()) {
	std::stringstream ss;
	ss << "<clause:" << c << ">";
	s = ss.str();
    }
    callstack.push_back(s);
    *rstack_top++ = (Value)pc;
    pc = c;
    while(*pc != 0) {
	Value v = *pc;
	if ((v & 3) == 1) {
	    push(v >> 2);
	} else if ((v & 3) == 2) {
	    push(v & (~2));
	} else if ((v & 3) == 3) {
	    prims[v>>2]();
	} else {
	    call((Value *)v);
	}
	pc++;
    }
    pc = (Value *)*--rstack_top;
    callstack.pop_back();
}


void op_if() {
    callstack.push_back("<prim:if>");
    Value false_clause = pop(), true_clause = pop(), v = pop();
    Value clause = (v != 0) ? true_clause : false_clause;
    Value *c = (Value *)clause;
    call(c);
    callstack.pop_back();
}

void op_print() {
    callstack.push_back("<prim:print>");
    std::cout << " " << pop();
    callstack.pop_back();
}

void op_add() {
    callstack.push_back("<prim:+>");
    push(pop() + pop());
    callstack.pop_back();
}

void op_sub() {
    callstack.push_back("<prim:->");
    int b = pop(), a = pop();
    push(a - b);
    callstack.pop_back();
}

void op_call() {
    callstack.push_back("<prim:call>");
    call((Value *)pop());
    callstack.pop_back();
}

void op_while() {
    callstack.push_back("<prim:while>");
    Value loop = pop(), predicate = pop();
    call((Value *)predicate);
    while (pop()) {
	call((Value *)loop);
	call((Value *)predicate);
    }
    callstack.pop_back();
}

void op_greater() {
    callstack.push_back("<prim:>>");
    int b = pop(), a = pop();
    push(a > b ? 1 : 0);
    callstack.pop_back();
}

void op_store() {
    callstack.push_back("<prim:!>");
    Value *addr = (Value *)pop();
    Value v = pop();
    *addr = v;
    callstack.pop_back();
}

void op_load() {
    callstack.push_back("<prim:@>");
    Value *addr = (Value *)pop();
    push(*addr);
    callstack.pop_back();
}

void op_load_char() {
    callstack.push_back("<prim:c@>");
    char *addr = (char *)pop();
    Value offset = (Value)addr % sizeof(Value);
    addr = (char *)((Value)addr & ~0x3);
    push((Value)addr[offset]);
    callstack.pop_back();
}

void op_drop() {
    callstack.push_back("<prim:drop>");
    (void)pop();
    callstack.pop_back();
}

void op_pick() {
    callstack.push_back("<prim:pick>");
    Value index = pop();
    index = stack_top - stack_bottom - index - 1;
    if (index < 0) {
	die("pick underflow");
    }
    push(stack_bottom[index]);
    callstack.pop_back();
}

void op_swap() {
    callstack.push_back("<prim:swap>");
    Value a = pop(), b = pop();
    push(a); push(b);
    callstack.pop_back();
}

void op_rot() {
    callstack.push_back("<prim:rot>");
    Value c = pop(), b = pop(), a = pop();
    push(b); push(c); push(a);
    callstack.pop_back();
}

void op_not() {
    callstack.push_back("<prim:not>");
    push(!pop());
    callstack.pop_back();
}

void op_stack_bottom() {
    callstack.push_back("<prim:sbase>");
    push((Value)stack_bottom);
    callstack.pop_back();
}

void op_stack_top() {
    callstack.push_back("<prim:sp>");
    push((Value)stack_top);
    callstack.pop_back();
}

void op_mul() {
    callstack.push_back("<prim:*>");
    push(pop() * pop());
    callstack.pop_back();
}

void op_divmod() {
    callstack.push_back("<prim:divmod>");
    Value b = pop(), a = pop();
    push(a / b);
    push(a % b);
    callstack.pop_back();
}

void var_print() {
    callstack.push_back("<prim:s:always-print>");
    push((Value)&stack_always_print);
    callstack.pop_back();
}

void op_print_string() {
    callstack.push_back("<prim:print-string>");
    std::cout << (char *)pop();
    callstack.pop_back();
}

void op_rstack_push() {
    callstack.push_back("r<");
    *rstack_top++ = pop();
    callstack.pop_back();
}

void op_rstack_pull() {
    callstack.push_back("r>");
    if (rstack_top == rstack_bottom) {
	die("Return stack underflow");
    }
    push(*--rstack_top);
    callstack.pop_back();
}

void op_rstack_copy() {
    callstack.push_back("r@");
    if (rstack_top == rstack_bottom) {
	die("Return stack underflow");
    }
    push(*(rstack_top-1));
    callstack.pop_back();
}

int main(int argc, char **argv) {
    stack_bottom = stack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    rstack_bottom = rstack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    pc = 0;

    PRIM(compiler, op_if,	"if");
    PRIM(compiler, op_print,	"print");
    PRIM(compiler, op_add,	"+");
    PRIM(compiler, op_sub,	"-");
    PRIM(compiler, op_call,	"call");
    PRIM(compiler, op_while,	"while");
    PRIM(compiler, op_greater, ">");
    PRIM(compiler, op_store,	"!");
    PRIM(compiler, op_load,	"@");
    PRIM(compiler, op_load_char, "@c");
    PRIM(compiler, op_pick,	"pick");
    PRIM(compiler, op_swap,	"swap");
    PRIM(compiler, op_rot,	"rot");
    PRIM(compiler, op_not,     "not");
    PRIM(compiler, op_drop,	"drop");
    PRIM(compiler, op_stack_top, "sp");
    PRIM(compiler, op_stack_bottom, "sbase");
    PRIM(compiler, op_mul,	"*");
    PRIM(compiler, op_divmod,	"divmod");
    PRIM(compiler, var_print,  "s:always-print");
    PRIM(compiler, op_print_string, "print-string");
    PRIM(compiler, op_rstack_push, "r<");
    PRIM(compiler, op_rstack_pull, "r>");
    PRIM(compiler, op_rstack_copy, "r@");

    std::string input;
    
    // Read from file
    if (!(argc > 1)) {
	std::ifstream f;
	f.open("stdlib.f");
	while(!std::getline(f, input).eof()) {
	    call(compiler.compile(l.lex(input)));
	}
    }

    // Read from standard input
    std::cout << "> "; std::getline(std::cin, input);
    while (input != "exit" && !std::cin.eof()) {
	try {
            call(compiler.compile(l.lex(input)));
            if (stack_always_print) {
                Value *v;
                for (v = stack_bottom; v != stack_top; v++) {
                    std::cout << " " << *v;
                }
            }
            std::cout << "  ok" << std::endl;
        } catch (UnknownTokenError e) {
            HighlightSourceLocation(e.second);
            std::cout << "Unknown token " << e.first << std::endl;
        } catch (NestedDefinitionError e) {
            HighlightSourceLocation(e.second);
            std::cout << "Nested definiton" << std::endl;
        } catch (UnterminatedStringError e) {
	    HighlightSourceLocation(e.second);
	    std::cout << "Unterminated string" << std::endl;
	}
	std::cout <<  "> "; std::getline(std::cin, input);
    }
    return 0;
}

void HighlightSourceLocation(SourceLocation loc) {
    std::cout << std::string(2 + loc.first, ' ')
              << std::string(loc.second, '^') << std::endl;
}
