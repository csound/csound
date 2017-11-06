clear
echo "Building Csound for WebAssembly..."
echo "Updating the Emscripten toolchain..."
cd ~/emsdk
./emsdk install sdk-incoming-64bit binaryen-master-64bit
./emsdk activate sdk-incoming-64bit binaryen-master-64bit
source ./emsdk_env.sh
export EMSCRIPTEN_ROOT=$EMSCRIPTEN
echo "Deleting previous build..."
cd ~/csound/csound/emscripten
rm /tmp/emscripten_temp/*.*
rm -rf build-wasm
echo "Building libsndfile for WebAssembly..."
sh download_and_build_libsndfile_wasm.sh
echo "Building the Csound library for WebAssembly..."
sh build-wasm.sh
echo "Updating the Csound examples for WebAssembly..."
sh update_example_libs_from_dist_wasm.sh
echo "Creating a release directory (dist-wasm) for Csound for WebAssembly..."
sh release-wasm.sh
echo "Copying dist-wasm files to gogins.github.io..."
sh copy_wasm_to_gogins_github_io.sh
echo "Finished building Csound for WebAssembly."
ls -ll /tmp/emscripten_temp
ls -ll build-wasm