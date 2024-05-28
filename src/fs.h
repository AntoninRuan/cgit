#ifndef FS_H
#define FS_H 1

#define LOCAL_REPO ".cgit"
#define DEFAULT_DIR_MODE 0755
#define DEFAULT_FILE_MODE 0444

int write_object(struct object *obj);
int read_object(char *checksum, struct object *obj);

#endif // FS_H
