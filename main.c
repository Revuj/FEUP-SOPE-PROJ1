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

#define NAME_LENGTH 255
#define SO_ERROR_NUMBER -1
#define FILE_INFO_SIZE 512
#define TIME_LENGHT 20
#define READ 0
#define WRITE 1
#define PIPE_ERROR_RETURN -1
#define FORK_ERROR_RETURN -1

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

typedef struct algorithms
{
    int md5;
    int sha1;
    int sha256;
} Algorithms;

variableStatus VStatus;
Number trackfileAndDir;
Algorithms algorithmStatus;
struct timespec start;
FILE *logFile;

int stdoutCopy;

pid_t parentPid;

struct timespec getInitialTime()
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    return start;
}

void writeToLogFile(char * description)
{
    struct timespec end;
    memset(&end, 0, sizeof(end));
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    float delta_ns = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    fprintf(logFile, "%.2f - %d - %s\n", delta_ns, getpid(), description);
    fflush(logFile);
}

void sigUser_handler(int signo)
{
    printf("PID = %d - ATINGIDO\n", getpid());
    fflush(stdout);
    if (signo == SIGUSR1)
    {
        trackfileAndDir.directories++;

        char *dirStr = malloc(sizeof(char) * 256);
        sprintf(dirStr, "New directory: %d/%d directories/files at this time.\n", trackfileAndDir.directories, trackfileAndDir.files);
        write(stdoutCopy, dirStr, sizeof(char) * strlen(dirStr));
        free(dirStr);


        if (VStatus.logfile)
            writeToLogFile("RECEIVED SIGUSR1");
    }
    else
    {
        trackfileAndDir.files++;

        if (VStatus.logfile)
            writeToLogFile("RECEIVED SIGUSR2");
    }
}

void sigint_handler(int signo)
{
    if (VStatus.logfile)
        writeToLogFile("PROGRAM TERMINADED (SIGTERM)");
    kill(-getpgid(getpid()), SIGTERM);
}

void sigchild_handler(int signo) {
    wait(NULL);
}

void saveInFile(int fd)
{
    if (dup2(fd, STDOUT_FILENO) == SO_ERROR_NUMBER)
    {
        perror("DUP2");
        exit(1);
    }
}

