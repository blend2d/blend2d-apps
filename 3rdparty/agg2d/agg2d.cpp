//----------------------------------------------------------------------------
// Agg2D - Version 1.0
// Based on Anti-Grain Geometry
// Copyright (C) 2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// 2007-01-25 Jerry Evans (jerry@novadsp.com)
//   Ported to AGG 2.4
//
// 2008-09-25 Jim Barry (jim@mvps.org)
//   Fixed errors in kerning
//
//----------------------------------------------------------------------------

#include "agg2d.h"

static const double g_approxScale = 2.0;

Agg2D::~Agg2D()
{
}

Agg2D::Agg2D() :
    m_rbuf(),
    m_pixFormatPre(m_rbuf),
    m_pixFormatCompPre(m_rbuf),
    m_renBasePre(m_pixFormatPre),
    m_renBaseCompPre(m_pixFormatCompPre),
    m_renSolid(m_renBasePre),
    m_renSolidComp(m_renBaseCompPre),

    m_allocator(),
    m_clipBox(0,0,0,0),

    m_blendMode(BlendSrcOver),
    m_imageBlendMode(BlendDst),
    m_imageBlendColor(0,0,0),

    m_scanline(),
    m_rasterizer(),

    m_masterAlpha(1.0),

    m_color{},
    m_gradient{},

    m_lineCap(CapRound),
    m_lineJoin(JoinRound),

    m_styleFlag{Solid, Solid},
    m_gradientMatrix{},
    m_gradientD1{0.0, 0.0},
    m_gradientD2{100.0, 100.0},

    m_imageFilter(Bilinear),
    m_imageResample(NoResample),
    m_imageFilterLut(agg::image_filter_bilinear(), true),

    m_gradientInterpolator{m_gradientMatrix[0], m_gradientMatrix[1]},
    m_linearGradientFunction(),
    m_radialGradientFunction(),

    m_lineWidth(1),
    m_evenOddFlag(false),

    m_path(),
    m_transform(),

    m_convCurve(m_path),
    m_convStroke(m_convCurve),

    m_pathTransform(m_convCurve, m_transform),
    m_strokeTransform(m_convStroke, m_transform)
{
    lineCap(m_lineCap);
    lineJoin(m_lineJoin);
}


//------------------------------------------------------------------------
void Agg2D::attach(unsigned char* buf, unsigned width, unsigned height, int stride)
{
    m_rbuf.attach(buf, width, height, stride);
    m_renBasePre.reset_clipping(true);
    m_renBaseCompPre.reset_clipping(true);

    resetTransformations();
    lineWidth(1.0),
    lineColor(0,0,0);
    fillColor(255,255,255);
    clipBox(0, 0, width, height);
    lineCap(CapRound);
    lineJoin(JoinRound);
    imageFilter(Bilinear);
    imageResample(NoResample);
    m_masterAlpha = 1.0;
    m_blendMode = BlendSrcOver;
}


//------------------------------------------------------------------------
void Agg2D::attach(Image& img)
{
    attach(img.renBuf.buf(), img.renBuf.width(), img.renBuf.height(), img.renBuf.stride());
}

//------------------------------------------------------------------------
void Agg2D::clipBox(double x1, double y1, double x2, double y2)
{
    m_clipBox = RectD(x1, y1, x2, y2);
    int rx1 = int(x1);
    int ry1 = int(y1);
    int rx2 = int(x2);
    int ry2 = int(y2);

    m_renBasePre.clip_box(rx1, ry1, rx2, ry2);
    m_renBaseCompPre.clip_box(rx1, ry1, rx2, ry2);

    m_rasterizer.clip_box(x1, y1, x2, y2);
}

//------------------------------------------------------------------------
void Agg2D::blendMode(BlendMode m)
{
    m_blendMode = m;
    m_pixFormatCompPre.comp_op(m);
}

//------------------------------------------------------------------------
Agg2D::BlendMode Agg2D::blendMode() const
{
    return m_blendMode;
}

//------------------------------------------------------------------------
void Agg2D::imageBlendMode(BlendMode m)
{
    m_imageBlendMode = m;
}

//------------------------------------------------------------------------
Agg2D::BlendMode Agg2D::imageBlendMode() const
{
    return m_imageBlendMode;
}

//------------------------------------------------------------------------
void Agg2D::imageBlendColor(Color c)
{
    m_imageBlendColor = c;
}

//------------------------------------------------------------------------
void Agg2D::imageBlendColor(unsigned r, unsigned g, unsigned b, unsigned a)
{
    imageBlendColor(Color(r, g, b, a));
}

//------------------------------------------------------------------------
Agg2D::Color Agg2D::imageBlendColor() const
{
    return m_imageBlendColor;
}

//------------------------------------------------------------------------
void Agg2D::masterAlpha(double a)
{
    m_masterAlpha = a;
}

//------------------------------------------------------------------------
double Agg2D::masterAlpha() const
{
    return m_masterAlpha;
}

