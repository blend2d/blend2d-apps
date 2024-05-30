// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include <stdio.h>
#include <string.h>
#include <type_traits>

#include "./app.h"
#include "./images_data.h"
#include "./module_blend2d.h"

#if defined(BLEND2D_APPS_ENABLE_AGG)
  #include "./module_agg.h"
#endif // BLEND2D_APPS_ENABLE_AGG

#if defined(BLEND2D_APPS_ENABLE_CAIRO)
  #include "./module_cairo.h"
#endif // BLEND2D_APPS_ENABLE_CAIRO

#if defined(BLEND2D_APPS_ENABLE_QT)
  #include "./module_qt.h"
#endif // BLEND2D_APPS_ENABLE_QT

#if defined(BLEND2D_APPS_ENABLE_SKIA)
  #include "./module_skia.h"
#endif // BLEND2D_APPS_ENABLE_SKIA

#define ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))

namespace blbench {

static const char* benchIdNameList[] = {
  "FillRectA",
  "FillRectU",
  "FillRectRot",
  "FillRoundU",
  "FillRoundRot",
  "FillTriangle",
  "FillPolyNZi10",
  "FillPolyEOi10",
  "FillPolyNZi20",
  "FillPolyEOi20",
  "FillPolyNZi40",
  "FillPolyEOi40",
  "FillWorld",
  "StrokeRectA",
  "StrokeRectU",
  "StrokeRectRot",
  "StrokeRoundU",
  "StrokeRoundRot",
  "StrokeTriangle",
  "StrokePoly10",
  "StrokePoly20",
  "StrokePoly40",
  "StrokeWorld"
};

static const char* benchCompOpList[] = {
  "SrcOver",
  "SrcCopy",
  "SrcIn",
  "SrcOut",
  "SrcAtop",
  "DstOver",
  "DstCopy",
  "DstIn",
  "DstOut",
  "DstAtop",
  "Xor",
  "Clear",
  "Plus",
  "Minus",
  "Modulate",
  "Multiply",
  "Screen",
  "Overlay",
  "Darken",
  "Lighten",
  "ColorDodge",
  "ColorBurn",
  "LinearBurn",
  "LinearLight",
  "PinLight",
  "HardLight",
  "SoftLight",
  "Difference",
  "Exclusion"
};

static const char* benchStyleModeList[] = {
  "Solid",
  "Linear@Pad",
  "Linear@Repeat",
  "Linear@Reflect",
  "Radial@Pad",
  "Radial@Repeat",
  "Radial@Reflect",
  "Conic",
  "Pattern_NN",
  "Pattern_BI"
};

static const int benchShapeSizeList[] = {
  8, 16, 32, 64, 128, 256
};

static constexpr uint32_t kBenchShapeSizeCount = uint32_t(ARRAY_SIZE(benchShapeSizeList));

const char benchBorderStr[] = "+--------------------+-------------+---------------+----------+----------+----------+----------+----------+----------+\n";
const char benchHeaderStr[] = "|%-20s"             "| CompOp      | Style         | 8x8      | 16x16    | 32x32    | 64x64    | 128x128  | 256x256  |\n";
const char benchDataFmt[]   = "|%-20s"             "| %-12s"     "| %-14s"       "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "|\n";

static const char* getCpuArchString() {
#if defined(_M_X64) || defined(__amd64) || defined(__amd64__) || defined(__x86_64) || defined(__x86_64__)
  return "x86_64";
#elif defined(_M_IX86) || defined(__i386) || defined(__i386__)
  return "x86";
#elif defined(_M_ARM64) || defined(__ARM64__) || defined(__aarch64__)
  return "aarch64";
#elif defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__) || defined(__thumb2__)
  return "aarch32";
#elif defined(_MIPS_ARCH_MIPS64) || defined(__mips64)
  return "mips64";
#elif defined(_MIPS_ARCH_MIPS32) || defined(_M_MRX000) || defined(__mips) || defined(__mips__)
  return "mips32";
#elif defined(__riscv) || defined(__riscv__)
  return sizeof(void*) >= 8 ? "riscv64" : "riscv32";
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
  return "ppc64";
#elif defined(_LOONGARCH_ARCH) || defined(__loongarch_arch) || defined(__loongarch64)
  return "la64";
#else
  return "unknown";
#endif
}

static const char* getFormatString(BLFormat format) {
  switch (format) {
    case BL_FORMAT_PRGB32: return "prgb32";
    case BL_FORMAT_XRGB32: return "xrgb32";
    case BL_FORMAT_A8    : return "a8";
    default:
      return "unknown";
  }
}

static uint32_t searchStringList(const char** listData, size_t listSize, const char* key) {
  for (size_t i = 0; i < listSize; i++)
    if (strcmp(listData[i], key) == 0)
      return uint32_t(i);
  return 0xFFFFFFFFu;
}

struct DurationFormat {
  char data[64];

