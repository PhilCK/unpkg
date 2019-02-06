# UNPKG

This library helps setup those annoying bits of a project workspaces.
You can specify what binaries or repos you need for you application in a
configuration file, call `unpkg` and it fetch those things.

Actually Unpkg does very little other than generate command line arguments for
you, so if you wish to clone a repo with git you need to have git installed in
the prompt you are calling Unpkg from.

## What

- Unpkg reads a config file, and calls the command line to setup your workspace.
- Unpkg uses [TOML](https://github.com/toml-lang/toml) for data description.
- Unpkg is C89 ish. Meaning I'm not 100%, but it works on the compilers I use.
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
- I want to easily setup a workspace, using the compiler I already have.
- I want to control that workspace after setup.

## How

- Build unpkg using the super complex instructions and custom build setup below.
- Add `unpkg.toml` to a directory and describe your data. Example in `test/`.
- Copy the exeutable to the directory, or add it to your path env var.
- Call `unpkg`.
- Start working.
- If you edit something you got via `unpkg` you have to deal with it.
- `unpkg <insert pkg name>` if you only want to process one item.
- E.g `unpkg premake-bin-win`

## UNPKG Doc

```toml
[package-name]           # Pkg name, used to select pkg manually
url = "http://*****.com" # Where to find the resource
type = "git"             # Can be git or archive
select = "foo.json"      # Optional: if you only want one file from the resource
platform = "Windows"     # Optional: Lock package to this platform, `Linux` and `macOS` are also valid
```

## Issues

- Some issues with parsing.
- Only supports unix style endings currently `\n`.
- Can only select a single file currently. BOO!
- Very untested.
- Single threaded.

## Build

with Clang

```
clang unpkg.c -o unpkg
```

With GCC

```
gcc unpkg.c -o unpkg
```

With MSVS you can do from command line, you may have to run something like
this first in a cmd prompt.

```
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
```

then you can do

```
cl unpkg.c
```

## Credits

| Who | What | 
|-----|------|   
| [Sean T.Barret](https://twitter.com/nothings) | [Stretch Buffer](https://github.com/nothings/stb/blob/master/stretchy_buffer.h) |


## License

MIT