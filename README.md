# Smyth
Conlang creation toolkit. The intent of this tool is to eventually bundle commonly
needed functionality into a single easily accessible application. Features currently
include
- An improved Lexurgy frontend that supports changing the font (size), as well as Unicode
  normalisation and simple JS scripting.
- A character map for finding and copying Unicode characters.

Currently, this application is only tested on Linux.

DISCLAIMER: This application is still in development and may not work as expected. 
However, the project file format is expected to remain stable.

## Building
Building this application requires:
- a recent C++ compiler (Clang 18 or later);
- a recent version of CMake (3.27 or later);
- an installation of Qt6 (6.6.1 or later should work);
- ICU (International Components for Unicode).

Other dependencies will be downloaded at build time.

To build the application, run the following commands
```bash
cmake -S . -B out -DSMYTH_QT_PREFIX_PATH=<path-to-qt>/<qt-version>/<platform>
cmake --build out
``` 
where `<qt-version>` is the version of Qt installed on your system, and `<platform>` 
the name of your platform. When in doubt, check your `Qt` directory for the version
and available platforms.

For example, my Qt installation is in `/opt/Qt`, my Qt version is `6.6.1`, and my
platform is x86_64, for which there is a `gcc_64` directory. Putting all of these
together, the path I need to specify ends up being `/opt/Qt/6.6.1/gcc_64`.
