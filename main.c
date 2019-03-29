#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define IS_BIT_SET(var, pos) ((var) & (1 << (pos)))

#define NAME_LENGTH 255
#define SO_ERROR_NUMBER -1
#define FILE_INFO_SIZE 512
#define TIME_LENGHT 20
#define READ 0
#define WRITE 1
#define PIPE_ERROR_RETURN -1
#define FORK_ERROR_RETURN -1

typedef struct active_variables {
    int analise_files; //-r
    int digit_print;   // -h
    int save_in_file;  // -o
    int logfile;       // -v
} variableStatus;

/*to keep track of the number of directories and files*/
typedef struct Number {
    int files;
    int directories;
} Number;

typedef struct algorithms {
    int md5;
    int sha1;
    int sha256;
} Algorithms;

variableStatus VStatus;
Number trackfileAndDir;
Algorithms algorithmStatus;

void completeVariableStatusStruct(int argc, char **argv) {
    int option;
    char *h_args = NULL;
    char *token;
    size_t i = 0;

    opterr = 0;

    while ((option = getopt (argc, argv,"h:ro:v")) != -1)
        switch (option){
            case 'r':
                VStatus.analise_files=1;
                break;
            case 'v':
                VStatus.logfile=1;
                break;
            case 'h':
                VStatus.digit_print=1;
                h_args=optarg;
                token = strsep(&h_args, ",");
                while (token != NULL) {
                    if (!strcmp(token, "md5") && algorithmStatus.md5 == 0)
                        algorithmStatus.md5 = 1;
                    else if (!strcmp(token, "sha1") && algorithmStatus.sha1 == 0)
                        algorithmStatus.sha1 = 1;
                    else if (!strcmp(token, "sha256") && algorithmStatus.sha256 == 0)
                        algorithmStatus.sha256 = 1;
                    else {
                        printf("Wrong values entered for option -h\n");
                        exit(1);
                    }
                    token = strsep(&h_args, ",");
                    i++;
                }

                if (i > 3) {
                    printf("Too many values entered for option -a\n");
                    exit(1);
                }
            case 'o':
                VStatus.save_in_file=1;
                break;
            case '?':
                if (optopt == 'h' || optopt == 'o')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                exit(1);
            default:
                exit(1);
        }
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

void removeNewLine(char *line) {
    char *pos;
    if ((pos = strchr(line, '\n')) != NULL)
        *pos = '\0';
}

void popenAlgorithm(const char *name, char *fileHash, char * algorithm) {

    int fd[2];
    pid_t pid;

    if (pipe(fd) == PIPE_ERROR_RETURN) {
        perror("Pipe Error");
        exit(1);
    }

    pid = fork();

    if (pid > 0) {
        close(fd[WRITE]);
        char fileInfo[NAME_LENGTH];
        read(fd[READ], fileInfo, NAME_LENGTH * 100);
        char *fileHashCopy = strtok(fileInfo, " ");
        removeNewLine(fileHashCopy);
        strcpy(fileHash, fileHashCopy);
        wait(NULL);
    }
    else if (pid == FORK_ERROR_RETURN) {
        perror("Fork error");
        exit(2);
    }
    else {
        close(fd[READ]);
        dup2(fd[WRITE], STDOUT_FILENO);
        execlp(algorithm, algorithm, name, NULL);
    }
}

void getFileType(const char *name, char *fileType) {
    int fd[2];
    pid_t pid;

    if (pipe(fd) < 0) {
        perror("Pipe Error");
        exit(1);
    }

    if ((pid = fork()) < 0) {
        perror("Fork Error");
        exit(2);
    }
    else if (pid > 0) {
        close(fd[WRITE]);
        char fileInfo[NAME_LENGTH];
        read(fd[READ], fileInfo, NAME_LENGTH);
        char *fileTypeCopy = strstr(fileInfo, " ");
        fileTypeCopy++;
        removeNewLine(fileTypeCopy);
        strcpy(fileType, fileTypeCopy);
        wait(NULL);
    }
    else {
        close(fd[READ]);
        dup2(fd[WRITE], STDOUT_FILENO);
        execlp("file", "file", name, NULL);
    }
}

void getHashes(const char *name, char *file_info) {
    
    if (algorithmStatus.md5) {
        char *md5sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, md5sum,"md5sum");
        strcat(file_info, ",");
        strcat(file_info, md5sum);
        free(md5sum);
    }

    if (algorithmStatus.sha1) {
        char *sha1sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, sha1sum,"sha1sum");
        strcat(file_info, ",");
        strcat(file_info, sha1sum);
        free(sha1sum);
    }

    if (algorithmStatus.sha256) {
        char *sha256sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, sha256sum,"sha256sum");
        strcat(file_info, ",");
        strcat(file_info, sha256sum);
        free(sha256sum);
    }
}

