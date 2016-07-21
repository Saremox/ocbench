# Readme

This project is aiming to assists people by the decision
what compression algorithm fits best for their data
and runtime requirements. For a given input data series
this tool will compress these and lists compression
ratio and time needed for each algorithm thats supported.

## Compiling

openCompressBench uses https://github.com/quixdb/squash as a backend.
to meet this dependency you have to compile squash for yourself or install
it through a package system if it's available on your system.

### Installing squash in home directory

Compiling squash is pretty straight forward I've tested ocbench with squash
commit
https://github.com/quixdb/squash/commit/67a2fd852d92ec7536eef52f0e235c7e66dd7878

```
git clone https://github.com/quixdb/squash.git
cd squash
git checkout 67a2fd852d92ec7536eef52f0e235c7e66dd7878
./autogen.sh --prefix=${HOME}/local
make -j$(nproc)
make install
```

As of this writing squash is installing his include files not correctly. In
order to get it working you have to symlink the main `squash.h`
```
ln ~/local/include/squash-0.8/squash.h ~/local/include/squash-0.8/squash/squash.h
```

### Compiling ocbench

If you followed the installation steps from above your need to export some
environment variables to get cmake finding your local squash installation.
```
export PATH="$HOME/local/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/local/lib"
export CMAKE_MODULE_PATH="$HOME/local/lib/cmake"
```

You've now met all prerequisites to `git clone` and make ocbench. Building
ocbench is as simple as:
```
git checkout 14381903aa04fce420c759e518632310ba16f6cd
cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make test
```

If all tests passes you're ready use the application.

**Note:** in the current state of development
`ocbench` only is capable of compressing one file and showing compressed size of
the file.

## Basic usage

```
$ ./ocBench
./ocBench openCompressBench Version 0.1
Copyright (C) 2016 Michael Strassberger <saremox@linux.com>
openCompressBench comes with ABSOLUTELY NO WARRANTY; for details
type `show w'.  This is free software, and you are welcome
to redistribute it under certain conditions; type `show c'
for details.

Usage: ./ocBench FILE_PATH


$ ./ocBench ocBench
Recv from child: compressed from 21296 bytes to 6456 bytes with lzma
```



## License

This Program is licensed under the terms of GNU General Public License, version 2
http://www.gnu.org/licenses/gpl-2.0.html
