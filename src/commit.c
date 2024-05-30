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
    load_index(&index);

    struct object last_commit = {0};
    struct commit commit = {0};
    char last_commit_checksum[DIGEST_LENGTH * 2 + 1] = {0};
    struct tree commit_tree = {0};
    get_last_commit(&last_commit);
    if (last_commit.size != 0) {
        hash_object(&last_commit, last_commit_checksum);
        commit_from_object(&commit, &last_commit);

        struct object last_commit_tree = {0};
        read_object(commit.tree, &last_commit_tree);
        get_tree(last_commit_tree.content, &commit_tree);
        free_object(&last_commit_tree);
    }
    free_object(&last_commit);

    struct entry *current = index.first_entry;
    while(current != NULL)
    {
        size_t entry_size = DIGEST_LENGTH * 2 + 2 + strlen(current->filename);
        char tmp[entry_size + 1];
        sprintf(tmp, "%s %s\n", current->checksum, current->filename);
        
        struct object object = {0};
        blob_from_file(current->filename, &object);

        add_object_to_tree(&commit_tree, current->filename, &object);

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
    hash_object(&commit_tree_obj, commit_tree_checksum);
    write_object(&commit_tree_obj);

    if(commit.tree != NULL)
    {
        free(commit.tree);
    }
    commit.tree = commit_tree_checksum;

    debug_print("Author: %s", commit.author);
    debug_print("Parent: %s", commit.parent);
    debug_print("Tree: %s", commit.tree);

    struct object commit_obj = {0};
    commit_to_object(&commit, &commit_obj);
    write_object(&commit_obj);
    char commit_checksum[DIGEST_LENGTH * 2 + 1];
    hash_object(&commit_obj, commit_checksum);

    update_head(commit_checksum);

    free_commit(&commit);
    free_object(&commit_obj);
    free_tree(&commit_tree);
    free_object(&commit_tree_obj);
    free_tree(&index);
    
    memset(&index, 0, sizeof(struct tree));
    save_index(&index);
}

int diff_blob(char *filename, struct object *a, struct object *b)
{   
    char a_path[7 + strlen(filename)], b_path[7 + strlen(filename)];
    sprintf(a_path, "/tmp/a%s", filename);
    sprintf(b_path, "/tmp/b%s", filename);

    tmp_dump(a, a_path);
    tmp_dump(b, b_path);

    char cmd[1000];
    FILE *f;

    snprintf(cmd, sizeof(cmd), "diff -u %s %s", a_path, b_path);
    f = popen(cmd, "w");
    pclose(f);
}

int diff_tree(char *cwd, struct tree *tree_a, struct tree *tree_b)
{  
    struct entry *current_a = NULL, *current_b = NULL;
    if (tree_a == NULL)
        current_a = NULL;
    else 
        current_a = tree_a->first_entry;

    if (tree_b == NULL)
        current_b = NULL;
    else
        current_b = tree_b->first_entry;
    
    while(current_a != NULL || current_b != NULL)
    {
        int res = 0;
        if (current_a == NULL)
        {
            res = 1;
        } else if (current_b == NULL)
        {
            res = -1;
        } else {
            res = strcmp(current_a->filename, current_b->filename);
        }
        size_t cwd_len = 0;
        if (*cwd != '\0')
            cwd_len = strlen(cwd);
        size_t filename_size = cwd_len + 1;
        char *filename = calloc(1, filename_size);
        strncpy(filename, cwd, filename_size);
        struct object a = {0}, b = {0};
        enum object_type type;
        if (res == 0)
        {
            read_object(current_a->checksum, &a);
            read_object(current_b->checksum, &b);
            if (current_a->type != current_b->type)
                return WRONG_OBJECT_TYPE;
            filename_size += strlen(current_a->filename) + 1;
            filename = realloc(filename, filename_size);
            strcat(filename, "/");
            strncat(filename, current_a->filename, strlen(current_a->filename));
            type = current_a->type;
            current_a = current_a->next;
            current_b = current_b->next;
        } else if (res < 0)
        {
            read_object(current_a->checksum, &a);
            filename_size += strlen(current_a->filename) + 1;
            filename = realloc(filename, filename_size);
            strcat(filename, "/");
            strncat(filename, current_a->filename, strlen(current_a->filename));
            type = current_a->type;
            current_a = current_a->next;
        } else
        {
            read_object(current_b->checksum, &b);
            filename_size += strlen(current_b->filename) + 1;
            filename = realloc(filename, filename_size);
            strcat(filename, "/");
            strncat(filename, current_b->filename, strlen(current_b->filename));
            type = current_b->type;
            current_b = current_b->next;
        }

        if (type == BLOB)
            diff_blob(filename, &a, &b);
        else if (type == TREE)
        {
            struct tree subtree_a = {0}, subtree_b = {0};
            if (res <= 0)
                get_tree(a.content, &subtree_a);

            if (res >= 0)
                get_tree(b.content, &subtree_b);

            init_tmp_diff_dir(filename);
            diff_tree(filename, &subtree_a, &subtree_b);
            free_tree(&subtree_a);
            free_tree(&subtree_b);
        }
        free(filename);
        free_object(&a);
        free_object(&b);
    }
}

void diff_commit(struct commit *commit_a, struct commit *commit_b)
{
    struct object tree_obj_a, tree_obj_b;
    read_object(commit_a->tree, &tree_obj_a);
    read_object(commit_b->tree, &tree_obj_b);

    struct tree tree_a, tree_b;
    get_tree(tree_obj_a.content, &tree_a);
    get_tree(tree_obj_b.content, &tree_b);

    diff_tree("", &tree_a, &tree_b);

    free_tree(&tree_a);
    free_tree(&tree_b);
    free_object(&tree_obj_a);
    free_object(&tree_obj_b);
}