#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  // First shape filld by a radial gradient.
  b2d::RadialGradient radial(150, 150, 150, 150, 150);
  radial.addStop(0.0, b2d::Argb32(0xFFFFFFFF));
  radial.addStop(1.0, b2d::Argb32(0xFFFF6F3F));

  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.setFillStyle(radial);
  ctx.fillCircle(150, 150, 130);

  // Second shape filled by a linear gradient.
  b2d::LinearGradient linear(160, 160, 380, 380);
  linear.addStop(0.0, b2d::Argb32(0xFFFFFFFF));
  linear.addStop(1.0, b2d::Argb32(0xFF3F9FFF));

  ctx.setCompOp(b2d::CompOp::kDifference);
  ctx.setFillStyle(linear);
  ctx.fillRound(160, 160, 220, 220, 20);

  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-5.bmp", codec, img);
  return 0;
}
