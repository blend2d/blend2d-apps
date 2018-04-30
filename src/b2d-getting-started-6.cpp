#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  b2d::LinearGradient linear(0, 0, 0, 400);
  linear.addStop(0.0, b2d::Argb32(0xFFFFFFFF));
  linear.addStop(1.0, b2d::Argb32(0xFF1F7FFF));

  b2d::Path2D path;
  path.moveTo(50, 50);
  path.cubicTo(200, 50, 50, 270, 200, 270);
  path.cubicTo(400, 270, 300, -150, 200, 390);

  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.setStrokeStyle(linear);
  ctx.setStrokeWidth(15);
  ctx.setStartCap(b2d::StrokeCap::kRound);
  ctx.setEndCap(b2d::StrokeCap::kButt);
  ctx.strokePath(path);

  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-6.bmp", codec, img);
  return 0;
}
