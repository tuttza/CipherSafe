#! /bin/bash
#set -e

LIBS_DIR="./ext_libs"
mkdir -p ${LIBS_DIR}

# Any new libraries need that are not made by us
# should be added here. We don't use git submodules :(.
# this function is responbisble for downloading and extracting the 
# needed library source. other functions should be made to fetch the source.
# those functions should follow this naming convention: get_<LIB_NAME>(); 

function get_libsodium() {	
	local BASE_URL="https://download.libsodium.org/libsodium/releases/"
	local LATEST_RELEASE=$(curl -s $BASE_URL | grep -oE 'libsodium-[0-9.]+-stable.tar.gz' | sort -V | tail -n 1)

	if [ -z "$LATEST_RELEASE" ]; then
		echo "Failed to determine the latest stable release of libsodium."
		exit 1
	fi

	local DOWNLOAD_URL="$BASE_URL$LATEST_RELEASE"
	echo "Downloading $LATEST_RELEASE..."
	curl -O "$DOWNLOAD_URL"
	echo "Download complete: $LATEST_RELEASE"

	echo "Extracting $LATEST_RELEASE..."
	tar -xzf "$LATEST_RELEASE" -C "${LIBS_DIR}"
	echo "Extraction complete."

	rm "${LATEST_RELEASE}"
}

function bootstrap_external_libs() {
	echo "Cloning mINI"
	git clone https://github.com/metayeti/mINI.git ${LIBS_DIR}/mINI

	echo "Cloning imgui"
	git clone https://github.com/ocornut/imgui.git ${LIBS_DIR}/imgui

	echo "Cloning doctest"
	git clone https://github.com/doctest/doctest.git ${LIBS_DIR}/docteset

	get_libsodium
}

#====[ Compile Library Functions ]====
function symlink_imgui_cmake() {
	echo "sym-linking imgui CMakeLists.txt file..."
	ln -sf $(pwd)/external_cmake/imgui/CMakeLists.txt "${LIBS_DIR}/imgui" 

	if [ -L "${LIBS_DIR}/imgui/CMakeLists.txt" ]; then
		local target=$(readlink -f "${LIBS_DIR}/imgui/CMakeLists.txt")

		if [ -e "$target" ]; then
			echo "Symlink ${LIBS_DIR}/imgui/CMakeLists.txt is valid and points to $target"
		else
			echo "Symlink ${LIBS_DIR}/imgui/CMakeLists.txt is broken (points to non-existent target)"
		fi
	else
		echo "${LIBS_DIR}/imgui/CMakeLists.txt is not a symlink"
	fi
}

function build_imgui() {
	symlink_imgui_cmake
	echo "Compiling imgui lib"
	cmake -S "${LIBS_DIR}/imgui" -B "${LIBS_DIR}/imgui/.build"
	pushd "${LIBS_DIR}/imgui/.build"
	make
	popd
}

function build_libsodium() {
	echo "Compiling libsodium"	
	pushd "${LIBS_DIR}/libsodium-stable/"
	/bin/bash -- ./configure
	make && make check
	sudo make install
	popd
}

#====[ MAIN ]====
echo "bootstraping CipherSafe dev environment..."
bootstrap_external_libs
build_imgui
build_libsodium