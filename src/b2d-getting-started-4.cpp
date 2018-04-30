#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  b2d::Image texture;
  b2d::Error err = b2d::ImageUtils::readImageFromFile(
    texture,                          // Image (destination).
    b2d::ImageCodec::builtinCodecs(), // Which codecs to use.
    "texture.jpeg");                  // File name in UTF-8.

  if (err) {
    std::printf("Failed to load a texture (err=%u)\n", err);
    return 1;
  }

  // Rotate by 45 degrees about a point at [200, 200].
  ctx.rotate(b2d::Math::kPi / 4.0, 200.0, 200.0);

  // Create a pattern.
  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.setFillStyle(b2d::Pattern(texture));
  ctx.fillRound(40.0, 40.0, 320.0, 320.0, 65.5);

  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-4.bmp", codec, img);
  return 0;
}
