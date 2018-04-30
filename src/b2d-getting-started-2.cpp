#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  // Coordinates can be specified now or changed later.
  b2d::LinearGradient linear(0, 0, 0, 400);

  // Color stops can be added in any order.
  linear.addStop(0.0, b2d::Argb32(0xFFFFFFFF));
  linear.addStop(0.5, b2d::Argb32(0xFF5FAFDF));
  linear.addStop(1.0, b2d::Argb32(0xFF2F5FDF));

  // `setFillStyle()` can be used for both colors and styles.
  ctx.setFillStyle(linear);

  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.fillRound(40.0, 40.0, 320.0, 320.0, 40.5);
  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-2.bmp", codec, img);
  return 0;
}
