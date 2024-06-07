#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "tree.h"
#include "includes.h"
#include "fs.h"
#include "objects.h"
#include "types.h"
#include "utils.h"

void free_entry(struct entry *entry)
{
    if (entry->checksum != NULL)
        free(entry->checksum);
    
    if (entry->filename != NULL)
        free(entry->filename);

}

void free_tree(tree_t *tree)
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

struct entry *find_entry(tree_t *tree, char* filename)
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

int add_to_tree(tree_t *tree, object_t *object, char *filename, enum file_mode mode)
{

    int new = 0;
    entry_t *entry = find_entry(tree, filename);
    if (entry == NULL)
    {
        new = 1;
        entry = calloc(sizeof(entry_t), 1);
    } else {
        free_entry(entry);
    }

    entry->type = object->object_type;
    entry->mode = mode;
    entry->filename = calloc(sizeof(char), strlen(filename) + 1);
    strncat(entry->filename, filename, strlen(filename));
    entry->checksum = calloc(sizeof(char), DIGEST_LENGTH);
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

int add_to_index(tree_t *index, char *filename, enum file_mode mode)
{
    int result = FS_OK;
    struct object object = {0};
    if (blob_from_file(filename, &object) != FS_OK)
    {
        return FILE_NOT_FOUND;
    }

    result = write_object(&object);
    if (result == FS_OK)
        add_to_tree(index, &object, filename, mode);

    if (result == OBJECT_ALREADY_EXIST)
        return FS_OK;

defer:
    free_object(&object);
    return result;
}

int remove_from_tree(tree_t *index, char *filename, int delete)
{
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

    if (delete)
        remove_object(entry->checksum);
    index->entries_size = index->entries_size - 1;
    free_entry(entry);
    free(entry);
    return FS_OK;
}

int tree_to_object(tree_t *tree, object_t *object)
{
    object->object_type = TREE;
    object->size = 0;
    object->content = NULL;

    struct entry *current = tree->first_entry;
    for(int i = 0; i < tree->entries_size; i++)
    {
        char *type = object_type_to_str(current->type);
        // Entry will be <mode>(in ASCII) + ' ' + <filename> + '\0' + <checksum>
        int mode_length = 6;
        if (current->mode == DIRECTORY)
            mode_length = 5;
        size_t entry_size = mode_length + 1 + strlen(current->filename) + 1 + DIGEST_LENGTH;
        char tmp[entry_size];
        sprintf(tmp, "%o %s", current->mode, current->filename);
        memcpy(tmp + entry_size - DIGEST_LENGTH, current->checksum, DIGEST_LENGTH);

        object->size = object->size + entry_size;
        object->content = realloc(object->content, object->size);
        memcpy((object->content) + object->size - entry_size, tmp, entry_size);
        current = current->next;
    }

    return 0;
}

int tree_from_object(tree_t *tree, object_t *object)
{
    tree->entries_size = 0;
    tree->first_entry = NULL;
    tree->last_entry = NULL;
    
    int i = 0, j = 0;
    while (j < object->size)
    {
        i = j;
        entry_t *entry = malloc(sizeof(entry_t));

        look_for(object->content, ' ', j);
        int pot_mode = strtol(object->content + i, NULL, 8);
        switch (pot_mode)
        {
            case DIRECTORY:
                entry->type = TREE;
                break;
            case REG_NONX_FILE:
            case REG_EXE_FILE:
            case SYM_LINK:
                entry->type = BLOB;
            case GIT_LINK:
                break;
            
            default:
                return INVALID_TREE;
                break;
        }
        entry->mode = (enum file_mode) pot_mode;

        i = j + 1;
        look_for(object->content, '\0', j);
        if (j - i == 0)
            return INVALID_TREE;
        entry->filename = malloc(j - i + 1);
        memcpy(entry->filename, object->content + i, j - i + 1);

        i = j + 1;
        entry->checksum = malloc(DIGEST_LENGTH);
        memcpy(entry->checksum, object->content + i, DIGEST_LENGTH);
        j += DIGEST_LENGTH + 1;

        entry->previous = tree->last_entry;
        entry->next = NULL;

        if(tree->last_entry == NULL)
        {
            tree->first_entry = entry;
        } else
        {
            entry->previous->next = entry;
        }
        tree->last_entry = entry;
        tree->entries_size ++;

    }

    return 0;
}

int add_object_to_tree(tree_t *tree, char* filename, enum file_mode mode, object_t *source)
{
    char top_folder_name[strlen(filename)];
    char path_left[strlen(filename)];
    int res = get_top_folder(filename, top_folder_name, path_left); 
    if (res > 0)
    {
        tree_t subtree = {0};
        object_t result = {0};
        result.object_type = TREE;
        entry_t *top_folder = find_entry(tree, top_folder_name);
        if(top_folder != NULL)
        {
            char checksum[DIGEST_LENGTH * 2 + 1];
            hash_to_hexa(top_folder->checksum, checksum);
            load_tree(checksum, &subtree);
            remove_from_tree(tree, top_folder->filename, 0);
        }

        add_to_tree(&subtree, source, path_left, mode);

        add_object_to_tree(&subtree, path_left, mode, source);

        tree_to_object(&subtree, &result);
        write_object(&result);
        remove_from_tree(tree, filename, 0);
        add_to_tree(tree, &result, top_folder_name, DIRECTORY);
        free_object(&result);
        free_tree(&subtree);
    } else {
        add_to_tree(tree, source, filename, mode);
        write_object(source);
    }
}
