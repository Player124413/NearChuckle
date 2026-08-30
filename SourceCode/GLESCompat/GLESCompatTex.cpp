// =============================================================================
//   GLESCompat texture module: texture objects, format translation
//   (desktop internal formats -> ES3), DXT software decode, luminance
//   emulation via swizzle, RECT/1D target emulation, FBO-based readback.
// =============================================================================
#include "GLESCompat_Impl.h"
namespace glescompat { bool DiagEnabled(); extern int g_nEsMajor; }
static inline bool DiagOn() { return glescompat::DiagEnabled(); }

namespace glescompat {

#define GL_TEXTURE_RECTANGLE_ARB_E 0x84F5
#define GL_TEXTURE_RECTANGLE_NV_E 0x845D
#define GL_PROXY_TEXTURE_RECTANGLE_E 0x84F7
#define GL_ABGR_EXT_E 0x8000

int g_nActiveUnit = 0;
int g_nClientUnit = 0;
STexUnit g_TexUnit[GC_MAX_UNITS];
static std::map<GLuint, STexObj> g_TexObjs;
static bool g_bTexInit = false;

// extra ES procs for this module
typedef void (APIENTRY *PFN_glTexParameteriv_t)(GLenum, GLenum, const GLint *);
static PFN_glTexParameteriv_t tp_glTexParameteriv = 0;
typedef void (APIENTRY *PFN_glGetTexLevelParameteriv_t)(GLenum, GLint, GLenum, GLint *);
static PFN_glGetTexLevelParameteriv_t tp_glGetTexLevelParameteriv = 0;

void TexInit(void)
{
  if (g_bTexInit)
    return;
  g_bTexInit = true;
  tp_glTexParameteriv = (PFN_glTexParameteriv_t)SDL_GL_GetProcAddress("glTexParameteriv");
  tp_glGetTexLevelParameteriv = (PFN_glGetTexLevelParameteriv_t)SDL_GL_GetProcAddress("glGetTexLevelParameteriv");
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    g_TexUnit[u].bEnabled2D = false;
    g_TexUnit[u].id2D = 0;
    g_TexUnit[u].env.Reset();
  }
}

void TexShutdown(void) { g_TexObjs.clear(); }

void STexEnv::Reset()
{
  mode = GL_MODULATE;
  color[0] = color[1] = color[2] = color[3] = 0;
  combineRGB = GL_MODULATE;
  combineA = GL_MODULATE;
  srcRGB[0] = GL_TEXTURE; srcRGB[1] = GL_PREVIOUS; srcRGB[2] = GL_CONSTANT;
  srcA[0] = GL_TEXTURE; srcA[1] = GL_PREVIOUS; srcA[2] = GL_CONSTANT;
  opRGB[0] = opRGB[1] = opRGB[2] = 0x1E01; // GL_SRC_COLOR
  opA[0] = opA[1] = opA[2] = 0x1E01;
  rgbScale = 1;
  alphaScale = 1;
}

STexObj *TexGet(GLuint name)
{
  if (!name)
    return 0;
  std::map<GLuint, STexObj>::iterator it = g_TexObjs.find(name);
  return it == g_TexObjs.end() ? 0 : &it->second;
}

void TexDelete(GLsizei n, const GLuint *names)
{
  // engine names may map to different real ES names (create-on-bind)
  if (es_glDeleteTextures)
  {
    for (int i = 0; i < n; i++)
    {
      std::map<GLuint, STexObj>::iterator it = g_TexObjs.find(names[i]);
      if (it != g_TexObjs.end() && it->second.es != names[i])
        es_glDeleteTextures(1, &it->second.es);
    }
    es_glDeleteTextures(n, names); // passthrough names (es == engine name)
  }
  for (int i = 0; i < n; i++)
    g_TexObjs.erase(names[i]);
}

// swizzle modes for emulated desktop formats
enum ESwiz { SW_NONE = 0, SW_LUM, SW_LA, SW_INT, SW_ALPHA };
static void ApplySwizzle(GLenum esTarget, int swiz)
{
  if (!tp_glTexParameteriv)
    return;
  GLint sw[4];
  const GLint R = 0x1903, G = 0x1904, B = 0x1905, A = 0x1906, ZERO = 0, ONE = 1;
  switch (swiz)
  {
  case SW_LUM: sw[0] = R; sw[1] = R; sw[2] = R; sw[3] = ONE; break;
  case SW_LA: sw[0] = R; sw[1] = R; sw[2] = R; sw[3] = G; break;
  case SW_INT: sw[0] = R; sw[1] = R; sw[2] = R; sw[3] = R; break;
  case SW_ALPHA: sw[0] = ZERO; sw[1] = ZERO; sw[2] = ZERO; sw[3] = R; break;
  default: return;
  }
  tp_glTexParameteriv(esTarget, 0x8E3D /*GL_TEXTURE_SWIZZLE_RGBA*/, sw);
}

