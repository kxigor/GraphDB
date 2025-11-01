#!/bin/bash

INCLUDES="-Isrc/storage/ -Isrc/query/"
INPUT_FILE=$1
OUTPUT_FILE=$2

COMPILER=g++
STANDART=20
FLAGS="-O2 -lboost_serialization"

if [ ! -f "$INPUT_FILE" ]; then
  echo "BAD INPUT FILE"
  exit -1
fi

CXXFLAGS=$(pkg-config --cflags fmt)
LDFLAGS=$(pkg-config --libs fmt)

$COMPILER -std=c++$STANDART $INCLUDES $INPUT_FILE -o $OUTPUT_FILE $CXXFLAGS $LDFLAGS $FLAGS
