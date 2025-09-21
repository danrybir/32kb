# 32kb

a little 2d top-down platformer game thing that, when compiled, stays below 32 kilobytes. _(at least, on linux arm64)_

## build

`make main`. that's basically it

### dependencies

- libc i suppose
- make??
- clang + lld
- upx (for compression)
- sdl2

## run

`./32kb [seed]`, or `./32kbupx [seed]`, both are identical (difference being `32kbupx` being compressed with upx)

## controls

- arrows move
- u/h/j/k aim the cursor
- space places/removes block..s?
- escape quits

## featues

- simple procedural terrain
- somewhat working collisions
- block placement and unplacement
- hashmap, apparently
- rgb332 for tiles

### this is optimized for size, not stability. expect weird stuff to happen

thx for checking this out :3c