void completeVariableStatusStruct(int argc, char **argv)
{
    int option;
    char *token;
    size_t i = 0;

    opterr = 0;

    while ((option = getopt(argc, argv, "h:ro:v")) != -1) {
        switch (option)
        {
        case 'r':
            VStatus.analise_files = 1;
            break;
        case 'v':
            VStatus.logfile = 1;
            char *logFileName = getenv("LOGFILENAME");
            if (logFileName == NULL)
            {
                perror("LOGFILENAME");
            }
            if ((logFile = fopen(logFileName, "w")) == NULL) {
                perror("LOGFILENAME");
            }
            break;
        case 'h':
            VStatus.digit_print = 1;
            token = strsep(&optarg, ",");
            while (token != NULL)
            {
                if (!strcmp(token, "md5") && algorithmStatus.md5 == 0)
                    algorithmStatus.md5 = 1;
                else if (!strcmp(token, "sha1") && algorithmStatus.sha1 == 0)
                    algorithmStatus.sha1 = 1;
                else if (!strcmp(token, "sha256") && algorithmStatus.sha256 == 0)
                    algorithmStatus.sha256 = 1;
                else
                {
                    printf("Wrong values entered for option -h\n");
                    exit(1);
                }
                token = strsep(&optarg, ",");
                i++;
            }
            if (i > 3)
            {
                printf("Too many values entered for option -h\n");
                exit(1);
            }
            break;
        case 'o':
            VStatus.save_in_file = 1;
            int fd = open(optarg, O_TRUNC | O_WRONLY);
            if (fd < 0) {
                perror(optarg);
                exit(1);
            }
            saveInFile(fd);
            close(fd);
            break;
        case '?':
            if (optopt == 'h' || optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            exit(1);
        default:
            exit(1);
        }
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

void removeNewLine(char *line)
{  
    char *pos;
    if ((pos=strchr(line, '\n')) != NULL)
        *pos = '\0';
}

void popenAlgorithm(const char *name, char *fileHash, char *algorithm)
{

    int fd[2];
    pid_t pid;

    if (pipe(fd) == PIPE_ERROR_RETURN)
    {
        perror("Pipe Error");
        exit(1);
    }

    pid = fork();

    if (pid > 0)
    {
        close(fd[WRITE]);
        wait(NULL);
        char fileInfo[NAME_LENGTH];
        read(fd[READ], fileInfo, NAME_LENGTH * 100);
        char *fileHashCopy = strtok(fileInfo, " ");
        removeNewLine(fileHashCopy);
        strcpy(fileHash, fileHashCopy);
        close(fd[READ]);
    }
    else if (pid == FORK_ERROR_RETURN)
    {
        perror("Fork error");
        exit(2);
    }
    else
    {
        close(fd[READ]);
        dup2(fd[WRITE], STDOUT_FILENO);
        execlp(algorithm, algorithm, name, NULL);
        close(fd[WRITE]);
    }
}

void getFileType(const char *name, char *fileType)
{
    int fd[2];
    pid_t pid;

    if (pipe(fd) < 0)
    {
        perror("Pipe Error");
        exit(1);
    }

    if ((pid = fork()) < 0)
    {
        perror("Fork Error");
        exit(2);
    }
    else if (pid > 0)
    {
        close(fd[WRITE]);
        wait(NULL);
        char fileInfo[NAME_LENGTH];
        read(fd[READ], fileInfo, NAME_LENGTH);
        char * fileCopy = malloc(sizeof(char) * NAME_LENGTH);
        if ((fileCopy = strstr(fileInfo, " ")) != NULL) {
            fileCopy++;
            removeNewLine(fileCopy);
            strcpy(fileType, fileCopy);
        }
        close(fd[READ]);
    }
    else
    {
        close(fd[READ]);
        dup2(fd[WRITE], STDOUT_FILENO);
        execlp("file", "file", name, NULL);
    }
}

void getHashes(const char *name, char *file_info)
{

    if (algorithmStatus.md5)
    {
        char *md5sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, md5sum, "md5sum");
        strcat(file_info, ",");
        strcat(file_info, md5sum);
        free(md5sum);
    }

    if (algorithmStatus.sha1)
    {
        char *sha1sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, sha1sum, "sha1sum");
        strcat(file_info, ",");
        strcat(file_info, sha1sum);
        free(sha1sum);
    }

    if (algorithmStatus.sha256)
    {
        char *sha256sum = malloc(NAME_LENGTH * sizeof(char));
        popenAlgorithm(name, sha256sum, "sha256sum");
        strcat(file_info, ",");
        strcat(file_info, sha256sum);
        free(sha256sum);
    }
}

/* funcao a completar e a compor*/
void analyseFile(char *name, char *file_info)
{
    struct stat stat_entry;
    if (stat(name, &stat_entry) == SO_ERROR_NUMBER)
    {
        perror("Error in analyseFile while trying to retrive information about file");
        exit(-1);
    }

    char creationDate[TIME_LENGHT];
    strftime(creationDate, TIME_LENGHT, "%Y-%m-%dT%H:%M:%S", localtime(&stat_entry.st_ctime));

    char modificationDate[TIME_LENGHT];
    strftime(modificationDate, TIME_LENGHT, "%Y-%m-%dT%H:%M:%S", localtime(&stat_entry.st_mtime));

    char *fileType = malloc(NAME_LENGTH * sizeof(char));
    getFileType(name, fileType);
    sprintf(file_info, "%s,%s,%ld,%s,%s,%s", name, fileType, stat_entry.st_size, fileAccess(&stat_entry), creationDate, modificationDate);

    if (VStatus.digit_print) // "-h" was input
        getHashes(name, file_info);

    free(fileType);
}

void printFileInfo(char *name)
{
    char *file_info = malloc(1024 * sizeof(char));
    analyseFile(name, file_info);
    printf("%s\n", file_info);
    fflush(stdout);
    free(file_info);
}

void readDirectory(char *dirName)
{
    printf("EXEC PID = %d\n", getpid());
    fflush(stdout);

    char *dirIgnore1 = ".";
    char *dirIgnore2 = "..";

    DIR *dir;
    struct dirent *dentry;
    char *name = malloc(sizeof(char) * NAME_LENGTH);
    int process_counter=0;

    if ((dir = opendir(dirName)) == NULL)
    { //directory was open unsucessfuly
        perror(dirName);
        exit(-1);
    }

    while ((dentry = readdir(dir)) != NULL)
    {
        struct stat stat_entry;

        sprintf(name, "%s/%s", dirName, dentry->d_name);

        if (stat(name, &stat_entry) == SO_ERROR_NUMBER)
        {
            perror(name);
            exit(-1);
        }

        if (S_ISREG(stat_entry.st_mode))
        {   
            if (VStatus.save_in_file) {
                if (VStatus.logfile)
                    writeToLogFile("SENT SIGUSR2");
                printf("PID = %d - MANDEI UM SINAL no ficheiro %s\n", getpid(), name);
                fflush(stdout);

                kill(parentPid, SIGUSR2);
            }
            printFileInfo(name);
            if (VStatus.logfile){
                char logInfo[266];
                sprintf(logInfo,"ANALIZED %s",dentry->d_name);
                writeToLogFile(logInfo);

            }
        }

        else if (S_ISDIR(stat_entry.st_mode))
        {
            if ((strcmp(dentry->d_name, dirIgnore1) != 0) && (strcmp(dentry->d_name, dirIgnore2) != 0))
            {
                if (VStatus.save_in_file) {
                    if (VStatus.logfile)
                        writeToLogFile("SENT SIGUSR1");
                    printf("PID = %d - MANDEI UM SINAL no ficheiro %s\n", getpid(), name);                
                    fflush(stdout);

                    kill(parentPid, SIGUSR1);
                }
                
                pid_t pid = fork();

                
                if (VStatus.logfile)
                    writeToLogFile("NEW PROCESS CREATED");
                
                if (pid < 0) {
                    printf("GANDA ERRO\n");
                    fflush(stdout);
                }
                if (pid == 0)
                {
                    readDirectory(name);
                    exit(55);
                }
                else if(pid > 0)
                {
                    sigset_t mask, oldmask;
                    sigemptyset(&mask);
                    sigaddset(&mask, SIGUSR1);
                    sigaddset(&mask, SIGUSR2);

                    sigprocmask(SIG_BLOCK, &mask, &oldmask);
                    sigsuspend(&oldmask);
                    //arranjar maneira de fazer os waits todos
                    wait(NULL);
                    sigprocmask(SIG_UNBLOCK, &mask, NULL);

                    process_counter++;
                }
                else {
                    perror("FORK");
                    exit(1);
                }
            }
        }
        name[0] = '\0';
    }

    free(name);
    closedir(dir);

    printf("%d LEAVING\n", getpid());
    fflush(stdout);
}

int main(int argc, char **argv, char **envp)
{
    stdoutCopy = dup(STDOUT_FILENO);

    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    sigaction(SIGINT, &action, NULL);

    action.sa_handler = sigchild_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGCHLD, &action, NULL);

    parentPid = getpid();


    memset(&VStatus, 0, sizeof(variableStatus));
    memset(&trackfileAndDir, 0, sizeof(Number));
    memset(&algorithmStatus, 0, sizeof(Algorithms));

    completeVariableStatusStruct(argc, argv);

    if(VStatus.logfile){
        memset(&start, 0, sizeof(start));
        start = getInitialTime();
    }

    if (VStatus.save_in_file)
    {
        action.sa_handler = sigUser_handler;
        action.sa_flags = 0;
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGUSR1, &action, NULL);
    }
    

    if (VStatus.analise_files)
    { // "-r" was input
        readDirectory(argv[argc - 1]);
    }

    else if (argc == 2 || (argc == 3 && VStatus.save_in_file))
    { // only 1 file to read
        printFileInfo(argv[argc - 1]);
    }

    if (VStatus.logfile)
        writeToLogFile("ENDED PROGRAM");

    return 0;
}

