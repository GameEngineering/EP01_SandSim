#!bin/sh

rm -rf bin
mkdir bin
cd bin

proj_name=App
proj_root_dir=$(pwd)/../

flags=(
    -w -s WASM=1 -s USE_WEBGL2=1 -s ASYNCIFY=1 -O3 -s ALLOW_MEMORY_GROWTH=1
)

# Include directories
inc=(
    -I ../third_party/include/           # Gunslinger includes
    -I ../include/
)

# Source files
src=(
    ../source/*.c
    ../source/render_passes/*.c
)

libs=(
)

# Build
emcc ${inc[*]} ${src[*]} ${flags[*]} -o $proj_name.html

cd ..



