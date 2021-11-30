#!/usr/bin/bash

mkdir ./p5/build
cd ./p5/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j16
cp ./mitscriptc ../../vm/mitscript
