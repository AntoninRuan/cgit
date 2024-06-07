#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "commit.h"
#include "fs.h"
#include "includes.h"
#include "objects.h"
#include "tree.h"
#include "types.h"

int commit_from_object(struct commit *commit, struct object *object)
{
    char tmp[object->size + 1];
    memcpy(tmp, object->content, object->size);
    tmp[object->size] = '\0';
    char *current_line = strtok(tmp, "\n");
    commit->tree = malloc(strlen(current_line) + 1);
    sprintf(commit->tree, "%s", current_line);

    current_line = strtok(NULL, "\n");
    commit->parent = malloc(strlen(current_line) + 1);
    sprintf(commit->parent, "%s", current_line);

    current_line = strtok(NULL, "\n");
    commit->author = malloc(strlen(current_line) + 1);
    sprintf(commit->author, "%s", current_line);
    return 0;
}

int commit_to_object(struct commit *commit, struct object *object)
{
    object->object_type = COMMIT;
    object->size = strlen(commit->tree) + 2;
    object->content = malloc(object->size);
    sprintf(object->content, "%s\n", commit->tree);

    object->size += strlen(commit->parent) + 1;
    object->content = realloc(object->content, object->size);
    strncat(object->content, commit->parent, strlen(commit->parent));
    strcat(object->content, "\n");

    object->size += strlen(commit->author);
    object->content = realloc(object->content, object->size);
    strncat(object->content, commit->author, strlen(commit->author));

    object->size --;
}

void free_commit(struct commit *commit)
{
    if (commit->author != NULL)
        free(commit->author);
    
    if (commit->parent != NULL)
        free(commit->parent);

    if (commit->tree != NULL)
        free(commit->tree);
}

int commit()
{
    struct tree index = {0};
    if (load_index(&index) == REPO_NOT_INITIALIZED)
    {
        return REPO_NOT_INITIALIZED;
    }

    struct object last_commit = {0};
    struct commit commit = {0};
    char last_commit_checksum[DIGEST_LENGTH * 2 + 1] = {0};
    struct tree commit_tree = {0};
    get_last_commit(&last_commit);
    if (last_commit.size != 0) {
        hash_object_str(&last_commit, last_commit_checksum);
        commit_from_object(&commit, &last_commit);

        struct object last_commit_tree = {0};
        read_object(commit.tree, &last_commit_tree);
        tree_from_object(&commit_tree, &last_commit_tree);
        free_object(&last_commit_tree);
    }
    free_object(&last_commit);

    struct entry *current = index.first_entry;
    while(current != NULL)
    {   
        struct object object = {0};
        blob_from_file(current->filename, &object);

        add_object_to_tree(&commit_tree, current->filename, current->mode, &object);

        free_object(&object);
        current = current->next;
    }

    if (commit.author == NULL)
    {
        char* author = "Antonin";
        commit.author = malloc(strlen(author) + 1);
        sprintf(commit.author, "%s", author);
    }


    commit.parent = realloc(commit.parent, DIGEST_LENGTH * 2 + 1);
    if (last_commit.size == 0)
    {
        sprintf(commit.parent, "%s", " ");
    } else
    {
        sprintf(commit.parent, "%s", last_commit_checksum);
    }

    struct object commit_tree_obj = {0};
    tree_to_object(&commit_tree, &commit_tree_obj);
    char *commit_tree_checksum = malloc(DIGEST_LENGTH * 2 + 1);
    hash_object_str(&commit_tree_obj, commit_tree_checksum);
    write_object(&commit_tree_obj);

    if(commit.tree != NULL)
    {
        free(commit.tree);
    }
    commit.tree = commit_tree_checksum;

    struct object commit_obj = {0};
    commit_to_object(&commit, &commit_obj);
    write_object(&commit_obj);
    char commit_checksum[DIGEST_LENGTH * 2 + 1];
    hash_object_str(&commit_obj, commit_checksum);

    update_current_branch_head(commit_checksum);

    free_commit(&commit);
    free_object(&commit_obj);
    free_tree(&commit_tree);
    free_object(&commit_tree_obj);
    free_tree(&index);
    
    memset(&index, 0, sizeof(struct tree));
    save_index(&index);
}

int diff_commit_with_working_tree(char *checksum, int for_print)
{
    struct object commit_obj = {0};
    if (read_object(checksum, &commit_obj) == OBJECT_DOES_NOT_EXIST)
    {
        return OBJECT_DOES_NOT_EXIST;
    }

    if (commit_obj.object_type != COMMIT)
        return WRONG_OBJECT_TYPE;

    struct commit commit = {0};
    commit_from_object(&commit, &commit_obj);

    struct object obj = {0};
    read_object(commit.tree, &obj);

    struct tree commit_tree = {0};
    tree_from_object(&commit_tree, &obj);

    remove_dir(TMP"/a");
    create_dir(TMP"/a");

    dump_tree(TMP"/a", &commit_tree);

#define DIFF_WT_BASE_CMD "diff -ru%s --exclude-from=.gitignore --color=%s "TMP"/a ./ > "LOCAL_REPO"/last.diff"

    char cmd[strlen(DIFF_WT_BASE_CMD) + 8];
    if (for_print)
    {
        sprintf(cmd, DIFF_WT_BASE_CMD, "N", "always");
    } else 
    {
        sprintf(cmd, DIFF_WT_BASE_CMD, "", "never");
    }

    FILE *p = popen(cmd, "w");
    pclose(p);

    free_tree(&commit_tree);
    free_commit(&commit);
    free_object(&obj);
    free_object(&commit_obj);
}

int diff_commit(char* checksum_a, char* checksum_b, int for_print)
{
    struct object commit_a_obj = {0}, commit_b_obj = {0};
    if (read_object(checksum_a, &commit_a_obj) == OBJECT_DOES_NOT_EXIST)
    {
        errno = 1;
        return OBJECT_DOES_NOT_EXIST;
    }
    if (read_object(checksum_b, &commit_b_obj) == OBJECT_DOES_NOT_EXIST)
    {
        free_object(&commit_a_obj);
        errno = 2;
        return OBJECT_DOES_NOT_EXIST;
    }

    struct commit commit_a = {0}, commit_b = {0};
    commit_from_object(&commit_a, &commit_a_obj);
    commit_from_object(&commit_b, &commit_b_obj);
    
    struct object tree_a_obj, tree_b_obj;
    read_object(commit_a.tree, &tree_a_obj);
    read_object(commit_b.tree, &tree_b_obj);

    struct tree tree_a, tree_b;
    tree_from_object(&tree_a, &tree_a_obj);
    tree_from_object(&tree_b, &tree_b_obj);

    remove_dir(TMP"/a");
    remove_dir(TMP"/b");

    create_dir(TMP"/a");
    create_dir(TMP"/b");

    dump_tree(TMP"/a", &tree_a);
    dump_tree(TMP"/b", &tree_b);

    FILE *f = popen("diff -ruN "TMP"/a "TMP"/b --color=always> "LOCAL_REPO"/last.diff", "w");
    pclose(f);

    free_tree(&tree_a);
    free_tree(&tree_b);
    free_commit(&commit_a);
    free_commit(&commit_b);
    free_object(&tree_a_obj);
    free_object(&tree_b_obj);
    free_object(&commit_a_obj);
    free_object(&commit_b_obj);
    return 0;
}