// ---------------------------------------------------------------------------
// DXT decode
// ---------------------------------------------------------------------------
static void DecodeColorBlock(const unsigned char *blk, int w, int h,
                             unsigned char *out, int pitch, bool bThreeBitAlpha)
{
  unsigned short c0 = (unsigned short)(blk[0] | (blk[1] << 8));
  unsigned short c1 = (unsigned short)(blk[2] | (blk[3] << 8));
  unsigned char cols[4][4];
  cols[0][0] = (unsigned char)((c0 >> 11) & 31) * 255 / 31;
  cols[0][1] = (unsigned char)((c0 >> 5) & 63) * 255 / 63;
  cols[0][2] = (unsigned char)(c0 & 31) * 255 / 31;
  cols[0][3] = 255;
  cols[1][0] = (unsigned char)((c1 >> 11) & 31) * 255 / 31;
  cols[1][1] = (unsigned char)((c1 >> 5) & 63) * 255 / 63;
  cols[1][2] = (unsigned char)(c1 & 31) * 255 / 31;
  cols[1][3] = 255;
  if (!bThreeBitAlpha && c0 > c1)
  {
    for (int i = 0; i < 3; i++)
      cols[2][i] = (unsigned char)((2 * cols[0][i] + cols[1][i]) / 3);
    cols[2][3] = 255;
    for (int i = 0; i < 3; i++)
      cols[3][i] = (unsigned char)((cols[0][i] + 2 * cols[1][i]) / 3);
    cols[3][3] = 255;
  }
  else
  {
    for (int i = 0; i < 3; i++)
      cols[2][i] = (unsigned char)((cols[0][i] + cols[1][i]) / 2);
    cols[2][3] = 255;
    cols[3][0] = cols[3][1] = cols[3][2] = 0;
    cols[3][3] = 0;
  }
  unsigned int bits = (unsigned int)blk[4] | ((unsigned int)blk[5] << 8) |
                      ((unsigned int)blk[6] << 16) | ((unsigned int)blk[7] << 24);
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++)
    {
      int ci = (bits >> (2 * (y * 4 + x))) & 3;
      if (x < w && y < h)
      {
        unsigned char *o = out + y * pitch + x * 4;
        o[0] = cols[ci][2]; // DXT stores BG - swap to RGB
        o[1] = cols[ci][1];
        o[2] = cols[ci][0];
        o[3] = cols[ci][3];
      }
    }
}

static void DecodeDXT1(const unsigned char *data, int w, int h, unsigned char *out)
{
  int bw = (w + 3) / 4, bh = (h + 3) / 4;
  for (int by = 0; by < bh; by++)
    for (int bx = 0; bx < bw; bx++)
    {
      int cw = w - bx * 4 < 4 ? w - bx * 4 : 4;
      int ch = h - by * 4 < 4 ? h - by * 4 : 4;
      DecodeColorBlock(data + (by * bw + bx) * 8, cw, ch,
                       out + (size_t)by * 4 * w * 4 + bx * 4 * 4, w * 4, true);
    }
}

static void DecodeDXT23(const unsigned char *data, int w, int h, unsigned char *out, bool bDXT3)
{
  int bw = (w + 3) / 4, bh = (h + 3) / 4;
  const int blocksPerRow = bw;
  for (int by = 0; by < bh; by++)
    for (int bx = 0; bx < bw; bx++)
    {
      const unsigned char *blk = data + (size_t)(by * blocksPerRow + bx) * (bDXT3 ? 16 : 16);
      int cw = w - bx * 4 < 4 ? w - bx * 4 : 4;
      int ch = h - by * 4 < 4 ? h - by * 4 : 4;
      DecodeColorBlock(blk + 8, cw, ch, out + (size_t)by * 4 * w * 4 + bx * 4 * 4, w * 4, !bDXT3);
      if (bDXT3)
      {
        for (int y = 0; y < ch; y++)
          for (int x = 0; x < cw; x++)
          {
            int idx4 = y * 4 + x;
            unsigned char a = (blk[idx4 / 2] >> ((idx4 & 1) * 4)) & 15;
            out[(size_t)by * 4 * w * 4 + bx * 4 * 4 + (size_t)y * w * 4 + x * 4 + 3] = (unsigned char)(a * 17);
          }
      }
    }
}

static void DecodeDXT5(const unsigned char *data, int w, int h, unsigned char *out)
{
  int bw = (w + 3) / 4, bh = (h + 3) / 4;
  for (int by = 0; by < bh; by++)
    for (int bx = 0; bx < bw; bx++)
    {
      const unsigned char *blk = data + (size_t)(by * bw + bx) * 16;
      int cw = w - bx * 4 < 4 ? w - bx * 4 : 4;
      int ch = h - by * 4 < 4 ? h - by * 4 : 4;
      DecodeColorBlock(blk + 8, cw, ch, out + (size_t)by * 4 * w * 4 + bx * 4 * 4, w * 4, false);
      unsigned char a0 = blk[0], a1 = blk[1];
      unsigned char alphas[8];
      alphas[0] = a0;
      alphas[1] = a1;
      if (a0 > a1)
      {
        for (int i = 0; i < 6; i++)
          alphas[2 + i] = (unsigned char)(((6 - i) * a0 + (1 + i) * a1) / 7);
      }
      else
      {
        for (int i = 0; i < 4; i++)
          alphas[2 + i] = (unsigned char)(((4 - i) * a0 + (1 + i) * a1) / 5);
        alphas[6] = 0;
        alphas[7] = 255;
      }
      unsigned long long bits = 0;
      for (int i = 0; i < 6; i++)
        bits |= (unsigned long long)blk[2 + i] << (8 * i);
      for (int y = 0; y < ch; y++)
        for (int x = 0; x < cw; x++)
        {
          int idx3 = y * 4 + x;
          unsigned char ai = (unsigned char)((bits >> (3 * idx3)) & 7);
          out[(size_t)by * 4 * w * 4 + bx * 4 * 4 + (size_t)y * w * 4 + x * 4 + 3] = alphas[ai];
        }
    }
}

