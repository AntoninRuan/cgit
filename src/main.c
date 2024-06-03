// Requirements
// commit
// branching
// revert
// pull/push

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "includes.h"
#include "objects.h"
#include "types.h"
#include "fs.h"
#include "tree.h"
#include "commit.h"

int main(int argc, char** argv)
{
    // struct tree index = {0};
    // load_index(&index);

    // add_to_index(&index, "src/commit.c");
    // add_to_index(&index, "src/commit.h");
    // add_to_index(&index, "src/fs.c");
    // add_to_index(&index, "src/fs.h");

    // save_index(&index);

    // commit();

    struct object a = {0}, b = {0};
    read_object("083fec39f677b81de863cb0b8575ac76e87b7bff", &a);
    read_object("fb5d38457520619b6e2f3b162c2a21a22b531cb4", &b);

    struct commit c_a, c_b;
    commit_from_object(&c_a, &a);
    commit_from_object(&c_b, &b);

    diff_commit_with_working_tree(&c_b);

    free_object(&a);
    free_object(&b);

    return 0;
}