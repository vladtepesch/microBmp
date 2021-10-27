# microBmp - Library

**MI**cro **C**ontroller **R**ead **O**nly **BMP** Library

A minimal library designed for reading bmp files without using dynamically allocated memory.

It works on a externally provided memory and uses a callback to get more image data 
to be independent of file systems or loaded data.

## design choices

 - no memory allocation
 - no os dependencies
 - the image is loaded row wise and not has to fit into memory at once
 - loading of data is handled to the user via callback
 - also supports working on fully loaded memory (without calling the user back)
 - user has to provide some memory block that serves as cache and that at least have must be 
   large enough to hold one image line and the palette (if palette based)

## currently supported format features

 - Indexed images 4bit 8bit  with arbitrary number of palette entries
 - 16bit images (like RGB565 or RGB555) with or without bitmasks (compression 3)
 - 24bit RGB images (accepts compression 3 if bit pattern is the standard one)
 - 32bit RGB images (accepts compression 3 if bit pattern is the standard one)

## currently missing features and drawbacks
 
 - no compression supported (besides compression 3 that really is not any compression but mapping of bits to color)
 - monochrome image
 - currently only supports mirrored images (bmp's default case), negative heights (top down) are (currently) not supported
 - only RGB is currently supported as output format
 - efficiency of data conversion currently not very high


## references

 - https://en.wikipedia.org/wiki/BMP_file_format
 - https://de.wikipedia.org/wiki/Windows_Bitmap
 - https://www.fileformat.info/format/bmp/egff.htm
 - https://github.com/Spoffy/BMPedded
 - https://github.com/madwyn/qdbmp
 - https://github.com/vallentin/LoadBMP