#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "includes.h"
#include "commit.h"
#include "fs.h"
#include "objects.h"
#include "tree.h"

#define ARGS_MAX_SIZE 256

// TODO fix a bug during checkout that does not always appear in gdb
// Change branch comportement to avoid checking out at branch creation
// Change object format to be fully compatible with git (tree and commit)
// Add commit message

int print_help()
{
    printf("Usage: cgit init\n");
    printf("       cgit add [FILES]\n");
    printf("       cgit remove [FILES]\n");
    printf("       cgit commit -m [MESSAGE]\n");
    printf("       cgit diff <COMMIT1> [COMMIT2]\n");
    printf("       cgit branch [BRANCH]\n");
    printf("       cgit checkout [BRANCH]\n");
    printf("       cgit reset <COMMIT>\n");
    printf("       cgit log\n");
    return 0;
}

int pop_arg(int *argc, char ***argv, char *buf)
{  
    if (*argc == 0)
    {
        return 1;
    }

    strcpy(buf, *argv[0]);
    *argc -= 1;
    *argv += 1;

    return 0;
}

int add(int argc, char **argv)
{
    char buf[ARGS_MAX_SIZE];
    int res;

    res = pop_arg(&argc, &argv, buf);
    if (res != 0)
    {
        printf("No file specified\n");
        return 0;
    }

    struct tree index = {0};
    if (load_index(&index) == REPO_NOT_INITIALIZED)
    {
        printf("Not a cgit repository\n");
        return 128;
    }

    do {
        if (add_file_to_index(&index, buf) == FILE_NOT_FOUND)
        {
            printf("File %s does not exist\n", buf);
            return 1;
        }
        res = pop_arg(&argc, &argv, buf);
    } while (res == 0);

    save_index(&index);
    free_tree(&index);

    return 0;
}

int remove_cmd(int argc, char **argv) 
{
    char buf[ARGS_MAX_SIZE];
    int res;

    res = pop_arg(&argc, &argv, buf);
    if (res != 0)
    {
        printf("No file specified\n");
        return 0;
    }

    struct tree index = {0};
    if (load_index(&index) == REPO_NOT_INITIALIZED)
    {
        printf("Not a cgit repository\n");
        return 128;
    }

    do {
        remove_file_from_index(&index, buf);
        res = pop_arg(&argc, &argv, buf);
    } while (res == 0);

    save_index(&index);
    free_tree(&index);

    return 0;
}

int commit_cmd(int argc, char **argv)
{
    if (commit("") == REPO_NOT_INITIALIZED)
    {
        printf("Not a cgit repository\n");
        return 128;
    }
}

int diff(int argc, char **argv)
{
    char checksum_a[ARGS_MAX_SIZE];
    char checksum_b[ARGS_MAX_SIZE];

    if(pop_arg(&argc, &argv, checksum_a) == 1)
    {
        printf("No commit specified\n");
        return 0;
    }

    if(pop_arg(&argc, &argv, checksum_b) == 0)
    {
        if (diff_commit(checksum_a, checksum_b, 1) == OBJECT_DOES_NOT_EXIST)
        {
            if(errno == 1)
            {
                printf("Could not find commit %s\n", checksum_a);
            } else if (errno == 2)
            {
                printf("Could not find commit %s\n", checksum_b);
            }
            return 0;
        }
    } else 
    {
        if (diff_commit_with_working_tree(checksum_a, 1) == OBJECT_DOES_NOT_EXIST)
        {
            printf("Could not find commit %s\n", checksum_a);
            return 0;
        }
    }

    FILE *p = popen("cat "LOCAL_REPO"/last.diff | less -R", "w");
    pclose(p);
    return 0;
}

int checkout(int argc, char **argv)
{
    char buf[ARGS_MAX_SIZE] = {0};

    if(pop_arg(&argc, &argv, buf) == 1)
    {
        printf("No branch name specified\n");
        return 0;
    }

    debug_print("Checking out on %s", buf);
    if (checkout_branch(buf) == BRANCH_DOES_NOT_EXIST)
    {
        printf("Branch %s does not exist, use cgit branch <name> to create one\n", buf);
    }
}

