#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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