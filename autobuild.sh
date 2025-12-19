rm -rf build
mkdir build
conan install . --build=missing -s build_type=Release -of build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
make -j8