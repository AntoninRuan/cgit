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
    struct object a = {0}, b = {0};
    read_object("5568f9722af893d2e6214617eefb62f40c9f8c69", &a);
    read_object("4640f65f9340784086a5ddee4e1a46cdc3274424", &b);

    struct commit c_a = {0}, c_b = {0};
    commit_from_object(&c_a, &a);
    commit_from_object(&c_b, &b);

    diff_commit(&c_a, &c_b);

    free_commit(&c_a);
    free_commit(&c_b);
    free_object(&a);
    free_object(&b);

    // struct tree index = {0};
    // load_index(&index);

    // add_to_index(&index, "src/commit.c");
    // add_to_index(&index, "src/commit.h");

    // save_index(&index);

    // commit();

    return 0;
}