#ifndef FS_H
#define FS_H 1

#include "types.h"

#define LOCAL_REPO ".cgit"
#define INDEX_FILE LOCAL_REPO"/index"
#define OBJECTS_DIR LOCAL_REPO"/objects"
#define REFS_DIR LOCAL_REPO"/refs"
#define HEADS_DIR REFS_DIR"/heads"
#define HEAD_FILE LOCAL_REPO"/HEAD"

#define TMP "/tmp"

#define DEFAULT_DIR_MODE 0755
#define DEFAULT_FILE_MODE 0444

#define FS_OK (0)
#define FS_ERROR (-1)
#define COMPRESSION_ERROR (-2)
#define REPO_NOT_INITIALIZED (-10)
#define OBJECT_ALREADY_EXIST (-20)
#define OBJECT_DOES_NOT_EXIST (-21)
#define WRONG_OBJECT_TYPE (-22)
#define BRANCH_ALREADY_EXIST (-23)
#define FILE_NOT_FOUND (-30)
#define ENTRY_NOT_FOUND (-31)

int local_repo_exist();
int index_exist();

int init_repo();
int blob_from_file(char *filename, struct object *object);
int write_object(struct object *obj);
int read_object(char *checksum, struct object *obj);
int save_index(struct tree *tree);
int load_index(struct tree *index);
int load_tree(char* checksum, struct tree *tree);
int update_head(char *new_head);
int get_last_commit(struct object *commit);
int tmp_dump(struct object *obj, char* filename);
int init_tmp_diff_dir(char* dir);
int new_branch(char* branch_name);

#endif // FS_H
