# Smyth
Conlang creation toolkit. The intent of this tool is to eventually bundle commonly
needed functionality into a single easily accessible application. Features currently
include
- An improved Lexurgy frontend that supports changing the font (size), as well as Unicode
  normalisation and simple JS scripting.
- A spreadsheet-based dictionary
- A character map for finding and copying Unicode characters.
- A notes tab for taking notes

Currently, this application is only tested on Linux.

DISCLAIMER: This application is still in development and may not work as expected. 
The project file format is currently NOT stable and may change at any time. 

## Building
Building this application requires:
- a recent C++ compiler (Clang 18 or later);
- a recent version of libstdc++ (libc++ isn’t tested and also won’t work unless you link Qt against it);
- a recent version of CMake (3.27 or later);
- an installation of Qt6 (6.6.1 or later should work);
- ICU (International Components for Unicode).

Other dependencies will be downloaded at build time.

To build the application, `cd` into the project directory and run the following commands
```bash
cmake -S . -B out -DSMYTH_QT_PREFIX_PATH=<path-to-qt>/<qt-version>/<platform>
cmake --build out -- -j`nproc`
``` 
where `<qt-version>` is the version of Qt installed on your system, and `<platform>` 
the name of your platform. When in doubt, check your `Qt` directory for the version
and available platforms.

For example, my Qt installation is in `/opt/Qt`, my Qt version is `6.6.1`, and my
platform is x86_64, for which there is a `gcc_64` directory. Putting all of these
together, the path I need to specify ends up being `/opt/Qt/6.6.1/gcc_64`.

If your Qt installation is split up across several directories, set
your path to `/usr/lib` or `/usr/lib64`. The path you need is the
one that contains `Qt6Config.cmake` in one of its subdirectories.
