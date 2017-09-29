#!/bin/bash

if [ ! -e ~/include/ctp ]; then
    mkdir ~/include/ctp
fi

cp ../lib/ctp/*.h ~/include/ctp/
cp thostmduserapi.so /usr/lib/libthostmduserapi.so
cp thosttraderapi.so /usr/lib/libthosttraderapi.so

sudo apt-get install zlib1g-dev