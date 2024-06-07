#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include "types.h"

#define INVALID_TREE (-1)

// Index file should follow the format
// number of entries\n
// entry1\n
// entry2\n
// ...

void free_tree(tree_t *index);
entry_t *find_entry(tree_t *index, char* filename);
void index_from_content(char* content, tree_t *tree);
int add_to_index(tree_t *index, char *filename, enum file_mode mode);
int remove_from_tree(tree_t *index, char *filename, int delete);
int tree_to_object(tree_t *tree, object_t *object);
int tree_from_object(tree_t *tree, object_t *object);
int add_object_to_tree(tree_t *tree, char* filename, enum file_mode mode, object_t *source);

#endif // INDEX_H