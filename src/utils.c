#include <math.h>
#include <stdio.h>

#include "utils.h"

int decimal_len(size_t size)
{
    if (size == 0)
        return 1;

    return floor(log10(size)) + 1;
}

int get_top_folder(char* path, char* top_folder, char* left) 
{
    int i = 0;
    for (;path[i] != '/' && path[i] != '\0'; i++);
    if (path[i] == '\0') return 0;
    path[i] = '\0';
    sprintf(top_folder, "%s", path);
    sprintf(left, "%s", path + i + 1);
    path[i] = '/';
    return 1;
}