//------------------------------------------------------------------------
Agg2D::RectD Agg2D::clipBox() const
{
    return m_clipBox;
}

//------------------------------------------------------------------------
void Agg2D::clearAll(Color c)
{
    c.premultiply();
    m_renBasePre.fill(c);
}

//------------------------------------------------------------------------
void Agg2D::clearAll(unsigned r, unsigned g, unsigned b, unsigned a)
{
    clearAll(Color(r, g, b, a));
}

//------------------------------------------------------------------------
void Agg2D::worldToScreen(double& x, double& y) const
{
    m_transform.transform(&x, &y);
}

//------------------------------------------------------------------------
void Agg2D::screenToWorld(double& x, double& y) const
{
    m_transform.inverse_transform(&x, &y);
}


//------------------------------------------------------------------------
double Agg2D::worldToScreen(double scalar) const
{
    double x1 = 0;
    double y1 = 0;
    double x2 = scalar;
    double y2 = scalar;
    worldToScreen(x1, y1);
    worldToScreen(x2, y2);
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) * 0.7071068;
}


//------------------------------------------------------------------------
double Agg2D::screenToWorld(double scalar) const
{
    double x1 = 0;
    double y1 = 0;
    double x2 = scalar;
    double y2 = scalar;
    screenToWorld(x1, y1);
    screenToWorld(x2, y2);
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)) * 0.7071068;
}


//------------------------------------------------------------------------
void Agg2D::alignPoint(double& x, double& y) const
{
    worldToScreen(x, y);
    x = floor(x) + 0.5;
    y = floor(y) + 0.5;
    screenToWorld(x, y);
}


//------------------------------------------------------------------------
bool Agg2D::inBox(double worldX, double worldY) const
{
    worldToScreen(worldX, worldY);
    return m_renBasePre.inbox(int(worldX), int(worldY));
}


//------------------------------------------------------------------------
Agg2D::Transformations Agg2D::transformations() const
{
    Transformations tr;
    m_transform.store_to(tr.affineMatrix);
    return tr;
}


//------------------------------------------------------------------------
void Agg2D::transformations(const Transformations& tr)
{
    m_transform.load_from(tr.affineMatrix);
    m_convCurve.approximation_scale(worldToScreen(1.0) * g_approxScale);
    m_convStroke.approximation_scale(worldToScreen(1.0) * g_approxScale);
}


//------------------------------------------------------------------------
void Agg2D::resetTransformations()
{
    m_transform.reset();
}


//------------------------------------------------------------------------
void Agg2D::rotate(double angle)          { m_transform *= agg::trans_affine_rotation(angle);   }
void Agg2D::skew(double sx, double sy)    { m_transform *= agg::trans_affine_skewing(sx, sy);   }
void Agg2D::translate(double x, double y) { m_transform *= agg::trans_affine_translation(x, y); }


//------------------------------------------------------------------------
void Agg2D::affine(const Affine& tr)
{
    m_transform *= tr;
    m_convCurve.approximation_scale(worldToScreen(1.0) * g_approxScale);
    m_convStroke.approximation_scale(worldToScreen(1.0) * g_approxScale);
}

//------------------------------------------------------------------------
void Agg2D::affine(const Transformations& tr)
{
    affine(agg::trans_affine(tr.affineMatrix[0], tr.affineMatrix[1], tr.affineMatrix[2],
                             tr.affineMatrix[3], tr.affineMatrix[4], tr.affineMatrix[5]));
}

//------------------------------------------------------------------------
void Agg2D::scale(double sx, double sy)
{
    m_transform *= agg::trans_affine_scaling(sx, sy);
    m_convCurve.approximation_scale(worldToScreen(1.0) * g_approxScale);
    m_convStroke.approximation_scale(worldToScreen(1.0) * g_approxScale);
}

//------------------------------------------------------------------------
void Agg2D::fillColor(Color c)
{
    m_color[FillSlot] = c;
    m_color[FillSlot].premultiply();
    m_styleFlag[FillSlot] = Solid;
}

//------------------------------------------------------------------------
void Agg2D::fillColor(unsigned r, unsigned g, unsigned b, unsigned a)
{
    fillColor(Color(r, g, b, a));
}

//------------------------------------------------------------------------
void Agg2D::noFill()
{
    m_color[FillSlot] = Color(0, 0, 0, 0);
    m_styleFlag[FillSlot] = None;
}

//------------------------------------------------------------------------
void Agg2D::lineColor(Color c)
{
    m_color[StrokeSlot] = c;
    m_color[StrokeSlot].premultiply();
    m_styleFlag[StrokeSlot] = Solid;
}

//------------------------------------------------------------------------
void Agg2D::lineColor(unsigned r, unsigned g, unsigned b, unsigned a)
{
    lineColor(Color(r, g, b, a));
}

//------------------------------------------------------------------------
void Agg2D::noLine()
{
    m_color[StrokeSlot] = Color(0, 0, 0, 0);
    m_styleFlag[StrokeSlot] = None;
}