static bool IsDXT(GLint fmt)
{
  return fmt == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
         fmt == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || fmt == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
}

// ---------------------------------------------------------------------------
// pixel conversion for desktop-only formats
// ---------------------------------------------------------------------------
static std::vector<unsigned char> g_Cvt;
static const void *ConvertPixels(GLsizei w, GLsizei h, GLenum &format,
                                 GLenum type, const void *data, int &swiz)
{
  swiz = SW_NONE;
  if (!data || type != GL_UNSIGNED_BYTE)
    return data;
  if (format == GL_LUMINANCE)
  {
    g_Cvt.resize((size_t)w * h);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
      g_Cvt[i] = s[i];
    format = 0x1903; // GL_RED
    swiz = SW_LUM;
    return &g_Cvt[0];
  }
  if (format == GL_INTENSITY)
  {
    g_Cvt.resize((size_t)w * h);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
      g_Cvt[i] = s[i];
    format = 0x1903;
    swiz = SW_INT;
    return &g_Cvt[0];
  }
  if (format == GL_ALPHA)
  {
    g_Cvt.resize((size_t)w * h);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
      g_Cvt[i] = s[i];
    format = 0x1903;
    swiz = SW_ALPHA;
    return &g_Cvt[0];
  }
  if (format == GL_LUMINANCE_ALPHA)
  {
    g_Cvt.resize((size_t)w * h * 2);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
    {
      g_Cvt[i * 2] = s[i * 2];
      g_Cvt[i * 2 + 1] = s[i * 2 + 1];
    }
    format = 0x8227; // wait - this is GL_RG; keep 0x1906? no.
    // ES3 GL_RG = 0x8227? Actually GL_RG = 0x8227 is correct for ES3.
    swiz = SW_LA;
    return &g_Cvt[0];
  }
  if (format == GL_BGR)
  {
    g_Cvt.resize((size_t)w * h * 3);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
    {
      g_Cvt[i * 3] = s[i * 3 + 2];
      g_Cvt[i * 3 + 1] = s[i * 3 + 1];
      g_Cvt[i * 3 + 2] = s[i * 3];
    }
    format = GL_RGB;
    return &g_Cvt[0];
  }
  if (format == GL_BGRA)
  {
    g_Cvt.resize((size_t)w * h * 4);
    const unsigned char *s = (const unsigned char *)data;
    for (size_t i = 0; i < (size_t)w * h; i++)
    {
      g_Cvt[i * 4] = s[i * 4 + 2];
      g_Cvt[i * 4 + 1] = s[i * 4 + 1];
      g_Cvt[i * 4 + 2] = s[i * 4];
      g_Cvt[i * 4 + 3] = s[i * 4 + 3];
    }
    format = GL_RGBA;
    return &g_Cvt[0];
  }
  return data;
}

static GLint MapInternalFormat(GLint internalFormat)
{
  switch (internalFormat)
  {
  case 1: return GL_LUMINANCE8;
  case 2: return GL_LUMINANCE8_ALPHA8;
  case 3: return GL_RGB8;
  case 4: return GL_RGBA8;
  default:
    return internalFormat;
  }
}

static GLenum EsTarget(GLenum target)
{
  if (target == GL_TEXTURE_1D || target == GL_TEXTURE_RECTANGLE_ARB_E ||
      target == GL_TEXTURE_RECTANGLE_NV_E)
    return GL_TEXTURE_2D;
  return target;
}

