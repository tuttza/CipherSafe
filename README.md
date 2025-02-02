# CipherSafe

A simple password manager.

CipherSafe is built with 4 key technologies:

0. CPP(11)
1. SQLITE (for datastore)
2. Dear IMGUI (for the UI) - 
3. libsodium-stable (encryption) - https://download.libsodium.org/libsodium/releases/
4. mINI (for ini config file parsing/writing)

## Building CipherSafe

### Build the Libs:
In order to build CipherSafe it depends on few 3rd partly libraries mentioned above.

There is a script in root of CipherSafe's source called: `bootstrap.sh` and it is responsible for downloading/extracting/compiling our 3rd party libaries into a directory called `./ext_libs` (which will not exist until your run `bootstrap.sh`)

#### Note*
If 3rd party libraries need to be built via `cmake` create a `CMakeLists.txt` file in `./external_cmake` and a symlink via a symlinker function in `bootstrap.sh`


#### Build the App
1. `cd CipherSafe/src`
2. `cmake -S ./ -B ./build`
3. `cd ./build`
4. `make`
