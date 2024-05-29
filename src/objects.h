#ifndef OBJECTS_H
#define OBJECTS_H 1

#include <stddef.h>
#include <zconf.h>

#define DIGEST_LENGTH 20
#define HEADER_MAX_SIZE 20

struct object
{
    char* content;
    size_t size;
    char* object_type;
};

size_t object_size(struct object *obj);
int full_object(struct object *obj, char* buffer, size_t buffer_size);
int uncompress_object(struct object *obj, char* compressed, uLongf comp_size);
int compress_object(struct object *obj, char* compressed, uLongf *comp_size);
void hash_object(struct object *obj, char* result);
int blob_from_file(char *filename, struct object *object);
void free_object(struct object *obj);

#endif // OBJECTS_H
