#include "dictionary.h"

Entry *dictionary_bottom, *dictionary_top;

void dictionary_init() {
    dictionary_bottom = dictionary_top = (Entry *)calloc(sizeof(Entry), 4096);
}

void dictionary_push(const char *key, Value value) {
    dictionary_top->key = key;
    dictionary_top->value = value;
    dictionary_top++;
}

const char *dictionary_key_for_value(Value value) {
    Entry *ptr = dictionary_top;
    const char *key = NULL;
    while (ptr-- != dictionary_bottom) {
	if (ptr->value == value) {
	    key = ptr->key;
	    break;
	}
    }

    return key;
}

Value dictionary_value_for_key(const char *key) {
    Entry *ptr = dictionary_top;
    Value value = SENTINEL;
    while (ptr-- != dictionary_bottom) {
	if (strcmp(ptr->key, key) == 0) {
	    value = ptr->value;
	    break;
	}
    }

    return value;
}
