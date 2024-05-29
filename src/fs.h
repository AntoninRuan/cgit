#ifndef FS_H
#define FS_H 1

#include "objects.h"

#define LOCAL_REPO ".cgit"
#define INDEX_FILE LOCAL_REPO"/index"
#define OBJECTS_REPO LOCAL_REPO"/objects"
#define DEFAULT_DIR_MODE 0755
#define DEFAULT_FILE_MODE 0444

#define FS_OK (0)
#define FS_ERROR (-1)
#define COMPRESSION_ERROR (-2)
#define REPO_NOT_INITIALIZED (-10)
#define OBJECT_ALREADY_EXIST (-20)
#define OBJECT_DOES_NOT_EXIST (-21)
#define FILE_NOT_FOUND (-22)
#define ENTRY_NOT_FOUND (-23)

int local_repo_exist();
int index_exist();

int write_object(struct object *obj);
int read_object(char *checksum, struct object *obj);

#endif // FS_H
