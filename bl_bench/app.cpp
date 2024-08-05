// This file is part of Blend2D project <https://blend2d.com>
//
// See LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include <stdio.h>
#include <string.h>

#include <cmath>
#include <limits>
#include <type_traits>
#include <tuple>

#include "app.h"
#include "images_data.h"
#include "backend_blend2d.h"

#if defined(BLEND2D_APPS_ENABLE_AGG)
  #include "backend_agg.h"
#endif // BLEND2D_APPS_ENABLE_AGG

#if defined(BLEND2D_APPS_ENABLE_CAIRO)
  #include "backend_cairo.h"
#endif // BLEND2D_APPS_ENABLE_CAIRO

#if defined(BLEND2D_APPS_ENABLE_QT)
  #include "backend_qt.h"
#endif // BLEND2D_APPS_ENABLE_QT

#if defined(BLEND2D_APPS_ENABLE_SKIA)
  #include "backend_skia.h"
#endif // BLEND2D_APPS_ENABLE_SKIA

#if defined(BLEND2D_APPS_ENABLE_COREGRAPHICS)
  #include "backend_coregraphics.h"
#endif // BLEND2D_APPS_ENABLE_COREGRAPHICS

#if defined(BLEND2D_APPS_ENABLE_JUCE)
  #include "backend_juce.h"
#endif // BLEND2D_APPS_ENABLE_JUCE

#define ARRAY_SIZE(X) uint32_t(sizeof(X) / sizeof(X[0]))

