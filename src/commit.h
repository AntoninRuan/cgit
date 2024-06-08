#ifndef COMMIT_H
#define COMMIT_H 1

#include "types.h"

int commit_from_object(commit_t *commit, object_t *object);
int commit_to_object(commit_t *commit, object_t *object);
void free_commit(commit_t *commit);
int diff_commit(char* checksum_a, char* checksum_b, int for_print);
int diff_commit_with_working_tree(char *checksum, int for_print);
int commit(char *msg);

#endif // COMMIT_H