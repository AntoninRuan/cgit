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
    } else {
        printf("Unknown command %s, try using %s -h\n", buf, cmd);
        return 0;
    }
}