#!/bin/bash

curl -O http://www.digip.org/jansson/releases/jansson-2.9.tar.gz
tar -xvzf jansson-2.9.tar.gz
mkdir -p ./lib
cd jansson-2.9
./configure; make
cp ./src/jansson.h ../headers
cp ./src/jansson_config.h ../headers
cp ./src/.libs/libjansson.a ../lib
cd ..
rm  jansson-2.9.tar.gz
echo ""
echo ""
echo Libraries and headers added into /lib and /headers

gcc ./src/*.c ./infrakit-instance-oneview.c -I./headers -L./lib -ljansson -lcurl -o infrakit-instance-oneview

