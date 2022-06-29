#!/bin/bash

sync() {
    host=$1
    dest=/opt/amax/node_wallet/data/contracts

    rsync -rav -e ssh \
        --include='*.abi' \
        --include='*.wasm' \
        --exclude='**/CMakeFiles' \
        --exclude='*.cmake' \
        --exclude='Makefile' \
        --exclude='*.md' \
        --exclude='CMakeCache.txt' \
        --exclude='CMakeLists.txt' \
        ./build/contracts/ ${host}:${dest}

    # rsync -rav -e ssh ./unittest.sh ${host}:/opt/amax/wallet/
}

sync jw
sync sh-test