#include <stdio.h>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>

#include <readline/readline.h>

#include "types.h"
#include "Compiler.h"

const int STACK_SIZE = 1024;
Value *stack_bottom, *stack_top;
Value *rstack_bottom, *rstack_top;
Value stack_always_print = 0;
std::vector<void (*)(void)> prims;
Value *pc;

Lexer l;
Compiler compiler;

#define PRIM(c, f, n) { int _prim_tag = prims.size(); prims.push_back(f); c.register_primitive(n, _prim_tag); }
void HighlightSourceLocation(SourceLocation loc);
char *CompleteToken(const char *token, int context);
void PrintReturnStack();

void die(char *msg) {
    std::cout << "--" << std::endl << msg << std::endl;

    std::cout << "Stack:";
    while (stack_bottom < stack_top) {
	std::cout << " " << *stack_bottom++;
    }
    std::cout << std::endl;

    std::cout << "Return stack:" << std::endl;
    PrintReturnStack();

    std::cout << "PC: " << pc << std::endl;

    exit(1);
}

void PrintReturnStack() {
    *rstack_top++ = (Value)pc;
    while (rstack_bottom < rstack_top) {
	Value *v = (Value *)*--rstack_top;
	if (v == 0) {
	    std::cout << "  <clause:0>";
	} else {
	    std::string name = compiler.name_for_value((Value *)*v);
	    if (!name.empty()) {
		if ((*v & 3) == 3) {
		    std::cout << "  <prim:" << name << ">";
		} else {
		    std::cout << "  " << name;
		}
	    } else {
		std::cout << "  <clause:" << v << ">";
	    }
	}
	std::cout << std::endl;
    }
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
    pc = c;
    while(1) {
	Value v = *pc;
	if (v == 0) {
	    if (rstack_top == rstack_bottom) {
		break;
	    } else {
		pc = (Value *)*--rstack_top;
	    }
	} else if ((v & 3) == 1) {
	    push(v >> 2);
	} else if ((v & 3) == 2) {
	    push(v & (~2));
	} else if ((v & 3) == 3) {
	    prims[v>>2]();
	} else {
	    *rstack_top++ = (Value)pc;
	    pc = (Value *)v - 1;
	}
	pc++;
    }
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

void op_while() {
    Value loop = pop(), predicate = pop();
    call((Value *)predicate);
    while (pop()) {
	call((Value *)loop);
	call((Value *)predicate);
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

void op_load_char() {
    char *addr = (char *)pop();
    Value offset = (Value)addr % sizeof(Value);
    addr = (char *)((Value)addr & ~0x3);
    push((Value)addr[offset]);
}

void op_drop() {
    (void)pop();
}

void op_pick() {
    Value index = pop();
    index = stack_top - stack_bottom - index - 1;
    if (index < 0) {
	die("pick underflow");
    }
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
    push(a % b);
    push(a / b);
}

void var_print() {
    push((Value)&stack_always_print);
}

void op_print_string() {
    std::cout << (char *)pop();
}

void op_rstack_push_cond() {
    Value v = pop(), pred = pop();
    if (pred) {
	*rstack_top++ = v;
    }
}

void op_rstack_pull() {
    if (rstack_top == rstack_bottom) {
	die("Return stack underflow");
    }
    push(*--rstack_top);
}

void op_rstack_copy() {
    if (rstack_top == rstack_bottom) {
	die("Return stack underflow");
    }
    push(*(rstack_top-1));
}

void op_emit() {
    std::cout << (char)pop();
}

void op_def() {
    Value *clause = (Value *)pop();
    std::string token = (char *)pop();
    compiler.dictionary()[token] = (Value)clause;
}

int main(int argc, char **argv) {
    rl_completion_entry_function = (Function *)&CompleteToken;

    stack_bottom = stack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    rstack_bottom = rstack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    pc = 0;

    PRIM(compiler, op_print,	"print");
    PRIM(compiler, op_add,	"+");
    PRIM(compiler, op_sub,	"-");
    PRIM(compiler, op_greater, ">");
    PRIM(compiler, op_store,	"!");
    PRIM(compiler, op_load,	"@");
    PRIM(compiler, op_load_char, "c@");
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
    PRIM(compiler, op_rstack_push_cond, "r<?");
    PRIM(compiler, op_rstack_pull, "r>");
    PRIM(compiler, op_rstack_copy, "r@");
    PRIM(compiler, op_emit,	"emit");
    PRIM(compiler, op_def,      ";");

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
    char *cinput = readline("> ");
    while (cinput && (input = cinput) != "exit") {
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
	if (!input.empty()) {
	    add_history(cinput);
	}
	cinput = readline("> ");
    }
    return 0;
}

void HighlightSourceLocation(SourceLocation loc) {
    std::cout << std::string(2 + loc.first, ' ')
              << std::string(loc.second, '^') << std::endl;
}

char *CompleteToken(const char *prefix, int state) {
    static std::map<std::string, Value>::iterator i;
    static int length;
    if (state == 0) {
	i = compiler.dictionary().begin();
	length = strlen(prefix);
    }

    while (i != compiler.dictionary().end()) {
	std::string token = (*i++).first;
	if (token.find(prefix, 0, length) == 0) {
	    return strdup(token.c_str());
	}
    } 
    
    return NULL;
}
