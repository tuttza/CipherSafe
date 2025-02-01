#! /bin/bash

echo "Cloning mINI"
git clone https://github.com/metayeti/mINI.git ./lib/

echo "Cloning imgui"
git clone https://github.com/ocornut/imgui.git ./lib/

echo "Cloning doctest"
git clone https://github.com/doctest/doctest.git ./lib

./sym_link_helper.sh