#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "includes.h"
#include "fs.h"
#include "objects.h"
#include "utils.h"

void free_entry(struct entry *entry)
{
    if (entry->checksum != NULL)
        free(entry->checksum);
    
    if (entry->filename != NULL)
        free(entry->filename);

}

void free_tree(struct tree *tree)
{
    struct entry *current = tree->first_entry;
    struct entry *next;
    while(current != NULL)
    {
        free_entry(current);
        next = current->next;
        free(current);
        current = next;   
    }
}

struct entry *find_entry(struct tree *tree, char* filename)
{
    struct entry *current = tree->first_entry;
    for (int i = 0; i < tree->entries_size; ++i)
    {
        if (strcmp(filename, current->filename) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void get_tree(char* content, struct tree *tree)
{
    char* current_line;
    current_line = strtok(content, "\n");
    tree->entries_size = strtol(current_line, NULL, 10);
    tree->first_entry = NULL;
    tree->last_entry = NULL;

    for(int i = 0; i < tree->entries_size; i++)
    {
        struct entry *entry = calloc(1, sizeof(struct entry));
        current_line = strtok(NULL, "\n");
        int j = 0;
        while(current_line[j] != ' ')
        {
            j++;
        }

        current_line[j] = '\0';
        entry->type = str_to_object_type(current_line);
        current_line += j + 1;
        j = 0;
        while(current_line[j] != ' ')
        {
            j++;
        }

        char* checksum = calloc(j + 1, sizeof(char));
        char* filename = calloc(strlen(current_line) - j, sizeof(char));
        strncat(checksum, current_line, j);
        strncat(filename, current_line + j + 1, strlen(current_line) - j);
        entry->checksum = checksum;
        entry->filename = filename;
        entry->next = NULL;

        if(i == 0)
        {
            tree->first_entry = entry;
            tree->last_entry = entry;
        } else 
        {
            entry->previous = tree->last_entry;
            tree->last_entry->next = entry;
        }
        tree->last_entry = entry;
    }
}

int add_to_tree(struct tree *tree, struct object *object, char *filename)
{

    int new = 0;
    struct entry *entry = find_entry(tree, filename);
    if (entry == NULL)
    {
        new = 1;
        entry = calloc(sizeof(struct entry), 1);
    } else {
        free_entry(entry);
    }

    entry->type = object->object_type;
    entry->filename = calloc(sizeof(char), strlen(filename) + 1);
    strncat(entry->filename, filename, strlen(filename));
    entry->checksum = calloc(sizeof(char), DIGEST_LENGTH * 2 + 1);
    hash_object(object, entry->checksum);
    // int res = write_object(object);
    // if (res != FS_OK && res != OBJECT_ALREADY_EXIST)
    // {
    //     return res;
    // }
    
    if(new)
    {
        if (tree->entries_size == 0)
        {
            tree->first_entry = entry;
            tree->last_entry = entry;
        } else
        {
            struct entry *current = tree->first_entry;
            while (current != NULL && (strncmp(current->filename, entry->filename, strlen(entry->filename)) < 0))
            {
                current = current->next;
            }

            if (current == NULL)
            {
                tree->last_entry->next = entry;
                entry->previous = tree->last_entry;
                tree->last_entry = entry;
            } else {
                if(current == tree->first_entry)
                {
                    tree->first_entry = entry;
                } else {
                    current->previous->next = entry;
                    entry->previous = current->previous;
                }

                entry->next = current;
                current->previous = entry;
            }
        }
        tree->entries_size ++;
    }
}

int add_to_index(struct tree *index, char *filename)
{
    struct object object = {0};
    if (blob_from_file(filename, &object) != FS_OK)
    {
        return FILE_NOT_FOUND;
    }

    add_to_tree(index, &object, filename);
    write_object(&object);
    free_object(&object);
    return FS_OK;
}

int remove_from_index(struct tree *index, char *filename)
{
    if (!local_repo_exist() || !index_exist())
    {
        return REPO_NOT_INITIALIZED;
    }

    struct entry *entry = find_entry(index, filename);
    if (entry == NULL)
    {
        return ENTRY_NOT_FOUND;
    }

    if(index->first_entry == entry)
    {
        index->first_entry = entry->next;
    } else 
    {
        entry->previous->next = entry->next;
    }

    if (index->last_entry == entry)
    {
        index->last_entry = entry->previous;
    } else 
    {
        entry->next->previous = entry->previous;
    }

    index->entries_size = index->entries_size - 1;
    free_entry(entry);
    free(entry);
    return FS_OK;
}

int tree_to_object(struct tree *tree, struct object *object)
{
    object->object_type = TREE;
    object->content = calloc(1, decimal_len(tree->entries_size) + 2);
    object->size = decimal_len(tree->entries_size) + 2;
    sprintf(object->content, "%li\n", tree->entries_size);

    struct entry *current = tree->first_entry;
    for(int i = 0; i < tree->entries_size; i++)
    {
        char *type = object_type_to_str(current->type);
        size_t entry_size = strlen(type) + DIGEST_LENGTH * 2 + 3 + strlen(current->filename);
        char tmp[entry_size + 1];
        object->size = object->size + entry_size;
        object->content = realloc(object->content, object->size);
        sprintf(tmp, "%s %s %s\n", type, current->checksum, current->filename);
        strncat(object->content, tmp, entry_size);
        current = current->next;
    }

    object->size --;
}

int add_object_to_tree(struct tree *tree, char* filename, struct object *source)
{
    char top_folder_name[strlen(filename)];
    char path_left[strlen(filename)];
    int res = get_top_folder(filename, top_folder_name, path_left); 
    if (res > 0)
    {
        struct tree subtree = {0};
        struct object result = {0};
        result.object_type = TREE;
        struct entry *top_folder = find_entry(tree, top_folder_name);
        if(top_folder != NULL)
        {
            load_tree(top_folder->checksum, &subtree);
            remove_from_index(tree, top_folder->filename);
        }

        add_to_tree(&subtree, source, path_left);

        add_object_to_tree(&subtree, path_left, source);

        tree_to_object(&subtree, &result);
        write_object(&result);
        remove_from_index(tree, filename);
        add_to_tree(tree, &result, top_folder_name);
        free_object(&result);
        free_tree(&subtree);
    } else {
        add_to_tree(tree, source, filename);
        write_object(source);
    }
}