//------------------------------------------------------------------------
void Agg2D::setLinearGradient(StyleSlot slot, double x1, double y1, double x2, double y2, Color c1, Color c2, Color c3)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        m_gradient[slot][i] = c1.gradient(c2, double(i) / 127.0);
        m_gradient[slot][i].premultiply();
    }
    for (; i < 256; i++)
    {
        m_gradient[slot][i] = c2.gradient(c3, double(i - 128) / 127.0);
        m_gradient[slot][i].premultiply();
    }

    double angle = atan2(y2-y1, x2-x1);
    m_gradientMatrix[slot].reset();
    m_gradientMatrix[slot] *= agg::trans_affine_rotation(angle);
    m_gradientMatrix[slot] *= agg::trans_affine_translation(x1, y1);
    m_gradientMatrix[slot] *= m_transform;
    m_gradientMatrix[slot].invert();
    m_gradientD1[slot] = 0.0;
    m_gradientD2[slot] = sqrt((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1));
    m_styleFlag[slot] = Linear;
    m_color[slot] = Color(0,0,0);  // Set some real color
}

void Agg2D::fillLinearGradient(double x1, double y1, double x2, double y2, Color c1, Color c2, Color c3)
{
    setLinearGradient(FillSlot, x1, y1, x2, y2, c1, c2, c3);
}

void Agg2D::lineLinearGradient(double x1, double y1, double x2, double y2, Color c1, Color c2, Color c3)
{
    setLinearGradient(StrokeSlot, x1, y1, x2, y2, c1, c2, c3);
}

void Agg2D::setRadialGradient(StyleSlot slot, double x, double y, double r, Color c1, Color c2, Color c3)
{
    int i;

    for (i = 0; i < 128; i++)
    {
        m_gradient[slot][i] = c1.gradient(c2, double(i) / 127.0);
        m_gradient[slot][i].premultiply();
    }
    for (; i < 256; i++)
    {
        m_gradient[slot][i] = c2.gradient(c3, double(i - 128) / 127.0);
        m_gradient[slot][i].premultiply();
    }

    m_gradientD2[slot] = worldToScreen(r);
    worldToScreen(x, y);
    m_gradientMatrix[slot].reset();
    m_gradientMatrix[slot] *= agg::trans_affine_translation(x, y);
    m_gradientMatrix[slot].invert();
    m_gradientD1[slot] = 0;
    m_styleFlag[slot] = Radial;
    m_color[slot] = Color(0,0,0);  // Set some real color
}

//------------------------------------------------------------------------
void Agg2D::fillRadialGradient(double x, double y, double r, Color c1, Color c2, Color c3)
{
    setRadialGradient(FillSlot, x, y, r, c1, c2, c3);
}

//------------------------------------------------------------------------
void Agg2D::lineRadialGradient(double x, double y, double r, Color c1, Color c2, Color c3)
{
    setRadialGradient(StrokeSlot, x, y, r, c1, c2, c3);
}

//------------------------------------------------------------------------
void Agg2D::lineWidth(double w)
{
    m_lineWidth = w;
    m_convStroke.width(w);
}


//------------------------------------------------------------------------
double Agg2D::lineWidth(double w) const
{
    return m_lineWidth;
}


//------------------------------------------------------------------------
void Agg2D::fillEvenOdd(bool evenOddFlag)
{
    m_evenOddFlag = evenOddFlag;
    m_rasterizer.filling_rule(evenOddFlag ? agg::fill_even_odd : agg::fill_non_zero);
}


//------------------------------------------------------------------------
bool Agg2D::fillEvenOdd() const
{
    return m_evenOddFlag;
}


//------------------------------------------------------------------------
void Agg2D::lineCap(LineCap cap)
{
    m_lineCap = cap;
    m_convStroke.line_cap((agg::line_cap_e)cap);
}


//------------------------------------------------------------------------
Agg2D::LineCap Agg2D::lineCap() const
{
    return m_lineCap;
}


//------------------------------------------------------------------------
void Agg2D::lineJoin(LineJoin join)
{
    m_lineJoin = join;
    m_convStroke.line_join((agg::line_join_e)join);
}


//------------------------------------------------------------------------
Agg2D::LineJoin Agg2D::lineJoin() const
{
    return m_lineJoin;
}


//------------------------------------------------------------------------
void Agg2D::addLine(double x1, double y1, double x2, double y2)
{
    m_path.move_to(x1, y1);
    m_path.line_to(x2, y2);
}

void Agg2D::fillRectangleI(int x1, int y1, int x2, int y2, Color color)
{
    color.premultiply();

    if (m_blendMode == BlendSrc)
    {
        m_renBasePre.copy_bar(x1, y1, x2, y2, color);
    }
    else if (m_blendMode == BlendSrcOver)
    {
        m_renBasePre.blend_bar(x1, y1, x2, y2, color, 0xFF);
    }
    else
    {
        m_renBaseCompPre.blend_bar(x1, y1, x2, y2, color, 0xFF);
    }
}

