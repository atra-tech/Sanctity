# Sanctity: Atra Engine Testbed

A small 3D project that can hardly be called a game yet. Simply used as a testing ground for features to either be integrated into the Atra Engine, or related game tools to be packaged in their own libraries.

The example application gives you nine copies of the same chunk of perlin noise based terrain heightmap. Each chunk also contains an example of floor and wall tiles at its centre. The character controller exemplifies the very simple cylinder and axis-aligned bounding box physics system. Each chunk also contains craters, part of a test for dynamic terrain heightmap deformation at runtime, combined with a smooth material change over the generated crater's area.

## Dependencies

* An OpenGL 3.3 compatible graphics adapter and driver
* SDL2
* SDL2\_image
* GLEW
* CGLM (provided with source)

## Compiling

#### Linux
Run `make` in the project's root directory to compile the application.
The resulting executable is `bin/sanctity`.
Executable must be run from the project's root directory to be able to find resources.

#### Windows
Windows compilation is not functional.

## Controls
- W, A, S and D to move.
