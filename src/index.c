#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "includes.h"
#include "fs.h"

void free_entry(struct entry *entry)
{
    if (entry->checksum != NULL)
        free(entry->checksum);
    
    if (entry->filename != NULL)
        free(entry->filename);

}

void free_index(struct index *index)
{
    struct entry *current = index->first_entry;
    struct entry *next;
    while(current != NULL)
    {
        free_entry(current);
        next = current->next;
        free(current);
        current = next;   
    }
}

struct entry *find_entry(struct index *index, char* filename)
{
    struct entry *current = index->first_entry;
    for (int i = 0; i < index->entries_size; i ++)
    {
        if (strncmp(filename, current->filename, strlen(filename)) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

int add_to_index(struct index *index, char *filename)
{
    if(!local_repo_exist() || !index_exist())
    {
        return REPO_NOT_INITIALIZED;
    }
    
    int result = FS_OK;
    int new = 0;

    struct entry *entry = find_entry(index, filename);
    if (entry == NULL)
    {
        new = 1;
        entry = calloc(sizeof(struct entry), 1);
    } else {
        free_entry(entry);
    }

    struct object object = {0};
    if (blob_from_file(filename, &object) != 0)
    {
        return FILE_NOT_FOUND;
    }

    entry->filename = calloc(sizeof(char), strlen(filename) + 1);
    strncat(entry->filename, filename, strlen(filename));
    entry->checksum = calloc(sizeof(char), DIGEST_LENGTH * 2 + 1);
    hash_object(&object, entry->checksum);
    int res = write_object(&object);
    if (res != FS_OK && res != OBJECT_ALREADY_EXIST)
    {
        defer(FS_ERROR);
    }
    
    if(new)
    {
        if(index->entries_size == 0)
        {
            index->first_entry = entry;
            index->last_entry = entry;
        } else
        {
            entry->previous = index->last_entry;
            index->last_entry->next = entry;
            index->last_entry = entry;
        }
        index->entries_size = index->entries_size + 1;
    }

defer:
    free_object(&object);
    return result;
}

int remove_from_index(struct index *index, char *filename)
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