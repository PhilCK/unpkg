# UNPKG

Single file lib to help setup workspaces without to much effort. This is a
very minimal library it will not do syncing or even any checking. It requires
that you have already got installed the components it needs (E.g. git).
It will use platform provided utils where available (E.g curl).

Basically for a given project I will likely have some binary data, some
3rdparty code, and or some setup information that I don't want in the main
git repo, for example a premake executable, or bulid script, or a 3rd party
repo.

## Why

- I don't like subtrees.
- I don't like submodules.
- I don't like setup bash/bat scripts.
- I don't like extra dependencies.
- I don't like complex cmd line programs.
- I'm stupid.
- I just want to easily setup a workspace, using the compiler I already have.

## Minimal Help

- Will `git clone <repo>`.
- Will _not_ sync repo, you have todo that.
- Can clone a repo and select a single file.
- Will download an archive and unzip.
- Will _not_ stop you overwritting archive if called again.
- Can download an archive and select a single file.

## Usage

- add unpkg.toml to a directory and describe your data. example in `test/`.
- call `unpkg`.
- start working.

## Issues

- The TOML parsing needs work.
- Can only select a single file currently.
- Very untested.

## Build

Unpkg file is a simple C file, to build ... 

with Clang ...

```
clang unpkg.c -o unpkg
```

With GCC ...

```
gcc unpkg.c -o unpkg
```

With MSVS ...

```
/* not tested atm */
```