#!/bin/bash

# 1. Check for input
if [ $# -eq 0 ]; then
    echo "Usage: ./compile_slang.sh shadername.slang"
    exit 1
fi

FILE=$1
BASE="${FILE%.*}"

# 2. Clean up old binaries for this specific shader
echo "Cleaning old binaries for ${BASE}..."
rm -f "${BASE}.vert.spv" "${BASE}.frag.spv" "${BASE}.spv"

# 3. Compile
echo "Compiling $FILE..."

# Generate separate SPIR-V files for vertex and fragment stages
slangc "$FILE" -target spirv -entry vertexMain -o "${BASE}.vert.spv"
slangc "$FILE" -target spirv -entry fragmentMain -o "${BASE}.frag.spv"

# 4. Final check
if [ $? -eq 0 ]; then
    echo "Done! Created ${BASE}.vert.spv and ${BASE}.frag.spv"
else
    echo "Error: Compilation failed for $FILE"
    exit 1
fi
