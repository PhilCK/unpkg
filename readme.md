# UNPKG

Single file lib to help setup workspaces without to much effort. This is a
very minimal library it will not do syncing or even any checking. It requires
that you have already got installed the components it needs (E.g. git).
It will use platform provided utils where available (E.g curl).

Basically for a given project I will likely have some binary data, some
3rdparty code, and or some setup information that I don't want in the main
git repo, for example a premake executable, or bulid script, or a 3rd party
repo.

## What

- Unpkg uses [TOML](https://github.com/toml-lang/toml) for data description.
- Unpkg is C89 ish. Meaning I'm not 100%, but it works on the compilers I use.
- Unpkg has no command line options.
- Unpkg only looks for `unpkg.toml`, edit the source if you wish to change that.
- Unpkg will `git clone <repo>`.
- Unpkg will _not_ sync repos, you have todo that.
- Unpkg can clone a repo and select a single file.
- Unpkg will download an archive and unzip.
- Unpkg will _not_ stop you overwritting archive if called again.
- Unpkg can download an archive and select a single file.
- Unpkg does not cache diddle or squat.

## Why

- I'm stupid.
- I don't like subtrees.
- I don't like submodules.
- I don't like setup bash/bat scripts.
- I don't like extra dependencies.
- I don't like complex cmd line programs. 
- I just want to easily setup a workspace, using the compiler I already have.

## How

- Build unpkg using the super complex instructions and custom build setup below.
- Add `unpkg.toml` to a directory and describe your data. Example in `test/`.
- Copy the exeutable to the directory, or add it to your path env var.
- Call `unpkg`.
- Start working.
- If you edit something you got via `unpkg` you have to deal with it.

## Issues

- The TOML parsing needs work, it can be fooled easily.
- Can only select a single file currently.
- Very untested.

## Build

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