#!/bin/bash

wget http://www.digip.org/jansson/releases/jansson-2.9.tar.gz
tar -xvzf jansson-2.9.tar.gz
cd jansson-2.9
./configure; make
cp ./src/jansson.h ../headers
cp ./src/jansson_config.h ../headers
cp ./src/.libs/libjansson.a ../lib
cd ..

echo ""
echo ""
echo Libraries and headers added into /lib and /headers
