#!/bin/bash


mkdir universal-14.1.0
#mkdir universal-8.11.1

lipo -create intel64-14.1.0/dectalk/libtts.dylib intel-14.1.0/dectalk/libtts.dylib -output universal-14.1.0/libtts.dylib
#lipo -create intel-8.11.1/dectalk/libtts.dylib ppc-8.11.1/dectalk/libtts.dylib -output universal-8.11.1/libtts.dylib

