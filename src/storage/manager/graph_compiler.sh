#!/bin/bash

INCLUDES="src/storage/"
INPUT_FILE=$1
OUTPUT_FILE=$2

COMPILER=g++
STANDART=20
FLAGS="-lfmt"

if [ ! -f "$INPUT_FILE" ]; then
  echo "BAD INPUT FILE"
  exit -1
fi

if ! pkg-config --exists fmt; then
  echo "fmt library not found. Please install fmt or set PKG_CONFIG_PATH."
  exit -1
fi

CXXFLAGS=$(pkg-config --cflags fmt)
LDFLAGS=$(pkg-config --libs fmt)

$COMPILER -std=c++$STANDART -I$INCLUDES $INPUT_FILE -o $OUTPUT_FILE $CXXFLAGS $LDFLAGS
