# EP01_SandSim

<p align="left">
  <img width="460" height="300" src="assets/vid_thumb.png">
</p>

https://www.youtube.com/watch?v=VLZjd_Y1gJ8

Inspired by Noita, this is a simple "falling sand" simulation to demonstrate the mechanics and ideas behind Cellular Automata. 

## Building

**NOTE(john)**: Currently requires at least **OpenGL v3.3** to run. This will be addressed in the future to allow for 
          previous opengl versions. 

There are multiple examples provided to show how to get up and running. For each of these examples: 
  - `cd` into the root directory for your example.
  - **windows**:
    - You'll need to have Visual Studio 2015 or greater.
    - From start menu, search for "x64 Native Tool Command Prompt for {Insert your Version Here}"
    - Navigate to where you have `EP01_SandSim` repo placed
    - run `proc\win\compile_win_cl.bat`
    - The executable will be placed in `bin\`
    - run `bin\SandSim.exe`
  - **mac**:
    - You'll need gcc
    - run `bash ./proc/osx/compile_osx_gcc.sh`
    - The exectuable will be placed in `bin/`
    - run `./bin/SandSim`
  - **linux**: 
    - You'll need gcc
    - run `bash ./proc/linux/compile_linux_gcc.sh`
    - The exectuable will be placed in `bin/`
    - run `./bin/SandSim`
    
