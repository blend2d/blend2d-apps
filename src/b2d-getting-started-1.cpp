#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);

  // Attach a rendering context into `img`.
  b2d::Context2D ctx(img);

  // Clear the image.
  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  // Fill some path.
  b2d::Path2D path;
  path.moveTo(92, 341);
  path.cubicTo(251, 21, 266, -136, 19, 198);
  path.cubicTo(29, 240, 304, 108, 392, 381);

  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.setFillStyle(b2d::Argb32(0xFFFFFFFF));
  ctx.fillPath(path);

  // Detach the rendering context from `img`.
  ctx.end();

  // Let's use some built-in codecs provided by Blend2D.
  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-1.bmp", codec, img);
  return 0;
}
