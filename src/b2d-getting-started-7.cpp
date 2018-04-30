#include <b2d/b2d.h>

int main(int argc, char* argv[]) {
  b2d::Image img(400, 400, b2d::PixelFormat::kPRGB32);
  b2d::Context2D ctx(img);

  ctx.setCompOp(b2d::CompOp::kSrc);
  ctx.fillAll();

  b2d::FontFace face;
  b2d::Error err = face.createFromFile("NotoSans-Regular.ttf");

  // We must handle a possible error returned by the loader.
  if (err) {
    std::printf("Failed to load a font-face (err=%u)\n", err);
    return 1;
  }

  b2d::Font font;
  font.createFromFace(face, 50.0f);

  ctx.setFillStyle(b2d::Argb32(0xFFFFFFFF));
  ctx.fillUtf8Text(b2d::Point(20, 80), font, "Hello Blend2D!");

  ctx.rotate(b2d::Math::kPi / 4.0);
  ctx.fillUtf8Text(b2d::Point(240, 80), font, "Rotated");

  ctx.end();

  b2d::ImageCodec codec = b2d::ImageCodec::codecByName(
    b2d::ImageCodec::builtinCodecs(), "BMP");

  b2d::ImageUtils::writeImageToFile(
    "b2d-getting-started-7.bmp", codec, img);
  return 0;
}
