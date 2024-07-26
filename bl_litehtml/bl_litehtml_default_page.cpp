static const char default_page_content[] = R"EOF(
<html>

<head>
<style>

body {
  margin: 0;
  padding: 10px;
  background-color: #000;
  color: #fff;
  font-size: 120%;
}

body > div {
}

h1, h2 {
  padding: 3px;
  margin: 0;
  background: #222;
  border-radius: 5px;
  font-size: 150%;
}

h2 {
  background: none;
  border: none;
  font-size: 125%;
}

.gallery {
  text-align: justify;
  font-size: 1px;
}

.gallery > div {
  display: inline-block;
  margin: 8px 8px 8px 0;
  width: 324px;
  height: 324px;
}
.gallery > div:last-child { margin-right: 0; }

a { color: #FFAF00; }
a:hover { color: #FFFFFF; }

</style>
</head>

<body>
<div>

<h1>Blend2D</h1>

<p>Blend2D is a high performance 2D vector graphics engine written in C++ and released under the Zlib license.
The engine utilizes a built-in JIT compiler to generate optimized pipelines at runtime and is capable of using
multiple threads to boost the performance beyond the possibilities of single-threaded rendering. Additionally,
the engine features a new rasterizer that has been written from scratch. It delivers superior performance while
quality is comparable to rasterizers used by AGG, FreeType, and Qt. The performance has been optimized by using
an innovative approach to index data that is built during rasterization and scanned during composition. The
rasterizer is robust and excels in rendering basic shapes, complex vector art, and text.</p>

<h2>Easy to Use API</h2>

<p>Blend2D is written in C++ but it provides both C and C++ APIs. Public functionality is provided through C
interface so that each feature of the library is accessible to both C and C++ users. The C API makes it possible
to use Blend2D from many programming languages which are able to interface with C (either natively or through
FFI). The primary goal of the C++ API is to make the library easy-to-use in C++ projects without the need of
managing resources manually. It is built on top of the C API and turns all objects requiring initialization and
finalization into smart objects that handle it in their constructors and destructors.</p>

<h2>Gradients Example</h2>

<div class="gallery">
  <div style="background: linear-gradient(45deg, #FFFFFF, #FF9F00, #FF0000);"></div>
  <div style="background: radial-gradient(ellipse at center, #FFFFFF, #FF9F00, #FF0000);"></div>
  <div style="background: conic-gradient(#FFFFFF, #FF9F00, #FF0000);"></div>
</div>

<h2>Blend2D History</h2>

<p>The Blend2D project started in 2015, as an experiment to use a JIT compiler for generating 2D pipelines. The
initial prototype did not use any SIMD and was already faster than both Qt and Cairo in composition of the
output from the rasterizer. It was probably not just about the pipeline itself as the rasterizer was already
optimized. However, the rasterizer was much worse than the one used today. The prototype was initially written
for fun and without any intentions to make a library out of it. But the performance was so impressive that it
built the foundation of Blend2D.</p>

<p>Later, when gradients, textures, and advanced composition operators were implemented, it was obvious that in
terms of performance Blend2D competes very well with other 2D renderers while keeping the same quality. Yet it
was uncertain which features the library should provide in the first place. Because of the high complexity with
text rendering in general, the initial idea was to only create a library that is able to render vector paths.
The users would simply pass such paths to render text. Finally, it became clear that a 2D framework without
native text support is not practical, so the idea of having basic support for TTF and OTF fonts has been
explored and finally implemented.</p>

<p>After basic text rendering, two more ideas were pursued: Faster rasterization and a better stroking engine
that would be able to offset curves without flattening. The work on the rasterizer started in late 2017 and
took around 3 months to complete, with some extra time to stabilize. Around 20 variations of different
rasterizers have been implemented and benchmarked so that the best approach for Blend2D was chosen and further
refined. Research into the new stroking engine started in 2018 and also required a few months of studying and
experimenting before the initial prototype was implemented. Although not considered initially, because of
positive results, the new stroking engine has been added to the beta release of Blend2D.</p>

<p>In the early 2020 a work on multi-threaded rendering context has begun. It took approximately 3 months to
design and implement the initial prototype that was on par with features provided by the single-threaded
rendering context. It was committed to master branch in April 2020 and released as a part of beta13 release.
Blend2D was most likely the first open-source 2D vector graphics engine that offered a multi-threaded /
asynchronous rendering.</p>

<h1>LiteHtml</h1>

<p>litehtml is a lightweight HTML rendering engine with CSS2/CSS3 support. Note that litehtml itself does not
draw any text, pictures or other graphics and that litehtml does not depend on any image/draw/font library.
You are free to use any library to draw images, fonts and any other graphics. litehtml just parses HTML/CSS
and places the HTML elements into the correct positions (renders HTML). To draw the HTML elements you have to
implement the simple callback interface document_container. This interface is really simple, check it out! The
document_container implementation is required to render HTML correctly.</p>

</div>
</body>
</html>
)EOF";

const char* get_default_page_content() { return default_page_content; }
