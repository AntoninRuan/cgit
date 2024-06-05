#include <assert.h>
#include <math.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "includes.h"
#include "utils.h"
#include "objects.h"

char *object_type_str[3] = {
    "blob", 
    "tree",
    "commit",
};

char* object_type_to_str(enum object_type type)
{
    return object_type_str[type];
}

enum object_type str_to_object_type(char* str)
{
    if(strncmp("tree", str, 4) == 0)
    {
        return TREE;
    } else if (strncmp("commit", str, 5) == 0)
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
    char* hr_size = malloc(decimal_len(obj->size) + 1);
    sprintf(hr_size, "%li", obj->size);

    size_t data_size = object_size(obj);
    assert(data_size <= buffer_size);

    char *writing = buffer;
    char *type = object_type_str[obj->object_type];
    memcpy(writing, type, strlen(type));
    writing += strlen(type);
    memcpy(writing, " ", 1);
    writing ++;
    memcpy(writing, hr_size, strlen(hr_size));
    writing += strlen(hr_size);
    memcpy(writing, "\0", 1);
    writing ++;
    memcpy(writing, obj->content, obj->size);

    free(hr_size);

    return 0;
}

void hash_object(struct object *obj, char *result)
{
    unsigned char md_buffer[DIGEST_LENGTH] = {0};
    size_t data_size = object_size(obj);
    char* data = malloc(data_size);
    full_object(obj, data, data_size);

    SHA1(data, data_size, md_buffer);

    for (int i = 0; i < DIGEST_LENGTH; i++)
    {
        sprintf((result + (2 * i)), "%.2x", md_buffer[i]);
    }

    free(data);
}

int uncompress_object(struct object *obj, char* compressed, uLongf comp_size)
{
    uLongf def_size = HEADER_MAX_SIZE;
    uLongf content_size = 0;
    char* deflated = malloc(def_size);
    int res = uncompress((Bytef *) deflated, &def_size, (Bytef *)compressed, comp_size);
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
    res = uncompress((Bytef *) deflated, &def_size, (Bytef *)compressed, comp_size);
    if(res != Z_OK) {
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

int compress_object(struct object *obj, char* compressed, uLongf *comp_size)
{
    size_t data_size = object_size(obj);
    char* data = malloc(data_size);
    full_object(obj, data, data_size);

    int res = compress((Bytef *)compressed, comp_size, (Bytef *)data, data_size);
    free(data);

    return res;
}

void free_object(struct object *obj)
{
    if (obj->content != NULL)
        free(obj->content);
}