void TexUploadLevel(GLenum target, GLint level, GLint internalFormat,
                    GLsizei w, GLsizei h, GLint border, GLenum format,
                    GLenum type, const void *data)
{
  if (es_glBindTexture == 0 || es_glTexImage2D == 0)
    return;
  GLenum esT = EsTarget(target);
  internalFormat = MapInternalFormat(internalFormat);

  // proxy targets: pretend success
  if (target == GL_PROXY_TEXTURE_2D || target == GL_PROXY_TEXTURE_1D ||
      target == GL_PROXY_TEXTURE_RECTANGLE_E)
    return;
  if (border != 0)
    WarnOnce(60, "texture border != 0 ignored");

  int swiz = SW_NONE;
  GLenum fmt2 = format;
  const void *d2 = ConvertPixels(w, h, fmt2, type, data, swiz);
  GLint ifmt2 = internalFormat;
  extern int g_nEsMajor;
  if (g_nEsMajor >= 3)
  {
    // ES3: sized formats + swizzles are available
    if (swiz == SW_LUM || swiz == SW_INT || swiz == SW_ALPHA)
      ifmt2 = 0x822B; // GL_R8
    else if (swiz == SW_LA)
      ifmt2 = 0x822F; // GL_RG8
    else if (ifmt2 == GL_LUMINANCE || ifmt2 == GL_LUMINANCE8 || ifmt2 == GL_LUMINANCE4)
    {
      ifmt2 = 0x822B;
      if (swiz == SW_NONE && fmt2 == GL_LUMINANCE)
      {
        fmt2 = 0x1903;
        swiz = SW_LUM;
      }
    }
    else if (ifmt2 == GL_LUMINANCE_ALPHA || ifmt2 == GL_LUMINANCE8_ALPHA8)
    {
      ifmt2 = 0x822F;
      if (fmt2 == GL_LUMINANCE_ALPHA)
      {
        fmt2 = 0x8227;
        swiz = SW_LA;
      }
    }
    else if (ifmt2 == GL_ALPHA8 || ifmt2 == GL_ALPHA)
    {
      ifmt2 = 0x822B;
      if (fmt2 == GL_ALPHA)
      {
        fmt2 = 0x1903;
        swiz = SW_ALPHA;
      }
    }
    else if (ifmt2 == GL_RGB || ifmt2 == GL_RGB8 || ifmt2 == GL_RGB5 || ifmt2 == GL_RGB4 || ifmt2 == GL_R3_G3_B2)
      ifmt2 = GL_RGB8;
    else if (ifmt2 == GL_RGBA || ifmt2 == GL_RGBA8 || ifmt2 == GL_RGBA4 || ifmt2 == GL_RGB5_A1 || ifmt2 == GL_ABGR_EXT_E)
      ifmt2 = GL_RGBA8;
    // ES3 requires internalformat to MATCH the pixel format exactly
    // (desktop GL accepts compatible subsets: RGB8+RGBA data is fine there,
    // INVALID_ENUM in ES3); the data format is the authority
    if (swiz == SW_NONE)
    {
      if (fmt2 == GL_RGBA)
        ifmt2 = GL_RGBA8;
      else if (fmt2 == GL_RGB)
        ifmt2 = GL_RGB8;
      else if (fmt2 == 0x1903 /*GL_RED*/ || fmt2 == GL_LUMINANCE)
        ifmt2 = 0x822B /*GL_R8*/;
      else if (fmt2 == 0x8227 /*GL_RG*/ || fmt2 == GL_LUMINANCE_ALPHA)
        ifmt2 = 0x822F /*GL_RG8*/;
      else if (fmt2 == GL_ALPHA)
        ifmt2 = 0x822B;
    }
  }
  else
  {
    // ES1/ES2: sized internal formats are REJECTED (INVALID_ENUM) and there
    // are no swizzles - pass the native desktop formats straight through
    // (GL_LUMINANCE/GL_LUMINANCE_ALPHA/GL_ALPHA/GL_RGB/GL_RGBA all exist)
    if (ifmt2 == 1) ifmt2 = GL_LUMINANCE;
    else if (ifmt2 == 2) ifmt2 = GL_LUMINANCE_ALPHA;
    else if (ifmt2 == 3) ifmt2 = GL_RGB;
    else if (ifmt2 == 4) ifmt2 = GL_RGBA;
    else
    {
      switch (ifmt2)
      {
      case GL_LUMINANCE8: ifmt2 = GL_LUMINANCE; break;
      case GL_LUMINANCE4: ifmt2 = GL_LUMINANCE; break;
      case GL_LUMINANCE8_ALPHA8: ifmt2 = GL_LUMINANCE_ALPHA; break;
      case GL_ALPHA8: ifmt2 = GL_ALPHA; break;
      case GL_RGB8: case GL_RGB5: case GL_RGB4: case GL_R3_G3_B2: ifmt2 = GL_RGB; break;
      case GL_RGBA8: case GL_RGBA4: case GL_RGB5_A1: ifmt2 = GL_RGBA; break;
      default: break;
      }
    }
  }

  // keep our object metadata fresh and rebind the target object: the engine
  // interleaves binds (its own cache binds 0 between uploads), so the ES
  // "currently bound" texture is NOT guaranteed to be the upload target -
  // desktop semantics demand the pixels land in the object bound at
  // glTexImage2D time, which our tracking knows precisely
  bool bCubeFace = (target >= 0x8515 && target <= 0x851C); // GL_TEXTURE_CUBE_MAP_POSITIVE_X.._NEGATIVE_Z
  GLuint bound = 0;
  if (g_nActiveUnit < GC_MAX_UNITS)
  {
    bound = bCubeFace ? g_TexUnit[g_nActiveUnit].idCube
                      : (g_TexUnit[g_nActiveUnit].id2D
                         ? g_TexUnit[g_nActiveUnit].id2D
                         : g_TexUnit[g_nActiveUnit].nLastBind);
  }
  STexObj *t = TexGet(bound);
  {
    static int s_nUploadLog = 0;
    if (s_nUploadLog < 400 && DiagOn())
    {
      s_nUploadLog++;
      GLog("upload: unit=%d id2D=%u found=%d es=%u lvl=%d ifmt=%x fmt=%x type=%x %dx%d",
           (int)g_nActiveUnit, (unsigned)bound, t ? 1 : 0, t ? t->es : 0,
           (int)level, (unsigned)ifmt2, (unsigned)fmt2, (unsigned)type,
           (int)w, (int)h);
    }
  }
  if (t && t->es)
    // glBindTexture must use a BIND point (2D / CUBE_MAP); the cube FACE is
    // only valid as a TexImage2D target - binding a face target fails and
    // the subsequent face upload would hit "no cube bound" INVALID_OPERATION
    es_glBindTexture(bCubeFace ? 0x8513 /*GL_TEXTURE_CUBE_MAP*/ : esT, t->es);
  es_glTexImage2D(esT, level, ifmt2, w, h, 0, fmt2, type, d2);
  {
    static int s_nUploadErr = 0;
    if (s_nUploadErr < 8 && DiagOn())
    {
      GLenum e2 = es_glGetError();
      if (e2)
      {
        s_nUploadErr++;
        GLog("upload err %x after TexImage2D", (unsigned)e2);
      }
      while (e2)
        e2 = es_glGetError();
    }
  }
  if (swiz != SW_NONE)
    ApplySwizzle(esT, swiz);
  if (t)
  {
    if (level > t->maxLevelUploaded)
      t->maxLevelUploaded = level;
    if (level < 16)
      t->nAllocMask |= (unsigned short)(1u << level);
    if (level == 0)
    {
      t->width = w;
      t->height = h;
      // ES completeness: the engine usually binds BEFORE uploading and never
      // rebinds (its own bind cache), so the mip-filter downgrade in TexBind
      // never fires - do it right here or every texture samples black
      switch (t->minFilter)
      {
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        if (es_glTexParameteri)
          es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
      }
    }
  }
}

