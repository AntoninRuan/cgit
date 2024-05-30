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

#include "fs.h"
#include "includes.h"
#include "tree.h"
#include "objects.h"
#include "utils.h"

int local_repo_exist()
{
    struct stat buffer;
    return stat(LOCAL_REPO, &buffer) == 0;
}

int index_exist()
{
    struct stat buffer;
    return stat(INDEX_FILE, &buffer) == 0;
}

int heads_dir_exist()
{
    struct stat buffer;
    return stat(HEADS_DIR, &buffer) == 0; 
}

int head_file_exist(size_t *head_size)
{
    struct stat buffer;
    int result = stat(HEAD_FILE, &buffer) == 0;
    if(head_size != NULL)
        *head_size = buffer.st_size;
    return result;
}

int init_repo()
{
    if(local_repo_exist())
    {
        return 0;
    }

    mkdir(LOCAL_REPO, DEFAULT_DIR_MODE);

    struct stat buffer = {0};
    if (stat(OBJECTS_DIR, &buffer) != 0)
        mkdir(OBJECTS_DIR, DEFAULT_DIR_MODE);

    if (!index_exist())
    {
        int fd = open(INDEX_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        FILE *index = fdopen(fd, "w");

        fprintf(index, "0\n");
        fclose(index);
    }

    if (!head_file_exist(NULL))
    {
        open(HEAD_FILE, O_CREAT, 0644);
    }

    if (stat(REFS_DIR, &buffer) != 0)
    {
        mkdir(REFS_DIR, DEFAULT_DIR_MODE);
    }

    if (!heads_dir_exist())
    {
        mkdir(HEADS_DIR, DEFAULT_DIR_MODE);
    }
}

int blob_from_file(char *filename, struct object *object)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        error_print("File %s not found", filename);
        return FILE_NOT_FOUND;
    }

    struct stat file_info;
    if (stat(filename, &file_info) != 0)
        return -1;
    
    object->object_type = str_to_object_type("blob");
    object->size = file_info.st_size;
    object->content = realloc(object->content, object->size);
    fread(object->content, 1, object->size, file);
    fclose(file);

    return FS_OK;
}

int write_object(struct object *obj)
{
    if(!local_repo_exist())
    {
        return REPO_NOT_INITIALIZED;
    }
    int result = FS_OK;

    struct stat buffer;
    if (stat(OBJECTS_DIR, &buffer) != 0)
    {
        mkdir(OBJECTS_DIR, DEFAULT_DIR_MODE);
    }
    
    DIR *objects_dir = opendir(OBJECTS_DIR);
    int objects_dir_fd = dirfd(objects_dir);

    char checksum[DIGEST_LENGTH * 2];
    hash_object(obj, checksum);

    int save_file_fd = openat(objects_dir_fd, checksum, O_CREAT | O_WRONLY | O_TRUNC, DEFAULT_FILE_MODE);
    if(save_file_fd == -1) {
        if (errno == EACCES)
        {
            debug_print("Object %s already exists", checksum);
            defer(OBJECT_ALREADY_EXIST);
        }
        defer(FS_ERROR);
    }
    FILE* save_file = fdopen(save_file_fd, "w");

    size_t data_size = object_size(obj);
    uLong comp_size = compressBound(data_size);
    char *compressed = malloc(comp_size);   
    
    int res = compress_object(obj, compressed, &comp_size);
    if (res != Z_OK)
    {
        defer(COMPRESSION_ERROR);
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
        return REPO_NOT_INITIALIZED;
    }
    int result = FS_OK;

    struct stat buffer;
    if (stat(OBJECTS_DIR, &buffer) != 0)
    {
        error_print("Object dir does not exist");
        return OBJECT_DOES_NOT_EXIST;
    }

    DIR *objects_dir = opendir(OBJECTS_DIR);
    int objects_dir_fd = dirfd(objects_dir);

    fstatat(objects_dir_fd, checksum, &buffer, 0);
    char file_content[buffer.st_size];
    int save_file_fd = openat(objects_dir_fd, checksum, O_RDONLY, DEFAULT_FILE_MODE);    
    if (save_file_fd == -1)
    {
        if(errno == EACCES)
        {    
            error_print("Object %s does not exist", checksum);
            defer(OBJECT_DOES_NOT_EXIST);
        }
        error_print("Cannot open file %s", checksum);
        defer(FS_ERROR);
    }

    FILE *save_file = fdopen(save_file_fd, "r");
    fread(file_content, 1, buffer.st_size, save_file);
    fclose(save_file);

    result = uncompress_object(obj, file_content, buffer.st_size);

defer:
    closedir(objects_dir);
    return result;
}

