#include <stdio.h>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include "types.h"
#include "Compiler.h"

const int STACK_SIZE = 1024;
//std::deque<Value> master_stack;
Value *stack_bottom, *stack_top;
Value stack_always_print = 0;
std::stack<Frame> return_stack;
std::vector<void (*)(void)> prims;

#define PRIM(c, f, n) { int _prim_tag = prims.size(); prims.push_back(f); c.register_primitive(n, _prim_tag); }
void HighlightSourceLocation(SourceLocation loc);

void die(char *msg) {
    printf("die: %s\n", msg);
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

Value call(Clause *c) {
    return_stack.push(std::make_pair(c, 0));
    Frame &f = return_stack.top();
    while(f.second < f.first->size()) {
	Value v = (*f.first)[f.second];
	if ((v & 3) == 1) {
	    push(v >> 2);
	} else if ((v & 3) == 2) {
	    push(v & (~2));
	} else if ((v & 3) == 3) {
	    prims[v>>2]();
	} else {
	    call((Clause *)v);
	}
	f.second++;
    }
}


void op_if() {
    Value false_clause = pop(), true_clause = pop(), v = pop();
    Value clause = (v != 0) ? true_clause : false_clause;
    Clause *c = (Clause *)clause;
    call(c);
}

void op_print() {
    std::cout << " " << pop();
}

void op_add() {
    push(pop() + pop());
}

void op_sub() {
    int b = pop(), a = pop();
    push(a - b);
}

void op_call() {
    call((Clause *)pop());
}

void op_while() {
    Value loop = pop(), predicate = pop();
    call((Clause *)predicate);
    while (pop()) {
	call((Clause *)loop);
	call((Clause *)predicate);
    }
}

void op_greater() {
    int b = pop(), a = pop();
    push(a > b ? 1 : 0);
}

void op_store() {
    Value *addr = (Value *)pop();
    Value v = pop();
    *addr = v;
}

void op_load() {
    Value *addr = (Value *)pop();
    push(*addr);
}

void op_drop() {
    (void)pop();
}

void op_pick() {
    Value index = pop();
    //index = master_stack.size() - index - 1;
    index = stack_top - stack_bottom - index - 1;
    if (index < 0) {
	die("pick underflow");
    }
    //push(master_stack[index]);
    push(stack_bottom[index]);
}

void op_swap() {
    Value a = pop(), b = pop();
    push(a); push(b);
}

void op_rot() {
    Value c = pop(), b = pop(), a = pop();
    push(b); push(c); push(a);
}

void op_not() {
    push(!pop());
}

void op_stack_bottom() {
    push((Value)stack_bottom);
}

void op_stack_top() {
    push((Value)stack_top);
}

void op_mul() {
  push(pop() * pop());
}

void op_divmod() {
  Value b = pop(), a = pop();
  push(a / b);
  push(a % b);
}

void var_print() {
  push((Value)&stack_always_print);
}

void op_print_string() {
    std::cout << *(std::string *)pop();
}

int main(int argc, char **argv) {
    stack_bottom = stack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);

    Lexer l;
    Compiler c;

    PRIM(c, op_if,	"if");
    PRIM(c, op_print,	"print");
    PRIM(c, op_add,	"+");
    PRIM(c, op_sub,	"-");
    PRIM(c, op_call,	"call");
    PRIM(c, op_while,	"while");
    PRIM(c, op_greater, ">");
    PRIM(c, op_store,	"!");
    PRIM(c, op_load,	"@");
    PRIM(c, op_pick,	"pick");
    PRIM(c, op_swap,	"swap");
    PRIM(c, op_rot,	"rot");
    PRIM(c, op_not,     "not");
    PRIM(c, op_drop,	"drop");
    PRIM(c, op_stack_top, "sp");
    PRIM(c, op_stack_bottom, "sbase");
    PRIM(c, op_mul,	"*");
    PRIM(c, op_divmod,	"divmod");
    PRIM(c, var_print,  "s:always-print");
    PRIM(c, op_print_string, "print-string");

    std::string input;
    
    // Read from file
    std::ifstream f;
    f.open("stdlib.f");
    while(!std::getline(f, input).eof()) {
	call(c.compile(l.lex(input)));
    }

    // Read from standard input
    std::cout << "> "; std::getline(std::cin, input);
    while (!input.empty()) {
	try {
            call(c.compile(l.lex(input)));
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