namespace blbench {

static constexpr uint32_t kSupportedBackends =
#if defined(BLEND2D_APPS_ENABLE_AGG)
  (1u << uint32_t(BackendKind::kAGG)) |
#endif
#if defined(BLEND2D_APPS_ENABLE_CAIRO)
  (1u << uint32_t(BackendKind::kCairo)) |
#endif
#if defined(BLEND2D_APPS_ENABLE_QT)
  (1u << uint32_t(BackendKind::kQt)) |
#endif
#if defined(BLEND2D_APPS_ENABLE_SKIA)
  (1u << uint32_t(BackendKind::kSkia)) |
#endif
#if defined(BLEND2D_APPS_ENABLE_JUCE)
  (1u << uint32_t(BackendKind::kJUCE)) |
#endif
#if defined(BLEND2D_APPS_ENABLE_COREGRAPHICS)
  (1u << uint32_t(BackendKind::kCoreGraphics)) |
#endif
  (1u << uint32_t(BackendKind::kBlend2D));

static const char* backendKindNameList[] = {
  "Blend2D",
  "AGG",
  "Cairo",
  "Qt",
  "Skia",
  "JUCE",
  "CoreGraphics"
};

static const char* testKindNameList[] = {
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
  "FillButterfly",
  "FillFish",
  "FillDragon",
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
  "StrokeButterfly",
  "StrokeFish",
  "StrokeDragon",
  "StrokeWorld"
};

static const char* compOpNameList[] = {
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

static const char* styleKindNameList[] = {
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

static const int benchShapeSizeList[kBenchShapeSizeCount] = {
  8, 16, 32, 64, 128, 256
};

const char benchBorderStr[] = "+--------------------+-------------+---------------+----------+----------+----------+----------+----------+----------+\n";
const char benchHeaderStr[] = "|%-20s"             "| CompOp      | Style         | 8x8      | 16x16    | 32x32    | 64x64    | 128x128  | 256x256  |\n";
const char benchDataFmt[]   = "|%-20s"             "| %-12s"     "| %-14s"       "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "| %-9s"   "|\n";

static const char* getOSString() {
#if defined(__ANDROID__)
  return "android";
#elif defined(__linux__)
  return "linux";
#elif defined(__APPLE__) && defined(__MACH__)
  return "osx";
#elif defined(__APPLE__)
  return "apple";
#elif defined(__DragonFly__)
  return "dragonflybsd";
#elif defined(__FreeBSD__)
  return "freebsd";
#elif defined(__NetBSD__)
  return "netbsd";
#elif defined(__OpenBSD__)
  return "openbsd";
#elif defined(__HAIKU__)
  return "haiku";
#elif defined(_WIN32)
  return "windows";
#else
  return "unknown";
#endif
}

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
    case BL_FORMAT_PRGB32:
      return "prgb32";

    case BL_FORMAT_XRGB32:
      return "xrgb32";

    case BL_FORMAT_A8:
      return "a8";

    default:
      return "unknown";
  }
}

static bool strieq(const char* a, const char* b) {
  size_t aLen = strlen(a);
  size_t bLen = strlen(b);

  if (aLen != bLen)
    return false;

  for (size_t i = 0; i < aLen; i++) {
    unsigned ac = (unsigned char)a[i];
    unsigned bc = (unsigned char)b[i];

    if (ac >= 'A' && ac <= 'Z') ac += uint32_t('a' - 'A');
    if (bc >= 'A' && bc <= 'Z') bc += uint32_t('a' - 'A');

    if (ac != bc)
      return false;
  }

  return true;
}

static uint32_t searchStringList(const char** listData, size_t listSize, const char* key) {
  for (size_t i = 0; i < listSize; i++)
    if (strieq(listData[i], key))
      return uint32_t(i);
  return 0xFFFFFFFFu;
}

static void spacesToUnderscores(char* s) {
  while (*s) {
    if (*s == ' ')
      *s = '_';
    s++;
  }
}

static BLArray<BLString> splitString(const char* s) {
  BLArray<BLString> arr;
  while (*s) {
    const char* end = strchr(s, ',');
    if (!end) {
      arr.append(BLString(s));
      break;
    }
    else {
      BLString part(BLStringView{s, (size_t)(end - s)});
      arr.append(part);
      s = end + 1;
    }
  }
  return arr;
}

static std::tuple<int, uint32_t> parseList(const char** listData, size_t listSize, const char* inputList, const char* parseErrorMsg) {
  int listOp = 0;
  uint32_t parsedMask = 0;
  BLArray<BLString> parts = splitString(inputList);

  for (const BLString& part : parts) {
    if (part.empty())
      continue;

    const char* p = part.data();
    int partOp = 0;

    if (p[0] == '-') {
      p++;
      partOp = -1;
    }
    else {
      partOp = 1;
    }

    if (listOp == 0) {
      listOp = partOp;
    }
    else if (listOp != partOp) {
      printf("ERROR: %s [%s]: specify either additive or subtractive list, but not both\n", parseErrorMsg, inputList);
      return std::tuple<int, uint32_t>(-2, 0);
    }

    uint32_t backendIdx = searchStringList(listData, listSize, p);
    if (backendIdx == 0xFFFFFFFFu) {
      printf("ERROR: %s [%s]: couldn't recognize '%s' part\n", parseErrorMsg, inputList, p);
      return std::tuple<int, uint32_t>(-2, 0);
    }

    parsedMask |= 1u << backendIdx;
  }

  return std::tuple<int, uint32_t>(listOp, parsedMask);
}

struct DurationFormat {
  char data[64];

