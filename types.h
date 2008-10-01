#ifndef __TYPES_H__
#define __TYPES_H__
#include <vector>
#include <memory>

typedef int Value;
typedef std::vector<Value> Clause;
typedef std::pair<Clause*, unsigned> Frame;
typedef std::pair<int, int> SourceLocation;
#define SOURCE_ERROR(error) class error : public std::pair<std::string, SourceLocation> { \
    public:\
    error(std::string token, SourceLocation loc) : std::pair<std::string, SourceLocation>(token, loc) {};\
};

SOURCE_ERROR(UnknownTokenError);
SOURCE_ERROR(NestedDefinitionError);
typedef std::pair<std::string, SourceLocation> NestedDefintionError;
#endif
