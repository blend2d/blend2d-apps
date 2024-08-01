// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef BLBENCH_BACKEND_BLEND2D_H
#define BLBENCH_BACKEND_BLEND2D_H

#include "backend.h"

namespace blbench {

Backend* createBlend2DBackend(uint32_t threadCount = 0, uint32_t cpuFeatures = 0);

} // {blbench}

#endif // BLBENCH_BACKEND_BLEND2D_H
