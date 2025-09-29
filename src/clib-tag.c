/*
    Author: Simone Stefanello
    Name: clib-tag.c
    Description: implement a "namespace" by insert in every function identifier a prefix for the library,
                 used as an agreement in C programming. 
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
#define LOG_MESSAGE_ADDED_PREFIX "Added prefix.\n"

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
        // only functions names
	    #if !defined(BSD) && !defined(__FreeBSD__)
        char *arg[] = {"ctags", "-x", "--kinds-C=f", "--_xformat=%N", path_to_source, NULL};
	    #else
	    char *arg[] = {"uctags", "-x", "--kinds-C=f", "--_xformat=%N", path_to_source, NULL};
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

            // search for all .c and .h
            if (ext && (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0)) 
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
        printf("Cannot open package.json\n");
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

        // build path to tag function file and tag functions without internal repetitions
        // adding 2 for snprintf because of / and the terminal char
        char functions[] = "functions";
        char functions_path[MAX_PATH + sizeof(functions) + 2] = "";
        snprintf(functions_path, sizeof(dir_path) + sizeof(functions) + 2, "%s/%s", dir_path, functions);

        char functions_uniq[] = "functions_uniq";
        char functions_uniq_path[MAX_PATH + sizeof(functions_uniq) + 2] = "";
        snprintf(functions_uniq_path, sizeof(dir_path) + sizeof(functions_uniq) + 2, "%s/%s", dir_path, functions_uniq);

        int log_check = 0;

        find_files(dir_path, functions_path, &log_check);

        if (log_check != 0)
        {
            // ignore deps already modified with prefix
            continue;
        }

        // here I call sed recursively on the .c and .h source files
        // now that I have functions_path

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
            return;
        }

        // build script_tag.sh and script_substitution to call with fork
        // (this enables only one call per dependency directory)
        char script_path[MAX_PATH] = "";
        strcat(script_path, dir_path);
        strcat(script_path, "/script_tag.sh");

        char script_path_substitution[MAX_PATH] = "";
        strcat(script_path_substitution, dir_path);
        strcat(script_path_substitution, "/script_substitution.sh");

        // create script_tag.sh
        FILE *fp = fopen(script_path, "a");

        if (fp == NULL)
        {
            // cannot open script.sh
            return;
        }
        
        // call script.sh 
        fprintf(fp, "sort %s -o %s\n", functions_path, functions_path);
        fprintf(fp, "touch %s\n", functions_uniq_path);
        fprintf(fp, "uniq %s > %s\n", functions_path, functions_uniq_path);
        #if !defined(BSD) && !defined(__FreeBSD__) && !defined(__APPLE__)
        fprintf(fp, "sed -i '/\\bmain\\b/d' %s\n", functions_uniq_path);
        #else
        fprintf(fp, "sed -i '' '/[[:<:]]main[[:>:]]/d' %s\n", functions_uniq_path);
        #endif
        fprintf(fp, "rm %s", functions_path);

        fclose(fp);

        // create script_substitution.sh
        fp = fopen(script_path_substitution, "a");

        if (fp == NULL)
        {
            // cannot open script.sh
            return;
        }

        #if !defined(BSD) && !defined(__FreeBSD__) && !defined(__APPLE__)
        /*
        find . -name "*.[ch]" -exec sh -c '
            for file do
                grep -o -f tag_functions_uniq "$file" |
                awk "{print \"/#include/!s/\\\\b\" \$1 \"\\\\b/author_dep_name_dep_\" \$1 \"/g\"}" |
                sort -u |
                sed -i -f - "$file"
            done
        ' sh {} +
        */
        fprintf(fp, "find %s -name \"*.[ch]\" -exec sh -c '\n", dir_path);
        fprintf(fp, "\tfor file do\n");
        fprintf(fp, "\t\tgrep -o -f %s \"$file\" |\n", functions_uniq_path);
        fprintf(fp, "\t\tawk \"{print \\\"/#include/!s/\\\\\\\\b\\\" \\$1 \\\"\\\\\\\\b/%s_%s_\\\" \\$1 \\\"/g\\\"}\" |\n", author_dep, name_dep);
        fprintf(fp, "\t\tsort -u |\n");
        fprintf(fp, "\t\tsed -i -f - \"$file\"\n");
        fprintf(fp, "\tdone\n");
        fprintf(fp, "' sh {} +\n");
        #else
        /*
		find . -name "*.[ch]" -exec bash -c '
		  for file do
		    grep -o -f functions_uniq "$file" | sort -u > "$file.syms"
		    awk -v p="samtools_htslib_" \
		        "{print \"/#include/!s/[[:<:]]\" \$1 \"[[:>:]]/\" p \$1 \"/g\"}" "$file.syms" > regole.sed
		    sed -f regole.sed "$file" > "$file.tmp" && mv "$file.tmp" "$file"
		    rm -f "$file.syms" regole.sed "$file.tmp"
		  done
		' bash {} +
		*/
        fprintf(fp, "find %s -name \"*.[ch]\" -exec bash -c '\n", dir_path);
        fprintf(fp, "\tfor file do\n");
        fprintf(fp, "\t\tgrep -o -f %s \"$file\" | sort -u > \"$file.syms\"\n", functions_uniq_path);
        fprintf(fp, "\t\tawk -v p=\"%s_%s_\" \\\n", author_dep, name_dep);
        fprintf(fp, "\t\t\"{print \\\"/#include/!s/[[:<:]]\\\" \\$1 \\\"[[:>:]]/\\\" p \\$1 \\\"/g\\\"}\" \"$file.syms\" > regole.sed\n");
        fprintf(fp, "\t\tsed -f regole.sed \"$file\" > \"$file.tmp\" && mv \"$file.tmp\" \"$file\"\n");
        fprintf(fp, "\t\trm -f \"$file.syms\" regole.sed \"$file.tmp\"\n");
        fprintf(fp, "\tdone\n");
        fprintf(fp, "' bash {} +");
        #endif

        fclose(fp);

        
        // divide rules file in more file of maximum 1000 lines
        // unico file, file di buffer in cui ogni volta carichi 1000 max righe
        // e allo script dai ogni volta quel file
        int r_2 = fork_for_script(script_path);

        if (r_2 != 0)
        {
            // error fork for script
            printf("Error: fork for script\n");
            continue;
        }

        int r_3 = fork_for_script(script_path_substitution);

        if (r_3 != 0)
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

    }

    closedir(dp);
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
