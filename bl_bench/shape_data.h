// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_SHAPE_DATA_H
#define BLBENCH_SHAPE_DATA_H

#include <blend2d.h>

namespace blbench {

enum class ShapeKind {
  kButterfly,
  kFish,
  kDragon,
  kWorld,
  kMaxValue = kWorld
};

struct ShapeData {
  size_t size;
  const char* commands;
  const BLPoint* vertices;
};

bool getShapeData(ShapeData& dst, ShapeKind kind);

class ShapeIterator {
public:
  size_t remaining;
  const char* commands;
  const BLPoint* vertices;

  explicit inline ShapeIterator(ShapeData data) noexcept {
    remaining = data.size;
    commands = data.commands;
    vertices = data.vertices;
  }

  inline bool hasCommand() const noexcept {
    return remaining != 0;
  }

  inline void next() noexcept {
    char command = commands[0];

    remaining--;
    commands++;

    switch (command) {
      case 'M': vertices += 1; break;
      case 'L': vertices += 1; break;
      case 'Q': vertices += 2; break;
      case 'C': vertices += 3; break;
      default:
        break;
    }
  }

  inline bool isClose() const noexcept { return commands[0] == 'Z'; }
  inline bool isMoveTo() const noexcept { return commands[0] == 'M'; }
  inline bool isLineTo() const noexcept { return commands[0] == 'L'; }
  inline bool isQuadTo() const noexcept { return commands[0] == 'Q'; }
  inline bool isCubicTo() const noexcept { return commands[0] == 'C'; }

  template<typename Index>
  inline BLPoint vertex(const Index& i) const noexcept { return vertices[i]; }

  template<typename Index>
  inline double x(const Index& i) const noexcept { return vertices[i].x; }

  template<typename Index>
  inline double y(const Index& i) const noexcept { return vertices[i].y; }
};

} // {blbench}

#endif // BLBENCH_SHAPE_DATA_H
