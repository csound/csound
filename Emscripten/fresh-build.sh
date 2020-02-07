#!/bin/bash
echo "Performing clean and fresh build..."
set -x
rm -rf deps/*
rm -rf build/*
emcc --clear-cache
bash download_and_build_libsndfile.sh
bash build.sh
bash update_example_libs_from_dist.sh
bash release.sh
set +x
echo "Finished clean and fresh build."