//------------------------------------------------------------------------
void Agg2D::line(double x1, double y1, double x2, double y2)
{
    m_path.remove_all();
    addLine(x1, y1, x2, y2);
    drawPath(StrokeOnly);
}


//------------------------------------------------------------------------
void Agg2D::triangle(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_path.remove_all();
    m_path.move_to(x1, y1);
    m_path.line_to(x2, y2);
    m_path.line_to(x3, y3);
    m_path.close_polygon();
    drawPath(FillAndStroke);
}


void Agg2D::rectangle(double x1, double y1, double x2, double y2)
{
    m_path.remove_all();
    m_path.move_to(x1, y1);
    m_path.line_to(x2, y1);
    m_path.line_to(x2, y2);
    m_path.line_to(x1, y2);
    m_path.close_polygon();
    drawPath(FillAndStroke);
}


//------------------------------------------------------------------------
void Agg2D::roundedRect(double x1, double y1, double x2, double y2, double r)
{
    m_path.remove_all();
    agg::rounded_rect rc(x1, y1, x2, y2, r);
    rc.normalize_radius();
    rc.approximation_scale(worldToScreen(1.0) * g_approxScale);
    m_path.concat_path(rc,0);
    drawPath(FillAndStroke);
}



//------------------------------------------------------------------------
void Agg2D::roundedRect(double x1, double y1, double x2, double y2, double rx, double ry)
{
    m_path.remove_all();
    agg::rounded_rect rc;
    rc.rect(x1, y1, x2, y2);
    rc.radius(rx, ry);
    rc.normalize_radius();
    m_path.concat_path(rc,0);
    drawPath(FillAndStroke);
}



//------------------------------------------------------------------------
void Agg2D::roundedRect(double x1, double y1, double x2, double y2,
                        double rx_bottom, double ry_bottom,
                        double rx_top,    double ry_top)
{
    m_path.remove_all();
    agg::rounded_rect rc;
    rc.rect(x1, y1, x2, y2);
    rc.radius(rx_bottom, ry_bottom, rx_top, ry_top);
    rc.normalize_radius();
    rc.approximation_scale(worldToScreen(1.0) * g_approxScale);
    m_path.concat_path(rc,0);
    drawPath(FillAndStroke);
}


//------------------------------------------------------------------------
void Agg2D::ellipse(double cx, double cy, double rx, double ry)
{
    m_path.remove_all();
    agg::bezier_arc arc(cx, cy, rx, ry, 0, 2*pi());
    m_path.concat_path(arc,0);
    m_path.close_polygon();
    drawPath(FillAndStroke);
}


//------------------------------------------------------------------------
void Agg2D::arc(double cx, double cy, double rx, double ry, double start, double sweep)
{
    m_path.remove_all();
    agg::bezier_arc arc(cx, cy, rx, ry, start, sweep);
    m_path.concat_path(arc,0);
    drawPath(StrokeOnly);
}


//------------------------------------------------------------------------
void Agg2D::star(double cx, double cy, double r1, double r2, double startAngle, int numRays)
{
    m_path.remove_all();
    double da = agg::pi / double(numRays);
    double a = startAngle;
    int i;
    for (i = 0; i < numRays; i++)
    {
        double x = cos(a) * r2 + cx;
        double y = sin(a) * r2 + cy;
        if (i) m_path.line_to(x, y);
        else   m_path.move_to(x, y);
        a += da;
        m_path.line_to(cos(a) * r1 + cx, sin(a) * r1 + cy);
        a += da;
    }
    closePolygon();
    drawPath(FillAndStroke);
}


//------------------------------------------------------------------------
void Agg2D::curve(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_path.remove_all();
    m_path.move_to(x1, y1);
    m_path.curve3(x2, y2, x3, y3);
    drawPath(StrokeOnly);
}


//------------------------------------------------------------------------
void Agg2D::curve(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
    m_path.remove_all();
    m_path.move_to(x1, y1);
    m_path.curve4(x2, y2, x3, y3, x4, y4);
    drawPath(StrokeOnly);
}


//------------------------------------------------------------------------
void Agg2D::polygon(double* xy, int numPoints)
{
    m_path.remove_all();
    m_path.concat_poly(xy,0,true);
    closePolygon();
    drawPath(FillAndStroke);
}


//------------------------------------------------------------------------
void Agg2D::polyline(double* xy, int numPoints)
{
    m_path.remove_all();
    m_path.concat_poly(xy,0,true);
    drawPath(StrokeOnly);
}

//------------------------------------------------------------------------
void Agg2D::resetPath() { m_path.remove_all(); }

//------------------------------------------------------------------------
void Agg2D::moveTo(double x, double y)
{
    m_path.move_to(x, y);
}

//------------------------------------------------------------------------
void Agg2D::moveRel(double dx, double dy)
{
    m_path.move_rel(dx, dy);
}


