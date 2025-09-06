// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib


#ifndef BLBENCH_CMDLINE_H_INCLUDED
#define BLBENCH_CMDLINE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace blbench {

class CmdLine {
public:
  int _argc;
  const char* const* _argv;

  CmdLine(int argc, const char* const* argv)
    : _argc(argc),
      _argv(argv) {}

  bool has_arg(const char* key) const {
    for (int i = 1; i < _argc; i++)
      if (strcmp(key, _argv[i]) == 0)
        return true;
    return false;
  }

  const char* value_of(const char* key, const char* default_value) const {
    size_t keySize = strlen(key);
    for (int i = 1; i < _argc; i++) {
      const char* val = _argv[i];
      if (strlen(val) >= keySize + 1 && val[keySize] == '=' && memcmp(val, key, keySize) == 0)
        return val + keySize + 1;
    }

    return default_value;
  }

  int value_as_int(const char* key, int default_value) const {
    const char* val = value_of(key, nullptr);
    if (val == nullptr || val[0] == '\0')
      return default_value;

    return atoi(val);
  }

  unsigned value_as_uint(const char* key, unsigned default_value) const {
    const char* val = value_of(key, nullptr);
    if (val == nullptr || val[0] == '\0')
      return default_value;

    int v = atoi(val);
    if (v < 0)
      return default_value;
    else
      return unsigned(v);
  }
};

} // {blbench}

#endif // BLBENCH_CMDLINE_H_INCLUDED
