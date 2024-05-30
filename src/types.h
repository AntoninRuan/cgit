#ifndef TYPES_H
#define TYPES_H 1

#include <stddef.h>

enum object_type
{
    BLOB,
    TREE,
    COMMIT
};

struct object
{
    char* content;
    size_t size;
    enum object_type object_type;
};

struct entry {
    char *checksum;
    char *filename;
    struct entry *previous;
    struct entry *next;
};

struct tree {
    size_t entries_size;
    struct entry *first_entry;
    struct entry *last_entry;
};

struct commit
{
    char *tree;
    char *parent;
    char *author;
};

#endif // TYPES_H