int create_dir(char *dir)
{   
    struct stat buffer = {0};
    int result = FS_OK;

    if(stat(dir, &buffer) == 0)
    {
        if (!S_ISREG(buffer.st_mode))
        {
            result = remove(dir);
            if (result != 0)
                return FS_ERROR;
        }
    } else {
        result = mkdir(dir, DEFAULT_DIR_MODE);
        if (result != 0)
            return FS_ERROR;
    }
}

int init_tmp_diff_dir(char* dir)
{
    int dirlen = 0;
    if (*dir != '\0')
        dirlen = strlen(dir);
    char path_a[dirlen + 8], path_b[dirlen + 8];
    sprintf(path_a, "/tmp/a/%s", dir);
    sprintf(path_b, "/tmp/b/%s", dir);

    if (create_dir(path_a) != FS_OK)
        return FS_ERROR;
    if (create_dir(path_b) != FS_OK)
        return FS_ERROR;
    

    return FS_OK;
}

int tmp_dump(struct object *obj, char* filename)
{
    if (obj == NULL)
    {
        open(filename, O_CREAT, 0644);
        return FS_OK;
    }

    FILE *file = fopen(filename, "w");
    fwrite(obj->content, obj->size, 1, file);
    fclose(file);

    return FS_OK;
}

int load_tree(char* checksum, struct tree *tree)
{
    struct object object;
    int res = read_object(checksum, &object);
    if (res != FS_OK)
        return res;

    if (object.object_type != TREE)
    {
        error_print("Object %s is not a tree", checksum);
        return WRONG_OBJECT_TYPE;
    }

    get_tree(object.content, tree);
    free_object(&object);

    return 0;
}

int load_index(struct tree *index)
{
    if(!local_repo_exist() || !index_exist())
    {
        return REPO_NOT_INITIALIZED;
    }

    FILE* index_file = fopen(INDEX_FILE, "r");
    struct stat buffer;
    stat(INDEX_FILE, &buffer);

    char* file_content = calloc(buffer.st_size + 1, sizeof(char));
    fread(file_content, buffer.st_size, 1, index_file);
    fclose(index_file);

    get_tree(file_content, index);

    free(file_content);

    return FS_OK;
}

int save_index(struct tree *tree)
{
    if(!local_repo_exist())
    {
        return REPO_NOT_INITIALIZED;
    }

    FILE *index_file = fopen(INDEX_FILE, "w");
    if(index_exist == NULL)
    {
        return FS_ERROR;
    }

    struct object object;
    tree_to_object(tree, &object);

    fwrite(object.content, object.size, 1, index_file);
    fclose(index_file);
    free_object(&object);
}

int get_last_commit(struct object *commit)
{
    size_t head_size = 0;
    if(!local_repo_exist() || !head_file_exist(&head_size) || !heads_dir_exist)
    {
        return REPO_NOT_INITIALIZED;
    }

    if(head_size == 0) return 0;

    FILE *head_file = NULL;
    head_file = fopen(HEAD_FILE, "r");
    char head_path[head_size];
    fread(head_path, head_size, 1, head_file);
    fclose(head_file);

    struct stat buffer = {0};
    if (stat(head_path, &buffer) != 0) return 0;

    char commit_checksum[buffer.st_size + 1];
    memset(commit_checksum, 0, buffer.st_size + 1);
    head_file = fopen(head_path, "r");
    fread(commit_checksum, buffer.st_size, 1, head_file);
    fclose(head_file);

    int res = read_object(commit_checksum, commit);
    if (res != 0) return FS_ERROR;

    if (commit->object_type != COMMIT) return WRONG_OBJECT_TYPE;

    return 0;
}

int update_head(char *new_head)
{
    size_t head_size = 0;
    if(!local_repo_exist() || !head_file_exist(&head_size) || !heads_dir_exist)
    {
        return REPO_NOT_INITIALIZED;
    }

    FILE *file;

    if(head_size != 0) {
        FILE *head_file = fopen(HEAD_FILE, "r");
        char branch[head_size];
        fread(branch, head_size, 1, head_file);
        fclose(head_file);
        file = fopen(branch, "w");        
    } else 
    {
        FILE *head_file = fopen(HEAD_FILE, "w");
        fprintf(head_file, "%s/master", HEADS_DIR);
        fclose(head_file);
        file = fopen(HEADS_DIR"/master", "w");
    }

    fwrite(new_head, strlen(new_head), 1, file);
    fclose(file);
    return 0;
}