void TexUploadCompressed(GLenum target, GLint level, GLint internalFormat,
                         GLsizei w, GLsizei h, GLint border, GLsizei sz,
                         const void *data)
{
  (void)sz;
  (void)border;
  if (!IsDXT(internalFormat))
  {
    WarnOnce(61, "unknown compressed format - skipped");
    return;
  }
  GLenum esT = EsTarget(target);
  std::vector<unsigned char> rgba((size_t)w * h * 4);
  switch (internalFormat)
  {
  case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
  case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    DecodeDXT1((const unsigned char *)data, w, h, &rgba[0]);
    break;
  case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    DecodeDXT23((const unsigned char *)data, w, h, &rgba[0], true);
    break;
  default:
    DecodeDXT5((const unsigned char *)data, w, h, &rgba[0]);
    break;
  }
  bool bCubeFace2 = (target >= 0x8515 && target <= 0x851C);
  GLuint bound = 0;
  if (g_nActiveUnit < GC_MAX_UNITS)
    bound = bCubeFace2 ? g_TexUnit[g_nActiveUnit].idCube
                       : (g_TexUnit[g_nActiveUnit].id2D
                          ? g_TexUnit[g_nActiveUnit].id2D
                          : g_TexUnit[g_nActiveUnit].nLastBind);
  STexObj *t = TexGet(bound);
  if (t && t->es)
    // pixels MUST land in the tracked object; the bind target must match the
    // upload target (a cube-face upload needs a CUBE_MAP bind, not 2D)
    es_glBindTexture(bCubeFace2 ? 0x8513 /*GL_TEXTURE_CUBE_MAP*/ : GL_TEXTURE_2D, t->es);
  es_glTexImage2D(esT, level, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &rgba[0]);
  if (t)
  {
    if (level < 16)
      t->nAllocMask |= (unsigned short)(1u << level);
    if (level > t->maxLevelUploaded)
      t->maxLevelUploaded = level;
    if (level == 0)
    {
      t->width = w;
      t->height = h;
    }
  }
}

GLboolean TexGetImage(GLenum target, GLint level, GLenum format, GLenum type, void *buf)
{
  // FBO readback
  if (es_glGenFramebuffers == 0 || es_glReadPixels == 0)
    return GL_FALSE;
  extern GLuint g_nFBO;
  if (g_nFBO == 0)
    return GL_FALSE;
  GLenum esT = EsTarget(target);
  GLuint bound = g_nActiveUnit < GC_MAX_UNITS ? g_TexUnit[g_nActiveUnit].id2D : 0;
  STexObj *t = TexGet(bound);
  if (!t)
    return GL_FALSE;
  GLint prevFbo = 0;
  if (es_glGetIntegerv)
    es_glGetIntegerv(0x8CA6, &prevFbo); // GL_DRAW_FRAMEBUFFER_BINDING
  es_glBindFramebuffer(0x8D40, g_nFBO);
  es_glFramebufferTexture2D(0x8D40, 0x8CE0, esT, t->es, level); // COLOR_ATTACHMENT0
  GLenum st = es_glCheckFramebufferStatus(0x8D40);
  GLboolean ok = GL_FALSE;
  if (st == 0x8CD5) // GL_FRAMEBUFFER_COMPLETE
  {
    std::vector<unsigned char> tmp((size_t)t->width * t->height * 4);
    es_glReadPixels(0, 0, t->width, t->height, GL_RGBA, GL_UNSIGNED_BYTE, &tmp[0]);
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
    {
      memcpy(buf, &tmp[0], tmp.size());
      ok = GL_TRUE;
    }
    else if (format == GL_RGB && type == GL_UNSIGNED_BYTE)
    {
      unsigned char *o = (unsigned char *)buf;
      for (int i = 0; i < t->width * t->height; i++)
      {
        o[i * 3] = tmp[i * 4];
        o[i * 3 + 1] = tmp[i * 4 + 1];
        o[i * 3 + 2] = tmp[i * 4 + 2];
      }
      ok = GL_TRUE;
    }
    else if (format == GL_ALPHA && type == GL_UNSIGNED_BYTE)
    {
      unsigned char *o = (unsigned char *)buf;
      for (int i = 0; i < t->width * t->height; i++)
        o[i] = tmp[i * 4 + 3];
      ok = GL_TRUE;
    }
    else if (format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE)
    {
      unsigned char *o = (unsigned char *)buf;
      for (int i = 0; i < t->width * t->height; i++)
        o[i] = tmp[i * 4];
      ok = GL_TRUE;
    }
  }
  es_glFramebufferTexture2D(0x8D40, 0x8CE0, esT, 0, level);
  es_glBindFramebuffer(0x8D40, (GLuint)prevFbo);
  return ok;
}

