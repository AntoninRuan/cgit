#ifndef COMMIT_H
#define COMMIT_H 1

#include "types.h"

int commit_from_object(struct commit *commit, struct object *object);
void free_commit(struct commit *commit);
void diff_commit(struct commit *commit_a, struct commit *commit_b);
void diff_commit_with_working_tree(struct commit *commit);
void revert_to(struct commit *commit);
int commit();

#endif // COMMIT_H