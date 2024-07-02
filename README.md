# Summed Area Font
OpenGL subpixel font render using summed area for scaling

# Implementation

This is a real-time font render that uses integral image to create high quality summed area scaling in the fragment shader. The method uses [Dual Source Blending](https://www.khronos.org/opengl/wiki_opengl/index.php?title=Dual_Source_Blending&redirect=no) and the fragment shader supports both subpixel and regular anti-aliasing. The fonts can be rendered at subpixel positions at any scale less that ~25% of font texture.

A high resolution font texture is generated using stb_truetype.h at startup. This texture is converted to an unsigned int integral texture. The fragment shader uses fetchTexel to calculate coverage of pixels. The method can be extended to support linear interpolation if subtexel precision is needed, but would require four times the number of fetchTexel. This is only necessary if the font resolution is close to the resolution of the font texture.

Summed area table or Integral image was introduced to computer graphics in 1984 by Frank Crow for use with mipmaps. It was used within the Viola–Jones object detection framework in 2001. https://en.wikipedia.org/wiki/Summed-area_table

# Build

## Prerequisites Linux
GLFW

## Build command on Linux
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Build command on Windows
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..
$ Open summedareafont.sln project in Visual Studio 17 2022 and build
```
# Screenshot

<img width="643" alt="Screenshot-SAF" src="https://github.com/DavidGuldbrandsen/SummedAreaFont/assets/98739117/cb84770a-ca36-4643-bfe2-567b4762eaa5">

# Acknowledgements

The idea to use summed area table for font rendering was create by David Norden Guldbrandsen and Jakob Hunsballe in 2022.
