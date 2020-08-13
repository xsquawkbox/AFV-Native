# AFV-native

AFV-Native is a portable-(ish) implementation of the Audio for VATSIM client
protocol.

If you are not a client developer, this is not of interest to you and you should
refer to the 
[approved software list](https://www.vatsim.net/documents/approved-software) to
find a client for your needs.


# Goals

AFV-Native has the following goals:

* Licensing must remain compatible with XSquawkBox (which is effectively
  proprietary and requires static linking for distribution/support reasons, so 
  GPL-like licenses are not acceptable).  The BSD 3-Clause has been chosen for 
  AFV-Native itself.

* Must work on the big three: Windows, Linux and MacOS.

* Must work fairly uniformly on the big three.

* Only needs to work on architectures where there's a meaningful piece of 
  software to integrate with - that is, a pilot or ATC simulator and client.

* Must have a reasonably straight-forward API for client developers.

## Using AFV-Native

It's recommended to use the [Conan](https://conan.io) package for AFV-Native 
whenever possible rather than any other integration as it's the primary 
integration method used in XSB and the most tested.

Simply add the xsquawkbox-public & bincrafters repos to your conan workspace, 
and require `AFV-Native/<version>@xsquawkbox/<release-type>`, then use the 
correct method for your generators & build system to include the AFV-Native 
library and headers.
 
## Building AFV-native

### Prerequisites

To build AFV-native, you will require an up-to-date copy of cmake and conan.
* [Conan](https://conan.io)
* [CMake](https://cmake.org)

After installing conan, add the XSquawkBox Open-Source and Bincrafters 
Repository to your search path.
```shell script
conan remote add xsquawkbox-public https://api.bintray.com/conan/akuneko/xsquawkbox
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
```

This will let you find the packages required that are not published to 
`conan-center`.

You will also need a suitable compiler.

Tested compilers are:
* gcc (Linux)
* MSVC 2019 (Windows)
* apple-clang (macOS)

### Building

```shell script
$ mkdir build && cd build
$ conan install ..
$ cmake ..
```

Then build the project appropriately.

You can vary your conan and cmake lines as necessary to select alternate target 
profiles, provide options to cmake, etc.

## Limitations (Portability)

AFV-native was written with the following assumptions:
* Everybody is LittleEndian.

It explictly is written to work with:
* Win32 x86_64 compiled using MSVC
* amd64 Linux compiled using gcc
* Macos compiled using apple-clang.

Previously, AFV-Native assumed x86/x64, but the rise of compiler
autovectorisation means that we don't need to explicitly optimise for those 
cases to get ideal mixing code as long as we ensure certain conditions can't 
exist - the compiler should be able to work that out on it's own.  The code 
should no longer contain any x86-specific code when compiled with gcc/clang and
should be usable on LittleEndian ARM as a result.

You do need an optimised bit-search function (GnuC calls this "count
leading/trailing zeros").  Almost all CPUs have a native instruction to do this
faster than the most well written, tightly rolled loop can, and gcc (and clang
by virtue of it's gcc compatibility) and MSVC both expose the instruction via a
compiler builtin or intrinsic respectively.  If you're not using gcc, clang or
MSVC, you'll likely need to fix the CryptoDTO sequence test to use the correct
function for your compiler.

For the most part, it should be OS independent.  Library dependencies have been
selected due to their portability and common availability.

BigEndian will require significant attention in the CryptoDto code paths as the
serialisation code currently takes the easy-way-out of just direct copying from
the variable memory into the buffer as CryptoDto itself uses LE representations.
Given there are little to no BigEndian flight simulators supported by VATSIM, 
this is probably not an issue.

## Limitations (Features)

AFV-Native 1.0 only implements as much as required for pilot clients.  It 
doesn't understand the inter-controller direct voice controls, nor does it
include support for cross-coupling or multi-transmitter voice (although all the
groundwork is there and when those systems are standardised, they can be
incorporated fairly easily) 

## Licensing

AFV-Native is made available under the 3-Clause BSD License.  See `COPYING.md`
for the precise licensing text.

More information about this is in the `docs` directory.
