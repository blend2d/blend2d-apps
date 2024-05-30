#include "bl_litehtml_container.h"

static constexpr double kPI = 3.14159265358979323846;

static inline bool isBorderVisible(const litehtml::border& b) noexcept {
  return b.width > 0 && b.style > litehtml::border_style_hidden && b.color.alpha != 0;
}

static inline bool borderSideEquals(const litehtml::border& a, const litehtml::border& b) noexcept {
  return a.color == b.color && a.style == b.style && a.width == b.width;
}

static inline BLRgba32 rgba32FromWebColor(litehtml::web_color c) noexcept {
  return BLRgba32(c.red, c.green, c.blue, c.alpha);
}

static inline BLRectI positionToRect(const litehtml::position& pos) noexcept {
  return BLRectI(pos.x, pos.y, pos.width, pos.height);
}

static inline BLEllipse positionToEllipse(const litehtml::position& pos) noexcept {
  double rx = pos.width / 2.0;
  double ry = pos.height / 2.0;
  return BLEllipse(pos.x + rx, pos.y + ry, rx, ry);
}

static void urlToPath(std::string& out, const char* src, const char* baseurl) {
  BLStringView src_view{src, strlen(src)};
  BLStringView base_view{baseurl, baseurl ? strlen(baseurl) : 0};

  if (src_view.size >= 2 && src_view.data[0] == '.' && src_view.data[1] == '/')
    src_view = BLStringView{src_view.data + 2, src_view.size - 2};

  if (base_view.size != 0) {
    out.append(base_view.data, base_view.size);
    out.append(1, '/');
  }

  out.append(src_view.data, src_view.size);
}

BLLiteHtmlContainer::BLLiteHtmlContainer() {
  _size.reset();
}

BLLiteHtmlContainer::~BLLiteHtmlContainer() {
}

const BLImage* BLLiteHtmlContainer::getImage(const char* src, const char* baseurl) const {
  std::string path;
  urlToPath(path, src, baseurl);

  auto it = _images.find(path);
  if (it != _images.end()) {
    return &it->second;
  }
  else {
    printf("Image not found path=%s (src=%s baseurl=%s)\n", path.c_str(), src, baseurl);
    return nullptr;
  }
}

litehtml::uint_ptr BLLiteHtmlContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
  litehtml::string_vector fontNames;
  litehtml::split_string(faceName, fontNames, ",");

  fontNames.push_back(std::string(_fallbackFont.data(), _fallbackFont.size()));

  BLFontQueryProperties properties {};
  properties.weight = int(weight);
  properties.style = italic == litehtml::font_style_normal ? BL_FONT_STYLE_NORMAL : BL_FONT_STYLE_ITALIC;

  BLFontFace face;
  BLFontCore font;

  blFontInit(&font);

  for (const auto& fontName : fontNames) {
    const char* fName = fontName.c_str();

    if (fontName == "monospace")
      fName = "Courier New";

    if (_fontManager.queryFace(fName, properties, face) == BL_SUCCESS) {
      BLFontMetrics blfm;
      blFontCreateFromFace(&font, &face, float(size));
      blFontGetMetrics(&font, &blfm);

      fm->ascent = int(blfm.ascent);
      fm->descent = int(blfm.descent);
      fm->height = int(blfm.ascent + blfm.descent + blfm.lineGap);
      fm->x_height = int(blfm.xHeight);

      BLFontCore* h = new BLFont(std::move(font.dcast()));
      return (litehtml::uint_ptr)h;
    }
  }

  return (litehtml::uint_ptr)nullptr;
}

void BLLiteHtmlContainer::delete_font(litehtml::uint_ptr hFont) {
  BLFont* font = (BLFont*)hFont;
  delete font;
}

