#include "frames.h"

// frames are defined here and exported in frames.h

// since we only have 13 columns, the 3 least significant digits in each row are ignored

const uint8_t xBitmap[] = {
  0x80, 0x08,  // #...........#
  0x20, 0x20,  // ..#.......#..
  0x10, 0x40,  // ...#.....#...
  0x05, 0x00,  // .....#.#.....
  0x02, 0x00,  // ......#......
  0x08, 0x80,  // ....#...#....
  0x10, 0x40,  // ...#.....#...
  0x40, 0x10,  // .#.........#.
  0x80, 0x08   // #...........#
};
MonoFrame xFrame = { .bitmap = xBitmap, .colorR = 255, .colorG = 0, .colorB = 0 };

const uint8_t circleBitmap[] = {
  0x07, 0x00,  // .....###.....
  0x10, 0x40,  // ...#.....#...
  0x40, 0x10,  // .#.........#.
  0x80, 0x08,  // #...........#
  0x80, 0x08,  // #...........#
  0x80, 0x08,  // #...........#
  0x40, 0x10,  // .#.........#.
  0x10, 0x40,  // ...#.....#...
  0x07, 0x00   // .....###.....
};
MonoFrame circleFrame = { .bitmap = circleBitmap, .colorR = 0, .colorG = 255, .colorB = 0 };
