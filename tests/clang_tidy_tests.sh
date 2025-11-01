#!/bin/bash

CLANG_TIDY_CONFIG=tests/.clang-tidy
CLANG_TIDY="clang-tidy-19 -p build/ --config-file=$CLANG_TIDY_CONFIG -extra-arg=-Wno-unknown-warning-option -extra-arg=-Wno-unused-command-line-argument -extra-arg=-Wno-invalid-command-line-argument --quiet"

for file in "$@"; do
    if [[ ! -f "$file" ]]; then
        echo "File $file does not exist." >&2
        exit 1
    fi
done

parallel -j$(nproc) "$CLANG_TIDY {}" ::: "$@"

if [[ $? -ne 0 ]]; then
    echo "Some files did not pass the check." >&2
    exit 1
else
    echo "All files passed the check."
    exit 0
fi