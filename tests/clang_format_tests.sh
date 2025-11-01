#!/bin/bash

CLANG_FORMAT_CONFIG=tests/.clang-format
CLANG_FORMAT_VERSION=19
CLANG_FORMAT="clang-format-$CLANG_FORMAT_VERSION --style=file:$CLANG_FORMAT_CONFIG"
exit_status=0

for file in "$@"; do
    if [[ ! -f "$file" ]]; then
        echo "File $file does not exist."
        exit_status=1
        continue
    fi

    $CLANG_FORMAT "$file" >temp.format

    if ! diff -q "$file" temp.format >/dev/null; then
        echo "File $file is not formatted correctly:"
        diff "$file" temp.format
        exit_status=1
    fi

    rm temp.format
done

if [[ $exit_status -ne 0 ]]; then
    echo "Some files did not pass the formatting check."
    exit 1
else
    echo "All files passed the formatting check."
    exit 0
fi
