#ifndef TYPES_H
#define TYPES_H 1

#include <stddef.h>

enum object_type
{
    BLOB,
    TREE,
    COMMIT
};

typedef struct object
{
    char* content;
    size_t size;
    enum object_type object_type;
} object_t;

enum file_mode
{
    DIRECTORY = 0040000,
    REG_NONX_FILE = 0100644,
    REG_EXE_FILE = 0100755,
    SYM_LINK = 0120000,
    GIT_LINK = 0160000,
};

/// @brief entry of a tree
/// checksum is an array of size DIGEST_LENGTH
/// filename is a C-string
typedef struct entry {
    enum file_mode mode;
    unsigned char *checksum;
    char *filename;
    enum object_type type;
    struct entry *previous;
    struct entry *next;
} entry_t;

typedef struct tree {
    size_t entries_size;
    struct entry *first_entry;
    struct entry *last_entry;
} tree_t;

typedef struct commit
{
    char *tree;
    char *parent;
    char *author;
    char *message;
} commit_t;

#endif // TYPES_H