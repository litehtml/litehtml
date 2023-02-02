#!/bin/bash
g++ -o html2bmp -O2 \
-I ../include \
html2bmp.cc \
-I ../containers/test \
../containers/test/Bitmap.cpp \
../containers/test/Font.cpp \
../containers/test/lodepng.cpp \
../containers/test/test_container.cpp \
-L../build -llitehtml \
-L../build/src/gumbo -lgumbo
