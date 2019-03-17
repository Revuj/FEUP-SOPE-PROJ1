#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define IS_BIT_SET(var, pos) ((var) & (1 << (pos)))

#define NAME_LENGTH 256
#define SO_ERROR_NUMBER -1
#define FILE_INFO_SIZE 512

typedef struct active_variables
{
    int analise_files; //-r
    int digit_print;   // -h
    int save_in_file;  // -o
    int logfile;       // -v
} variableStatus;

/*to keep track of the number of directories and files*/
typedef struct Number
{
    int files;
    int directories;
} Number;

char *fileType(struct stat *stat_entry)
{
    if (S_ISREG(stat_entry->st_mode))
        return "regular";
    else if (S_ISDIR(stat_entry->st_mode))
        return "directory";
    else if (S_ISCHR(stat_entry->st_mode))
        return "character special";
    else if (S_ISBLK(stat_entry->st_mode))
        return "block special";
    else if (S_ISFIFO(stat_entry->st_mode))
        return "fifo";
    else if (S_ISLNK(stat_entry->st_mode))
        return "symbolic link";
    else if (S_ISSOCK(stat_entry->st_mode))
        return "socket";
    else
        return "unknown";
}

//consultar link para as flags
//http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
char *fileAccess(struct stat *stat_entry)
{ //perguntar ao professor se é suposto serem apenas as permissões do owner
    if ((stat_entry->st_mode & S_IRWXU) == S_IRWXU)
        return "wrx";
    else if ((stat_entry->st_mode & S_IRUSR) == S_IRUSR)
        return "r";
    else if ((stat_entry->st_mode & S_IWUSR) == S_IWUSR)
        return "w";
    else if ((stat_entry->st_mode & S_IXUSR) == S_IXUSR)
        return "x";

    return " ";
}

/* funcao a completar e a compor*/
void analyseFile(const char *name, char *file_info)
{
    struct stat stat_entry;
    if (stat(name, &stat_entry) == SO_ERROR_NUMBER)
    {
        perror("Error in analyseFile while trying to retrive information about file");
        exit(1);
    }
    //file name, file_type, filesize(bytes),file_access  , file permission change, file modification date
    sprintf(file_info, "%s, %s, %ld, %s, %s, %s", name, fileType(&stat_entry), stat_entry.st_size, fileAccess(&stat_entry), ctime(&stat_entry.st_ctime), ctime(&stat_entry.st_mtime));
}

void completeVariableStatusStruct(variableStatus *Vstatus, int argc, char **argv)
{

    if (Vstatus == NULL || argc <= 1)
    {
        return;
    }

    for (int i = 1; i < argc; i++)
    {

        if (strstr(argv[i], "-r") != NULL)
        {
            Vstatus->analise_files = 1;
        }
        else if (strstr(argv[i], "-h") != NULL)
        {
            Vstatus->digit_print = 1;
        }
        else if (strstr(argv[i], "-o") != NULL)
        {
            Vstatus->save_in_file = 1;
        }
        else if (strstr(argv[i], "-v") != NULL)
        {
            Vstatus->logfile = 1;
        }
    }
}

void printFileInfo(const char *name) {
    char *file_info = malloc(512 * sizeof(char));
    analyseFile(name, file_info);
    printf("%s\n", file_info);
}

int readDirectory(char *dirName)
{
    char *dirIgnore1 = ".";
    char *dirIgnore2 = "..";

    DIR *dir;
    struct dirent *dentry;
    char *name = malloc(sizeof(char) * NAME_LENGTH);

    if ((dir = opendir(dirName)) == NULL)
    { //directory was open unsucessfuly
        perror(dirName);
        return 1;
    }

    while ((dentry = readdir(dir)) != NULL)
    {
        struct stat stat_entry;

        sprintf(name, "%s/%s", dirName, dentry->d_name);

        if (stat(name, &stat_entry) == SO_ERROR_NUMBER)
        {
            perror(name);
            return 3;
        }

        if (S_ISREG(stat_entry.st_mode))
        {
            printFileInfo(name);
        }
        else if (S_ISDIR(stat_entry.st_mode))
        {
            if ((strcmp(dentry->d_name, dirIgnore1) != 0) && (strcmp(dentry->d_name, dirIgnore2) != 0)) {
                printFileInfo(name);
                readDirectory(name);
            }
        }
    }

    return 0;
}

int main(int argc, char **argv, char **envp)
{
    //char * utilitario = "foresinc";
    //char *pathName = ".";
    // if (strcmp(argv[0], utilitario) != 0)
    // {
    //     fprintf(stderr, "Utilitario invalido\n");
    // }

    variableStatus VStatus;
    memset(&VStatus, 0, sizeof(variableStatus));

    Number trackfileAndDir;
    memset(&trackfileAndDir, 0, sizeof(Number));

    completeVariableStatusStruct(&VStatus, argc, argv);

    printf("-r: %d\n", VStatus.analise_files);
    printf("-h: %d\n", VStatus.digit_print);

    printf("-o: %d\n", VStatus.save_in_file);

    printf("-v: %d\n", VStatus.logfile);

    char *enviro = getenv("LOGFILENAME");
    if (enviro != NULL)
    {
        printf("%s\n", enviro);
    }

    if (VStatus.analise_files == 1) // "-r" was input
    {
        return readDirectory(argv[2]);
    }
    else if (argc == 2) // only 1 file to read
    {
        printFileInfo(argv[1]);
    }

    return 0;
}
