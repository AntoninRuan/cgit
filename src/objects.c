#include <assert.h>
#include <math.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "includes.h"
#include "fs.h"
#include "tree.h"
#include "utils.h"
#include "objects.h"

char *object_type_str[3] = {
    "blob",
    "tree",
    "commit",
};

char *object_type_to_str(enum object_type type)
{
    return object_type_str[type];
}

enum object_type str_to_object_type(char *str)
{
    if (strncmp("tree", str, 4) == 0)
    {
        return TREE;
    }
    else if (strncmp("commit", str, 5) == 0)
    {
        return COMMIT;
    }

    return BLOB;
}

size_t header_size(struct object *obj)
{
    return strlen(object_type_str[obj->object_type]) + 2 + decimal_len(obj->size);
}

size_t object_size(struct object *obj)
{
    return header_size(obj) + obj->size;
}

int full_object(struct object *obj, char *buffer, size_t buffer_size)
{
    char *hr_size = malloc(decimal_len(obj->size) + 1);
    sprintf(hr_size, "%li", obj->size);

    size_t data_size = object_size(obj);
    assert(data_size <= buffer_size);

    char *writing = buffer;
    char *type = object_type_str[obj->object_type];
    memcpy(writing, type, strlen(type));
    writing += strlen(type);
    memcpy(writing, " ", 1);
    writing++;
    memcpy(writing, hr_size, strlen(hr_size));
    writing += strlen(hr_size);
    memcpy(writing, "\0", 1);
    writing++;
    memcpy(writing, obj->content, obj->size);

    free(hr_size);

    return 0;
}

/// @brief Hash object and copy it in result
/// @param obj
/// @param result char array of size DIGEST_LENGTH, it is not a C-string.
void hash_object(object_t *obj, unsigned char *result)
{
    size_t data_size = object_size(obj);
    char data[data_size];
    full_object(obj, data, data_size);

    SHA1(data, data_size, result);
    return;
}

void hash_to_hexa(unsigned char *hash, char *result)
{
    for (int i = 0; i < DIGEST_LENGTH; i ++)
        sprintf((result + (2 * i)), "%.2x", hash[i]);

    return;
}

/// @brief Hash object and copy its hexa representation in result
/// @param obj
/// @param result A C-string of length equals DIGEST_LENGTH * 2
void hash_object_str(object_t *obj, char *result)
{
    unsigned char md_buffer[DIGEST_LENGTH] = {0};
    hash_object(obj, md_buffer);

    hash_to_hexa(md_buffer, result);
    return;
}

int uncompress_object(struct object *obj, char *compressed, uLongf comp_size)
{
    uLongf def_size = HEADER_MAX_SIZE;
    uLongf content_size = 0;
    char *deflated = malloc(def_size);
    int res = uncompress((Bytef *)deflated, &def_size, (Bytef *)compressed, comp_size);
    if (res != Z_OK && res != Z_BUF_ERROR)
    {
        return res;
    }
    int header_size = strlen(deflated) + 1;
    def_size = header_size;
    strtok(deflated, " ");
    char *hr_size = strtok(NULL, " ");
    content_size = strtol(hr_size, NULL, 10);
    def_size += content_size;
    deflated = realloc(deflated, def_size);
    res = uncompress((Bytef *)deflated, &def_size, (Bytef *)compressed, comp_size);
    if (res != Z_OK)
    {
        return res;
    }
    char *content_type_tmp = strtok(deflated, " ");

    obj->size = content_size;
    obj->object_type = str_to_object_type(content_type_tmp);
    obj->content = malloc(obj->size);
    memcpy(obj->content, deflated + header_size, obj->size);
    free(deflated);

    return 0;
}

int compress_object(struct object *obj, char *compressed, uLongf *comp_size)
{
    size_t data_size = object_size(obj);
    char *data = malloc(data_size);
    full_object(obj, data, data_size);

    int res = compress((Bytef *)compressed, comp_size, (Bytef *)data, data_size);
    free(data);

    return res;
}

int cat_object(int fd, object_t *obj)
{

    switch (obj->object_type)
    {
    case BLOB:
    case COMMIT:
        write(fd, obj->content, obj->size);
        break;

    case TREE:
        tree_t tree = {0};
        tree_from_object(&tree, obj);

        entry_t *current = tree.first_entry;
        while(current != NULL)
        {
            char buf[DIGEST_LENGTH * 2 + 1];
            hash_to_hexa(current->checksum, buf);
            dprintf(fd, "%.6o %s %s %s\n", current->mode, object_type_to_str(current->type), buf, current->filename);
            current = current->next;
        }

        free_tree(&tree);
        break;

    default:
        break;
    }

    free_object;
    return 0;
}

void free_object(struct object *obj)
{
    if (obj->content != NULL)
        free(obj->content);
}