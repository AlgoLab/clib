/*
    Author: Simone Stefanello
    Name: clib-tag.c
    Description: implement a "namespace" by insert in every function identifier a prefix for the library,
                 used as an agreement in C programming. 
*/
/*
// for ntfw
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ftw.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "common/clib-clone.h"
#include "common/clib-tag.h"
#include "fs/fs.h"
#include "parson/parson.h"

#define FUNC_INPUT 20
#define MAX_PROCESS 4

#define SOURCE_PREFIX "m_"

#define LOG_MESSAGE_ADDED_PREFIX "Added prefix.\n"

// used to remove temp and functions tags file 
/*int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rm_rf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}*/

// used to cpy the modified source content in a temp file
int cpy(char *fnDest, char *fnSrc)
{
    FILE *fpDest, *fpSrc;
    int c;

    if ((fpDest = fopen(fnDest, "w")) && (fpSrc = fopen(fnSrc, "r"))) 
    {
        while ((c = getc(fpSrc)) != EOF)
        {
            putc(c, fpDest);
        }
        fclose(fpDest);
        fclose(fpSrc);

        return 0;
    }
    return -1;
}

int append(char *file_dest, char *file_src)
{
    FILE *fp_dest, *fp_source;

    int c;

    fp_dest = fopen(file_dest, "a");
    fp_source = fopen(file_src, "r");

    if (fp_dest == NULL || fp_source == NULL)
    {
        printf("Error: cannot open %s or %s\n", file_dest, file_src);
        return 1;
    }

    fseek(fp_source, 0, SEEK_END);
    int size_source = ftell(fp_source);
    rewind(fp_source);

    char *source = (char *)calloc(size_source, sizeof(char));
    if (source != NULL)
    {
        fwrite(source, sizeof(char), size_source, fp_dest);
    }
    
    fclose(fp_dest);
    fclose(fp_source);

    // ok
    return 0;
}

