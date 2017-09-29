#!/bin/bash

if [ ! -e ../install ]; then
    mkdir ../install
fi

cd ../install
cmake ..
make