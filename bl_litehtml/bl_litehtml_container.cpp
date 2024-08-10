#include "bl_litehtml_container.h"

#include <limits.h>

#include <chrono>
#include <string>
#include <unordered_map>

namespace std {

template<>
struct hash<BLString> {
  inline size_t operator()(const BLString& str) const {
    size_t h = 11;
    for (char c : str) {
      h = h * 177 + c;
    }
    return h;
  }
};

}

// BLLiteHtml - Utilities
// ======================

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

static BLRectI accumulateDirtyRects(const litehtml::position::vector& dirty_rects) noexcept {
  if (dirty_rects.empty())
    return BLRectI{};

  BLBoxI dirty_acc(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
  for (const litehtml::position& dirty_rect : dirty_rects) {
    dirty_acc = BLBoxI(blMin(dirty_acc.x0, dirty_rect.x),
                       blMin(dirty_acc.y0, dirty_rect.y),
                       blMax(dirty_acc.x1, dirty_rect.right()),
                       blMax(dirty_acc.y1, dirty_rect.bottom()));
  }

  return BLRectI(dirty_acc.x0, dirty_acc.y0, dirty_acc.x1 - dirty_acc.x0, dirty_acc.y1 - dirty_acc.y0);
}

namespace BLLiteHtmlUtils {

BLStringView extractPath(BLStringView url) {
  const char* q = std::find(url.begin(), url.end(), '?');
  const char* h = std::find(url.begin(), url.end(), '#');

  const char* end = std::min(q, h);
  return BLStringView{url.data, size_t(end - url.data)};
}

}

// BLLiteHtml - Container
// ======================

class BLLiteHtmlContainer : public litehtml::document_container
{
public:
  BLLiteHtmlDocument* _doc;
  BLGlyphBuffer _glyphBuffer;
  std::unordered_map<BLString, BLImage> _images;

  explicit BLLiteHtmlContainer(BLLiteHtmlDocument* doc);
  virtual ~BLLiteHtmlContainer() override;

  inline BLLiteHtmlDocument* doc() const noexcept { return _doc; }

  const BLImage* getImage(const char* src , const char* baseurl) const;

  virtual litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
  virtual void delete_font(litehtml::uint_ptr hFont) override;
  virtual int text_width(const char* text, litehtml::uint_ptr hFont) override;
  virtual void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;

  virtual int pt_to_px(int pt) const override;
  virtual int get_default_font_size() const override;
  virtual const char* get_default_font_name() const override;
  virtual void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
  virtual void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
  virtual void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;

  virtual void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url) override;
  virtual void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) override;
  virtual void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::linear_gradient& gradient) override;
  virtual void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::radial_gradient& gradient) override;
  virtual void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::conic_gradient& gradient) override;
  virtual void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;

  virtual void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
  virtual void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
  virtual void del_clip() override;
  virtual void get_client_rect(litehtml::position& client) const override;

  virtual std::shared_ptr<litehtml::element> create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;

  virtual void set_caption(const char* caption) override;
  virtual void set_base_url(const char* base_url) override;
  virtual void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
  virtual void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
  virtual void set_cursor(const char* cursor) override;
  virtual void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;

  virtual void get_media_features(litehtml::media_features& media) const override;
  virtual void get_language(litehtml::string& language, litehtml::string & culture) const override;
  virtual litehtml::string resolve_color(const litehtml::string& color) const override;
};

BLLiteHtmlContainer::BLLiteHtmlContainer(BLLiteHtmlDocument* doc)
  : _doc(doc) {}

BLLiteHtmlContainer::~BLLiteHtmlContainer() {
}

const BLImage* BLLiteHtmlContainer::getImage(const char* src, const char* baseurl) const {
  BLString path = _doc->resolveLink(BLStringView{src, strlen(src)}, true);

  auto it = _images.find(path);
  if (it != _images.end()) {
    return &it->second;
  }
  else {
    return nullptr;
  }
}