int BLLiteHtmlContainer::text_width(const char* text, litehtml::uint_ptr hFont) {
  BLFont* font = (BLFont*)hFont;

  _glyphBuffer.clear();
  _glyphBuffer.setUtf8Text(text);

  if (blFontShape(font, &_glyphBuffer) == BL_SUCCESS) {
    BLTextMetrics m;
    blFontGetTextMetrics(font, &_glyphBuffer, &m);

    int w = int(m.advance.x);
    return w;
  }
  else {
    return 0;
  }
}

void BLLiteHtmlContainer::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {
  BLContext* ctx = (BLContext*)hdc;
  BLFont* font = (BLFont*)hFont;

  int ascent = int(font->metrics().ascent);
  ctx->fillUtf8Text(BLPoint(pos.x, pos.y + ascent), *font, text, SIZE_MAX, rgba32FromWebColor(color));
}

int BLLiteHtmlContainer::pt_to_px(int pt) const {
  return (int) ((double) pt * 1.3333333333);
}

int BLLiteHtmlContainer::get_default_font_size() const {
  return 14;
}

const char* BLLiteHtmlContainer::get_default_font_name() const {
  return "Arial";
}

void BLLiteHtmlContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {
  BLContext* ctx = (BLContext*)hdc;

  if (marker.image.empty()) {
    if (marker.marker_type == litehtml::list_style_type_square) {
      ctx->fillRect(positionToRect(marker.pos), rgba32FromWebColor(marker.color));
    }
    else if (marker.marker_type == litehtml::list_style_type_disc) {
      ctx->fillEllipse(positionToEllipse(marker.pos), rgba32FromWebColor(marker.color));
    }
    else if (marker.marker_type == litehtml::list_style_type_circle) {
      ctx->strokeEllipse(positionToEllipse(marker.pos), rgba32FromWebColor(marker.color));
    }
    else {
      // TOOD: ???
    }
  }
  else {
    const BLImage* img = getImage(marker.image.c_str(), marker.baseurl);
    if (img) {
      ctx->blitImage(positionToRect(marker.pos), *img);
    }
  }
}

void BLLiteHtmlContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {
  std::string path;
  path.append(_baseUrl.data(), _baseUrl.size());
  if (!path.empty() && path[path.size() - 1] != '/')
    path.append(1, '/');

  size_t pathIndex = path.size();
  urlToPath(path, src, baseurl);

  BLImage img;
  if (img.readFromFile(path.c_str()) == BL_SUCCESS) {
    path.replace(0, pathIndex, "");
    _images[path] = img;
  }
  else {
    printf("Failed to read image %s (src=%s baseurl=%s)\n", path.data(), src, baseurl);
  }
}

void BLLiteHtmlContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {
  const BLImage* img = getImage(src, baseurl);

  if (img) {
    sz.width = img->width();
    sz.height = img->height();
  }
  else {
    sz.width = 0;
    sz.height = 0;
  }
}

static void render_insert_gradient_stops(BLGradient& gradient, const std::vector<litehtml::background_layer::color_point>& color_points, double offset_scale = 1.0) {
  for (const auto& color_point : color_points) {
    gradient.addStop(double(color_point.offset) * offset_scale, rgba32FromWebColor(color_point.color));
  }
}

static void render_styled(BLContext* ctx, const litehtml::background_layer& layer, const BLObjectCore& style) noexcept {
  BLRectI rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height);
  ctx->fillRect(rect, static_cast<const BLVarCore&>(style));
}

