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

size_t header_size(struct object *obj)
{
    return strlen(obj->object_type) + 2 + decimal_len(obj->size);
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
    memcpy(writing, obj->object_type, strlen(obj->object_type));
    writing += strlen(obj->object_type);
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
    obj->object_type = malloc(strlen(content_type_tmp) + 1);
    strncpy(obj->object_type, content_type_tmp, strlen(content_type_tmp) + 1);
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

int blob_from_file(char *filename, struct object *object)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL)
        return -1;

    struct stat file_info;
    if (stat(filename, &file_info) != 0)
        return -1;
    
    object->object_type = "blob";
    object->size = file_info.st_size;
    object->content = realloc(object->content, object->size);
    fread(object->content, 1, object->size, file);
    fclose(file);

    return 0;
}

void free_object(struct object *obj)
{
    if (obj->content != NULL)
        free(obj->content);
}

// int main(void)
// {
//     char result[SHA_DIGEST_LENGTH * 2] = {0};

//     char* str = "Hello, world!\n";
//     hash_object(str, strlen(str), "blob", result);
//     printf("%s\n", result);
// }