// call ctags
int fork_ctags(char *functions_path,
              char *path_to_source)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error: cannot call ctags");
        return -1;
    }
    else
    if (pid == 0)
    {

        // child process
        // O_APPEND to append 
        // O_CREAT | O_APPEND | O_WRONLY
        int fd = open(functions_path, 
                      O_CREAT | O_APPEND | O_WRONLY,
                      S_IRUSR | S_IWUSR);
        if (fd < 0) 
        {
            perror("open file failed");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) 
        {
            perror("dup2 stdout failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) 
        {
            perror("dup2 stderr failed");
            exit(EXIT_FAILURE);
        }

        close(fd);

        // calling ctags and format functions:
        //
        // {identifier}\t{line}\t{kind}
	    #if !defined(BSD) && !defined(__FreeBSD__)
        char *arg[] = {"ctags", "-x", "--_xformat=%N\t%n\t%{kind}", path_to_source, NULL};
	    #else
	    char *arg[] = {"uctags", "-x", "--_xformat=%N\t%n\t%{kind}", path_to_source, NULL};
	    #endif

        execvp(arg[0], arg);

        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            return 1;
        }
        /*
        char functionsproject_tags[MAX_PATH] = "";
        strcat(functionsproject_tags, getenv("PWD"));
        strcat(functionsproject_tags, "/deps/functionsproject_tags");*/

        // add error checking
        // append(functionsproject_tags, functions_path);

        // append to functionproject_tags this function file for this dep
        //append(,functions_path);

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

int fork_sort(char *path)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error: cannot call sort\n");
    }
    if (pid == 0)
    {
        
        char *args[] = {"sort", path, "-o", path, NULL};

        execvp(args[0], args);

        exit(EXIT_FAILURE);
    }
    else
    {
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            return 1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

int fork_uniq(char *path, char *path_to_sorted_file)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error: cannot call sort\n");
    }
    if (pid == 0)
    {
        // child process
        // O_APPEND to append 
        // O_CREAT | O_WRONLY
        int fd = open(path_to_sorted_file, 
                      O_CREAT | O_WRONLY,
                      S_IRUSR | S_IWUSR);
        if (fd < 0) 
        {
            perror("open file failed");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) 
        {
            perror("dup2 stdout failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) 
        {
            perror("dup2 stderr failed");
            exit(EXIT_FAILURE);
        }

        close(fd);

        char *args[] = {"uniq", path, NULL};

        execvp(args[0], args);

        exit(EXIT_FAILURE);
    }
    else
    {
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            return 1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

// ***********************************************************************
// NOT USED
// ***********************************************************************
// int first_command: 0 if first time calling, =! 0 otherwise
// path_to_source is the path to the actual .c file in a dep directory
// path_to_modified is the path to the new modified source with the prefix 

// parameters: path_to_sed_script, path_to_source
int fork_sed(char *path_to_source, char *path_to_modified, char *path_to_sed_script)
{
    printf("source: %s\n", path_to_source);
    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error: cannot call sed.");
        return -1;
    }
    else if (pid == 0)
    {   /*
        // child process
        // O_APPEND to append 
        // O_CREAT | O_APPEND | O_WRONLY
        // path_to_modified
        int fd = open("/dev/null", 
                      O_CREAT | O_WRONLY,
                      S_IRUSR | S_IWUSR);
        if (fd < 0) 
        {
            perror("open file failed");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) 
        {
            perror("dup2 stdout failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) 
        {
            perror("dup2 stderr failed");
            exit(EXIT_FAILURE);
        } */

        // child process

        
        #if !defined(BSD) && !defined(__APPLE__) && !defined(__FreeBSD__)
        // Ubuntu or Linux in general
        char *args[] = {"sed", "-f", path_to_sed_script, "-i", path_to_source, NULL};
        #else
        // Mac and BSD (add FreeBSD and OpenBSD)
        char *args[] = {"sed", "-E", "-f", path_to_sed_script, "-i", "''", path_to_source, NULL};
        #endif

        execvp(args[0], args);

        exit(EXIT_FAILURE);
    }
    else 
    {
        // parent process
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            printf("Error: waitpid");
            return 1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

int count_rows(FILE *fp)
{
    rewind(fp);
    char chr = getc(fp);

    int count_rows = 0;
    while (chr != EOF)
    {
        if (chr == '\n')
        {
            ++count_rows;
        }

        chr = getc(fp);
    }
    rewind(fp);

    return count_rows;
}

// ******************************************************************************
// NOT USED
// ******************************************************************************
// recursive function to explodre deps directory in a clib root project directory
// tree:
// 
// clib-project
// |
// ----deps
//     |
//     ----deps_1
//     ----deps_2
//     ...
//     ----deps_n
// |
// ----src
// ----test
// ...
// 
// deps_1 to deps_n are dep directories
// in every dep direcotries this function substitute old source file
// with new source files with functions identifier with new prefix
//
// if path is a directory, path to modified is NULL, if path is a file, build path_to_modified
void substitute_files(char *path, char *path_to_sed_script, char *path_to_modified, char *author_dep, char *name_dep) 
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) 
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp)) != NULL) 
    {
        // d_name is file's name (or dir name)
        char *name = entry->d_name;

        // don't check special directories "." and ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        // build absolute path to file (or directory)
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

        // Ottiene informazioni sul file
        struct stat info;
        if (stat(full_path, &info) == -1) 
        {
            perror("stat");
            continue; // Non interrompere per errori su singoli file
        }


        if (S_ISDIR(info.st_mode)) 
        {
            // Se è una directory, esegui la ricorsione
            substitute_files(full_path, path_to_sed_script, path_to_modified, author_dep, name_dep);
        } else if (S_ISREG(info.st_mode)) 
        {
            // check for the extension
            char *ext = strrchr(name, '.');

            // substitute .c and .h source
            if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0)) 
            {
                // build path_to_modified
                char path_to_modified[MAX_PATH] = "";
                strcat(path_to_modified, path);
                strcat(path_to_modified, "/m_");
                strcat(path_to_modified, name);
                
                int r = fork_sed(full_path, path_to_modified, path_to_sed_script);
                if (r == 0)
                {
                    printf("%s modified correctly!\n", full_path);
                    //rm_rf(full_path);
                    //rename(path_to_modified, full_path);
                }
                else
                {
                    printf("Error: %s not modified correctly with error %d!\n", full_path, r);
                }
            }
        }
    }

    closedir(dp);
}

