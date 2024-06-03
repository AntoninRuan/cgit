#ifndef COMMIT_H
#define COMMIT_H 1

#include "types.h"

int commit_from_object(struct commit *commit, struct object *object);
void free_commit(struct commit *commit);
void diff_commit(struct commit *commit_a, struct commit *commit_b, int for_print);
void diff_commit_with_working_tree(struct commit *commit, int for_print);
int commit();

#endif // COMMIT_H