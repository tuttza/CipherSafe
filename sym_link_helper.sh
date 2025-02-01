#! /bin/bash

function imgui_sym() {
    echo "sym-linking imgui CMakeLists.txt file..."
    ln -sf $(pwd)/external_cmake/imgui/CMakeLists.txt ./lib/imgui 

    if [ -L ./lib/imgui/CMakeLists.txt ]; then
	    target=$(readlink -f ./lib/imgui/CMakeLists.txt)

	    if [ -e "$target" ]; then
	        echo "Symlink ./lib/imgui/CMakeLists.txt is valid and points to $target"
	    else
	        echo "Symlink ./lib/imgui/CMakeLists.txt is broken (points to non-existent target)"
	    fi
    else
	    echo "./lib/imgui/CMakeLists.txt is not a symlink"
    fi
}

imgui_sym
