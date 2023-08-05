// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_SHAPES_DATA_H
#define BLBENCH_SHAPES_DATA_H

#include "./module.h"

namespace blbench {

struct ShapesData {
  enum Id {
    kIdWorld = 0,
    kIdCount
  };

  const BLPoint* data;
  size_t count;
};

bool getShapesData(ShapesData& dst, uint32_t id);

} // {blbench}

#endif // BLBENCH_SHAPES_DATA_H
