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

#define parse_field(commit, ptr, field, k_ofs, v_ofs, endline) \
    if (strcmp(ptr + k_ofs, #field) == 0) \
    { \
        commit->field = malloc(endline - v_ofs);\
        commit->field[endline - v_ofs - 1] = '\0'; \
        memcpy(commit->field, ptr + v_ofs + 1, endline - v_ofs - 1);\
    }

int commit_from_object(commit_t *commit, object_t *object)
{
    int i = 0;
    while (i < object->size)
    {
        int endline = i;
        look_for(object->content, '\n', endline);
        if (endline - i == 0) // Two consecutives lines feed: Begin of commit message
        {
            i = endline + 1;
            break;
        }

        int j = i;
        look_for(object->content, ' ', j);
        object->content[j] = '\0';

        parse_field(commit, object->content, tree, i, j, endline)
        else parse_field(commit, object->content, parent, i, j, endline)
        else parse_field(commit, object->content, author, i, j, endline)
        else parse_field(commit, object->content, committer, i, j, endline)

        object->content[j] = ' ';
        i = endline + 1;
    }

    if(object->size - i > 0)
    {
        debug_print("There is a commit msg");
        commit->message = malloc(object->size - i + 1);
        commit->message[object->size - i] = '\0';
        memcpy(commit->message, object->content + i, object->size - i);
    }

    return 0;
}

int commit_to_object(commit_t *commit, object_t *object)
{
    object->object_type = COMMIT;
    object->size = 5 + strlen(commit->tree) + 2; // len('tree ' + <tree> + '\n' + '\0')
    object->content = malloc(object->size);
    sprintf(object->content, "tree %s\n", commit->tree);

    if (commit->parent != NULL)
    {
        object->size += 7 + strlen(commit->parent) + 1; // len('parent ' + <parent> + '\n')
        object->content = realloc(object->content, object->size);
        strcat(object->content, "parent ");
        strncat(object->content, commit->parent, strlen(commit->parent));
        strcat(object->content, "\n");
    }

    object->size += 7 + strlen(commit->author) + 1; // len('author ' + <author> + '\n')
    object->content = realloc(object->content, object->size);
    strcat(object->content, "author ");
    strncat(object->content, commit->author, strlen(commit->author));
    strcat(object->content, "\n");

    object->size += 10 + strlen(commit->committer) + 2; // len('committer ' + <committer> + '\n' + '\n')
    object->content = realloc(object->content, object->size);
    strcat(object->content, "committer ");
    strncat(object->content, commit->author, strlen(commit->author));
    strcat(object->content, "\n\n");
    
    object->size += strlen(commit->message) + 1;
    object->content = realloc(object->content, object->size);
    strncat(object->content, commit->message, strlen(commit->message));
    strcat(object->content, "\n");

    object->size --;
    return 0;
}

void free_commit(commit_t *commit)
{
    if (commit->author != NULL)
        free(commit->author);
    
    if (commit->committer != NULL)
        free(commit->committer);

    if (commit->parent != NULL)
        free(commit->parent);

    if (commit->tree != NULL)
        free(commit->tree);

    if (commit->message != NULL)
        free(commit->message);
}

int commit(char *msg)
{
    tree_t index = {0};
    if (load_index(&index) == REPO_NOT_INITIALIZED)
    {
        return REPO_NOT_INITIALIZED;
    }

    object_t last_commit = {0};
    commit_t commit = {0};
    char last_commit_checksum[DIGEST_LENGTH * 2 + 1] = {0};
    tree_t commit_tree = {0};
    get_last_commit(&last_commit);
    if (last_commit.size != 0) {
        hash_object_str(&last_commit, last_commit_checksum);
        commit_from_object(&commit, &last_commit);

        object_t last_commit_tree = {0};
        read_object(commit.tree, &last_commit_tree);
        tree_from_object(&commit_tree, &last_commit_tree);
        free_object(&last_commit_tree);
    }
    free_object(&last_commit);

    entry_t *current = index.first_entry;
    while(current != NULL)
    {   
        object_t object = {0};
        blob_from_file(current->filename, &object);

        add_object_to_tree(&commit_tree, current->filename, current->mode, &object);

        free_object(&object);
        current = current->next;
    }

    char* author = "Antonin";
    commit.author = realloc(commit.author, strlen(author) + 1);
    commit.committer = realloc(commit.committer, strlen(author) + 1);
    sprintf(commit.author, "%s", author);
    sprintf(commit.committer, "%s", author);
    
    if (last_commit.size != 0)
    {
        commit.parent = realloc(commit.parent, DIGEST_LENGTH * 2 + 1);
        sprintf(commit.parent, "%s", last_commit_checksum);
    }

    if (msg[0] != '\0')
    {    
        commit.message =  calloc(1, strlen(msg) + 1);
        sprintf(commit.message, "%s", msg);
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