litehtml::uint_ptr BLLiteHtmlContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
  litehtml::string_vector fontNames;
  litehtml::split_string(faceName, fontNames, ",");

  fontNames.push_back(std::string(_doc->_fallbackFamily.data(), _doc->_fallbackFamily.size()));

  BLFontQueryProperties properties {};
  properties.weight = int(weight);
  properties.style = italic == litehtml::font_style_normal ? BL_FONT_STYLE_NORMAL : BL_FONT_STYLE_ITALIC;

  BLFontFace face;
  BLFontCore font;

  blFontInit(&font);

  for (const auto& fontName : fontNames) {
    const char* fName = fontName.c_str();

    if (fontName == "monospace" && !_doc->_monospaceFamily.empty())
      fName = _doc->_monospaceFamily.data();

    if (_doc->_fontManager.queryFace(fName, properties, face) == BL_SUCCESS) {
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
  BLString path = _doc->resolveLink(BLStringView{src, strlen(src)}, true);
  BLImage img;

  if (img.readFromFile(path.data()) == BL_SUCCESS) {
    // path.replace(0, pathIndex, "");
    _images[path] = img;
  }
  else {
    printf("Failed to load an image %s (src=%s baseurl=%s)\n", path.data(), src, baseurl);
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

  render_insert_gradient_stops(style, gradient.color_points, 1.0);
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
  BLSizeI size = _doc->viewportSize();

  client.x = 0;
  client.y = 0;
  client.width = size.w;
  client.height = size.h;
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
  (void)el;

  if (_doc->onLinkClick) {
    litehtml::document::ptr doc = _doc->_document;
    BLString resolved = _doc->resolveLink(BLStringView{url, strlen(url)}, false);

    _doc->onLinkClick(resolved.data());
  }
}

void BLLiteHtmlContainer::set_cursor(const char* cursor) {
  if (_doc->onSetCursor)
    _doc->onSetCursor(cursor);
}

void BLLiteHtmlContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) {
  BLString resolved = _doc->resolveLink(BLStringView{url.data(), url.size()}, true);

  if (resolved.empty())
    return;

  BLArray<uint8_t> content;
  BLFileSystem::readFile(resolved.data(), content);

  if (!content.empty()) {
    text.assign((const char*)content.data(), content.size());
  }
  else {
    printf("Failed to load a CSS file: %s\n", resolved.data());
  }
}

void BLLiteHtmlContainer::get_media_features(litehtml::media_features& media) const {
  BLSizeI size = _doc->viewportSize();

  media.type = litehtml::media_type_screen;
  media.width = size.w;
  media.height = size.h;
  media.device_width = size.w;
  media.device_height = size.h;
  media.color = 8;
  media.monochrome = 0;
  media.color_index = 256;
  media.resolution = 96;
}

void BLLiteHtmlContainer::get_language(litehtml::string& language, litehtml::string & culture) const {}

litehtml::string BLLiteHtmlContainer::resolve_color(const litehtml::string& color) const { return ""; }

// BLLiteHtml - Document
// =====================

static bool isRelativePath(BLStringView str) noexcept {
  return str.size >= 2 && str.data[0] == '.' && str.data[1] == '/';
}

static bool isParentRelativePath(BLStringView str) noexcept {
  return str.size >= 2 && str.data[0] == '.' && str.data[1] == '.' && (str.size == 2 || str.data[2] == '/');
}

static bool endsWithSlash(BLStringView str) noexcept {
  return str.size && str.data[str.size - 1] == '/';
}

BLLiteHtmlDocument::BLLiteHtmlDocument() {}

BLLiteHtmlDocument::~BLLiteHtmlDocument() noexcept {
  _document.reset();
  if (_container)
    delete _container;
}

BLSizeI BLLiteHtmlDocument::documentSize() const {
  if (!_document)
    return BLSizeI{};

  return BLSizeI(_document->width(), _document->height());
}

void BLLiteHtmlDocument::setViewportSize(const BLSizeI& sz) {
  if (_viewportSize != sz) {
    _viewportSize = sz;
    if (_document)
      _document->render(sz.w);
  }
}

void BLLiteHtmlDocument::setViewportPosition(const BLPointI& pt) {
  if (_viewportPosition != pt) {
    _viewportPosition = pt;
  }
}

double BLLiteHtmlDocument::averageFrameDuration() const noexcept {
  double sum = 0.0;
  for (size_t i = 0; i < _renderTimeCount; i++) {
    sum += _renderTime[i];
  }
  return sum / double(_renderTimeCount);
}

BLString BLLiteHtmlDocument::resolveLink(BLStringView link, bool stripParams) {
  BLString resolved;

  if (isRelativePath(link)) {
    link = BLStringView{link.data + 2, link.size - 2};
  }
  else if (link.size == 1 && link.data[0] == '.')
    link = BLStringView{nullptr, 0};

  resolved.assign(BLStringView{_url.data(), _urlPathEnd});

  bool dropBaseName = true;

  while (isParentRelativePath(link) || dropBaseName) {
    const char* p = resolved.end();
    const char* b = resolved.begin();

    while (p != b && p[-1] != '/')
      p--;

    if (p == b)
      break;

    resolved.truncate((size_t)(--p - resolved.begin()));

    if (dropBaseName) {
      dropBaseName = false;
    }
    else {
      size_t cut = std::min<size_t>(link.size, 3);
      link = BLStringView{link.data + cut, link.size - cut};
    }
  }

  if (!endsWithSlash(resolved.view()))
    resolved.append('/');

  resolved.append(link);

  if (stripParams) {
    BLStringView path = BLLiteHtmlUtils::extractPath(resolved.view());
    resolved.truncate(path.size);
  }
  return resolved;
}

void BLLiteHtmlDocument::acceptLink(const char* url) {
  _acceptedLinkURL.assign(url);
  _linkAccepted = true;
}

void BLLiteHtmlDocument::createFromURL(BLStringView url, BLStringView masterCSS) {
  BLString path(BLLiteHtmlUtils::extractPath(url));
  BLStringView params{path.end(), url.size - path.size()};

  BLFileInfo fileInfo;
  BLResult result;

  result = BLFileSystem::fileInfo(path.data(), &fileInfo);
  if (result != BL_SUCCESS) {
    return;
  }

  if (fileInfo.isDirectory()) {
    if (!endsWithSlash(path.view()))
      path.append('/');
  }

  size_t urlPathEnd = path.size();

  if (fileInfo.isDirectory()) {
    path.append("index.html");
  }

  BLArray<uint8_t> content;
  result = BLFileSystem::readFile(path.data(), content);

  if (result != BL_SUCCESS) {
    return;
  }

  _url.assign(BLStringView{path.data(), urlPathEnd});
  _url.append(params);
  _urlPathEnd = urlPathEnd;

  BLStringView cv{reinterpret_cast<const char*>(content.data()), content.size()};
  createFromHTML(cv, masterCSS);
}

void BLLiteHtmlDocument::createFromHTML(BLStringView content, BLStringView masterCSS) {
  BLLiteHtmlContainer* container = new BLLiteHtmlContainer(this);

  _viewportPosition.reset();
  _linkAccepted = false;
  _acceptedLinkURL.reset();

  _document = litehtml::document::createFromString(std::string(content.data, content.size), container, std::string(masterCSS.data, masterCSS.size));

  if (_container)
    delete _container;

  _container = container;
  _document->render(viewportSize().w);
}

BLRectI BLLiteHtmlDocument::mouseLeave() {
  if (!_document)
    return BLRectI{};

  litehtml::position::vector dirty_rects;

  if (!_document->on_mouse_leave(dirty_rects))
    return BLRectI{};

  return accumulateDirtyRects(dirty_rects);
}

BLRectI BLLiteHtmlDocument::mouseMove(const BLPointI& pos) {
  if (!_document)
    return BLRectI{};

  BLPoint vp = pos - _viewportPosition;
  litehtml::position::vector dirty_rects;

  if (!_document->on_mouse_over(pos.x, pos.y, vp.x, vp.y, dirty_rects))
    return BLRectI{};

  return accumulateDirtyRects(dirty_rects);
}

BLRectI BLLiteHtmlDocument::mouseDown(const BLPointI& pos, Button button) {
  if (!_document)
    return BLRectI{};

  if (button != Button::kLeft)
    return BLRectI{};

  BLPoint vp = pos - _viewportPosition;
  litehtml::position::vector dirty_rects;

  if (!_document->on_lbutton_down(pos.x, pos.y, vp.x, vp.y, dirty_rects))
    return BLRectI{};

  return accumulateDirtyRects(dirty_rects);
}

BLRectI BLLiteHtmlDocument::mouseUp(const BLPointI& pos, Button button) {
  if (!_document)
    return BLRectI{};

  if (button != Button::kLeft)
    return BLRectI{};

  BLPoint vp = pos - _viewportPosition;
  litehtml::position::vector dirty_rects;

  if (!_document->on_lbutton_up(pos.x, pos.y, vp.x, vp.y, dirty_rects))
    return BLRectI{};

  return accumulateDirtyRects(dirty_rects);
}

void BLLiteHtmlDocument::draw(BLContext& ctx) {
  if (!_document)
    return;

  BLPointI vp = viewportPosition();
  BLSizeI vs = viewportSize();

  litehtml::position clip;
  clip.x = 0;
  clip.y = 0;
  clip.width = vs.w;
  clip.height = vs.h;

  auto startTime = std::chrono::high_resolution_clock::now();

  _document->draw((uintptr_t)&ctx, -vp.x, -vp.y, &clip);

  auto endTime = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = endTime - startTime;

  _renderTimeIndex = (_renderTimeIndex + 1) & 31;
  _renderTime[_renderTimeIndex] = duration.count() * 1000;

  if (_renderTimeCount < 32u)
    _renderTimeCount++;
}
