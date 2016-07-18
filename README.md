# Readme

This project is aiming to assists people by the decision
what compression algorithm fits best for their data
and runtime requirements. For a given input data series
this tool will compress these and lists compression
ratio and time needed for each algorithm thats supported.

## Compiling

openCompressBench uses https://github.com/quixdb/squash as a backend.
to meet this dependency you can either:
  1. compile squash yourself
  2. add `-DBUILD_SQUASH=1` as an cmake argument

### Compilation with SQUASH prefinstalled
Your have to ensure that your local compiler installation can find squash's
header and library files to compile openCompressBench successfully
```
$ cmake
```

### Compilation with SQUASH not prefinstalled
You have to ensure to meet all dependencies of squash to get a successful build.
```
$ cmake -DBUILD_SQUASH=1
```

Than u finished creating the cmake build enviroment you only need to invoke
`make`. To test your build type `make test`. If doesn't complain about failed
test you can now test the application or install it with `make install`

**Note:** in the current state of development
`ocbench` does nothing.

## License

This Program is licensed under the terms of GNU General Public License, version 2
http://www.gnu.org/licenses/gpl-2.0.html
