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

Compiling squash is pretty straight forward. So far I've tested ocbench with
squash commit
https://github.com/quixdb/squash/commit/67a2fd852d92ec7536eef52f0e235c7e66dd7878
but this instructions should also apply to later commits.

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

If you followed the installation steps from above, you need to export some
environment variables, so cmake can find your local squash installation.
```
export PATH="$HOME/local/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/local/lib"
export CMAKE_MODULE_PATH="$HOME/local/lib/cmake"
```

You've now met all prerequisites to `git clone` and make ocbench. Building
ocbench is as simple as:
```
git checkout 5ae4ae9f148a309e33d01510f478c3faa77beecf
cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make test
```

If all tests passes you're ready to use the application.

**Note:** in the current state of development
`ocbench` only is capable of parsing a directory adding all files into a
list and compressing one of them and showing compressed size of the file.

## Basic usage

```
$ ./src/ocBench --help
Usage: ./src/ocBench [OPTION]... --directory=./

Options:
  -D, --database   PATH to sqlite3 database file
                   Default: ./results.sqlite3
  -d, --directory  PATH to directory which will be analyzed
                   Default: current working directory
  -c, --codecs     STRING comma seperated list of codecs which will
                   be used. Format: "plugin:codec;codec,plugin:..."
                   Default: "bzip2:bzip2,lzma:xz,zlib:gzip"
  -w, --Worker     INT ammount of worker processes.
                   Default: 1

  -v, --verbose    more verbosity equals --log-level=2
  -l, --log-level  INT 0 LOG_ERR
                       1 LOG_WARN
                       2 LOG_INFO
                       3 LOG_DEBUG
  -h, --help       Print this page
  -u, --Usage      Print this page
  -V, --version    Print version of ./src/ocBench

$ ./src/ocBench --directory ./ -l 3
[...]
[DEBUG] (***/src/ocbench.c:  87): Open Dir: ./CMakeFiles/CMakeDirectoryInformation.cmake/
[DEBUG] (***/src/ocbench.c: 122): Add: ./CMakeFiles/CMakeDirectoryInformation.cmake with a size of 633 bytes
[DEBUG] (***/src/ocbench.c: 105): Testing File: feature_tests.c
[...]
[DEBUG] (***/src/ocbench.c: 363): Plugin:    bzip2 with Codec:    bzip2
[DEBUG] (***/src/ocbench.c: 363): Plugin:     lzma with Codec:       xz
[DEBUG] (***/src/ocbench.c: 363): Plugin:     zlib with Codec:     gzip
[...]
Recv from child: compressed from 21296 bytes to 6456 bytes with lzma
```



## License

This Program is licensed under the terms of GNU General Public License, version 2
http://www.gnu.org/licenses/gpl-2.0.html
