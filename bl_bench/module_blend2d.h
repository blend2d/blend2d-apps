// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_MODULE_BLEND2D_H
#define BLBENCH_MODULE_BLEND2D_H

#include "module.h"

namespace blbench {

BenchModule* createBlend2DModule(uint32_t threadCount = 0, uint32_t cpuFeatures = 0);

} // {blbench}

#endif // BLBENCH_MODULE_BLEND2D_H