/* funcao a completar e a compor*/
void analyseFile(char *name, char *file_info)
{
    struct stat stat_entry;
    if (stat(name, &stat_entry) == SO_ERROR_NUMBER) {
        perror("Error in analyseFile while trying to retrive information about file");
        exit(-1);
    }

    char creationDate[TIME_LENGHT];
    strftime(creationDate, TIME_LENGHT, "%Y-%m-%dT%H:%M:%S", localtime(&stat_entry.st_ctime));

    char modificationDate[TIME_LENGHT];
    strftime(modificationDate, TIME_LENGHT, "%Y-%m-%dT%H:%M:%S", localtime(&stat_entry.st_mtime));
    
    //file name, file_type, filesize(bytes),file_access  , file permission change, file modification date
    char *fileType = malloc(NAME_LENGTH * sizeof(char));
    getFileType(name, fileType);
    sprintf(file_info, "%s,%s,%ld,%s,%s,%s", name, fileType, stat_entry.st_size, fileAccess(&stat_entry), creationDate, modificationDate);

    if (VStatus.digit_print) // "-h" was input
        getHashes(name, file_info);

    free(fileType);
}

void printFileInfo(char *name) {
    char *file_info = malloc(1024 * sizeof(char));
    analyseFile(name, file_info);
    printf("%s\n", file_info);
    free(file_info);
}

void readDirectory(char *dirName) {
    char *dirIgnore1 = ".";
    char *dirIgnore2 = "..";

    int status;
    DIR *dir;
    struct dirent *dentry;
    char *name = malloc(sizeof(char) * NAME_LENGTH);

    if ((dir = opendir(dirName)) == NULL) { //directory was open unsucessfuly
        perror(dirName);
        exit(-1);
    }

    while ((dentry = readdir(dir)) != NULL) {
        
        struct stat stat_entry;

        sprintf(name, "%s/%s", dirName, dentry->d_name);

        if (stat(name, &stat_entry) == SO_ERROR_NUMBER) {
            perror(name);
            exit(-1);
        }

        if (S_ISREG(stat_entry.st_mode)) {
            printFileInfo(name);
        }

        else if (S_ISDIR(stat_entry.st_mode)) {
            if ((strcmp(dentry->d_name, dirIgnore1) != 0) && (strcmp(dentry->d_name, dirIgnore2) != 0)) {
                pid_t pid = fork();
                if (pid == 0) {
                    readDirectory(name);
                    break;
                }
                else {
                    wait(&status);
                }
            }
        }
    }

    free(name);
    closedir(dir);
    exit(0);
}

int main(int argc, char **argv, char **envp) {
    
    memset(&VStatus, 0, sizeof(variableStatus));
    memset(&trackfileAndDir, 0, sizeof(Number));
    memset(&algorithmStatus, 0, sizeof(Algorithms));
    
    completeVariableStatusStruct(argc, argv);

    char *enviro = getenv("LOGFILENAME");
    if (enviro != NULL) {
        printf("%s\n", enviro);
    }

    if (VStatus.analise_files) { // "-r" was input
        readDirectory(argv[argc - 1]);
    }

    else if (argc == 2) { // only 1 file to read
        printFileInfo(argv[argc - 1]);
    }

    return 0;
}