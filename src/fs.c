#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/comp.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#include "includes.h"
#include "fs.h"
#include "objects.h"

int local_repo_exist()
{
    struct stat buffer;
    return stat(LOCAL_REPO, &buffer) == 0;
}

int write_object(struct object *obj)
{
    if(!local_repo_exist())
    {
        return -2;
    }
    int result = 0;

    struct stat buffer;
    if (stat(LOCAL_REPO"/objects", &buffer) != 0)
    {
        mkdir(LOCAL_REPO"/objects", DEFAULT_DIR_MODE);
    }
    
    DIR *objects_dir = opendir(LOCAL_REPO"/objects");
    int objects_dir_fd = dirfd(objects_dir);

    char checksum[DIGEST_LENGTH * 2];
    hash_object(obj, checksum);

    int save_file_fd = openat(objects_dir_fd, checksum, O_CREAT | O_WRONLY | O_TRUNC, 0774);
    if(save_file_fd == -1) {
        error_print("Object %s already exists", checksum);
        defer(-1);
    }
    FILE* save_file = fdopen(save_file_fd, "w");

    size_t data_size = object_size(obj);
    uLong comp_size = compressBound(data_size);
    char *compressed = malloc(comp_size);   
    
    int res = compress_object(obj, compressed, &comp_size);
    if (res != Z_OK)
    {
        defer(res);
    }
    fwrite(compressed, comp_size, 1, save_file);
    free(compressed);
    fclose(save_file);

defer:   
    closedir(objects_dir);
    return result;
}

int read_object(char *checksum, struct object *obj)
{
    if(!local_repo_exist())
    {
        return -2;
    }
    int result = 0;

    struct stat buffer;
    if (stat(LOCAL_REPO"/objects", &buffer) != 0)
    {
        error_print("Object dir does not exist");
        return -1;
    }

    DIR *objects_dir = opendir(LOCAL_REPO"/objects");
    int objects_dir_fd = dirfd(objects_dir);

    int save_file_fd = openat(objects_dir_fd, checksum, O_RDONLY, DEFAULT_FILE_MODE);    
    if (save_file_fd == -1)
    {
        error_print("Object %s does not exist", checksum);
        defer(-1);
    }
    fstatat(objects_dir_fd, checksum, &buffer, 0);
    char* file_content = malloc(buffer.st_size);

    FILE *save_file = fdopen(save_file_fd, "r");
    fread(file_content, 1, buffer.st_size, save_file);
    fclose(save_file);

    int res = uncompress_object(obj, file_content, buffer.st_size);
    if(res != 0)
    {
        defer(res);
    }
    
    free(file_content);

defer:
    closedir(objects_dir);
    return result;
}

int main(void)
{
    struct object obj = {0};
    obj.content = "Hello, world!\n";
    obj.size = strlen(obj.content);
    obj.object_type = "blob";

    return write_object(&obj);

    // read_object("af5626b4a114abcb82d63db7c8082c3c4756e51b", &obj);

    // debug_print("Object type is \"%s\"", obj.object_type);
    // debug_print("Content size is %li", obj.size);
    // debug_print("Content is %.*s", (int)obj.size, obj.content);

    // free_object(&obj);
    // return 0;
}