int reset(int argc, char **argv)
{
    char buf[ARGS_MAX_SIZE];

    if(pop_arg(&argc, &argv, buf) == 1)
    {
        printf("You must give a commit to reset to\n");
        return 0;
    }

    int res = reset_to(buf);
    if (res == OBJECT_DOES_NOT_EXIST)
    {
        printf("There is no commit named %s\n", buf);
    } else if (res == WRONG_OBJECT_TYPE)
    {
        printf("Object %s is not a commit and thus cannot be reset to\n", buf);
    }
}

int branch(int argc, char **argv)
{
    char buf[ARGS_MAX_SIZE];

    if (pop_arg(&argc, &argv, buf) == 1)
    {
        if (dump_branches() == REPO_NOT_INITIALIZED)
        {
            printf("Not a cgit repository\n");
            return 128;
        }
        FILE *p = popen("cat "TMP"/branches | less", "w");
        pclose(p);
        return 0;
    } 

    if (new_branch(buf) == BRANCH_ALREADY_EXIST)
    {
        printf("Branch %s already exist\n", buf);
    }

    return 0;
}

int log_cmd(int argc, char **argv)
{
    dump_log();
    FILE *p = popen("cat "LOG_FILE" | less", "w");
    pclose(p);
}

int cat_file(int argc, char **argv)
{
    char buf[ARGS_MAX_SIZE];

    if(pop_arg(&argc, &argv, buf) == 1)
    {
        printf("usage: cgit cat-file <object>\n");
        return 129;
    }

    object_t obj = {0};
    int res = read_object(buf, &obj);
    if (res != FS_OK)
    {
        if (res == OBJECT_DOES_NOT_EXIST)
        {
            printf("fatal: not a valid object name %s\n", buf);
            return 128;
        }

        if (res == REPO_NOT_INITIALIZED)
        {
            printf("Not a cgit repository\n");
            return 128;
        }
    }

    cat_object(STDIN_FILENO, &obj);
    free_object(&obj);
    
    return 0;
}

int show_index(int argc, char **argv)
{
    tree_t index = {0};
    load_index(&index);

    object_t obj = {0};
    tree_to_object(&index, &obj);
    free_tree(&index);

    cat_object(STDIN_FILENO, &obj);
    free_object(&obj);

    return 0;
}

int main(int argc, char **argv)
{
    char cmd[ARGS_MAX_SIZE];
    char buf[ARGS_MAX_SIZE];

    pop_arg(&argc, &argv, cmd);
    if(pop_arg(&argc, &argv, buf) == 1)
    {
        print_help();
        return 0;
    }

    if (strcmp(buf, "init") == 0)
    {
        return init_repo();
    } else if (strcmp(buf, "add") == 0)
    {
        return add(argc, argv);
    } else if (strcmp(buf, "remove") == 0)
    {
        return remove_cmd(argc, argv);
    } else if (strcmp(buf, "commit") == 0)
    {
        return commit_cmd(argc, argv);
    } else if (strcmp(buf, "diff") == 0)
    {
        return diff(argc, argv);
    } else if (strcmp(buf, "branch") == 0)
    {
        return branch(argc, argv);
    } else if (strcmp(buf, "checkout") == 0)
    {
        return checkout(argc, argv);
    } else if (strcmp(buf, "reset") == 0)
    {
        return reset(argc, argv);
    } else if (strcmp(buf, "log") == 0)
    {
        return log_cmd(argc, argv);
    } else if (strcmp(buf, "-h") == 0) {
        return print_help();
    } else if (strcmp(buf, "cat-file") == 0) 
    {  
        return cat_file(argc, argv);
    } else if (strcmp(buf, "show-index") == 0) 
    {  
        return show_index(argc, argv);
    } else if (strcmp(buf, "test") == 0) 
    {  
        object_t obj = {0};
        read_object("192f287aebddb0080e6ea7cb567d76d78b54dee2", &obj);

        tree_t tree = {0};
        tree_from_object(&tree, &obj);
        free_object(&obj);

        debug_print("entries_size: %li", tree.entries_size);
        debug_print("first_entry->filename: %s", tree.first_entry->filename);
        debug_print("last_entry->filename: %s", tree.last_entry->filename);

        tree_to_object(&tree, &obj);
        write_object(&obj);

        free_tree(&tree);
        free_object(&obj);
    } else {
        printf("Unknown command %s, try using %s -h\n", buf, cmd);
        return 0;
    }
}