// this function is similar to substitute_files,
// search for .c files only (because of function prototypes are all there)
// and call ctags to make functions tag file
// return 1 if log file found (already prefixed library)
void find_files(char *path, char *functions_path, int *log_check) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) 
    {
        perror("opendir");
        return; 
    }

    while ((entry = readdir(dp)) != NULL) 
    {
        char *name = entry->d_name;

        // don't check special directories "." and ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        // build absolute path
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

        // file infos
        struct stat info;
        if (stat(full_path, &info) == -1) 
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(info.st_mode)) 
        {
            find_files(full_path, functions_path, log_check);
        } else if (S_ISREG(info.st_mode)) 
        {
            char *ext = strrchr(name, '.');

            // only .c source
            if (ext && strcmp(ext, ".c") == 0) 
            {
                int r = fork_ctags(functions_path, full_path);

                if (r != 0)
                {
                    printf("Error %d: problem with ctags", r);
                }
            }

            // check if log file exist
            if (strcmp(name, "log") == 0)
            {
                *log_check = 1;
            }
        }
    }

    closedir(dp);
}

int parse_json(char *file_path, char *author, char *name)
{
    JSON_Value *root_value;
    JSON_Object *object;

    root_value = json_parse_file(file_path);

    if (root_value == NULL)
    {
        printf("Cannot open package.json");
        return 1;
    }

    if (author == NULL || name == NULL)
    {
        // problem with json
        return 2;
    }

    object = json_value_get_object(root_value);

    strcat(author, strdup(json_object_get_string(object, "author")));
    strcat(name, strdup(json_object_get_string(object, "name")));

    if (strlen(author) == 0 || strlen(name) == 0)
    {
        // problem with json
        return 2;
    }

    return 0;
}

int build_sed_script(Tag *tags, char *path_to_sed_script, int rows, char *author, char *name)
{
    // build prefix
    char prefix[MAX_CHAR] = "";
    strcat(prefix, author);
    strcat(prefix, "_");
    strcat(prefix, name);

    FILE *fp = fopen(path_to_sed_script, "a");

    if (fp == NULL)
    {
        printf("Error: cannot open %s sed script file\n", path_to_sed_script);
        return 1;
    }

    char substitute_command[MAX_CHAR] = "";

    // /#include/!s/\btags[i].identifier\b/prefix_tags[i].identifier/g
    for (int i = 0; i < rows; i++)
    {
        if (strcmp(tags[i].kind, "function") == 0)
        {
            // main shouldn't be prefixed 
            if (strcmp(tags[i].identifier, "main") == 0)
            {
                continue;
            }
            strcat(substitute_command, "/#include/!");
            strcat(substitute_command, "s/");

            #if !defined(BSD) && !defined(__APPLE__) && !defined(__FreeBSD__)
            // Ubuntu or Linux in general
            strcat(substitute_command, "\\b");
            #else
            // Mac and BSD (add FreeBSD and OpenBSD)
            strcat(substitute_command, "[[:<:]]");
            #endif

            strcat(substitute_command, tags[i].identifier);

            #if !defined(BSD) && !defined(__APPLE__) && !defined(__FreeBSD__)
            // Ubuntu or Linux in general
            strcat(substitute_command, "\\b");
            #else
            // Mac and BSD (add FreeBSD and OpenBSD)
            strcat(substitute_command, "[[:>:]]");
            #endif

            strcat(substitute_command, "/");

/*
            #if !defined(BSD) && !defined(__APPLE__) && !defined(__FreeBSD__)
            // Nothing to add
            #else
            // Mac and BSD (add FreeBSD and OpenBSD)
            strcat(substitute_command, "\1");
            #endif
*/

            strcat(substitute_command, prefix);
            strcat(substitute_command, "_");
            strcat(substitute_command, tags[i].identifier);

/*
            #if !defined(BSD) && !defined(__APPLE__) && !defined(__FreeBSD__)
            // Nothing to add
            #else
            // Mac and BSD (add FreeBSD and OpenBSD)
            strcat(substitute_command, "\2");
            #endif
*/          

            strcat(substitute_command, "/g");
            fprintf(fp, "%s\n", substitute_command);
            memset(substitute_command, 0, sizeof(substitute_command));
        }
    }

    fclose(fp);

    return 0;
}