  inline void format(double cpms) {
    if (cpms <= 0.1)
      snprintf(data, 64, "%0.4f", cpms);
    else if (cpms <= 1.0)
      snprintf(data, 64, "%0.3f", cpms);
    else if (cpms < 10.0)
      snprintf(data, 64, "%0.2f", cpms);
    else if (cpms < 100.0)
      snprintf(data, 64, "%0.1f", cpms);
    else
      snprintf(data, 64, "%llu", (unsigned long long)std::round(cpms));
  }
};

BenchApp::BenchApp(int argc, char** argv)
  : _cmdLine(argc, argv),
    _isolated(false),
    _deepBench(false),
    _saveImages(false),
    _backends(kSupportedBackends),
    _repeat(1),
    _quantity(0) {}

BenchApp::~BenchApp() {}

void BenchApp::printAppInfo() const {
  BLRuntimeBuildInfo buildInfo;
  BLRuntime::queryBuildInfo(&buildInfo);

  printf(
    "Blend2D Benchmarking Tool\n"
    "\n"
    "Blend2D Information:\n"
    "  Version    : %u.%u.%u\n"
    "  Build Type : %s\n"
    "  Compiled By: %s\n"
    "\n",
    buildInfo.majorVersion,
    buildInfo.minorVersion,
    buildInfo.patchVersion,
    buildInfo.buildType == BL_RUNTIME_BUILD_TYPE_DEBUG ? "Debug" : "Release",
    buildInfo.compilerInfo);

  fflush(stdout);
}

void BenchApp::printOptions() const {
  const char no_yes[][4] = { "no", "yes" };

  printf(
    "The following options are supported / used:\n"
    "  --width=N         [%u] Canvas width to use for rendering\n"
    "  --height=N        [%u] Canvas height to use for rendering\n"
    "  --quantity=N      [%d] Render calls per test (0 = adjust depending on test duration)\n"
    "  --size-count=N    [%u] Number of size iterations (1=8x8 -> 6=8x8..256x256)\n"
    "  --comp-op=<list>  [%s] Benchmark a specific composition operator\n"
    "  --repeat=N        [%d] Number of repeats of each test to select the best time\n"
    "  --backends=<list> [%s] Backends to use (use 'a,b' to select few, '-xxx' to disable)\n"
    "  --save-images     [%s] Save each generated image independently (use with --quantity)\n"
    "  --save-overview   [%s] Save generated images grouped by sizes  (use with --quantity)\n"
    "  --deep            [%s] More tests that use gradients and textures\n"
    "  --isolated        [%s] Use Blend2D isolated context (useful for development only)\n"
    "\n",
    _width,
    _height,
    _quantity,
    _sizeCount,
    _compOp == 0xFFFFFFFF ? "all" : compOpNameList[_compOp],
    _repeat,
    _backends == kSupportedBackends ? "all" : "...",
    no_yes[_saveImages],
    no_yes[_saveOverview],
    no_yes[_deepBench],
    no_yes[_isolated]
  );

  fflush(stdout);
}

void BenchApp::printBackends() const {
  printf("Backends supported (by default all built-in backends are enabled):\n");
  const char disabled_enabled[][16] = { "disabled", "enabled" };

  for (uint32_t backendIdx = 0; backendIdx < kBackendKindCount; backendIdx++) {
    uint32_t backendMask = 1u << backendIdx;

    if (backendMask & kSupportedBackends) {
      printf("  - %-15s [%s]\n",
        backendKindNameList[backendIdx],
        disabled_enabled[(_backends & backendMask) != 0]);
    }
    else {
      printf("  - %-15s [not supported]\n",
        backendKindNameList[backendIdx]);
    }
  }

  printf("\n");
  fflush(stdout);
}

bool BenchApp::parseCommandLine() {
  _width = _cmdLine.valueAsUInt("--width", _width);
  _height = _cmdLine.valueAsUInt("--height", _height);
  _compOp = 0xFFFFFFFFu;
  _sizeCount = _cmdLine.valueAsUInt("--size-count", _sizeCount);
  _quantity = _cmdLine.valueAsUInt("--quantity", _quantity);
  _repeat = _cmdLine.valueAsUInt("--repeat", _repeat);

  _saveImages = _cmdLine.hasArg("--save-images");
  _saveOverview = _cmdLine.hasArg("--save-overview");
  _deepBench = _cmdLine.hasArg("--deep");
  _isolated = _cmdLine.hasArg("--isolated");

  const char* compOpString = _cmdLine.valueOf("--compOp", nullptr);
  const char* backendString = _cmdLine.valueOf("--backend", nullptr);

  if (_width < 10|| _width > 4096) {
    printf("ERROR: Invalid --width=%u specified\n", _width);
    return false;
  }

  if (_height < 10|| _height > 4096) {
    printf("ERROR: Invalid --height=%u specified\n", _height);
    return false;
  }

  if (_sizeCount == 0 || _sizeCount > kBenchShapeSizeCount) {
    printf("ERROR: Invalid --size-count=%u specified\n", _sizeCount);
    return false;
  }

  if (_quantity > 100000u) {
    printf("ERROR: Invalid --quantity=%u specified\n", _quantity);
    return false;
  }

  if (_repeat <= 0 || _repeat > 100) {
    printf("ERROR: Invalid --repeat=%u specified\n", _repeat);
    return false;
  }

  if (_saveImages && !_quantity) {
    printf("ERROR: Missing --quantity argument; it must be provided when --save-images is used\n");
    return false;
  }

  if (_saveOverview && !_quantity) {
    printf("ERROR: Missing --quantity argument; it must be provided when --save-overview is used\n");
    return false;
  }

  if (compOpString && strcmp(compOpString, "all") != 0) {
    _compOp = searchStringList(compOpNameList, ARRAY_SIZE(compOpNameList), compOpString);
    if (_compOp == 0xFFFFFFFF) {
      printf("ERROR: Invalid composition operator [%s] specified\n", compOpString);
      return false;
    }
  }

  if (backendString && strcmp(backendString, "all") != 0) {
    std::tuple<int, uint32_t> v = parseList(backendKindNameList, kBackendKindCount, backendString, "Invalid --backend list");

    if (std::get<0>(v) == -2)
      return false;

    if (std::get<0>(v) == 1)
      _backends = std::get<1>(v);
    else if (std::get<0>(v) == -1)
      _backends &= ~std::get<1>(v);
  }

  return true;
}

bool BenchApp::init() {
  if (_cmdLine.hasArg("--help")) {
    info();
    exit(0);
  }

  if (!parseCommandLine()) {
    info();
    exit(1);
  }

  return readImage(_spriteData[0], "#0", _resource_babelfish_png, sizeof(_resource_babelfish_png)) &&
         readImage(_spriteData[1], "#1", _resource_ksplash_png  , sizeof(_resource_ksplash_png  )) &&
         readImage(_spriteData[2], "#2", _resource_ktip_png     , sizeof(_resource_ktip_png     )) &&
         readImage(_spriteData[3], "#3", _resource_firewall_png , sizeof(_resource_firewall_png ));
}

void BenchApp::info() {
  BLRuntimeBuildInfo buildInfo;
  BLRuntime::queryBuildInfo(&buildInfo);

  printAppInfo();
  printOptions();
  printBackends();
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

bool BenchApp::isBackendEnabled(BackendKind backendKind) const {
  return (_backends & (1u << uint32_t(backendKind))) != 0;
}

bool BenchApp::isStyleEnabled(StyleKind style) const {
  if (_deepBench)
    return true;

  // If this is not a deep run we just select the following styles to be tested:
  return style == StyleKind::kSolid     ||
         style == StyleKind::kLinearPad ||
         style == StyleKind::kRadialPad ||
         style == StyleKind::kConic     ||
         style == StyleKind::kPatternNN ||
         style == StyleKind::kPatternBI ;
}

void BenchApp::serializeSystemInfo(JSONBuilder& json) const {
  BLRuntimeSystemInfo systemInfo;
  BLRuntime::querySystemInfo(&systemInfo);

  json.beforeRecord().addKey("environment").openObject();
  json.beforeRecord().addKey("os").addString(getOSString());
  json.closeObject(true);

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
  for (uint32_t sizeId = 0; sizeId < _sizeCount; sizeId++) {
    json.addStringf("%dx%d", benchShapeSizeList[sizeId], benchShapeSizeList[sizeId]);
  }
  json.closeArray();
  json.beforeRecord().addKey("repeat").addUInt(_repeat);
  json.closeObject(true);
}

int BenchApp::run() {
  BenchParams params{};
  params.screenW = _width;
  params.screenH = _height;
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
        Backend* backend = createBlend2DBackend(0, features[i]);
        runBackendTests(*backend, params, json);
        delete backend;
      }
    }
  }
  else {
    if (isBackendEnabled(BackendKind::kBlend2D)) {
      Backend* backend = backend = createBlend2DBackend(0);
      runBackendTests(*backend, params, json);
      delete backend;

      backend = createBlend2DBackend(2);
      runBackendTests(*backend, params, json);
      delete backend;

      backend = createBlend2DBackend(4);
      runBackendTests(*backend, params, json);
      delete backend;
    }

#if defined(BLEND2D_APPS_ENABLE_AGG)
    if (isBackendEnabled(BackendKind::kAGG)) {
      Backend* backend = createAggBackend();
      runBackendTests(*backend, params, json);
      delete backend;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_CAIRO)
    if (isBackendEnabled(BackendKind::kCairo)) {
      Backend* backend = createCairoBackend();
      runBackendTests(*backend, params, json);
      delete backend;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_QT)
    if (isBackendEnabled(BackendKind::kQt)) {
      Backend* backend = createQtBackend();
      runBackendTests(*backend, params, json);
      delete backend;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_SKIA)
    if (isBackendEnabled(BackendKind::kSkia)) {
      Backend* backend = createSkiaBackend();
      runBackendTests(*backend, params, json);
      delete backend;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_JUCE)
    if (isBackendEnabled(BackendKind::kJUCE)) {
      Backend* backend = createJuceBackend();
      runBackendTests(*backend, params, json);
      delete backend;
    }
#endif

#if defined(BLEND2D_APPS_ENABLE_COREGRAPHICS)
    if (isBackendEnabled(BackendKind::kCoreGraphics)) {
      Backend* backend = createCGBackend();
      runBackendTests(*backend, params, json);
      delete backend;
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

int BenchApp::runBackendTests(Backend& backend, BenchParams& params, JSONBuilder& json) {
  char fileName[256];
  char styleString[128];

  BLImage overviewImage;
  BLContext overviewCtx;

  if (_saveOverview) {
    overviewImage.create(1 + ((_width + 1) * _sizeCount), _height + 2, BL_FORMAT_XRGB32);
    overviewCtx.begin(overviewImage);
  }

  double cpms[kBenchShapeSizeCount] {};
  uint64_t cpmsTotal[kBenchShapeSizeCount] {};
  DurationFormat fmt[kBenchShapeSizeCount] {};

  uint32_t compOpFirst = BL_COMP_OP_SRC_OVER;
  uint32_t compOpLast  = BL_COMP_OP_SRC_COPY;

  if (_compOp != 0xFFFFFFFFu) {
    compOpFirst = compOpLast = _compOp;
  }

  json.beforeRecord().openObject();
  json.beforeRecord().addKey("name").addString(backend.name());
  backend.serializeInfo(json);
  json.beforeRecord().addKey("records").openArray();

  for (uint32_t compOp = compOpFirst; compOp <= compOpLast; compOp++) {
    params.compOp = BLCompOp(compOp);
    if (!backend.supportsCompOp(params.compOp))
      continue;

    for (uint32_t styleIdx = 0; styleIdx < kStyleKindCount; styleIdx++) {
      StyleKind style = StyleKind(styleIdx);
      if (!isStyleEnabled(style) || !backend.supportsStyle(style))
        continue;
      params.style = style;

      // Remove '@' from the style name if not running a deep benchmark.
      strcpy(styleString, styleKindNameList[styleIdx]);

      if (!_deepBench) {
        char* x = strchr(styleString, '@');
        if (x != nullptr) x[0] = '\0';
      }

      memset(cpmsTotal, 0, sizeof(cpmsTotal));

      printf(benchBorderStr);
      printf(benchHeaderStr, backend._name);
      printf(benchBorderStr);

      for (uint32_t testIdx = 0; testIdx < kTestKindCount; testIdx++) {
        params.testKind = TestKind(testIdx);

        if (_saveOverview) {
          overviewCtx.fillAll(BLRgba32(0xFF000000u));
          overviewCtx.strokeRect(BLRect(0.5, 0.5, overviewImage.width() - 1, overviewImage.height() - 1), BLRgba32(0xFFFFFFFF));
        }

        for (uint32_t sizeId = 0; sizeId < _sizeCount; sizeId++) {
          params.shapeSize = benchShapeSizeList[sizeId];
          uint64_t duration = runSingleTest(backend, params);

          cpms[sizeId] = double(params.quantity) * double(1000) / double(duration);
          cpmsTotal[sizeId] += cpms[sizeId];

          if (_saveOverview) {
            overviewCtx.blitImage(BLPointI(1 + (sizeId * (_width + 1)), 1), backend._surface);
            overviewCtx.fillRect(BLRectI(1 + (sizeId * (_width + 1)) + _width, 1, 1, _height), BLRgba32(0xFFFFFFFF));
            if (sizeId == _sizeCount - 1) {
              snprintf(fileName, 256, "%s-%s-%s-%s.png",
                backend._name,
                testKindNameList[uint32_t(params.testKind)],
                compOpNameList[uint32_t(params.compOp)],
                styleString);
              spacesToUnderscores(fileName);
              overviewImage.writeToFile(fileName);
            }
          }

          if (_saveImages) {
            // Save only the last two as these are easier to compare visually.
            if (sizeId >= _sizeCount - 2) {
              snprintf(fileName, 256, "%s-%s-%s-%s-%c.png",
                backend._name,
                testKindNameList[uint32_t(params.testKind)],
                compOpNameList[uint32_t(params.compOp)],
                styleString,
                'A' + sizeId);
              spacesToUnderscores(fileName);
              backend._surface.writeToFile(fileName);
            }
          }
        }

        for (uint32_t sizeId = 0; sizeId < _sizeCount; sizeId++)
          fmt[sizeId].format(cpms[sizeId]);

        printf(benchDataFmt,
          testKindNameList[uint32_t(params.testKind)],
          compOpNameList[uint32_t(params.compOp)],
          styleString,
          fmt[0].data,
          fmt[1].data,
          fmt[2].data,
          fmt[3].data,
          fmt[4].data,
          fmt[5].data);

        json.beforeRecord()
            .openObject()
            .addKey("test").addString(testKindNameList[uint32_t(params.testKind)])
            .comma().alignTo(36).addKey("compOp").addString(compOpNameList[uint32_t(params.compOp)])
            .comma().alignTo(58).addKey("style").addString(styleString);

        json.addKey("rcpms").openArray();
        for (uint32_t sizeId = 0; sizeId < _sizeCount; sizeId++) {
          json.addStringWithoutQuotes(fmt[sizeId].data);
        }
        json.closeArray();

        json.closeObject();
      }

      for (uint32_t sizeId = 0; sizeId < _sizeCount; sizeId++)
        fmt[sizeId].format(cpmsTotal[sizeId]);

      printf(benchBorderStr);
      printf(benchDataFmt,
        "Total",
        compOpNameList[uint32_t(params.compOp)],
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

uint64_t BenchApp::runSingleTest(Backend& backend, BenchParams& params) {
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
      backend.run(*this, params);
      if (backend._duration >= kMinimumDurationUS) {
        // Make this the first attempt to reduce the time of benchmarking.
        attempt = 1;
        duration = backend._duration;
        break;
      }

      if (backend._duration < 100)
        params.quantity *= 10;
      else if (backend._duration < 500)
        params.quantity *= 3;
      else
        params.quantity *= 2;
    }
  }

  while (attempt < _repeat) {
    backend.run(*this, params);

    if (duration > backend._duration)
      duration = backend._duration;
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
