#ifndef BL_LITEHTML_CONTAINER_H_INCLUDED
#define BL_LITEHTML_CONTAINER_H_INCLUDED

#include <stdio.h>

#include <blend2d.h>
#include <litehtml.h>

namespace BLLiteHtmlUtils {

BLStringView extractPath(BLStringView url);

};

class BLLiteHtmlContainer;

class BLLiteHtmlDocument {
public:
  static constexpr uint32_t kAvgFrameCount = 16;

  enum class Button {
    kLeft = 0,
    kRight = 1,
    kMiddle = 2,
    kUnknown = 0xFF
  };

  BLLiteHtmlContainer* _container {};
  litehtml::document::ptr _document {};

  bool _linkAccepted = false;
  bool _urlIsDirectory = false;

  BLFontManager _fontManager;
  BLString _fallbackFamily;
  BLString _monospaceFamily;

  BLString _url;
  size_t _urlPathEnd {};
  BLString _acceptedLinkURL;

  BLSizeI _viewportSize {};
  BLPointI _viewportPosition {};

  uint32_t _renderTimeCount {};
  uint32_t _renderTimeIndex = 31;
  double _renderTime[32] {};

  BLLiteHtmlDocument();
  ~BLLiteHtmlDocument() noexcept;

  inline litehtml::document::ptr& document() { return _document; }
  inline const litehtml::document::ptr& document() const { return _document; }

  BLFontManager& fontManager() { return _fontManager; }
  const BLFontManager& fontManager() const { return _fontManager; }

  //! \name Document & Viewport
  //! \{

  BLSizeI documentSize() const;

  inline BLSizeI viewportSize() const noexcept { return _viewportSize; }
  inline BLPointI viewportPosition() const noexcept { return _viewportPosition; }

  void setViewportSize(const BLSizeI& sz);
  void setViewportPosition(const BLPointI& pt);

  //! \}

  //! \name Performance
  //! \{

  inline double lastFrameDuration() const noexcept { return _renderTime[_renderTimeIndex]; }
  double averageFrameDuration() const noexcept;

  //! \}

  //! \name Link management
  //! \{

  inline const BLString& url() const noexcept { return _url; }
  BLString resolveLink(BLStringView link, bool stripParams);

  inline bool linkAccepted() const noexcept { return _linkAccepted; }
  inline const BLString& acceptedLinkURL() const noexcept { return _acceptedLinkURL; }

  void acceptLink(const char* url);

  //! \}

  void createFromURL(BLStringView url, BLStringView masterCSS);
  void createFromHTML(BLStringView content, BLStringView masterCSS);

  inline void setFontManager(const BLFontManager& mgr) { _fontManager = mgr; }

  inline void setFallbackFamily(const BLString& name) { _fallbackFamily.assign(name); }
  inline void setFallbackFamily(BLStringView name) { _fallbackFamily.assign(name); }

  inline void setMonospaceFamily(const BLString& name) { _monospaceFamily.assign(name); }
  inline void setMonospaceFamily(BLStringView name) { _monospaceFamily.assign(name); }

  BLRectI mouseLeave();
  BLRectI mouseMove(const BLPointI& pos);
  BLRectI mouseDown(const BLPointI& pos, Button button);
  BLRectI mouseUp(const BLPointI& pos, Button button);

  // Callbacks from litehtml.
  std::function<void(const char*)> onSetCursor;
  std::function<void(const char*)> onLinkClick;

  void draw(BLContext& ctx);
};

#endif // BL_LITEHTML_CONTAINER_H_INCLUDED