static int g_nBindTrace = 0;
void TexBind(GLenum target, GLuint name)
{
  if (DiagOn() && g_nBindTrace < 40)
  {
    g_nBindTrace++;
    STexObj *tt = TexGet(name);
    GLog("texbind: unit=%d tgt=%x name=%u found=%d es=%u %dx%d", (int)g_nActiveUnit, (unsigned)target, (unsigned)name, tt?1:0, tt?tt->es:0, tt?tt->width:0, tt?tt->height:0);
  }
  GLenum esT = EsTarget(target);
  if (g_nActiveUnit >= GC_MAX_UNITS)
    return;
  if (target == GL_TEXTURE_CUBE_MAP)
  {
    // track cube maps too (normalize/light cube maps are uploaded by the
    // engine right after binding - the upload needs the object)
    STexObj *tc = TexGet(name);
    GLuint esCube = name;
    if (!tc && name != 0)
    {
      STexObj nt;
      if (es_glGenTextures == 0)
        return;
      es_glGenTextures(1, &nt.es);
      nt.target = GL_TEXTURE_CUBE_MAP;
      g_TexObjs[name] = nt;
      tc = &g_TexObjs[name];
      esCube = tc->es;
    }
    if (tc)
      esCube = tc->es;
    if (es_glBindTexture)
      es_glBindTexture(GL_TEXTURE_CUBE_MAP, esCube);
    g_TexUnit[g_nActiveUnit].idCube = name;
    return;
  }
  // Desktop GL silently creates a new texture object when an unused name is
  // bound; ES3 REJECTS unknown names (INVALID_OPERATION) and binds nothing.
  // Far Cry allocates its own names, so unknown binds are normal here:
  // auto-create a real ES texture and map engine name -> real ES name.
  STexObj *t = TexGet(name);
  GLuint esName = name;
  if (!t && name != 0)
  {
    STexObj nt;
    if (es_glGenTextures == 0)
      return;
    es_glGenTextures(1, &nt.es);
    nt.target = GL_TEXTURE_2D;
    nt.bRectangle = (target == GL_TEXTURE_RECTANGLE_ARB_E ||
                     target == GL_TEXTURE_RECTANGLE_NV_E);
    g_TexObjs[name] = nt;
    t = &g_TexObjs[name];
    esName = t->es;
  }
  if (t)
    esName = t->es;
  if (es_glBindTexture)
    es_glBindTexture(esT, esName);
  g_TexUnit[g_nActiveUnit].id2D = name;
  if (name != 0)
    g_TexUnit[g_nActiveUnit].nLastBind = name;
  else if (g_TexUnit[g_nActiveUnit].nLastBind)
  {
    // never let a 0-bind unbind the ES unit - re-hang the last real texture
    STexObj *tl = TexGet(g_TexUnit[g_nActiveUnit].nLastBind);
    if (tl && tl->es && es_glBindTexture)
      es_glBindTexture(GL_TEXTURE_2D, tl->es);
    g_TexUnit[g_nActiveUnit].id2D = g_TexUnit[g_nActiveUnit].nLastBind;
  }
  if (t)
  {
    // ES completeness: a mipmapped min filter with no mips renders black
    if (t->maxLevelUploaded == 0)
    {
      switch (t->minFilter)
      {
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        if (es_glTexParameteri)
          es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        break;
      }
    }
  }
}

void TexEnableDisable(GLenum cap, bool bOn)
{
  if (cap == GL_TEXTURE_1D || cap == GL_TEXTURE_2D)
  {
    if (g_nActiveUnit < GC_MAX_UNITS)
      g_TexUnit[g_nActiveUnit].bEnabled2D = bOn;
    if (DiagOn() && g_nBindTrace < 24)
    {
      g_nBindTrace++;
      GLog("texendbg: unit=%d on=%d", (int)g_nActiveUnit, (int)bOn);
    }
    if (es_glEnable && bOn)
      es_glEnable(GL_TEXTURE_2D);
    if (es_glDisable && !bOn)
      es_glDisable(GL_TEXTURE_2D);
  }
}

} // namespace glescompat

// =============================================================================
//  exported GL texture API
// =============================================================================
using namespace glescompat;

extern "C" {

void APIENTRY glGenTextures(GLsizei n, const GLuint *names)
{
  if (es_glGenTextures)
    es_glGenTextures(n, (GLuint *)names);
  for (int i = 0; i < n; i++)
  {
    STexObj t;
    t.es = ((GLuint *)names)[i];
    g_TexObjs[((GLuint *)names)[i]] = t;
  }
}
void APIENTRY glDeleteTextures(GLsizei n, const GLuint *names) { TexDelete(n, names); }
GLboolean APIENTRY glIsTexture(GLuint name) { return TexGet(name) ? GL_TRUE : GL_FALSE; }

extern "C" void GLESCompat_Drain(const char *tag);
// glescompat::DiagEnabled declared at the top of this file

void APIENTRY glBindTexture(GLenum target, GLuint name)
{
  GLESCompat_Drain("glBindTexture");
  TexBind(target, name);
  GLESCompat_Drain("glBindTexture*end");
}

void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                           GLsizei w, GLsizei h, GLint border, GLenum format,
                           GLenum type, const void *data)
{
  if (target == GL_TEXTURE_1D)
  {
    h = 1;
    target = GL_TEXTURE_2D;
  }
  else if (target == GL_TEXTURE_RECTANGLE_ARB_E || target == GL_TEXTURE_RECTANGLE_NV_E)
  {
    target = GL_TEXTURE_2D;
  }
  TexUploadLevel(target, level, internalFormat, w, h, border, format, type, data);
}

