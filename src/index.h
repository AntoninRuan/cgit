#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>

// Index file should follow the format
// number of entries\n
// entry1\n
// entry2\n
// ...
struct entry {
    char *checksum;
    char *filename;
    struct entry *previous;
    struct entry *next;
};

struct index {
    size_t entries_size;
    struct entry *first_entry;
    struct entry *last_entry;
};

void free_index(struct index *index);
int add_to_index(struct index *index, char *filename);
int remove_from_index(struct index *index, char *filename);

#endif // INDEX_H