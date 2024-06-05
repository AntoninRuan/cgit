#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include "types.h"

// Index file should follow the format
// number of entries\n
// entry1\n
// entry2\n
// ...

void free_tree(struct tree *index);
struct entry *find_entry(struct tree *index, char* filename);
void get_tree(char* content, struct tree *tree);
int add_to_index(struct tree *index, char *filename);
int remove_from_index(struct tree *index, char *filename, int delete);
int tree_to_object(struct tree *tree, struct object *object);
int add_object_to_tree(struct tree *tree, char* filename, struct object *source);

#endif // INDEX_H