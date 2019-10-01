# AFV-native

## Building AFV-native

### Prerequisites

To build AFV-native, you will require an up-to-date copy of cmake and conan.
* [Conan](https://conan.io)
* [CMake](https://cmake.org)

After installing conan, add the XSquawkBox Open-Source and Bincrafters Repository to your search path.
```shell script
conan remote add xsquawkbox-public https://api.bintray.com/conan/akuneko/xsquawkbox
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

You will also need a suitable compiler.

Tested compilers are:
* gcc (Linux)
* MSVC 2019 (Windows)

This will let you find the packages required that are not published to `conan-center` (yet)

### Building

```shell script
$ mkdir build && cd build
$ conan install ..
$ cmake ..
```

Then build the project appropriately.

You can vary your conan and cmake lines as necessary to select alternate target profiles,
provide options to cmake, etc.

## Limitations

AFV-native was written with the following assumptions:
* Everybody has intel-style Compiler Intrinsics (true for gcc6+ + recent msvc)
* Everybody has SSE2 at the very least.
* Everybody is x86_64. (although it will be easy to fix for 32-bit systems)
* Everybody is LittleEndian.

It explictly is written to work with:
* Win32 x86_64 compiled using MSVC
* amd64 Linux compiled using gcc
* (coming soon) Macos compiled using apple-clang.

At this time, AFV-Native only implements as much as required for pilot clients.  It doesn't understand the 
inter-controller direct voice controls, nor does it include support for crosscoupling or multi-trannsmitter voice 
(although all the groundwork is there and when those systems are standardised, they can be incorporated fairly easily) 

For the most part, it should be OS independent, but some of the libraries it uses may not work with other operating 
systems.

It should be possible to port this to LittleEndian ARM with a bit of effort (mostly deoptimising the mixer and a few 
other places where SIMD is used).

BigEndian will require significant attention in the CryptoDto code paths as the serialisation code
currently takes the easy-way-out of just direct copying from the variable memory into the buffer as CryptoDto itself
uses LE representations.  Given there are little to no BigEndian flight simulators supported by VATSIM, this is probably 
not an issue.

## Licensing

AFV-Native is made available under the 3-Clause BSD License.  See `COPYING.md` for the precise licensing text.

More information about this is in the `docs` directory.

