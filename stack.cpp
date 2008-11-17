#include <stdio.h>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include <readline/readline.h>

#include "types.h"
#include "Compiler.h"
#include "dictionary.h"

const int STACK_SIZE = 1024;
Value *stack_bottom, *stack_top;
Value *rstack_bottom, *rstack_top;
Value stack_always_print = 0;
Value *heap;
std::vector<void (*)(void)> prims;
Value *pc;
Value *interrupt_handler;
int last_signal = 0;

Lexer l;
Compiler compiler;

#define PRIM(c, f, n) { int _prim_tag = prims.size(); prims.push_back(f); c.register_primitive(n, _prim_tag); }
void HighlightSourceLocation(SourceLocation loc);
char *CompleteToken(const char *token, int context);
void PrintReturnStack();
void PrintDebugInfo();

void halt(char *msg) {
    std::cout << "--" << std::endl << msg << std::endl;
    PrintDebugInfo();
    *rstack_top++ = (Value)pc - sizeof(Value);
    pc = 0;
}

void die(char *msg) {
    halt(msg);
    exit(1);
}

void PrintReturnStack() {
    *rstack_top++ = (Value)pc;
    Value *rstack_cur = rstack_top;
    while (rstack_bottom < rstack_cur) {
	Value *v = (Value *)*--rstack_cur;
	if (v == 0) {
	    std::cout << "  <repl>";
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
	std::cout << " (" << v << ")" << std::endl;
    }
    rstack_top--;
}

void PrintDebugInfo() {
    std::cout << "Stack:";
    Value *stack_cur = stack_bottom;
    while (stack_cur < stack_top) {
	std::cout << " " << *stack_cur++;
    }
    std::cout << std::endl;

    std::cout << "Return stack:" << std::endl;
    PrintReturnStack();

    std::cout << "PC: " << pc << std::endl;
}

void push(Value v) {
    *stack_top++ = v;
}

Value pop() {
    if (stack_top == stack_bottom) {
	die("Fatal stack underflow");
    }
    Value v = *--stack_top;
    return v;
}

bool can_pop(int cells) {
    if (stack_top - (cells - 1) == stack_bottom) {
	halt("Stack underflow");
	return false;
    } else {
	return true;
    }
}
#define will_pop(n) if(!can_pop(n)) { return; }

Value call(Value *c) {
    *rstack_top++ = 0;
    pc = c;
    while(1) {
	Value v = *pc;
	if ((v & 3) == 1) {
	    push(v >> 2);
	} else if ((v & 3) == 2) {
	    push(v & (~2));
	} else if ((v & 3) == 3) {
	    prims[v>>2]();
	} else {
	    *rstack_top++ = (Value)pc;
	    pc = (Value *)v - 1;
	}

	if (last_signal) {
	    *rstack_top++ = (Value)pc;
	    pc = interrupt_handler - 1;
	    push(last_signal);
	    last_signal = 0;
	}

	if (pc == 0) {
	    break;
	}

	pc++;
    }
}

void op_print() {
    will_pop(1);

    std::cout << pop();
}

void op_add() {
    will_pop(2);

    push(pop() + pop());
}

void op_sub() {
    will_pop(2);

    int b = pop(), a = pop();
    push(a - b);
}

void op_greater() {
    will_pop(2);

    int b = pop(), a = pop();
    push(a > b ? 1 : 0);
}

void op_store() {
    will_pop(2);

    Value *addr = (Value *)pop();
    Value v = pop();
    *addr = v;
}

void op_load() {
    will_pop(1);

    Value *addr = (Value *)pop();
    push(*addr);
}

void op_load_char() {
    will_pop(1);

    char *addr = (char *)pop();
    Value offset = (Value)addr % sizeof(Value);
    addr = (char *)((Value)addr & ~0x3);
    push((Value)addr[offset]);
}

void op_drop() {
    will_pop(1);

    (void)pop();
}

void op_pick() {
    will_pop(1);

    Value index = pop();
    index = stack_top - stack_bottom - index - 1;
    if (index < 0) {
	die("pick underflow");
    }
    push(stack_bottom[index]);
}

void op_swap() {
    will_pop(2);

    Value a = pop(), b = pop();
    push(a); push(b);
}

void op_rot() {
    will_pop(3);

    Value c = pop(), b = pop(), a = pop();
    push(b); push(c); push(a);
}

void op_not() {
    will_pop(1);

    push(!pop());
}

void var_stack_bottom() {
    push((Value)&stack_bottom);
}

void var_stack_top() {
    push((Value)&stack_top);
}

void var_pc() {
    push((Value)&pc);
}

void op_mul() {
    will_pop(2);

    push(pop() * pop());
}

void op_remquo() {
    will_pop(2);

    Value b = pop(), a = pop();
    push(a % b);
    push(a / b);
}

void var_print() {
    push((Value)&stack_always_print);
}

void op_print_string() {
    will_pop(2);

    std::cout << (char *)pop();
}

void op_rstack_push() {
    will_pop(1);

    *rstack_top++ = pop();
}

void op_rstack_push_cond() {
    will_pop(2);

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
    will_pop(1);

    std::cout << (char)pop();
    std::flush(std::cout);
}

void var_heap() {
    push((Value)&heap);
}

void op_key() {
    struct termios old_ios, new_ios;
    tcgetattr(STDIN_FILENO, &old_ios);
    new_ios = old_ios;
    cfmakeraw(&new_ios);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_ios);
    push(getchar());
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_ios);
}