  inline void format(double cpms) {
    if (cpms < 10)
      snprintf(data, 64, "%0.2f", cpms);
    if (cpms < 100)
      snprintf(data, 64, "%0.1f", cpms);
    else
      snprintf(data, 64, "%llu", (unsigned long long)std::round(cpms));
  }
};

BenchApp::BenchApp(int argc, char** argv)
  : _argc(argc),
    _argv(argv),
    _isolated(false),
    _deepBench(false),
    _saveImages(false),
    _repeat(1),
    _quantity(0) {}

BenchApp::~BenchApp() {}

bool BenchApp::hasArg(const char* key) const {
  int argc = _argc;
  char** argv = _argv;

  for (int i = 1; i < argc; i++) {
    if (strcmp(key, argv[i]) == 0)
      return true;
  }

  return false;
}

const char* BenchApp::valueOf(const char* key) const {
  int argc = _argc;
  char** argv = _argv;

  size_t keySize = strlen(key);
  for (int i = 1; i < argc; i++) {
    const char* val = argv[i];
    if (strlen(val) >= keySize + 1 && val[keySize] == '=' && memcmp(val, key, keySize) == 0)
      return val + keySize + 1;
  }

  return nullptr;
}

int BenchApp::intValueOf(const char* key, int defaultValue) const {
  int argc = _argc;
  char** argv = _argv;

  size_t keySize = strlen(key);
  for (int i = 1; i < argc; i++) {
    const char* val = argv[i];
    if (strlen(val) >= keySize + 1 && val[keySize] == '=' && memcmp(val, key, keySize) == 0) {
      const char* s = val + keySize + 1;
      return atoi(s);
    }
  }

  return defaultValue;
}

bool BenchApp::init() {
  _isolated = hasArg("--isolated");
  _deepBench = hasArg("--deep");
  _saveImages = hasArg("--save");
  _compOp = 0xFFFFFFFFu;
  _compOpString = nullptr;
  _repeat = intValueOf("--repeat", 1);
  _quantity = intValueOf("--quantity", 0);

  _disableAgg = hasArg("--disable-agg");
  _disableBlend2D = hasArg("--disable-blend2d");
  _disableCairo = hasArg("--disable-cairo");
  _disableQt = hasArg("--disable-qt");
  _disableSkia = hasArg("--disable-skia");

  if (_repeat <= 0 || _repeat > 100) {
    printf("ERROR: Invalid repeat [%d] specified\n", _repeat);
    return false;
  }

  if (_quantity > 100000u) {
    printf("ERROR: Invalid quantity [%d] specified\n", _quantity);
    return false;
  }

  if (_saveImages && !_quantity) {
    printf("ERROR: Missing --quantity argument; it must be provided when --save is enabled\n");
    return false;
  }

  _compOpString = valueOf("--compOp");
  if (_compOpString != NULL)
    _compOp = searchStringList(benchCompOpList, ARRAY_SIZE(benchCompOpList), _compOpString);

  info();

  return readImage(_spriteData[0], "#0", _resource_babelfish_png, sizeof(_resource_babelfish_png)) &&
         readImage(_spriteData[1], "#1", _resource_ksplash_png  , sizeof(_resource_ksplash_png  )) &&
         readImage(_spriteData[2], "#2", _resource_ktip_png     , sizeof(_resource_ktip_png     )) &&
         readImage(_spriteData[3], "#3", _resource_firewall_png , sizeof(_resource_firewall_png ));
}

void BenchApp::info() {
  BLRuntimeBuildInfo buildInfo;
  BLRuntime::queryBuildInfo(&buildInfo);

  const char no_yes[][4] = { "no", "yes" };

  printf(
    "Blend2D Benchmarking Tool\n"
    "\n"
    "Blend2D Information:\n"
    "  Version    : %u.%u.%u\n"
    "  Build Type : %s\n"
    "  Compiled By: %s\n",
    buildInfo.majorVersion,
    buildInfo.minorVersion,
    buildInfo.patchVersion,
    buildInfo.buildType == BL_RUNTIME_BUILD_TYPE_DEBUG ? "Debug" : "Release",
    buildInfo.compilerInfo);

  printf(
    "\n"
    "The following options are supported/used:\n"
    "  --save       [%s] Save all generated images as .png files\n"
    "  --deep       [%s] More tests that use gradients and textures\n"
    "  --isolated   [%s] Use Blend2D isolated context (useful for development)\n"
    "  --repeat=N   [%d] Number of repeats of each test to select the best time\n"
    "  --quantity=N [%d] Override the default quantity of each operation\n"
    "  --comp-op=X  [%s] Benchmark a specific composition operator\n"
    "\n",
    no_yes[_deepBench],
    no_yes[_saveImages],
    no_yes[_isolated],
    _repeat,
    _quantity,
    _compOpString ? _compOpString : "");
}

bool BenchApp::readImage(BLImage& image, const char* name, const void* data, size_t size) noexcept {
  BLResult result = image.readFromData(data, size);
  if (result != BL_SUCCESS) {
    printf("Failed to read an image '%s' used for benchmarking\n", name);
    return false;
  }
  else {
    return true;
  }
}

BLImage BenchApp::getScaledSprite(uint32_t id, uint32_t size) const {
  auto it = _scaledSprites.find(size);
  if (it != _scaledSprites.end()) {
    return it->second[id];
  }

  SpriteData scaled;
  for (uint32_t i = 0; i < kBenchNumSprites; i++) {
    BLImage::scale(
      scaled[i],
      _spriteData[i],
      BLSizeI(size, size), BL_IMAGE_SCALE_FILTER_BILINEAR);
  }
  _scaledSprites.emplace(size, scaled);
  return scaled[id];
}

bool BenchApp::isStyleEnabled(uint32_t style) {
  if (_deepBench)
    return true;

  // If this is not a deep run we just select the following styles to be tested:
  return style == kBenchStyleSolid     ||
         style == kBenchStyleLinearPad ||
         style == kBenchStyleRadialPad ||
         style == kBenchStyleConic     ||
         style == kBenchStylePatternNN ||
         style == kBenchStylePatternBI ;
}

void BenchApp::serializeSystemInfo(JSONBuilder& json) const {
  BLRuntimeSystemInfo systemInfo;
  BLRuntime::querySystemInfo(&systemInfo);

  json.beforeRecord().addKey("cpu").openObject();
  json.beforeRecord().addKey("arch").addString(getCpuArchString());
  json.beforeRecord().addKey("vendor").addString(systemInfo.cpuVendor);
  json.beforeRecord().addKey("brand").addString(systemInfo.cpuBrand);
  json.closeObject(true);
}

void BenchApp::serializeParams(JSONBuilder& json, const BenchParams& params) const {
  json.beforeRecord().addKey("screen").openObject();
  json.beforeRecord().addKey("width").addUInt(params.screenW);
  json.beforeRecord().addKey("height").addUInt(params.screenH);
  json.beforeRecord().addKey("format").addString(getFormatString(params.format));
  json.closeObject(true);
}

void BenchApp::serializeOptions(JSONBuilder& json, const BenchParams& params) const {
  json.beforeRecord().addKey("options").openObject();
  json.beforeRecord().addKey("quantity").addUInt(params.quantity);
  json.beforeRecord().addKey("sizes").openArray();
  for (uint32_t sizeId = 0; sizeId < kBenchShapeSizeCount; sizeId++) {
    json.addStringf("%dx%d", benchShapeSizeList[sizeId], benchShapeSizeList[sizeId]);
  }
  json.closeArray();
  json.beforeRecord().addKey("repeat").addUInt(_repeat);
  json.closeObject(true);
}

int BenchApp::run() {
  BenchParams params{};
  params.screenW = 600;
  params.screenH = 512;
  params.format = BL_FORMAT_PRGB32;
  params.strokeWidth = 2.0;

  BLString jsonContent;
  JSONBuilder json(&jsonContent);

  json.openObject();

  serializeSystemInfo(json);
  serializeParams(json, params);
  serializeOptions(json, params);

  json.beforeRecord().addKey("runs").openArray();

  if (_isolated) {
    BLRuntimeSystemInfo si;
    BLRuntime::querySystemInfo(&si);

    // Only use features that could actually make a difference.
    static const uint32_t x86Features[] = {
      BL_RUNTIME_CPU_FEATURE_X86_SSE2,
      BL_RUNTIME_CPU_FEATURE_X86_SSE3,
      BL_RUNTIME_CPU_FEATURE_X86_SSSE3,
      BL_RUNTIME_CPU_FEATURE_X86_SSE4_1,
      BL_RUNTIME_CPU_FEATURE_X86_SSE4_2,
      BL_RUNTIME_CPU_FEATURE_X86_AVX,
      BL_RUNTIME_CPU_FEATURE_X86_AVX2,
      BL_RUNTIME_CPU_FEATURE_X86_AVX512
    };

    const uint32_t* features = x86Features;
    uint32_t featureCount = ARRAY_SIZE(x86Features);

    for (uint32_t i = 0; i < featureCount; i++) {
      if ((si.cpuFeatures & features[i]) == features[i]) {
        BenchModule* mod = createBlend2DModule(0, features[i]);
        runModule(*mod, params, json);
        delete mod;
      }
    }
  }
  else {
    BenchModule* mod = nullptr;

    if (!_disableBlend2D) {
      mod = createBlend2DModule(0);
      runModule(*mod, params, json);
      delete mod;

      mod = createBlend2DModule(2);
      runModule(*mod, params, json);
      delete mod;

      mod = createBlend2DModule(4);
      runModule(*mod, params, json);
      delete mod;
    }

    // mod = createBlend2DModule(0, 0xFFFFFFFFu);
    // runModule(*mod, params);
    // delete mod;

#if defined(BLEND2D_APPS_ENABLE_AGG)
    if (!_disableAgg) {
      mod = createAggModule();
      runModule(*mod, params, json);
      delete mod;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_CAIRO)
    if (!_disableCairo) {
      mod = createCairoModule();
      runModule(*mod, params, json);
      delete mod;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_QT)
    if (!_disableQt) {
      mod = createQtModule();
      runModule(*mod, params, json);
      delete mod;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_SKIA)
    if (!_disableSkia) {
      mod = createSkiaModule();
      runModule(*mod, params, json);
      delete mod;
    }
#endif
  }

  json.closeArray(true);
  json.closeObject(true);
  json.nl();

  printf("\n");
  fputs(jsonContent.data(), stdout);

  return 0;
}

int BenchApp::runModule(BenchModule& mod, BenchParams& params, JSONBuilder& json) {
  char fileName[256];
  char styleString[128];

  double cpms[kBenchShapeSizeCount] {};
  uint64_t cpmsTotal[kBenchShapeSizeCount] {};
  DurationFormat fmt[kBenchShapeSizeCount] {};

  uint32_t compOpFirst = BL_COMP_OP_SRC_OVER;
  uint32_t compOpLast  = BL_COMP_OP_SRC_COPY;

  if (_compOp != 0xFFFFFFFFu) {
    compOpFirst = compOpLast = _compOp;
  }

  json.beforeRecord().openObject();
  json.beforeRecord().addKey("name").addString(mod.name());
  mod.serializeInfo(json);
  json.beforeRecord().addKey("records").openArray();

  for (uint32_t compOp = compOpFirst; compOp <= compOpLast; compOp++) {
    params.compOp = BLCompOp(compOp);
    if (!mod.supportsCompOp(params.compOp))
      continue;

    for (uint32_t style = 0; style < kBenchStyleCount; style++) {
      if (!isStyleEnabled(style) || !mod.supportsStyle(style))
        continue;
      params.style = style;

      // Remove '@' from the style name if not running a deep benchmark.
      strcpy(styleString, benchStyleModeList[style]);

      if (!_deepBench) {
        char* x = strchr(styleString, '@');
        if (x != NULL) x[0] = '\0';
      }

      memset(cpmsTotal, 0, sizeof(cpmsTotal));

      printf(benchBorderStr);
      printf(benchHeaderStr, mod._name);
      printf(benchBorderStr);

      for (uint32_t testId = 0; testId < kBenchIdCount; testId++) {
        params.benchId = testId;

        for (uint32_t sizeId = 0; sizeId < kBenchShapeSizeCount; sizeId++) {
          params.shapeSize = benchShapeSizeList[sizeId];
          uint64_t duration = runSingleTest(mod, params);

          cpms[sizeId] = double(params.quantity) * double(1000) / double(duration);
          cpmsTotal[sizeId] += cpms[sizeId];

          if (_saveImages) {
            // Save only the last two as these are easier to compare visually.
            if (sizeId >= kBenchShapeSizeCount - 2) {
              snprintf(fileName, 256, "%s-%s-%s-%s-%c.png",
                mod._name,
                benchIdNameList[params.benchId],
                benchCompOpList[params.compOp],
                styleString,
                'A' + sizeId);
              mod._surface.writeToFile(fileName);
            }
          }
        }

        for (uint32_t sizeId = 0; sizeId < kBenchShapeSizeCount; sizeId++)
          fmt[sizeId].format(cpms[sizeId]);

        printf(benchDataFmt,
          benchIdNameList[params.benchId],
          benchCompOpList[params.compOp],
          styleString,
          fmt[0].data,
          fmt[1].data,
          fmt[2].data,
          fmt[3].data,
          fmt[4].data,
          fmt[5].data);

        json.beforeRecord()
            .openObject()
            .addKey("test").addString(benchIdNameList[params.benchId])
            .comma().alignTo(36).addKey("compOp").addString(benchCompOpList[params.compOp])
            .comma().alignTo(58).addKey("style").addString(styleString);

        json.addKey("rcpms").openArray();
        for (uint32_t sizeId = 0; sizeId < kBenchShapeSizeCount; sizeId++) {
          json.addStringWithoutQuotes(fmt[sizeId].data);
        }
        json.closeArray();

        json.closeObject();
      }

      for (uint32_t sizeId = 0; sizeId < kBenchShapeSizeCount; sizeId++)
        fmt[sizeId].format(cpmsTotal[sizeId]);

      printf(benchBorderStr);
      printf(benchDataFmt,
        "Total",
        benchCompOpList[params.compOp],
        styleString,
        fmt[0].data,
        fmt[1].data,
        fmt[2].data,
        fmt[3].data,
        fmt[4].data,
        fmt[5].data);
      printf(benchBorderStr);
      printf("\n");
    }
  }

  json.closeArray(true);
  json.closeObject(true);

  return 0;
}

uint64_t BenchApp::runSingleTest(BenchModule& mod, BenchParams& params) {
  constexpr uint32_t kInitialQuantity = 25;
  constexpr uint32_t kMinimumDurationUS = 1000;
  constexpr uint32_t kMaxRepeatIfNoImprovement = 10;

  uint32_t attempt = 0;
  uint64_t duration = std::numeric_limits<uint64_t>::max();
  uint32_t noImprovement = 0;

  params.quantity = _quantity;

  if (_quantity == 0u) {
    // If quantity is zero it means to deduce it based on execution time of each test.
    params.quantity = kInitialQuantity;
    for (;;) {
      mod.run(*this, params);
      if (mod._duration >= kMinimumDurationUS) {
        // Make this the first attempt to reduce the time of benchmarking.
        attempt = 1;
        duration = mod._duration;
        break;
      }

      if (mod._duration < 100)
        params.quantity *= 10;
      else if (mod._duration < 500)
        params.quantity *= 3;
      else
        params.quantity *= 2;
    }
  }

  while (attempt < _repeat) {
    mod.run(*this, params);

    if (duration > mod._duration)
      duration = mod._duration;
    else
      noImprovement++;

    if (noImprovement >= kMaxRepeatIfNoImprovement)
      break;

    attempt++;
  }

  return duration;
}

} // {blbench}

int main(int argc, char* argv[]) {
  blbench::BenchApp app(argc, argv);

  if (!app.init()) {
    printf("Failed to initialize bl_bench.\n");
    return 1;
  }

  return app.run();
}
