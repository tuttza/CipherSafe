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

#### Dear IMGui
If you have just cloned this repo you will need run the `./sym_link_helper.sh` script
which will symlink the CMakeLists.txt file used to build Dear ImGui library.

0. `./sym_link_helper.sh`
1. `cd CipherSafe/lib/imgui`
2. `cmake -S ./ -B ./build`
3. `cd ./build`
4. `make`

#### libsodium

0. `cd CipherSafe/lib/libsodium-stable`
1. `./configure`
2. `make && make check`
4. `sudo make install`

#### Build the App
1. `cd CipherSafe/src`
2. `cmake -S ./ -B ./build`
3. `cd ./build`
4. `make`