//------------------------------------------------------------------------
void Agg2D::lineTo(double x, double y)
{
    m_path.line_to(x, y);
}


//------------------------------------------------------------------------
void Agg2D::lineRel(double dx, double dy)
{
    m_path.line_rel(dx, dy);
}


//------------------------------------------------------------------------
void Agg2D::horLineTo(double x)
{
    m_path.hline_to(x);
}


//------------------------------------------------------------------------
void Agg2D::horLineRel(double dx)
{
    m_path.hline_rel(dx);
}


//------------------------------------------------------------------------
void Agg2D::verLineTo(double y)
{
    m_path.vline_to(y);
}


//------------------------------------------------------------------------
void Agg2D::verLineRel(double dy)
{
    m_path.vline_rel(dy);
}


//------------------------------------------------------------------------
void Agg2D::arcTo(double rx, double ry,
                  double angle,
                  bool largeArcFlag,
                  bool sweepFlag,
                  double x, double y)
{
    m_path.arc_to(rx, ry, angle, largeArcFlag, sweepFlag, x, y);
}


//------------------------------------------------------------------------
void Agg2D::arcRel(double rx, double ry,
                   double angle,
                   bool largeArcFlag,
                   bool sweepFlag,
                   double dx, double dy)
{
    m_path.arc_rel(rx, ry, angle, largeArcFlag, sweepFlag, dx, dy);
}


//------------------------------------------------------------------------
void Agg2D::quadricCurveTo(double xCtrl, double yCtrl,
                           double xTo,   double yTo)
{
    m_path.curve3(xCtrl, yCtrl, xTo, yTo);
}


//------------------------------------------------------------------------
void Agg2D::quadricCurveRel(double dxCtrl, double dyCtrl,
                            double dxTo,   double dyTo)
{
    m_path.curve3_rel(dxCtrl, dyCtrl, dxTo, dyTo);
}


//------------------------------------------------------------------------
void Agg2D::quadricCurveTo(double xTo, double yTo)
{
    m_path.curve3(xTo, yTo);
}


//------------------------------------------------------------------------
void Agg2D::quadricCurveRel(double dxTo, double dyTo)
{
    m_path.curve3_rel(dxTo, dyTo);
}


//------------------------------------------------------------------------
void Agg2D::cubicCurveTo(double xCtrl1, double yCtrl1,
                         double xCtrl2, double yCtrl2,
                         double xTo,    double yTo)
{
    m_path.curve4(xCtrl1, yCtrl1, xCtrl2, yCtrl2, xTo, yTo);
}


//------------------------------------------------------------------------
void Agg2D::cubicCurveRel(double dxCtrl1, double dyCtrl1,
                          double dxCtrl2, double dyCtrl2,
                          double dxTo,    double dyTo)
{
    m_path.curve4_rel(dxCtrl1, dyCtrl1, dxCtrl2, dyCtrl2, dxTo, dyTo);
}


//------------------------------------------------------------------------
void Agg2D::cubicCurveTo(double xCtrl2, double yCtrl2,
                         double xTo,    double yTo)
{
    m_path.curve4(xCtrl2, yCtrl2, xTo, yTo);
}


//------------------------------------------------------------------------
void Agg2D::cubicCurveRel(double xCtrl2, double yCtrl2,
                          double xTo,    double yTo)
{
    m_path.curve4_rel(xCtrl2, yCtrl2, xTo, yTo);
}

//------------------------------------------------------------------------
void Agg2D::addEllipse(double cx, double cy, double rx, double ry, Direction dir)
{
    agg::bezier_arc arc(cx, cy, rx, ry, 0, (dir == CCW) ? 2*pi() : -2*pi());
    m_path.concat_path(arc,0);
    m_path.close_polygon();
}

//------------------------------------------------------------------------
void Agg2D::closePolygon()
{
   m_path.close_polygon();
}


//------------------------------------------------------------------------
void Agg2D::imageFilter(ImageFilter f)
{
    m_imageFilter = f;
    switch(f)
    {
        case NoFilter: break;
        case Bilinear: m_imageFilterLut.calculate(agg::image_filter_bilinear(), true); break;
    }
}


//------------------------------------------------------------------------
Agg2D::ImageFilter Agg2D::imageFilter() const
{
    return m_imageFilter;
}


//------------------------------------------------------------------------
void Agg2D::imageResample(ImageResample f)
{
    m_imageResample = f;
}


//------------------------------------------------------------------------
Agg2D::ImageResample Agg2D::imageResample() const
{
    return m_imageResample;
}


