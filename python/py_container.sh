#!/bin/bash
g++ -O2 \
-I ../include \
-o py_container.so -fPIC -shared \
py_container.cpp \
-L../build -llitehtml \
-L../build/src/gumbo -lgumbo
