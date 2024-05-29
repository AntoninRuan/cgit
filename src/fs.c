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
#include "index.h"
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

void dump_index(struct index *index)
{
    printf("index:\n");
    printf("\tentries_size: %ld\n", index->entries_size);
    printf("\tentries:\n");
    struct entry *current = index->first_entry;
    for(int i = 0; i < index->entries_size; i ++)
    {
        printf("\t\tfilename: %s, checksum: %s\n", current->filename, current->checksum);
        current = current->next;
    }
}

int load_index(struct index *index)
{
    if(!local_repo_exist() || !index_exist())
    {
        return REPO_NOT_INITIALIZED;
    }

    FILE* index_file = fopen(INDEX_FILE, "r");
    struct stat buffer;
    stat(INDEX_FILE, &buffer);

    char* file_content = calloc(sizeof(char), buffer.st_size + 1);
    fread(file_content, buffer.st_size, 1, index_file);
    fclose(index_file);

    char* current_line;
    current_line = strtok(file_content, "\n");
    index->entries_size = strtol(current_line, NULL, 10);
    
    for(int i = 0; i < index->entries_size; i++)
    {
        current_line = strtok(NULL, "\n");
        int j = 0;
        while(current_line[j] != ' ')
        {
            j++;
        }

        char* checksum = calloc(sizeof(char), j + 1);
        char* filename = calloc(sizeof(char), strlen(current_line) - j);
        strncat(checksum, current_line, j);
        strncat(filename, current_line + j + 1, strlen(current_line) - j);

        struct entry *entry = malloc(sizeof(struct entry));
        entry->checksum = checksum;
        entry->filename = filename;
        entry->next = NULL;

        if(i == 0)
        {
            index->first_entry = entry;
        } else 
        {
            entry->previous = index->last_entry;
            index->last_entry->next = entry;
        }
        index->last_entry = entry;
        debug_print("checksum: %s, filename: %s", entry->checksum, entry->filename);
    }

    free(file_content);

    return FS_OK;
}

int save_index(struct index *index)
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

    fprintf(index_file, "%li\n", index->entries_size);

    struct entry *current = index->first_entry;
    for(int i = 0; i < index->entries_size; i++)
    {
        fprintf(index_file, "%s %s\n", current->checksum, current->filename);
        current = current->next;
    }

    fclose(index_file);
}

int write_object(struct object *obj)
{
    if(!local_repo_exist())
    {
        return REPO_NOT_INITIALIZED;
    }
    int result = FS_OK;

    struct stat buffer;
    if (stat(OBJECTS_REPO, &buffer) != 0)
    {
        mkdir(OBJECTS_REPO, DEFAULT_DIR_MODE);
    }
    
    DIR *objects_dir = opendir(OBJECTS_REPO);
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
    if (stat(OBJECTS_REPO, &buffer) != 0)
    {
        error_print("Object dir does not exist");
        return OBJECT_DOES_NOT_EXIST;
    }

    DIR *objects_dir = opendir(OBJECTS_REPO);
    int objects_dir_fd = dirfd(objects_dir);

    int save_file_fd = openat(objects_dir_fd, checksum, O_RDONLY, DEFAULT_FILE_MODE);    
    if (save_file_fd == -1)
    {
        if(errno == EACCES)
        {    
            error_print("Object %s does not exist", checksum);
            defer(OBJECT_DOES_NOT_EXIST);
        }
        defer(FS_ERROR);
    }
    fstatat(objects_dir_fd, checksum, &buffer, 0);
    char* file_content = malloc(buffer.st_size);

    FILE *save_file = fdopen(save_file_fd, "r");
    fread(file_content, 1, buffer.st_size, save_file);
    fclose(save_file);

    int res = uncompress_object(obj, file_content, buffer.st_size);
    if(res != Z_OK)
    {
        defer(COMPRESSION_ERROR);
    }
    
    free(file_content);

defer:
    closedir(objects_dir);
    return result;
}

int main(void)
{
    struct index index = {0};
    load_index(&index);

    // struct object obj = {0};
    // obj.content = "Hello, world!\n";
    // obj.size = strlen(obj.content);
    // obj.object_type = "blob";

    // return write_object(&obj);

    // read_object("af5626b4a114abcb82d63db7c8082c3c4756e51b", &obj);

    // debug_print("Object type is \"%s\"", obj.object_type);
    // debug_print("Content size is %li", obj.size);
    // debug_print("Content is %.*s", (int)obj.size, obj.content);

    // free_object(&obj);

    add_to_index(&index, "src/fs.c");

    dump_index(&index);

    save_index(&index);

    free_index(&index);

    return 0;
}