void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xo, GLint yo,
                              GLsizei w, GLsizei h, GLenum format, GLenum type,
                              const void *data)
{
  GLESCompat_Drain("glTexSubImage2D");
  if (target == GL_TEXTURE_1D)
  {
    target = GL_TEXTURE_2D;
    yo = 0;
  }
  else if (target == GL_TEXTURE_RECTANGLE_ARB_E || target == GL_TEXTURE_RECTANGLE_NV_E)
    target = GL_TEXTURE_2D;
  int swiz = SW_NONE;
  GLenum fmt2 = format;
  const void *d2 = ConvertPixels(w, h, fmt2, type, data, swiz);
  // resolve the REAL upload target: the engine interleave-binds (even 0)
  // through its own virtual stage cache, so the ES "current" texture is not
  // the one the desktop call refers to - dynamically updated textures (menu
  // video, fonts, console) otherwise land on nothing and stay black
  bool bCubeFace3 = (target >= 0x8515 && target <= 0x851C);
  GLuint bound = 0;
  if (g_nActiveUnit < GC_MAX_UNITS)
    bound = bCubeFace3 ? g_TexUnit[g_nActiveUnit].idCube
                       : (g_TexUnit[g_nActiveUnit].id2D
                          ? g_TexUnit[g_nActiveUnit].id2D
                          : g_TexUnit[g_nActiveUnit].nLastBind);
  STexObj *t = TexGet(bound);
  if (t && t->es)
    es_glBindTexture(bCubeFace3 ? 0x8513 /*GL_TEXTURE_CUBE_MAP*/ : GL_TEXTURE_2D, t->es);
  if (t && level < 16 && !(t->nAllocMask & (1u << level)))
  {
    // streamed sub-update for a level that was never allocated: ES3 has no
    // implicit allocation - allocate first or the update is INVALID_OPERATION
    GLenum esT2 = GL_TEXTURE_2D;
    GLint ifmt3 = GL_RGBA8;
    if (g_nEsMajor < 3)
      ifmt3 = (fmt2 == GL_RGB) ? GL_RGB : (fmt2 == GL_RGBA ? GL_RGBA : GL_LUMINANCE);
    else
    {
      if (fmt2 == GL_RGB) ifmt3 = GL_RGB8;
      else if (fmt2 == GL_RGBA) ifmt3 = GL_RGBA8;
      else if (fmt2 == 0x1903 || fmt2 == GL_LUMINANCE) ifmt3 = 0x822B;
      else if (fmt2 == 0x8227 || fmt2 == GL_LUMINANCE_ALPHA) ifmt3 = 0x822F;
      else if (fmt2 == GL_ALPHA) ifmt3 = 0x822B;
    }
    if (es_glTexImage2D)
      es_glTexImage2D(esT2, level, ifmt3, w, h, 0, fmt2, type, 0);
    if (level < 16)
      t->nAllocMask |= (unsigned short)(1u << level);
  }
  if (es_glTexSubImage2D)
    es_glTexSubImage2D(GL_TEXTURE_2D, level, xo, yo, w, h, fmt2, type, d2);
  if (DiagOn())
  {
    static int s_nSubLog = 0;
    if (s_nSubLog < 6)
    {
      s_nSubLog++;
      GLenum e3 = es_glGetError();
      GLog("subimg: bound=%u t=%p es=%u lvl=%d %dx%d fmt=%x err=%x",
           (unsigned)bound, (void *)t, t ? t->es : 0, (int)level,
           (int)w, (int)h, (unsigned)fmt2, (unsigned)e3);
      while (e3)
        e3 = es_glGetError();
    }
  }
}

void APIENTRY glCompressedTexImage2DARB(GLenum target, GLint level, GLint internalFormat,
                                        GLsizei w, GLsizei h, GLint border,
                                        GLsizei imageSize, const void *data)
{
  GLESCompat_Drain("glCompressedTexImage2DARB");
  if (target == GL_TEXTURE_1D)
  {
    target = GL_TEXTURE_2D;
    h = 1;
  }
  else if (target == GL_TEXTURE_RECTANGLE_ARB_E || target == GL_TEXTURE_RECTANGLE_NV_E)
    target = GL_TEXTURE_2D;
  TexUploadCompressed(target, level, internalFormat, w, h, border, imageSize, data);
}

void APIENTRY glCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xo, GLint yo,
                                           GLsizei w, GLsizei h, GLenum format,
                                           GLsizei imageSize, const void *data)
{
  (void)xo; (void)yo;
  WarnOnce(62, "compressed TexSubImage - full re-upload path");
  // fall back to re-uploading the whole level from the compressed block set
  glCompressedTexImage2DARB(target, level, (GLint)format, w, h, 0, imageSize, data);
}

void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat,
                               GLint x, GLint y, GLsizei w, GLsizei h, GLint border)
{
  if (es_glCopyTexImage2D)
    es_glCopyTexImage2D(EsTarget(target), level, internalFormat, x, y, w, h, border);
  STexObj *t = TexGet(g_nActiveUnit < GC_MAX_UNITS ? g_TexUnit[g_nActiveUnit].id2D : 0);
  if (t)
  {
    if (level > t->maxLevelUploaded)
      t->maxLevelUploaded = level;
    if (level == 0)
    {
      t->width = w;
      t->height = h;
    }
  }
}
void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xo, GLint yo,
                                  GLint x, GLint y, GLsizei w, GLsizei h)
{
  if (es_glCopyTexSubImage2D)
    es_glCopyTexSubImage2D(EsTarget(target), level, xo, yo, x, y, w, h);
}

