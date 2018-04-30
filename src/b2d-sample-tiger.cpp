#include <stdio.h>
#include <b2d/b2d.h>

#include "./b2d-sample-tiger.h"

#define CACHE_STROKE 0

struct TigerPath {
  B2D_INLINE TigerPath()
    : fillColor(0),
      strokeColor(0),
      fillRule(b2d::FillRule::kNonZero),
      fill(false),
      stroke(false) {}
  B2D_INLINE ~TigerPath() {}

  b2d::Path2D path;
  b2d::Path2D strokedPath;
  b2d::StrokeParams strokeParams;

  b2d::Argb32 fillColor;
  b2d::Argb32 strokeColor;

  uint32_t fillRule;
  bool fill;
  bool stroke;
};

struct Tiger {
  Tiger() {
    init(
      TigerData::commands, B2D_ARRAY_SIZE(TigerData::commands),
      TigerData::points, B2D_ARRAY_SIZE(TigerData::points));
  }

  ~Tiger() {
    destroy();
  }

  void init(const char* commands, int commandCount, const float* points, int pointCount) {
    size_t c = 0;
    size_t p = 0;

    float h = TigerData::height;

    while (c < commandCount) {
      TigerPath* tp = new TigerPath();

      // Fill params.
      switch (commands[c++]) {
        case 'N': tp->fill = false; break;
        case 'F': tp->fill = true; tp->fillRule = b2d::FillRule::kNonZero; break;
        case 'E': tp->fill = true; tp->fillRule = b2d::FillRule::kEvenOdd; break;
      }

      // Stroke params.
      switch (commands[c++]) {
        case 'N': tp->stroke = false; break;
        case 'S': tp->stroke = true; break;
      }

      switch (commands[c++]) {
        case 'B': tp->strokeParams.setLineCaps(b2d::StrokeCap::kButt); break;
        case 'R': tp->strokeParams.setLineCaps(b2d::StrokeCap::kRound); break;
        case 'S': tp->strokeParams.setLineCaps(b2d::StrokeCap::kSquare); break;
      }

      switch (commands[c++]) {
        case 'M': tp->strokeParams.setStrokeJoin(b2d::StrokeJoin::kMiter); break;
        case 'R': tp->strokeParams.setStrokeJoin(b2d::StrokeJoin::kRound); break;
        case 'B': tp->strokeParams.setStrokeJoin(b2d::StrokeJoin::kBevel); break;
      }

      tp->strokeParams.setMiterLimit(points[p++]);
      tp->strokeParams.setStrokeWidth(points[p++]);

      // Stroke & Fill style.
      tp->strokeColor.setValue(b2d::ArgbUtil::pack32(0xFF, int(points[p + 0] * 255.0f), int(points[p + 1] * 255.0f), int(points[p + 2] * 255.0f)));
      tp->fillColor.setValue(b2d::ArgbUtil::pack32(0xFF, int(points[p + 3] * 255.0f), int(points[p + 4] * 255.0f), int(points[p + 5] * 255.0f)));
      p += 6;

      // Path.
      int i, count = int(points[p++]);
      for (i = 0 ; i < count; i++) {
        switch (commands[c++]) {
          case 'M': tp->path.moveTo(points[p], h - points[p + 1]); p += 2; break;
          case 'L': tp->path.lineTo(points[p], h - points[p + 1]); p += 2; break;
          case 'C': tp->path.cubicTo(points[p], h - points[p + 1], points[p + 2], h - points[p + 3], points[p + 4], h - points[p + 5]); p += 6; break;
          case 'E': tp->path.close(); break;
        }
      }

      tp->path.compact();

      if (CACHE_STROKE && tp->stroke) {
        b2d::Stroker stroker;
        stroker.setParams(tp->strokeParams);
        stroker.stroke(tp->strokedPath, tp->path);
      }

      paths.append(tp);
    }
  }

  void destroy() {
    for (size_t i = 0, count = paths.size(); i < count; i++)
      delete paths[i];
    paths.reset();
  }

  void render(b2d::Context2D& ctx) {
    ctx.save();
    ctx.setCompOp(b2d::CompOp::kSrcOver);

    for (size_t i = 0, count = paths.size(); i < count; i++) {
      const TigerPath* tp = paths[i];

      if (tp->fill) {
        ctx.setFillStyle(tp->fillColor);
        ctx.setFillRule(tp->fillRule);
        ctx.fillPath(tp->path);
      }

      if (tp->stroke) {
        if (CACHE_STROKE) {
          ctx.setFillStyle(tp->strokeColor);
          ctx.fillPath(tp->strokedPath);
        }
        else {
          ctx.setStrokeStyle(tp->strokeColor);
          ctx.setStrokeParams(tp->strokeParams);
          ctx.strokePath(tp->path);
        }
      }
    }

    ctx.restore();
  }

  b2d::Array<TigerPath*> paths;
};

int main(int argc, char* argv[]) {
  Tiger tiger;

  b2d::Image img(TigerData::width, TigerData::height, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx;

  bool prerender = true;
  uint32_t quantity = 1000;

  ctx.begin(img);
  ctx.setFillStyle(b2d::Argb32(0xFFFFFFFF));
  ctx.fillAll();
  if (prerender)
    tiger.render(ctx);
  ctx.end();

  b2d::ImageUtils::writeImageToFile(
    "b2d-sample-tiger.bmp", b2d::ImageCodec::codecByName(b2d::ImageCodec::builtinCodecs(), "BMP"), img);

  ctx.begin(img);
  uint32_t ticks = b2d::CpuTicks::now();
  for (uint32_t i = 0; i < quantity; i++) {
    ctx.fillAll();
    tiger.render(ctx);
  }
  ticks = b2d::CpuTicks::now() - ticks;
  ctx.end();

  printf("Ticks: %u [%u frames]\n", ticks, quantity);
  printf("Speed: %0.1f [fps]\n", (double(quantity) / double(ticks)) * 1000.0);

  return 0;
}