int fork_for_script(char *path_script)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        printf("Error: cannot fork sed\n");
        return -1;
    }
    if (pid == 0)
    {
        char *args[] = {"sh", path_script, NULL};

        execvp(args[0], args);

        exit(EXIT_FAILURE);
    }
    else
    {
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // problem with waitpid
            printf("Error: waitpid error\n");
            return -1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

// search for .c files
// explore directory deps
// if find_files find log file it means that dep is already modified so the function ignore it
void find_dir(const char *path) 
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    // TODO: add file checking, if entry isn't a directory,
    //       continue
    while ((entry = readdir(dp)) != NULL) 
    {
        char *name = entry->d_name;

        // ignore special directories "." and ".." 
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) 
        {
            continue;
        }

        // build absolute path
        char dir_path[MAX_PATH];
        snprintf(dir_path, sizeof(dir_path), "%s/%s", path, name);

        printf("Directory: %s\n", dir_path);

        // build path to tag function file
        char functions_path[MAX_PATH] = "";
        char functions[] = "functions";
        snprintf(functions_path, sizeof(functions_path), "%s/%s", dir_path, functions);

        int log_check = 0;

        find_files(dir_path, functions_path, &log_check);

        if (log_check != 0)
        {
            // ignore deps already modified with prefix
            continue;
        }

        // here I call sed recursively on the .c and .h source files
        // now that I have functions_path
        int rows = rows_tags_file(functions_path);

        char package_json_path[MAX_PATH] = "";
        strcpy(package_json_path, dir_path);
        strcat(package_json_path, "/package.json");
        
        char author_dep[MAX_CHAR] = "";
        char name_dep[MAX_CHAR] = "";

        // obtain author and name for prefix
        int r = parse_json(package_json_path, author_dep, name_dep);

        if (r > 0)
        {
            printf("Errore: author o name == NULL");
            // json problem so skip directory
            continue;
        }

        // struct with tags identifiers
        Tag *tags = parse_tags_file(functions_path, rows);

        // build general sed script file for a dep directory
        char sed_script_path[MAX_PATH] = "";
        strcat(sed_script_path, dir_path);
        strcat(sed_script_path, "/source_sed_script.sed");

        // build sed script for substitution
        int r_1 = build_sed_script(tags, sed_script_path, rows, author_dep, name_dep);

        if (r_1 == 1)
        {
            printf("Error: cannot build %s\n", sed_script_path);
            continue;
        }
   
        // needed for uniq call
        int r_2 = fork_sort(sed_script_path);

        if (r_2 > 0)
        {
            // continue to the next dir
            printf("Error: cannot fork sort.");
            continue;
        }

        // file with no lines repeated
        char sorted_sed_script[MAX_PATH] = "";
        strcat(sorted_sed_script, dir_path);
        strcat(sorted_sed_script, "/sed_script_sorted.sed");

        // delete repeated lines
        int r_3 = fork_uniq(sed_script_path, sorted_sed_script);

        if (r_3 > 0)
        {
            // error fork_uniq
            printf("Error: fork_uniq\n");
            continue;
        }

        // build script.sh to call with fork
        // (this enables only one call per dependency directory)
        char script_path[MAX_PATH] = "";
        strcat(script_path, dir_path);
        strcat(script_path, "/script.sh");

        FILE *fp = fopen(script_path, "a");

        if (fp == NULL)
        {
            // cannot open script.sh
            continue;
        }
        
        /*
        find %S -name '*.[ch]' | parallel -j 4 'sed -f %s {} > {}.tmp && mv {}.tmp {}'
        */

        // call script.sh 
        // uses parallel for parallel processing source files
        // MAX_PROCESS is the max process number that can use parallel
        fprintf(fp, "find %s -name '*.[ch]' ", dir_path);
        fprintf(fp, "| parallel -j %d \"sed -f %s {} > {}.tmp && mv {}.tmp {}\"", MAX_PROCESS, sorted_sed_script);

        fclose(fp);
        
        int r_4 = fork_for_script(script_path);

        if (r_4 != 0)
        {
            // error fork for script
            printf("Error: fork for script\n");
            continue;
        }

        // substitute_files(dir_path, sed_script_path, NULL, author_dep, name_dep);

        // create log file
        // this log, if exists, only contains "Added prefix."
        // When clib tag is called again on a already prefixed dep, it search for log file
        // and if exist, skip the dep directory for time reasons
        char log_path[MAX_CHAR] = "";
        strcpy(log_path, dir_path);
        strcat(log_path, "/log");

        FILE *fp_log;

        fp_log = fs_open(log_path, "a");

        if (fp_log == NULL)
        {
            printf("Error: impossible to create log file.\n");
            printf("Error: dependency %s not modified with prefix.\n", name_dep);
            continue;
        }

        fs_fwrite(fp_log, LOG_MESSAGE_ADDED_PREFIX);

        fclose(fp_log);

        // remove useless files
        char path_to_tags[MAX_PATH] = "";

        strcpy(path_to_tags, dir_path);
        strcat(path_to_tags, "/functions");

        //rm_rf(path_to_tags);
        //rm_rf(sed_script_path);

        free(tags);

    }

    closedir(dp);
}