void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
  GLESCompat_Drain("glTexParameteri");
  GLenum esT = EsTarget(target);
  switch (pname)
  {
  case GL_TEXTURE_MIN_FILTER:
  case GL_TEXTURE_MAG_FILTER:
  case GL_TEXTURE_WRAP_S:
  case GL_TEXTURE_WRAP_T:
  case GL_TEXTURE_WRAP_R:
    if (es_glTexParameteri)
      es_glTexParameteri(esT, pname, param);
    if (pname == GL_TEXTURE_MIN_FILTER && g_nActiveUnit < GC_MAX_UNITS)
    {
      STexObj *t = TexGet(g_TexUnit[g_nActiveUnit].id2D);
      if (t)
        t->minFilter = param;
    }
    break;
  case GL_TEXTURE_BORDER_COLOR:   // desktop/ES3.2-only: silently ignored
  case 0x8016:                    // GL_TEXTURE_PRIORITY (desktop only)
  case 0x8136:                    // GL_TEXTURE_LOD_BIAS (desktop only)
  case 0x2500:                    // GL_TEXTURE_ENV_* leftovers
    break;
  default:
    WarnOnce(61, "glTexParameteri: desktop-only pname ignored");
    break;
  }
  GLESCompat_Drain("glTexParameteri*end");
}
void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
  glTexParameteri(EsTarget(target), pname, (GLint)param);
}
void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
  glTexParameteri(EsTarget(target), pname, (GLint)params[0]);
  (void)pname;
}
void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
  glTexParameteri(EsTarget(target), pname, params[0]);
  (void)pname;
}

void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
  GLESCompat_Drain("glTexEnvi");
  if (target != GL_TEXTURE_ENV)
  {
    if (es_glTexParameteri)
      es_glTexParameteri(GL_TEXTURE_2D, pname, param);
    return;
  }
  if (g_nActiveUnit >= GC_MAX_UNITS)
    return;
  STexEnv &e = g_TexUnit[g_nActiveUnit].env;
  switch (pname)
  {
  case GL_TEXTURE_ENV_MODE: e.mode = param; break;
  case GL_COMBINE_RGB: e.combineRGB = param; break;
  case GL_COMBINE_ALPHA: e.combineA = param; break;
  case GL_SRC0_RGB: e.srcRGB[0] = param; break;
  case GL_SRC1_RGB: e.srcRGB[1] = param; break;
  case GL_SRC2_RGB: e.srcRGB[2] = param; break;
  case GL_SRC0_ALPHA: e.srcA[0] = param; break;
  case GL_SRC1_ALPHA: e.srcA[1] = param; break;
  case GL_SRC2_ALPHA: e.srcA[2] = param; break;
  case GL_OPERAND0_RGB: e.opRGB[0] = param; break;
  case GL_OPERAND1_RGB: e.opRGB[1] = param; break;
  case GL_OPERAND2_RGB: e.opRGB[2] = param; break;
  case GL_OPERAND0_ALPHA: e.opA[0] = param; break;
  case GL_OPERAND1_ALPHA: e.opA[1] = param; break;
  case GL_OPERAND2_ALPHA: e.opA[2] = param; break;
  case GL_RGB_SCALE: e.rgbScale = (GLfloat)param; break;
  case GL_ALPHA_SCALE: e.alphaScale = (GLfloat)param; break;
  }
}
void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
  glTexEnvi(target, pname, (GLint)param);
}
void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
  if (target != GL_TEXTURE_ENV)
    return;
  if (pname == GL_TEXTURE_ENV_COLOR && g_nActiveUnit < GC_MAX_UNITS)
    memcpy(g_TexUnit[g_nActiveUnit].env.color, params, 16);
  else
    glTexEnvf(target, pname, params[0]);
}
void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
  GLfloat f[4];
  for (int i = 0; i < 4; i++)
    f[i] = (GLfloat)params[i];
  if (pname == GL_TEXTURE_ENV_COLOR)
    glTexEnvfv(target, pname, f);
  else
    glTexEnvf(target, pname, (GLfloat)params[0]);
}

void APIENTRY glActiveTextureARB(GLenum tex)
{
  GLESCompat_Drain("glActiveTextureARB");
  int u = tex - GL_TEXTURE0;
  if (u >= 0 && u < GC_MAX_UNITS)
    g_nActiveUnit = u;
  if (es_glActiveTexture)
    es_glActiveTexture(tex);
}
void APIENTRY glActiveTexture(GLenum tex) { glActiveTextureARB(tex); }

void APIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *buf)
{
  TexGetImage(EsTarget(target), level, format, type, buf);
}

void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
  STexObj *t = TexGet(g_nActiveUnit < GC_MAX_UNITS ? g_TexUnit[g_nActiveUnit].id2D : 0);
  switch (pname)
  {
  case GL_TEXTURE_WIDTH:
    params[0] = t ? t->width : 0;
    return;
  case GL_TEXTURE_HEIGHT:
    params[0] = t ? (target == GL_TEXTURE_1D ? 1 : t->height) : 0;
    return;
  case GL_TEXTURE_INTERNAL_FORMAT:
    params[0] = GL_RGBA8;
    return;
  case GL_TEXTURE_COMPRESSED:
    params[0] = 0;
    return;
  case GL_TEXTURE_RED_SIZE:
  case GL_TEXTURE_GREEN_SIZE:
  case GL_TEXTURE_BLUE_SIZE:
  case GL_TEXTURE_ALPHA_SIZE:
    params[0] = 8;
    return;
  default:
    params[0] = 0;
    return;
  }
}

} // extern "C"
