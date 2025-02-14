#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Structure to hold command-specific options
typedef struct {
    // build options
    int release;
    // clean options
    // doc options
    int open;
    // publish options
    // run options
    char *bin_name;
    // test options
    char *test_name_or_pattern;
    // new options
    char *package_name;
    // init options
    char *project_name;
    char *template;
    // search options
    char *query;
    // install options
    char *package;
    // add options
    char *file;
    // remove options
    char *package_to_remove;
    // bench options
    char *bench_name;
    // update options
    // uninstall options
    char *package_to_uninstall;

    // Common options (if any) could be added here.
} CommandOptions;

void print_help(const char *program_name) {
    printf("Usage: %s <command> [options]\n", program_name);
    printf("\nCommands:\n");
    printf("  build [-r|--release]\n");
    printf("  check\n");
    printf("  clean\n");
    printf("  doc [--open]\n");
    printf("  publish\n");
    printf("  run [--bin <name>]\n");
    printf("  test [<test_name_or_pattern>]\n");
    printf("  new <package_name>\n");
    printf("  init [<project_name>] [--template <template>]\n");
    printf("  search <query>\n");
    printf("  install [<package>]\n");
    printf("  add <file>\n");
    printf("  remove <package>\n");
    printf("  bench [<bench_name>]\n");
    printf("  update\n");
    printf("  uninstall <package>\n");
    printf("  version\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    const char *command = argv[1];
    int opt;
    CommandOptions options = {0};

    if (strcmp(command, "build") == 0) {
        static struct option long_options[] = {
            {"release", no_argument, 0, 'r'},
            {0, 0, 0, 0}
        };
        while ((opt = getopt_long(argc - 1, &argv[1], "r", long_options, NULL)) != -1) {
            switch (opt) {
                case 'r':
                    options.release = 1;
                    break;
                case '?':
                    return 1;
                default:
                  break;
            }
        }
        printf("Command: build\n");
        printf("Release: %d\n", options.release);

    } else if (strcmp(command, "check") == 0) {
        printf("Command: check\n");
    } else if (strcmp(command, "clean") == 0) {
        printf("Command: clean\n");
    } else if (strcmp(command, "doc") == 0) {
        static struct option long_options[] = {
            {"open", no_argument, 0, 'o'},
            {0, 0, 0, 0}
        };
        while ((opt = getopt_long(argc - 1, &argv[1], "o", long_options, NULL)) != -1) {
            switch (opt) {
                case 'o':
                    options.open = 1;
                    break;
                case '?':
                    return 1;
                default:
                  break;
            }
        }
        printf("Command: doc\n");
        printf("Open: %d\n", options.open);
    } else if (strcmp(command, "publish") == 0) {
        printf("Command: publish\n");
    } else if (strcmp(command, "run") == 0) {
      static struct option long_options[] = {
          {"bin", required_argument, 0, 'b'},
          {0, 0, 0, 0}
      };
      while ((opt = getopt_long(argc - 1, &argv[1], "b:", long_options, NULL)) != -1) {
          switch (opt) {
              case 'b':
                  options.bin_name = optarg;
                  break;
              case '?':
                  return 1;
              default:
                break;
          }
      }
      printf("Command: run\n");
      printf("Bin Name: %s\n", options.bin_name);

    } else if (strcmp(command, "test") == 0) {
        if (optind < argc - 1) {
          options.test_name_or_pattern = argv[optind];
        }

        printf("Command: test\n");
        printf("Test Name/Pattern: %s\n", options.test_name_or_pattern);
    } else if (strcmp(command, "new") == 0) {
        if (optind < argc - 1) {
          options.package_name = argv[optind + 1];
          printf("Package is: %s\n", options.package_name);
          printf("Last is: %s\n", argv[argc -1]);
          printf("Num options: %d\n", argc);

          lua_State *L = luaL_newstate();
          luaL_openlibs(L);

          // Work with lua API
          if (luaL_dofile(L, "coffee-new.lua") == LUA_OK) {
            printf("coffee-new.lua read ok\n");
            lua_pop(L, lua_gettop(L));
          }

          // Put the function to be called onto the stack
          lua_getglobal(L, "check_dir");
          lua_pushstring(L, options.package_name);
          printf("Calling check_dir\n");




          // Execute my_function with 1 argument and 1 return value
          int result = lua_pcall(L, 1, 1, 0);
          printf("Called check_dir. Status: %d\n", result);
          if (result == LUA_OK) {
            printf("ok check_dir\n");

            // Check if the return is an integer
            if (lua_isinteger(L, -1)) {
              printf("Return value ok\n");

              // Convert the return value to integer
              int result = lua_tointeger(L, -1);

              // Pop the return value
              lua_pop(L, 1);
              printf("Result: %d\n", result);
            }
            // Remove the function from the stack
            lua_pop(L, lua_gettop(L));
          } else {
            fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
            exit(1);
          }
    lua_close(L);
        } else {
          fprintf(stderr, "Error: 'new' command requires a <package_name> argument.\n");
          return 1;
        }
        printf("Command: new\n");
        printf("Package Name: %s\n", options.package_name);
    } else if (strcmp(command, "init") == 0) {
        static struct option long_options[] = {
            {"template", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        while ((opt = getopt_long(argc - 1, &argv[1], "", long_options, NULL)) != -1) {
            switch (opt) {
                case 't':
                    options.template = optarg;
                    break;
                case '?':
                    return 1;
                default:
                  break;
            }
        }
        if (optind < argc - 1) {
          options.project_name = argv[optind];
        }
        printf("Command: init\n");
        printf("Project Name: %s\n", options.project_name);
        printf("Template: %s\n", options.template);

    } else if (strcmp(command, "search") == 0) {
      if (optind < argc - 1) {
        options.query = argv[optind];
      } else {
        fprintf(stderr, "Error: 'search' command requires a <query> argument.\n");
        return 1;
      }
      printf("Command: search\n");
      printf("Query: %s\n", options.query);
    } else if (strcmp(command, "install") == 0) {
      if (optind < argc - 1) {
        options.package = argv[optind];
      }
      printf("Command: install\n");
      printf("Package: %s\n", options.package);
    } else if (strcmp(command, "add") == 0) {
      if (optind < argc - 1) {
        options.file = argv[optind];
      } else {
        fprintf(stderr, "Error: 'add' command requires a <file> argument.\n");
        return 1;
      }
      printf("Command: add\n");
      printf("File: %s\n", options.file);
    } else if (strcmp(command, "remove") == 0) {
      if (optind < argc - 1) {
        options.package_to_remove = argv[optind];
      } else {
        fprintf(stderr, "Error: 'remove' command requires a <package> argument.\n");
        return 1;
      }
      printf("Command: remove\n");
      printf("Package: %s\n", options.package_to_remove);
    } else if (strcmp(command, "bench") == 0) {
      if (optind < argc - 1) {
        options.bench_name = argv[optind];
      }
      printf("Command: bench\n");
      printf("Bench Name: %s\n", options.bench_name);
  } else if (strcmp(command, "bench") == 0) {
      if (optind < argc - 1) {
        options.bench_name = argv[optind];
      }
      printf("Command: bench\n");
      printf("Bench Name: %s\n", options.bench_name);
    } else if (strcmp(command, "update") == 0) {
        printf("Command: update\n");
    } else if (strcmp(command, "uninstall") == 0) {
      if (optind < argc - 1) {
        options.package_to_uninstall = argv[optind];
      } else {
        fprintf(stderr, "Error: 'uninstall' command requires a <package> argument.\n");
        return 1;
      }
      printf("Command: uninstall\n");
      printf("Package: %s\n", options.package_to_uninstall);
    } else if (strcmp(command, "version") == 0) {
        printf("Command: version\n");
    } else {
        print_help(argv[0]);
        return 1;
    }

    return 0;
}
