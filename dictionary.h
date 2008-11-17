#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__
#include "types.h"

typedef struct {
    const char *key;
    Value value;
} Entry;

extern Entry *dictionary_bottom, *dictionary_top;
void dictionary_init();
void dictionary_push(const char *key, Value value);
const char *dictionary_key_for_value(Value value);
Value dictionary_value_for_key(const char *key);
#endif __DICTIONARY_H__
