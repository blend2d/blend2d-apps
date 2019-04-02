#include <blend2d.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
  BLImage img(480, 480, BL_FORMAT_PRGB32);
  BLContext ctx(img);

  ctx.setCompOp(BL_COMP_OP_SRC_COPY);
  ctx.fillAll();

  // Read an image from file.
  BLImage texture;
  BLResult err = texture.readFromFile(
    "texture.jpeg",
    BLImageCodec::builtInCodecs());

  // Basic error handling is necessary as we need some IO.
  if (err) {
    printf("Failed to load a texture (err=%u)\n", err);
    return 1;
  }

  // Create a pattern and use it to fill a rounded-rect.
  BLPattern pattern(texture);

  ctx.setCompOp(BL_COMP_OP_SRC_OVER);
  ctx.setFillStyle(pattern);
  ctx.fillRoundRect(40.0, 40.0, 400.0, 400.0, 45.5);

  ctx.end();

  BLImageCodec codec;
  codec.findByName(BLImageCodec::builtInCodecs(), "BMP");
  img.writeToFile("bl-getting-started-3.bmp", codec);

  return 0;
}
