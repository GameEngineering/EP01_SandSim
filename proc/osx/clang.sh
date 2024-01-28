#!bin/sh

rm -rf bin
mkdir bin
cd bin

proj_root_dir=$(pwd)/../

flags=(
	-std=c99 -ObjC
)

# Include directories
inc=(
	-I ../third_party/include/
	-I ../include/				
)

# Source files
src=(
	../source/main.c
	../source/render_passes/*.c
)

fworks=(
	-framework OpenGL
	-framework CoreFoundation 
	-framework CoreVideo 
	-framework IOKit 
	-framework Cocoa 
	-framework Carbon

)

clang -O3 ${fworks[*]} ${inc[*]} ${src[*]} ${flags[*]} -o SandSim

cd ..



