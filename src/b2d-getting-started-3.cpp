#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  // We need some image, so let's read some file.
  b2d::Image texture;
  b2d::Error err = b2d::ImageUtils::readImageFromFile(
    texture,                          // Image (destination).
    b2d::ImageCodec::builtinCodecs(), // Which codecs to use.
    "texture.jpeg");                  // File name in UTF-8.

  // Since we do IO a basic error handling is necessary
  if (err) {
    std::printf("Failed to load a texture (err=%u)\n", err);
    return 1;
  }

  // Create a pattern and use it to fill a rounded-rect.
  b2d::Pattern pattern(texture);

  ctx.setCompOp(b2d::CompOp::kSrcOver);
  ctx.setFillStyle(pattern);
  ctx.fillRound(40.0, 40.0, 320.0, 320.0, 40.5);

  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-3.bmp", codec, img);
  return 0;
}
