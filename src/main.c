// Requirements
// commit
// branching
// revert
// pull/push

#include <stdio.h>

#include "commit.h"

int main(int argc, char** argv) {
    // struct object obj = {0};
    // obj.content = "Hello, world!\n";
    // obj.size = strlen(obj.content);
    // obj.object_type = "blob";

    // return write_object(&obj);

    // read_object("af5626b4a114abcb82d63db7c8082c3c4756e51b", &obj);

    // debug_print("Object type is \"%s\"", obj.object_type);
    // debug_print("Content size is %li", obj.size);
    // debug_print("Content is %.*s", (int)obj.size, obj.content);

    // free_object(&obj);

    // add_to_index(&index, "src/fs.c");
    // add_to_index(&index, "src/test/foo");
    // add_to_index(&index, ".gitignore");
    // add_to_index(&index, "src/fs.h");

    // dump_index(&tree);

    commit();

    return 0;
}