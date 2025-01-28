# clib(1)

  ![Build Status](https://github.com/clibs/clib/actions/workflows/tests.yml/badge.svg)
  [![Codacy Badge](https://app.codacy.com/project/badge/Grade/a196ec36c31349e18b6e4036eab1d02c)](https://www.codacy.com/gh/clibs/clib?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=clibs/clib&amp;utm_campaign=Badge_Grade)

  Package manager for the C programming language.

  ![c package manager screenshot](https://i.cloudup.com/GwqOU2hh9Y.png)

## Installation

  Dependencies:
  
   - Expects [libcurl](http://curl.haxx.se/libcurl/) to be installed and linkable.

   - Expects [ctags] (https://github.com/universal-ctags/ctags) to be installed.

   - Expects [parallel] (can be installed with sudo apt install parallel) to be installed.

  With git (cloning branch clone_git):

```sh
$ git clone --branch clone_git https://github.com/AlgoLab/clib /tmp/clib
$ cd /tmp/clib
$ make install
```

  Ubuntu:

```sh
# install libcurl
$ sudo apt-get install libcurl4-gnutls-dev -qq
# clone
$ git clone --branch clone_git https://github.com/AlgoLab/clib /tmp/clib && cd /tmp/clib
# build
$ make
# put on path
$ sudo make install
```

  Fedora:

```sh
# install libcurl
$ sudo dnf install libcurl-devel
# clone
$ git clone --branch clone_git https://github.com/AlgoLab/clib /tmp/clib && cd /tmp/clib
# build
$ make
# put on path
$ sudo make install
```


## About

  Basically the lazy-man's copy/paste promoting smaller C utilities, also
  serving as a nice way to discover these sort of libraries. Clib has a wiki where it gets his clib designed package (usually C small utilities). This clib fork can download and install non clib designed package from github. Actually, it can download every repository (written in C) from GitHub. It's aim is to be a general purpose package manager for every C library available on GitHub.

  You should use `clib(1)` to fetch these files for you and check them into your repository, the end-user and contributors should not require having `clib(1)` installed. This allows `clib(1)` to fit into any new or existing C workflow without friction.

  The wiki [listing of packages](https://github.com/clibs/clib/wiki/Packages) acts as the "registry" and populates the `clib-search(1)` results.

## Usage

```
  clib <command> [options]

  Options:

    -h, --help     Output this message
    -V, --version  Output version information

  Commands:

    init                 Start a new project
    i, install [name...] Install one or more packages
    up, update [name...] Update one or more packages
    upgrade [version]    Upgrade clib to a specified or latest version\
    configure [name...]  Configure one or more packages
    build [name...]      Build one or more packages
    search [query]       Search for packages
    help <cmd>           Display help for cmd
```

Create a directory for your project. Create a dir called deps for your dependencies. Create a dir called src for your source files.
You can use

```
  clib init
```
to initialize in your directory your clib package (it creates a clib.json manifest for your clib project but you can omit this passage).

Clib project root directory tree:

```
  .
  ├── clib.json
  ├── deps
  │   └── dep_1
  │       ├── dep_1.c
  │       └── dep_1.h
  └── src
      ├── source_1.c
      └── source_1.h
```

## clib install command
 
 This command must be executed in a clib project directory (it must contain /deps directory).
 It can download clib designed and non clib designed packages. 
 For non clib designed packages it creates in the package directory in /deps a manifest package.json
 that contains general information about the package: author, name and version.

 Argument format is:
```
  author_repo/name_repo@version
```

Example:
```
  clib install madler/zlib@1.2
```

## clib update command

This command must be executed in a clib project directory (it must contains /deps directory).
It can update to a specific version an installed package or a non installed package. 
The input accepted is:
```
  author_repo/name
```
or 
```
  author_repo/name@version
```

If specified a version, clib will update the package to that specific version, otherwise it will install from scratch
to the latest version.

More about the Command Line Interface [here](https://github.com/clibs/clib/wiki/Command-Line-Interface).

## Examples

 More examples and best practices at [BEST_PRACTICE.md](https://github.com/clibs/clib/blob/master/BEST_PRACTICE.md).

 Install a few dependencies to `./deps`:

```sh
$ clib install clibs/ms clibs/commander
```

 Install them to `./src` instead:

```sh
$ clib install clibs/ms clibs/commander -o src
```

 When installing libraries from the `clibs` org you can omit the name:

```sh
$ clib install ms file hash
```

 Install some executables:

```sh
$ clib install visionmedia/mon visionmedia/every visionmedia/watch
```
## clib tag

Clib tag is a command that adds for every directory in the deps directory, so dependency,
the prefix "author_name_" (author and name of the dependency) to every function declared
in every source file of the dependency. The aim of this command is to limit names conflicts
of libraries using the standard of C community: adding the library prefix to the functions.
It can be used from a clib project root directory: it adds prefix for every dependency installed 
with 
```
clib install
```


## clib.json

 Example of a clib.json explicitly listing the source:

```json
{
  "name": "term",
  "version": "0.0.1",
  "repo": "clibs/term",
  "description": "Terminal ansi escape goodies",
  "keywords": ["terminal", "term", "tty", "ansi", "escape", "colors", "console"],
  "license": "MIT",
  "src": ["src/term.c", "src/term.h"]
}
```

 Example of a clib.json for an executable:

```json
{
  "name": "mon",
  "version": "1.1.1",
  "repo": "visionmedia/mon",
  "description": "Simple process monitoring",
  "keywords": ["process", "monitoring", "monitor", "availability"],
  "license": "MIT",
  "install": "make install"
}
```

Example of a package.json in a dependency directory not listed in the clib wiki

```json
{
  "author": "samtools"
  "name": "htslib",
  "version": "0.1.0"
}
```


 Example of using Clib for a non clib designed package
 (the command must be executed in a clib project dir (it must contain /deps dir)):

```sh
$ clib install madler/zlib
```

 See [explanation of clib.json](https://github.com/clibs/clib/wiki/Explanation-of-clib.json) for more details.

## Contributing

 If you're interested in being part of this initiative let me know and I'll add you to the `clibs` organization so you can create repos here and contribute to existing ones.
 
 If you have any issues, questions or suggestions, please open an issue [here](https://github.com/clibs/clib/issues). 
 
 You can also find us on Gitter: https://gitter.im/clibs/clib
 
 Also feel free to open a GitHub Discussion [here](https://github.com/clibs/clib/discussions).

 Before committing to the repository, please run `make commit-hook`. This installs a commit hook which formats `.c` and `.h` files.

## Articles

  - [Introducing Clib](https://medium.com/code-adventures/b32e6e769cb3) - introduction to clib
  - [The Advent of Clib: the C Package Manager](https://web.archive.org/web/20200128184218/http://blog.ashworth.in/2014/10/19/the-advent-of-clib-the-c-package-manager.html) - overview article about clib
