rm -rf build
mkdir build
conan install . --build=missing -s build_type=RelWithDebInfo -of build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
make -j8