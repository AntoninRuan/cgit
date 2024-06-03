#ifndef COMMIT_H
#define COMMIT_H 1

#include "types.h"

int commit_from_object(struct commit *commit, struct object *object);
void free_commit(struct commit *commit);
int diff_commit(char* checksum_a, char* checksum_b, int for_print);
int diff_commit_with_working_tree(char *checksum, int for_print);
int commit();

#endif // COMMIT_H