//------------------------------------------------------------------------
void Agg2D::transformImage(const Image& img,    int imgX1,    int imgY1,    int imgX2,    int imgY2,
                                             double dstX1, double dstY1, double dstX2, double dstY2)
{
    resetPath();
    moveTo(dstX1, dstY1);
    lineTo(dstX2, dstY1);
    lineTo(dstX2, dstY2);
    lineTo(dstX1, dstY2);
    closePolygon();
    double parallelogram[6] = { dstX1, dstY1, dstX2, dstY1, dstX2, dstY2 };
    renderImage(img, imgX1, imgY1, imgX2, imgY2, parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImage(const Image& img, double dstX1, double dstY1, double dstX2, double dstY2)
{
    resetPath();
    moveTo(dstX1, dstY1);
    lineTo(dstX2, dstY1);
    lineTo(dstX2, dstY2);
    lineTo(dstX1, dstY2);
    closePolygon();
    double parallelogram[6] = { dstX1, dstY1, dstX2, dstY1, dstX2, dstY2 };
    renderImage(img, 0, 0, img.renBuf.width(), img.renBuf.height(), parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImage(const Image& img, int imgX1, int imgY1, int imgX2, int imgY2,
                           const double* parallelogram)
{
    resetPath();
    moveTo(parallelogram[0], parallelogram[1]);
    lineTo(parallelogram[2], parallelogram[3]);
    lineTo(parallelogram[4], parallelogram[5]);
    lineTo(parallelogram[0] + parallelogram[4] - parallelogram[2],
           parallelogram[1] + parallelogram[5] - parallelogram[3]);
    closePolygon();
    renderImage(img, imgX1, imgY1, imgX2, imgY2, parallelogram);
}


//------------------------------------------------------------------------
void Agg2D::transformImage(const Image& img, const double* parallelogram)
{
    resetPath();
    moveTo(parallelogram[0], parallelogram[1]);
    lineTo(parallelogram[2], parallelogram[3]);
    lineTo(parallelogram[4], parallelogram[5]);
    lineTo(parallelogram[0] + parallelogram[4] - parallelogram[2],
           parallelogram[1] + parallelogram[5] - parallelogram[3]);
    closePolygon();
    renderImage(img, 0, 0, img.renBuf.width(), img.renBuf.height(), parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImagePath(const Image& img,    int imgX1,    int imgY1,    int imgX2,    int imgY2,
                                                 double dstX1, double dstY1, double dstX2, double dstY2)
{
    double parallelogram[6] = { dstX1, dstY1, dstX2, dstY1, dstX2, dstY2 };
    renderImage(img, imgX1, imgY1, imgX2, imgY2, parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImagePath(const Image& img, double dstX1, double dstY1, double dstX2, double dstY2)
{
    double parallelogram[6] = { dstX1, dstY1, dstX2, dstY1, dstX2, dstY2 };
    renderImage(img, 0, 0, img.renBuf.width(), img.renBuf.height(), parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImagePath(const Image& img, int imgX1, int imgY1, int imgX2, int imgY2,
                               const double* parallelogram)
{
    renderImage(img, imgX1, imgY1, imgX2, imgY2, parallelogram);
}

//------------------------------------------------------------------------
void Agg2D::transformImagePath(const Image& img, const double* parallelogram)
{
    renderImage(img, 0, 0, img.renBuf.width(), img.renBuf.height(), parallelogram);
}



//------------------------------------------------------------------------
void Agg2D::drawPath(DrawPathFlag flag)
{
    m_rasterizer.reset();
    switch(flag)
    {
    case FillOnly:
        if (m_styleFlag[FillSlot] != None)
        {
            m_rasterizer.add_path(m_pathTransform);
            render(FillSlot);
        }
        break;

    case StrokeOnly:
        if (m_styleFlag[StrokeSlot] != None && m_lineWidth > 0.0)
        {
            m_rasterizer.add_path(m_strokeTransform);
            render(StrokeSlot);
        }
        break;

    case FillAndStroke:
        if (m_styleFlag[FillSlot] != None)
        {
            m_rasterizer.add_path(m_pathTransform);
            render(FillSlot);
        }

        if (m_styleFlag[StrokeSlot] != None && m_lineWidth > 0.0)
        {
            m_rasterizer.add_path(m_strokeTransform);
            render(StrokeSlot);
        }
        break;

    case FillWithLineColor:
        if (m_styleFlag[StrokeSlot] != None)
        {
            m_rasterizer.add_path(m_pathTransform);
            render(StrokeSlot);
        }
        break;
    }
}



//------------------------------------------------------------------------
class Agg2DRenderer
{
public:
    //--------------------------------------------------------------------
    template<class BaseRenderer, class SolidRenderer>
    void static render(Agg2D& gr, BaseRenderer& renBase, SolidRenderer& renSolid, Agg2D::StyleSlot slot)
    {
        typedef agg::span_allocator<agg::rgba8> span_allocator_type;
        typedef agg::renderer_scanline_aa<BaseRenderer, span_allocator_type, Agg2D::LinearGradientSpan> RendererLinearGradient;
        typedef agg::renderer_scanline_aa<BaseRenderer, span_allocator_type, Agg2D::RadialGradientSpan> RendererRadialGradient;

        switch (gr.m_styleFlag[slot])
        {
            case Agg2D::None: {
                return;
            }

            case Agg2D::Solid: {
                renSolid.color(gr.m_color[slot]);
                agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, renSolid);
                return;
            }

            case Agg2D::Linear: {
                Agg2D::LinearGradientSpan span(/*gr.m_allocator, */
                                               gr.m_gradientInterpolator[slot],
                                               gr.m_linearGradientFunction,
                                               gr.m_gradient[slot],
                                               gr.m_gradientD1[slot],
                                               gr.m_gradientD2[slot]);
                RendererLinearGradient ren(renBase,gr.m_allocator,span);
                agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, ren);
                return;
            }

            case Agg2D::Radial: {
                Agg2D::RadialGradientSpan span(/*gr.m_allocator, */
                                                gr.m_gradientInterpolator[slot],
                                                gr.m_radialGradientFunction,
                                                gr.m_gradient[slot],
                                                gr.m_gradientD1[slot],
                                                gr.m_gradientD2[slot]);
                RendererRadialGradient ren(renBase,gr.m_allocator,span);
                agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, ren);
                return;
            }
        }
    }


    //--------------------------------------------------------------------
    class SpanConvImageBlend
    {
    public:
        SpanConvImageBlend(Agg2D::BlendMode m, Agg2D::Color c) :
            m_mode(m), m_color(c)
        {}

        void convert(Agg2D::Color* span, int x, int y, unsigned len) const
        {
            unsigned l2;
            Agg2D::Color* s2;
            if(m_mode != Agg2D::BlendDst)
            {
                l2 = len;
                s2 = span;
                typedef agg::comp_op_adaptor_clip_to_dst_rgba_pre<Agg2D::Color, agg::order_rgba> OpType;
                do
                {
                    OpType::blend_pix(m_mode,
                                      (Agg2D::Color::value_type*)s2,
                                      m_color.r,
                                      m_color.g,
                                      m_color.b,
                                      Agg2D::Color::base_mask,
                                      agg::cover_full);
                    ++s2;
                }
                while(--l2);
            }
            if(m_color.a < Agg2D::Color::base_mask)
            {
                l2 = len;
                s2 = span;
                unsigned a = m_color.a;
                do
                {
                    s2->r = (s2->r * a) >> Agg2D::Color::base_shift;
                    s2->g = (s2->g * a) >> Agg2D::Color::base_shift;
                    s2->b = (s2->b * a) >> Agg2D::Color::base_shift;
                    s2->a = (s2->a * a) >> Agg2D::Color::base_shift;
                    ++s2;
                }
                while(--l2);
            }
        }

    private:
        Agg2D::BlendMode m_mode;
        Agg2D::Color     m_color;
    };




    //--------------------------------------------------------------------
    template<class BaseRenderer, class SolidRenderer, class Rasterizer, class Scanline>
    void static render(Agg2D& gr, BaseRenderer& renBase, SolidRenderer& renSolid, Rasterizer& ras, Scanline& sl)
    {
        typedef agg::span_allocator<agg::rgba8> span_allocator_type;
        typedef agg::renderer_scanline_aa<BaseRenderer,span_allocator_type,Agg2D::LinearGradientSpan> RendererLinearGradient;
        typedef agg::renderer_scanline_aa<BaseRenderer,span_allocator_type,Agg2D::RadialGradientSpan> RendererRadialGradient;

        int slot = 0;

        switch (gr.m_styleFlag[slot])
        {
            case Agg2D::None: {
                return;
            }

            case Agg2D::Solid: {
                renSolid.color(gr.m_color[slot]);
                agg::render_scanlines(ras, sl, renSolid);
                return;
            }

            case Agg2D::Linear: {
                Agg2D::LinearGradientSpan span(gr.m_gradientInterpolator[slot],
                                            gr.m_linearGradientFunction,
                                            gr.m_gradient[slot],
                                            gr.m_gradientD1[slot],
                                            gr.m_gradientD2[slot]);
                RendererLinearGradient ren(renBase,gr.m_allocator,span);
                agg::render_scanlines(ras, sl, ren);
                return;
            }

            case Agg2D::Radial: {
                Agg2D::RadialGradientSpan span(gr.m_gradientInterpolator[slot],
                                               gr.m_radialGradientFunction,
                                               gr.m_gradient[slot],
                                               gr.m_gradientD1[slot],
                                               gr.m_gradientD2[slot]);
                RendererRadialGradient ren(renBase,gr.m_allocator,span);
                agg::render_scanlines(ras, sl, ren);
                return;
            }
        }
    }

    //--------------------------------------------------------------------
    template<class BaseRenderer, class Interpolator>
    static void renderImage(Agg2D& gr, const Agg2D::Image& img,
                            BaseRenderer& renBase, Interpolator& interpolator)
    {
        Agg2D::Image& imgc = const_cast<Agg2D::Image&>(img);
        Agg2D::PixFormatPre img_pixf(imgc.renBuf);
        typedef agg::image_accessor_clone<Agg2D::PixFormatPre> img_source_type;
        img_source_type source(img_pixf);

        SpanConvImageBlend blend(gr.m_imageBlendMode, gr.m_imageBlendColor);
        if (gr.m_imageFilter == Agg2D::NoFilter)
        {

            typedef agg::span_image_filter_rgba_nn<img_source_type,Interpolator> SpanGenType;
            typedef agg::span_converter<SpanGenType,SpanConvImageBlend> SpanConvType;
            typedef agg::renderer_scanline_aa<BaseRenderer,Agg2D::SpanAllocator,SpanGenType> RendererType;

            SpanGenType sg(source,interpolator);
            SpanConvType sc(sg, blend);
            RendererType ri(renBase,gr.m_allocator,sg);
            agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, ri);
        }
        else
        {
            bool resample = (gr.m_imageResample == Agg2D::ResampleAlways);
            if(gr.m_imageResample == Agg2D::ResampleOnZoomOut)
            {
                double sx, sy;
                interpolator.transformer().scaling_abs(&sx,&sy);
                if (sx > 1.125 || sy > 1.125)
                {
                    resample = true;
                }
            }

            if (resample)
            {
                typedef agg::span_image_resample_rgba_affine<img_source_type> SpanGenType;
                typedef agg::span_converter<SpanGenType,SpanConvImageBlend> SpanConvType;
                typedef agg::renderer_scanline_aa<BaseRenderer,Agg2D::SpanAllocator,SpanGenType> RendererType;

                SpanGenType sg(source,interpolator,gr.m_imageFilterLut);
                SpanConvType sc(sg, blend);
                RendererType ri(renBase,gr.m_allocator,sg);
                agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, ri);
            }
            else
            {
                typedef agg::span_image_filter_rgba_bilinear<img_source_type,Interpolator> SpanGenType;
                typedef agg::span_converter<SpanGenType,SpanConvImageBlend> SpanConvType;
                typedef agg::renderer_scanline_aa<BaseRenderer,Agg2D::SpanAllocator,SpanGenType> RendererType;

                SpanGenType sg(source,interpolator);
                SpanConvType sc(sg, blend);
                RendererType ri(renBase,gr.m_allocator,sg);
                agg::render_scanlines(gr.m_rasterizer, gr.m_scanline, ri);
            }
        }
    }
};


//------------------------------------------------------------------------
void Agg2D::render(StyleSlot slot)
{
    if (m_blendMode == BlendSrcOver)
        Agg2DRenderer::render(*this, m_renBasePre, m_renSolidComp, slot);
    else
        Agg2DRenderer::render(*this, m_renBaseCompPre, m_renSolidComp, slot);
}

//------------------------------------------------------------------------
void Agg2D::renderImage(const Image& img, int x1, int y1, int x2, int y2,
                        const double* parl)
{
    agg::trans_affine mtx((double)x1,
                          (double)y1,
                          (double)x2,
                          (double)y2,
                          parl);
    mtx *= m_transform;
    mtx.invert();

    m_rasterizer.reset();
    m_rasterizer.add_path(m_pathTransform);

    typedef agg::span_interpolator_linear<agg::trans_affine> Interpolator;
    Interpolator interpolator(mtx);

    Agg2DRenderer::renderImage(*this,img, m_renBaseCompPre, interpolator);
}

//------------------------------------------------------------------------
void Agg2D::blendImage(Image& img,
                       int imgX1, int imgY1, int imgX2, int imgY2,
                       double dstX, double dstY, unsigned alpha)
{
    worldToScreen(dstX, dstY);
    PixFormatPre pixF(img.renBuf);
    Rect r(imgX1, imgY1, imgX2, imgY2);
    m_renBaseCompPre.blend_from(pixF, &r, int(dstX)-imgX1, int(dstY)-imgY1, alpha);
}


//------------------------------------------------------------------------
void Agg2D::blendImage(Image& img, double dstX, double dstY, unsigned alpha)
{
    worldToScreen(dstX, dstY);
    PixFormatPre pixF(img.renBuf);
    m_renBasePre.blend_from(pixF, 0, int(dstX), int(dstY), alpha);
    m_renBaseCompPre.blend_from(pixF, 0, int(dstX), int(dstY), alpha);
}


//------------------------------------------------------------------------
void Agg2D::copyImage(Image& img,
                      int imgX1, int imgY1, int imgX2, int imgY2,
                      double dstX, double dstY)
{
    worldToScreen(dstX, dstY);
    Rect r(imgX1, imgY1, imgX2, imgY2);
    m_renBasePre.copy_from(img.renBuf, &r, int(dstX)-imgX1, int(dstY)-imgY1);
}

//------------------------------------------------------------------------
void Agg2D::copyImage(Image& img, double dstX, double dstY)
{
    worldToScreen(dstX, dstY);
    m_renBasePre.copy_from(img.renBuf, 0, int(dstX), int(dstY));
}