void BLLiteHtmlContainer::draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url) {
	if (!layer.clip_box.width && !layer.clip_box.height)
    return;

	if (url.empty() || (!layer.clip_box.width && !layer.clip_box.height) )
		return;

  BLContext* ctx = (BLContext*)hdc;
  const BLImage* img = getImage(url.data(), base_url.data());

  if (!img)
    return;

  int x = layer.border_box.x;
  int y = layer.border_box.y;
  int w = layer.border_box.width;
  int h = layer.border_box.height;

  if (img->width() >= w && img->height() >= h) {
    ctx->blitImage(BLRectI(x, y, w, h), *img);
    return;
  }

  BLExtendMode extendMode = BL_EXTEND_MODE_REPEAT;
  switch (layer.repeat) {
    case litehtml::background_repeat_repeat   : extendMode = BL_EXTEND_MODE_REPEAT; break;
    case litehtml::background_repeat_repeat_x : extendMode = BL_EXTEND_MODE_REPEAT_X_PAD_Y; break;
    case litehtml::background_repeat_repeat_y : extendMode = BL_EXTEND_MODE_PAD_X_REPEAT_Y; break;
    case litehtml::background_repeat_no_repeat: extendMode = BL_EXTEND_MODE_PAD; break;
  }

  BLPattern pattern(*img, extendMode, BLMatrix2D::makeTranslation(x, y));
  render_styled(ctx, layer, pattern);
}

void BLLiteHtmlContainer::draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) {
	if (!layer.clip_box.width && !layer.clip_box.height)
    return;

	if (color == litehtml::web_color::transparent)
		return;

  // printf("Rendering solid: %08X\n", rgba32FromWebColor(color).value);

  BLContext* ctx = (BLContext*)hdc;
  BLRectI rect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height);

  ctx->fillRect(rect, rgba32FromWebColor(color));
}

void BLLiteHtmlContainer::draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::linear_gradient& gradient) {
	if (!layer.clip_box.width && !layer.clip_box.height)
    return;

  BLContext* ctx = (BLContext*)hdc;
  BLGradient style(BLLinearGradientValues(gradient.start.x, gradient.start.y, gradient.end.x, gradient.end.y));

  render_insert_gradient_stops(style, gradient.color_points);
  render_styled(ctx, layer, style);
}

void BLLiteHtmlContainer::draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::radial_gradient& gradient) {
	if (!layer.clip_box.width && !layer.clip_box.height)
    return;

  double cx = double(gradient.position.x);
  double cy = double(gradient.position.y);

  double rx = double(gradient.radius.x);
  double ry = double(gradient.radius.y);

  BLContext* ctx = (BLContext*)hdc;
  BLGradient style(BLRadialGradientValues(0.0, 0.0, 0.0, 0.0, rx));

  if (rx != ry) {
    double aspect_ratio = double(ry) / double(rx);

    BLMatrix2D m = BLMatrix2D::makeScaling(1.0, aspect_ratio);
    m.postTranslate(cx, cy);
    style.setTransform(m);
  }
  else {
    BLMatrix2D m = BLMatrix2D::makeTranslation(cx, cy);
    style.setTransform(m);
  }

  render_insert_gradient_stops(style, gradient.color_points);
  render_styled(ctx, layer, style);
}

void BLLiteHtmlContainer::draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::conic_gradient& gradient) {
	if (!layer.clip_box.width && !layer.clip_box.height)
    return;

  BLContext* ctx = (BLContext*)hdc;
  BLGradient style(BLConicGradientValues(gradient.position.x, gradient.position.y, gradient.angle - kPI / 2.0, 1.0));

  render_insert_gradient_stops(style, gradient.color_points, 1.0 / 360.0);
  render_styled(ctx, layer, style);
}

void BLLiteHtmlContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {
  BLContext* ctx = (BLContext*)hdc;

  if (isBorderVisible(borders.left)) {
    const litehtml::border& b = borders.left;
    if (borderSideEquals(b, borders.right) && borderSideEquals(b, borders.top) && borderSideEquals(b, borders.bottom)) {
      ctx->strokeRect(
        double(draw_pos.x) + double(b.width) * 0.5,
        double(draw_pos.y) + double(b.width) * 0.5,
        double(draw_pos.width) - double(b.width),
        double(draw_pos.height) - double(b.width), rgba32FromWebColor(b.color));
      return;
    }
  }

  if (isBorderVisible(borders.left)) {
    ctx->fillRect(
      BLRectI(
        draw_pos.x,
        draw_pos.y,
        borders.left.width,
        draw_pos.height),
      rgba32FromWebColor(borders.left.color));
  }

  if (isBorderVisible(borders.top)) {
    ctx->fillRect(
      BLRectI(
        draw_pos.x,
        draw_pos.y,
        draw_pos.width,
        borders.top.width),
      rgba32FromWebColor(borders.top.color));
  }

  if (isBorderVisible(borders.right)) {
    ctx->fillRect(
      BLRectI(
        draw_pos.x + draw_pos.width - borders.right.width,
        draw_pos.y,
        borders.right.width,
        draw_pos.height),
      rgba32FromWebColor(borders.right.color));
  }

  if (isBorderVisible(borders.bottom)) {
    ctx->fillRect(
      BLRectI(
        draw_pos.x,
        draw_pos.y + draw_pos.height - borders.bottom.width,
        draw_pos.width,
        borders.bottom.width),
      rgba32FromWebColor(borders.bottom.color));
  }
}

void BLLiteHtmlContainer::transform_text(litehtml::string& text, litehtml::text_transform tt) {
  // TODO: TRANSFORM TEXT.
}

void BLLiteHtmlContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) {
}

void BLLiteHtmlContainer::del_clip() {
}

void BLLiteHtmlContainer::get_client_rect(litehtml::position& client) const {
  client.x = 0;
  client.y = 0;
  client.width = _size.w;
  client.height = _size.h;
}

std::shared_ptr<litehtml::element> BLLiteHtmlContainer::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) {
  return std::shared_ptr<litehtml::element>();
}

void BLLiteHtmlContainer::set_caption(const char* caption) {

}

void BLLiteHtmlContainer::set_base_url(const char* base_url) {

}

void BLLiteHtmlContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) {

}

void BLLiteHtmlContainer::on_anchor_click(const char* url, const litehtml::element::ptr& el) {

}

void BLLiteHtmlContainer::set_cursor(const char* cursor) {
  printf("SetCursor %s\n", cursor);
}

void BLLiteHtmlContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) {
  BLStringView url_string{url.c_str(), url.size()};

  const char* url_questionmark = strchr(url_string.data, '?');

  if (url_questionmark) {
    url_string.size = size_t(url_questionmark - url_string.data);
  }

  if (url_string.size == 0)
    return;

  BLStringView base{nullptr, 0};
  const char* s = url_string.data + url_string.size;
  while (s != url_string.data) {
    if (s[-1] == '/' || s[-1] == '\\') {
      base.reset(url_string.data, size_t(s - url_string.data) - 1);
      break;
    }
    s--;
  }

  if (base.size >= 2 && base.data[0] == '.' && base.data[1] == '/')
    base = BLStringView{base.data + 2, base.size - 2};

  if (base.size)
    baseurl.assign(base.data, base.size);

  BLString path;
  path.append(_baseUrl);
  path.append('/');
  path.append(url_string);

  BLArray<uint8_t> content;
  BLFileSystem::readFile(path.data(), content);

  if (!content.empty()) {
    printf("Loaded CSS file %s (baseurl=%s)\n", url_string.data, baseurl.c_str());
    text.assign((const char*)content.data(), content.size());
  }
}

void BLLiteHtmlContainer::get_media_features(litehtml::media_features& media) const {
  media.type = litehtml::media_type_screen;
  media.width = _size.w;
  media.height = _size.h;
  media.device_width = _size.w;
  media.device_height = _size.h;
  media.color = 8;
  media.monochrome = 0;
  media.color_index = 256;
  media.resolution = 96;
}

void BLLiteHtmlContainer::get_language(litehtml::string& language, litehtml::string & culture) const {}

litehtml::string BLLiteHtmlContainer::resolve_color(const litehtml::string& color) const { return ""; }
