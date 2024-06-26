#ifndef OBJECTS_H
#define OBJECTS_H 1

#include <stddef.h>
#include <zconf.h>

#include "types.h"

#define HEADER_MAX_SIZE 20

char* object_type_to_str(enum object_type type);
enum object_type str_to_object_type(char* str);
size_t object_size(struct object *obj);
int full_object(struct object *obj, char* buffer, size_t buffer_size);
int uncompress_object(struct object *obj, char* compressed, uLongf comp_size);
int compress_object(struct object *obj, char* compressed, uLongf *comp_size);
void hash_object(object_t *obj, unsigned char *result);
void hash_object_str(struct object *obj, char* result);
void hash_to_hexa(unsigned char *hash, char *result);
int cat_object(int fd, object_t *obj);
void free_object(struct object *obj);

#endif // OBJECTS_H