void var_interrupt_handler() {
    push((Value)&interrupt_handler);
}

void handle_SIGINT(int signal) {
    last_signal = signal;
}

void IncludeFile(std::string filename) {
    std::ifstream f;
    f.open(filename.c_str());
    std::string input;
    while(!std::getline(f, input).eof()) {
	call(compiler.compile(l.lex(input)));
    }
}

void op_include() {
    will_pop(1);

    IncludeFile((char *)pop());
}

void var_dict_top() {
    push((Value)&dictionary_top);
}

void var_dict_bottom() {
    push((Value)&dictionary_bottom);
}

int main(int argc, char **argv) {
    rl_completion_entry_function = (Function *)&CompleteToken;

    stack_bottom = stack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    rstack_bottom = rstack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);
    heap = (Value *)calloc(sizeof(Value), 4096);
    pc = 0;

    dictionary_init();

    sigset_t SIGINT_set; sigemptyset(&SIGINT_set);
    sigaddset(&SIGINT_set, SIGINT);
    if (sigprocmask(SIG_BLOCK, &SIGINT_set, NULL) != 0) {
	halt("Could not block signals");
    }
    signal(SIGINT, handle_SIGINT);

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
    PRIM(compiler, var_stack_top, "sp");
    PRIM(compiler, var_stack_bottom, "sbase");
    PRIM(compiler, var_pc,	"pc");
    PRIM(compiler, op_mul,	"*");
    PRIM(compiler, op_remquo,	"remquo");
    PRIM(compiler, var_print,  "s:always-print");
    PRIM(compiler, op_print_string, "print-string");
    PRIM(compiler, op_rstack_push,  "r<");
    PRIM(compiler, op_rstack_push_cond, "r<?");
    PRIM(compiler, op_rstack_pull, "r>");
    PRIM(compiler, op_rstack_copy, "r@");
    PRIM(compiler, op_emit,	"emit");
    PRIM(compiler, var_heap,    "heap");
    PRIM(compiler, op_key,	"key");
    PRIM(compiler, PrintDebugInfo, "d:info");
    PRIM(compiler, var_interrupt_handler, "interrupt-handler");
    PRIM(compiler, op_include,  "include");
    PRIM(compiler, var_dict_top, "dp");
    PRIM(compiler, var_dict_bottom, "dbase");

    std::string input;
    
    // Read from file
    if (!(argc > 1)) {
	IncludeFile("stdlib.f");
    }

    // Read from standard input
    char *cinput = readline("> ");
    while (cinput && (input = cinput) != "exit") {
	sigprocmask(SIG_UNBLOCK, &SIGINT_set, NULL);
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
	sigprocmask(SIG_BLOCK, &SIGINT_set, NULL);
	cinput = readline("> ");
    }
    return 0;
}

void HighlightSourceLocation(SourceLocation loc) {
    std::cout << std::string(2 + loc.first, ' ')
              << std::string(loc.second, '^') << std::endl;
}

char *CompleteToken(const char *prefix, int state) {
    static Entry *it;
    static int length;
    if (state == 0) {
	it = dictionary_top;
	length = strlen(prefix);
    }

    while (it-- != dictionary_bottom) {
	std::string token = it->key;
	if (token.find(prefix, 0, length) == 0) {
	    return strdup(token.c_str());
	}
    } 
    
    return NULL;
}
