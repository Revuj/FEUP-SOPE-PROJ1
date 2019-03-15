#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

int readDirectory(char *dirName)
{
    char * dirIgnore1 = ".";
    char * dirIgnore2 = "..";

    DIR *dir;
    struct dirent *dentry;
    char * name = malloc(sizeof(char)*20);

    if ((dir = opendir(dirName)) == NULL)
    { //directory was open unsucessfuly
        perror(dirName);
        return 1;
    }

    while ((dentry = readdir(dir)) != NULL)
    {
        struct stat stat_entry;
        
        sprintf(name, "%s/%s", dirName, dentry->d_name);

        printf("%s - ", name);

        if (stat(name, &stat_entry) == -1)
        {
            perror(name);
            return 3;
        }

        if (S_ISREG(stat_entry.st_mode))
        {
            printf("Is Regular\n");
        }
        else if (S_ISDIR(stat_entry.st_mode))
        {
            printf("Is Directory\n");
            if ((strcmp(dentry->d_name, dirIgnore1) != 0) && (strcmp(dentry->d_name, dirIgnore2) != 0))
                readDirectory(name);
        }
    }

    return 0;
}

int main(int argc, char **argv, char **envp)
{
    int recursive = 1;
    if (recursive) // "-r" was input
    { 
        return readDirectory(argv[1]);
    }
    return 0;
}
