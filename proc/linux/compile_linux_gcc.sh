#!bin/sh

rm -rf bin
mkdir bin
cd bin

proj_root_dir=$(pwd)/../

flags=(
	-std=gnu99 -Wl,--no-as-needed -ldl -lGL -lX11 -pthread -lXi
)

# Include directories
inc=(
	-I ../include/							# SandSim includes
	-I ../third_party/include/
)

# Source files
src=(
	../source/main.c
	../source/render_passes/*.c
)

# Build
gcc -O3 ${inc[*]} ${src[*]} ${flags[*]} -lm -o SandSim

cd ..


