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

    // debug_print("master: %i", branch_exist("master"));
    // debug_print("main: %i", branch_exist("main"));
    // struct tree index = {0};
    // load_index(&index);

    // add_to_index(&index, "src/commit.c");
    // add_to_index(&index, "src/commit.h");

    // save_index(&index);

    // commit();

    new_branch("new_feature");

    return 0;
}