// count rows from a file with path
int rows_tags_file(char *path_tags)
{
    FILE *fp;

    fp = fopen(path_tags, "r");
    
    if (fp == NULL)
    {
        printf("Error: cannot open file %s", path_tags);
        return -1;
    }
    else
    {
        printf("Opening %s successful.\n", path_tags);
    }
    
    int rows = count_rows(fp);

    fclose(fp);

    return rows;
}

Tag *parse_tags_file(const char *path_tags, int rows)
{
    // filename : functions
    // open functions to parse it
    FILE *fp;

    fp = fopen(path_tags, "r");
    
    if (fp == NULL)
    {
        printf("Error: cannot open file %s", path_tags);
        return NULL;
    }
    else
    {
        printf("Opening %s successfully.\n", path_tags);
    }

    Tag *tags = (Tag *)calloc(rows, sizeof(Tag));

    if (tags == NULL)
    {
        printf("Error: tags not allocated.");
        return NULL;
    }

    // Build matrix lines
    char *line = NULL;
    size_t len_line = 0;
    
    char *token = NULL;
    
    // parse functions tag in file functions
    // identifier -- line -- kind
    int r = getline(&line, &len_line, fp);
    for (int i = 0; i < rows && r != -1; i++)
    {
        token = strtok(line, TAB_DELIM);
        strcat(tags[i].identifier, token);
        token = strtok(NULL, TAB_DELIM);
        strcat(tags[i].line, token);
        token = strtok(NULL, TAB_DELIM);
        strncat(tags[i].kind, token, strlen(token) - 1);
        r = getline(&line, &len_line, fp);
    }

    free(line);
    fclose(fp);
    
    return tags; 
}

int main(int argc, char *argv[])
{
    // build absolute path to /deps
    char path[MAX_PATH] = "";
    strcpy(path, getenv("PWD"));
    strcat(path, "/deps");

    // check if /deps directory exist
    struct stat sb;

    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        find_dir(path);
    }
    else
    {
        printf("Cannot open %s.\n", path);
    }

    return 0;
}
