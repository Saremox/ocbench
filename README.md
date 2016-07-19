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

After you met the dependencies you just need to call `cmake` in the cloned
directory or at any location you want with the -B switch of cmake see
`man cmake`.
Than you finished creating the cmake build environment you only need to invoke
`make`. To test your build type `make test`. If doesn't complain about failed
test you can now test the application or install it with `make install`

**Note:** in the current state of development
`ocbench` does nothing.

## License

This Program is licensed under the terms of GNU General Public License, version 2
http://www.gnu.org/licenses/gpl-2.0.html
