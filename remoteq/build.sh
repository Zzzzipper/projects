#!/bin/sh

rm -rf ./.objs && rm -rf ./bin && qmake client.pro && make && rm -rf ./build-server && mkdir build-server && cmake . -Bbuild-server && cd ./build-server && make && pwd

