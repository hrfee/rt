# Structure

* `src/`: `.cpp` files.
* `main/`: Main executable and source.
* `include/`: `.hpp` files.
* `maps/`: Scene and model files.
* `scrots/`: Screenshots & renders.
* `objparser/`: Contains the objparser script for convert wavefront OBJ files to the internal format.

# Build

Compilation has only been completed on Linux, modification to build files likely required to use non-system GLFW.

Submodules are used, ensure they are available:
```
$ git submodule update --init --recursive
```
Additionally, ensure CMake, OpenGL and GLFW are available on the system.
```sh
$ cmake -S . -B build # generate build files
$ cmake --build build -j $(nproc) # compile
$ cmake --build build --target clean # clean
```
# Targets

* `main/main`: Main ray-tracer application.
* `timings/timings`: Program to comparing triangle/sphere intersection performance.
