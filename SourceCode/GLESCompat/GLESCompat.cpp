// =============================================================================
//   GLESCompat core: ES proc loading, tracked state, CPU fixed-function
//   vertex pipeline (transform / lighting / fog), shader cache, immediate
//   mode, client arrays, display lists and the proc-address export.
//   Textures live in GLESCompatTex.cpp.
// =============================================================================
#include "GLESCompat_Impl.h"
#include <string>
#include <stdarg.h>

// ---------------------------------------------------------------------------
// ES procedure pointer storage (file scope - matches the header's externs)
// ---------------------------------------------------------------------------
#define ESDEF(name) PFN_es_##name es_##name = 0;
ESDEF(glActiveTexture) ESDEF(glAttachShader) ESDEF(glBindBuffer)
ESDEF(glBindFramebuffer) ESDEF(glBindTexture) ESDEF(glBlendColor)
ESDEF(glBlendEquationSeparate) ESDEF(glBlendFunc) ESDEF(glBlendFuncSeparate)
ESDEF(glBufferData) ESDEF(glBufferSubData) ESDEF(glCheckFramebufferStatus)
ESDEF(glClear) ESDEF(glClearColor) ESDEF(glClearDepthf) ESDEF(glClearStencil)
ESDEF(glColorMask) ESDEF(glCompileShader) ESDEF(glCompressedTexImage2D)
ESDEF(glFramebufferTexture2D)
ESDEF(glCopyTexImage2D) ESDEF(glCopyTexSubImage2D) ESDEF(glCreateProgram)
ESDEF(glCreateShader) ESDEF(glCullFace) ESDEF(glDeleteBuffers)
ESDEF(glDeleteProgram) ESDEF(glDeleteShader) ESDEF(glDeleteTextures)
ESDEF(glDepthFunc) ESDEF(glDepthMask) ESDEF(glDepthRangef) ESDEF(glDisable)
ESDEF(glDisableVertexAttribArray) ESDEF(glDrawArrays) ESDEF(glDrawElements)
ESDEF(glEnable) ESDEF(glEnableVertexAttribArray) ESDEF(glFinish) ESDEF(glFlush)
ESDEF(glFrontFace) ESDEF(glGenBuffers) ESDEF(glGenFramebuffers)
ESDEF(glGenTextures) ESDEF(glGenerateMipmap) ESDEF(glGetError)
ESDEF(glGetFloatv) ESDEF(glGetIntegerv) ESDEF(glGetBufferSubData)
ESDEF(glGetProgramInfoLog) ESDEF(glGetProgramiv) ESDEF(glGetShaderInfoLog)
ESDEF(glGetShaderiv) ESDEF(glGetString) ESDEF(glGetUniformLocation)
ESDEF(glHint) ESDEF(glIsEnabled) ESDEF(glLineWidth) ESDEF(glLinkProgram)
ESDEF(glMapBufferRange) ESDEF(glPixelStorei) ESDEF(glPolygonOffset)
ESDEF(glReadPixels) ESDEF(glScissor) ESDEF(glShaderSource) ESDEF(glStencilFunc)
ESDEF(glStencilMask) ESDEF(glStencilOp) ESDEF(glTexImage2D)
ESDEF(glTexParameterf) ESDEF(glTexParameteri) ESDEF(glTexSubImage2D)
ESDEF(glUniform1f) ESDEF(glUniform1i) ESDEF(glUniform2f) ESDEF(glUniform4fv)
ESDEF(glUniformMatrix4fv) ESDEF(glUnmapBuffer) ESDEF(glUseProgram)
ESDEF(glVertexAttribPointer) ESDEF(glViewport)
#undef ESDEF

typedef void (APIENTRY *PFN_glBindAttribLocation)(GLuint, GLuint, const char *);
static PFN_glBindAttribLocation p_glBindAttribLocation = 0;
typedef void (APIENTRY *PFN_glSampleCoverage)(GLfloat, GLboolean);
static PFN_glSampleCoverage p_glSampleCoverage = 0;

namespace glescompat {

// ---------------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------------
void GLog(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char buf[1024];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  SDL_Log("GLESCompat: %s", buf);
}

void WarnOnce(int id, const char *what)
{
  static unsigned long long mask = 0;
  if (id >= 0 && id < 64)
  {
    unsigned long long bit = 1ULL << id;
    if (mask & bit)
      return;
    mask |= bit;
  }
  GLog("[once] %s", what);
}

static GLenum g_lastError = GL_NO_ERROR;
void GSetError(GLenum e)
{
  if (g_lastError == GL_NO_ERROR)
    g_lastError = e;
}

bool LoadESProcs(void)
{
#define ELOAD(name) es_##name = (PFN_es_##name)SDL_GL_GetProcAddress(#name)
  ELOAD(glActiveTexture); ELOAD(glAttachShader); ELOAD(glBindBuffer);
  ELOAD(glBindFramebuffer); ELOAD(glBindTexture); ELOAD(glBlendColor);
  ELOAD(glBlendEquationSeparate); ELOAD(glBlendFunc); ELOAD(glBlendFuncSeparate);
  ELOAD(glBufferData); ELOAD(glBufferSubData); ELOAD(glCheckFramebufferStatus);
  ELOAD(glClear); ELOAD(glClearColor); ELOAD(glClearDepthf); ELOAD(glClearStencil);
  ELOAD(glColorMask); ELOAD(glCompileShader); ELOAD(glCompressedTexImage2D);
  ELOAD(glFramebufferTexture2D);
  ELOAD(glCopyTexImage2D); ELOAD(glCopyTexSubImage2D); ELOAD(glCreateProgram);
  ELOAD(glCreateShader); ELOAD(glCullFace); ELOAD(glDeleteBuffers);
  ELOAD(glDeleteProgram); ELOAD(glDeleteShader); ELOAD(glDeleteTextures);
  ELOAD(glDepthFunc); ELOAD(glDepthMask); ELOAD(glDepthRangef); ELOAD(glDisable);
  ELOAD(glDisableVertexAttribArray); ELOAD(glDrawArrays); ELOAD(glDrawElements);
  ELOAD(glEnable); ELOAD(glEnableVertexAttribArray); ELOAD(glFinish); ELOAD(glFlush);
  ELOAD(glFrontFace); ELOAD(glGenBuffers); ELOAD(glGenFramebuffers);
  ELOAD(glGenTextures); ELOAD(glGenerateMipmap); ELOAD(glGetError);
  ELOAD(glGetFloatv); ELOAD(glGetIntegerv); ELOAD(glGetBufferSubData);
  ELOAD(glGetProgramInfoLog); ELOAD(glGetProgramiv); ELOAD(glGetShaderInfoLog);
  ELOAD(glGetShaderiv); ELOAD(glGetString); ELOAD(glGetUniformLocation);
  ELOAD(glHint); ELOAD(glIsEnabled); ELOAD(glLineWidth); ELOAD(glLinkProgram);
  ELOAD(glMapBufferRange); ELOAD(glPixelStorei); ELOAD(glPolygonOffset);
  ELOAD(glReadPixels); ELOAD(glScissor); ELOAD(glShaderSource); ELOAD(glStencilFunc);
  ELOAD(glStencilMask); ELOAD(glStencilOp); ELOAD(glTexImage2D);
  ELOAD(glTexParameterf); ELOAD(glTexParameteri); ELOAD(glTexSubImage2D);
  ELOAD(glUniform1f); ELOAD(glUniform1i); ELOAD(glUniform2f); ELOAD(glUniform4fv);
  ELOAD(glUniformMatrix4fv); ELOAD(glUnmapBuffer); ELOAD(glUseProgram);
  ELOAD(glVertexAttribPointer); ELOAD(glViewport);
#undef ELOAD
  p_glBindAttribLocation = (PFN_glBindAttribLocation)SDL_GL_GetProcAddress("glBindAttribLocation");
  p_glSampleCoverage = (PFN_glSampleCoverage)SDL_GL_GetProcAddress("glSampleCoverage");
  return es_glGetIntegerv != 0 && es_glCreateProgram != 0;
}

void FrameTick(void) {}

// ---------------------------------------------------------------------------
// math
// ---------------------------------------------------------------------------
void MatIdentity(Mat4 &o)
{
  memset(&o.m[0], 0, sizeof(o.m));
  o.m[0] = o.m[5] = o.m[10] = o.m[15] = 1.0f;
}

void MatMul(Mat4 &o, const Mat4 &a, const Mat4 &b) // o = a*b (column-major)
{
  Mat4 r;
  for (int c = 0; c < 4; c++)
    for (int rw = 0; rw < 4; rw++)
    {
      float s = 0;
      for (int k = 0; k < 4; k++)
        s += a.m[k * 4 + rw] * b.m[c * 4 + k];
      r.m[c * 4 + rw] = s;
    }
  o = r;
}

// ---------------------------------------------------------------------------
// tracked state
// ---------------------------------------------------------------------------
#define MAX_MV 32
#define MAX_PJ 8
#define MAX_TX 8

struct SMatrixSet {
  Mat4 cur;
  Mat4 *stack;
  int depth, maxDepth;
};

static SMatrixSet g_mv, g_pj, g_tx[GC_MAX_UNITS];
static Mat4 g_colmat;
static SMatrixSet *g_pColStack = 0;
static Mat4 g_colstack[4];
static int g_nMatrixMode = GL_MODELVIEW;
static Mat4 g_NormalMat;   // inverse-transpose 3x3 (stored 4x4, w unused)
static bool g_bNormalDirty = true;

static SMatrixSet *ModeSet(int mode)
{
  switch (mode)
  {
  case GL_MODELVIEW:
    return &g_mv;
  case GL_PROJECTION:
    return &g_pj;
  case 0x1703: // GL_COLOR matrix mode
    return g_pColStack;
  default:
    if (mode >= GL_TEXTURE0 && mode <= GL_TEXTURE0 + GC_MAX_UNITS - 1)
      return &g_tx[mode - GL_TEXTURE0];
    if (mode >= GL_TEXTURE && mode < GL_TEXTURE + GC_MAX_UNITS)
      return &g_tx[mode - GL_TEXTURE];
    return 0;
  }
}

struct SLightState {
  bool bOn;
  SLight l;
};
static SLightState g_Lights[GC_MAX_LIGHTS];
static SMaterial g_MatFront, g_MatBack;
static bool g_bLighting = false, g_bLightLocal = false, g_bLightTwoSide = false;
static GLfloat g_LightModelAmb[4] = {0.2f, 0.2f, 0.2f, 1};
static bool g_bColorMaterial = false;
static GLint g_nColorMatFace = GL_FRONT_AND_BACK, g_nColorMatParam = GL_AMBIENT_AND_DIFFUSE;
static bool g_bNormalize = false;

static bool g_bFog = false;
static GLint g_nFogMode = GL_EXP;
static GLfloat g_fFogDensity = 1, g_fFogStart = 0, g_fFogEnd = 1;
static GLfloat g_FogColor[4] = {0, 0, 0, 0};

static bool g_bAlphaTest = false;
static GLint g_nAlphaFunc = GL_ALWAYS;
static GLfloat g_fAlphaRef = 0;

static GLfloat g_fPointSize = 1;
static GLint g_nShadeModel = GL_SMOOTH;
static GLfloat g_fLineWidth = 1;

static GLfloat g_ClearColor[4] = {0, 0, 0, 0};
static GLfloat g_fClearDepth = 1;
static GLint g_nClearStencil = 0;

// g_TexUnit / g_nActiveUnit / g_nClientUnit live in GLESCompatTex.cpp

// client arrays
static SClientArray g_arrVertex, g_arrNormal, g_arrColor, g_arrSecondary, g_arrFogCoord;
static SClientArray g_arrTexCoord[GC_MAX_UNITS];
static GLuint g_nArrayBuffer = 0, g_nElementBuffer = 0;

// immediate mode
static bool g_bInBegin = false;
static GLenum g_nBeginMode = GL_TRIANGLES;
struct SImmVert {
  float pos[3];
  float tc[GC_MAX_UNITS][2];
  float nrm[3];
  float col[4];
};
static std::vector<SImmVert> g_Imm;
static GLfloat g_CurColor[4] = {1, 1, 1, 1};
static GLfloat g_CurSecColor[3] = {0, 0, 0};
static GLfloat g_CurNormal[3] = {0, 0, 1};
static GLfloat g_CurTC[GC_MAX_UNITS][4]; // s,t,r,q per unit

// display lists live in g_ListsX (see below)

// scratch buffers
static GLuint g_nStreamVBO = 0, g_nStreamEBO = 0;
GLuint g_nFBO = 0;
static std::vector<SStreamVertex> g_Stream;
static std::vector<GLushort> g_Idx16;
static std::vector<GLuint> g_Idx32;
static bool g_bES32Idx = false;

// program cache
struct SProgram {
  GLuint prog;
  GLint uMVP, uPSize, uARef, uFogColor, uRect[GC_MAX_UNITS], uEnvColor[GC_MAX_UNITS];
};
struct SProgKey {
  GLuint mask; // bitfield hash of the state below
  bool operator<(const SProgKey &k) const { return mask < k.mask; }
};
static std::map<GLuint, SProgram> g_ProgMap;
static SProgram g_CurProg;
static bool g_bProgValid = false;

// config -> key
struct SProgCfg {
  unsigned texMask : 4; // unit i enabled
  unsigned fogOn : 1;
  unsigned alphaOn : 1;
  unsigned alphaFunc : 3;
  unsigned texMode0 : 4;
  unsigned texMode1 : 4;
  unsigned texMode2 : 4;
  unsigned texMode3 : 4;
  unsigned rectMask : 4; // unit bound to rectangle texture
  unsigned combine0 : 1; // uses GLSL combine expression (vs simple mode)
  unsigned combine1 : 1;
  unsigned combine2 : 1;
  unsigned combine3 : 1;
};
static GLuint KeyHash(const SProgCfg &c)
{
  unsigned int h = 2166136261u;
  unsigned char buf[32];
  memset(buf, 0, sizeof(buf));
  buf[0] = (unsigned char)(c.texMask | (c.fogOn << 4) | (c.alphaOn << 5) | (c.alphaFunc << 6));
  buf[1] = (unsigned char)(c.texMode0 | (c.texMode1 << 4));
  buf[2] = (unsigned char)(c.texMode2 | (c.texMode3 << 4));
  buf[3] = (unsigned char)(c.rectMask | (c.combine0 << 4) | (c.combine1 << 5) | (c.combine2 << 6) | (c.combine3 << 7));
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    STexEnv &e = g_TexUnit[u].env;
    buf[4 + u] = (unsigned char)((e.combineRGB & 31) | ((e.combineA & 7) << 5));
  }
  for (size_t i = 0; i < sizeof(buf); i++)
  {
    h ^= buf[i];
    h *= 16777619u;
  }
  return (GLuint)h;
}

// ---------------------------------------------------------------------------
// texture-object shims used by the core (implemented in GLESCompatTex.cpp)
// ---------------------------------------------------------------------------
static STexObj *BoundTex(int unit)
{
  if (!g_TexUnit[unit].bEnabled2D || !g_TexUnit[unit].id2D)
    return 0;
  STexObj *t = TexGet(g_TexUnit[unit].id2D);
  return t;
}

// ---------------------------------------------------------------------------
// CPU vertex pipeline
// ---------------------------------------------------------------------------
struct SLitVert {
  GLfloat rgb[3], a;
};

static inline void TransformPoint(const Mat4 &m, const float *p, float *o)
{
  o[0] = m.m[0] * p[0] + m.m[4] * p[1] + m.m[8] * p[2] + m.m[12];
  o[1] = m.m[1] * p[0] + m.m[5] * p[1] + m.m[9] * p[2] + m.m[13];
  o[2] = m.m[2] * p[0] + m.m[6] * p[1] + m.m[10] * p[2] + m.m[14];
  o[3] = m.m[3] * p[0] + m.m[7] * p[1] + m.m[11] * p[2] + m.m[15];
}

static void UpdateNormalMat()
{
  if (!g_bNormalDirty)
    return;
  const Mat4 &m = g_mv.cur;
  // inverse-transpose of upper-left 3x3 (using full inverse of 3x3)
  float a = m.m[0], b = m.m[4], c = m.m[8];
  float d = m.m[1], e = m.m[5], f = m.m[9];
  float g = m.m[2], h = m.m[6], i = m.m[10];
  float A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
  float det = a * A + b * B + c * C;
  if (fabs(det) < 1e-20f)
    det = 1;
  float id = 1.0f / det;
  Mat4 &o = g_NormalMat;
  o.m[0] = A * id;                  o.m[4] = -(c * i - b * h) * id;   o.m[8] = (b * f - c * e) * id;
  o.m[1] = B * id;                  o.m[5] = (a * i - c * g) * id;    o.m[9] = -(a * f - c * d) * id;
  o.m[2] = C * id;                  o.m[6] = -(a * h - b * g) * id;   o.m[10] = (a * e - b * d) * id;
  o.m[3] = o.m[7] = o.m[11] = o.m[12] = o.m[13] = o.m[14] = 0;
  o.m[15] = 1;
  g_bNormalDirty = false;
}

static inline float FogFactor(float distEye)
{
  switch (g_nFogMode)
  {
  case GL_LINEAR:
  {
    float f = (g_fFogEnd - distEye) / (g_fFogEnd - g_fFogStart);
    return f < 0 ? 0 : (f > 1 ? 1 : f);
  }
  case GL_EXP:
    return expf(-g_fFogDensity * distEye);
  default: // GL_EXP2
  {
    float x = g_fFogDensity * distEye;
    return expf(-x * x);
  }
  }
}

// per-vertex fixed-function lighting (one-sided, infinite viewer)
static void LightVertex(const float *posObj, const float *nrmObj,
                        const GLfloat *baseCol, SLitVert &out)
{
  const SMaterial &M = g_MatFront;
  const GLfloat *cd = baseCol;
  GLfloat amb[4] = {M.ambient[0] * g_LightModelAmb[0], M.ambient[1] * g_LightModelAmb[1],
                    M.ambient[2] * g_LightModelAmb[2], M.ambient[3] * g_LightModelAmb[3]};
  GLfloat diff[4] = {M.diffuse[0], M.diffuse[1], M.diffuse[2], M.diffuse[3]};
  GLfloat spec[4] = {M.specular[0], M.specular[1], M.specular[2], M.specular[3]};
  if (g_bColorMaterial)
  {
    switch (g_nColorMatParam)
    {
    case GL_AMBIENT:
    case GL_AMBIENT_AND_DIFFUSE:
      amb[0] = cd[0] * g_LightModelAmb[0]; amb[1] = cd[1] * g_LightModelAmb[1];
      amb[2] = cd[2] * g_LightModelAmb[2]; amb[3] = cd[3] * g_LightModelAmb[3];
      if (g_nColorMatParam == GL_AMBIENT)
        break;
      // fallthrough
    case GL_DIFFUSE:
      diff[0] = cd[0]; diff[1] = cd[1]; diff[2] = cd[2]; diff[3] = cd[3];
      break;
    case GL_SPECULAR:
      spec[0] = cd[0]; spec[1] = cd[1]; spec[2] = cd[2]; spec[3] = cd[3];
      break;
    case GL_EMISSION:
      // rare: fold into amb
      amb[0] += cd[0]; amb[1] += cd[1]; amb[2] += cd[2];
      break;
    }
  }
  float r = M.emission[0] + amb[0], gg = M.emission[1] + amb[1], bb = M.emission[2] + amb[2];

  float pe[4];
  TransformPoint(g_mv.cur, posObj, pe);
  float ne[3];
  {
    const Mat4 &n = g_NormalMat;
    ne[0] = n.m[0] * nrmObj[0] + n.m[4] * nrmObj[1] + n.m[8] * nrmObj[2];
    ne[1] = n.m[1] * nrmObj[0] + n.m[5] * nrmObj[1] + n.m[9] * nrmObj[2];
    ne[2] = n.m[2] * nrmObj[0] + n.m[6] * nrmObj[1] + n.m[10] * nrmObj[2];
    if (g_bNormalize)
    {
      float l = sqrtf(ne[0] * ne[0] + ne[1] * ne[1] + ne[2] * ne[2]);
      if (l > 1e-20f)
      {
        l = 1 / l;
        ne[0] *= l; ne[1] *= l; ne[2] *= l;
      }
    }
  }

  for (int li = 0; li < GC_MAX_LIGHTS; li++)
  {
    if (!g_Lights[li].bOn)
      continue;
    const SLight &L = g_Lights[li].l;
    float Lv[3];
    float att = 1;
    if (L.position[3] != 0) // positional
    {
      float lp[4];
      TransformPoint(g_mv.cur, L.position, lp);
      Lv[0] = lp[0] - pe[0]; Lv[1] = lp[1] - pe[1]; Lv[2] = lp[2] - pe[2];
      float d = sqrtf(Lv[0] * Lv[0] + Lv[1] * Lv[1] + Lv[2] * Lv[2]);
      if (d > 1e-20f)
      {
        Lv[0] /= d; Lv[1] /= d; Lv[2] /= d;
      }
      if (L.attConst != 1 || L.attLin != 0 || L.attQuad != 0)
        att = 1.0f / (L.attConst + L.attLin * d + L.attQuad * d * d);
    }
    else // directional
    {
      Lv[0] = -L.position[0]; Lv[1] = -L.position[1]; Lv[2] = -L.position[2];
      float l = sqrtf(Lv[0] * Lv[0] + Lv[1] * Lv[1] + Lv[2] * Lv[2]);
      if (l > 1e-20f)
      {
        l = 1 / l;
        Lv[0] *= l; Lv[1] *= l; Lv[2] *= l;
      }
    }
    float ndl = ne[0] * Lv[0] + ne[1] * Lv[1] + ne[2] * Lv[2];
    if (ndl <= 0)
      continue;
    float spot = 1;
    if (L.spotCutoff < 180)
    {
      float sd[3];
      TransformPoint(g_mv.cur, L.spotDir, sd);
      float sl = sqrtf(sd[0] * sd[0] + sd[1] * sd[1] + sd[2] * sd[2]);
      if (sl > 1e-20f)
      {
        sd[0] /= sl; sd[1] /= sl; sd[2] /= sl;
        float cd2 = -(Lv[0] * sd[0] + Lv[1] * sd[1] + Lv[2] * sd[2]);
        if (cd2 < cosf(L.spotCutoff * ((float)M_PI / 180)))
          continue;
        spot = powf(cd2, L.spotExp);
      }
    }
    r += att * spot * (L.ambient[0] * amb[0] + L.diffuse[0] * diff[0] * ndl);
    gg += att * spot * (L.ambient[1] * amb[1] + L.diffuse[1] * diff[1] * ndl);
    bb += att * spot * (L.ambient[2] * amb[2] + L.diffuse[2] * diff[2] * ndl);
    // specular, infinite viewer: H = normalize(L + V), V=(0,0,1)
    float hx = Lv[0], hy = Lv[1], hz = Lv[2] + 1;
    float hl = sqrtf(hx * hx + hy * hy + hz * hz);
    if (hl > 1e-20f)
    {
      hl = 1 / hl;
      hx *= hl; hy *= hl; hz *= hl;
      float ndh = ne[0] * hx + ne[1] * hy + ne[2] * hz;
      if (ndh > 0)
      {
        float s = powf(ndh, M.shininess) * att * spot;
        r += L.specular[0] * spec[0] * s;
        gg += L.specular[1] * spec[1] * s;
        bb += L.specular[2] * spec[2] * s;
      }
    }
  }
  out.rgb[0] = r < 0 ? 0 : (r > 1 ? 1 : r);
  out.rgb[1] = gg < 0 ? 0 : (gg > 1 ? 1 : gg);
  out.rgb[2] = bb < 0 ? 0 : (bb > 1 ? 1 : bb);
  out.a = diff[3];
}

// ---------------------------------------------------------------------------
// shader cache
// ---------------------------------------------------------------------------
static const char *VS_SRC =
  "attribute vec4 aPos;\n"
  "attribute vec4 aCol;\n"
  "attribute vec2 aTC0;\n"
  "attribute vec2 aTC1;\n"
  "attribute vec2 aTC2;\n"
  "attribute vec2 aTC3;\n"
  "attribute float aFog;\n"
  "uniform mat4 uMVP;\n"
  "uniform float uPSize;\n"
  "varying vec4 vCol;\n"
  "varying vec2 vTC0;\n"
  "varying vec2 vTC1;\n"
  "varying vec2 vTC2;\n"
  "varying vec2 vTC3;\n"
  "varying float vFog;\n"
  "void main(){ gl_Position = uMVP * aPos; gl_PointSize = uPSize;\n"
  " vCol = aCol; vTC0 = aTC0; vTC1 = aTC1; vTC2 = aTC2; vTC3 = aTC3; vFog = aFog; }\n";

// texel expressions: q = previous color, t = texel, k = env color, c = vertex color
static void BuildCombine(char *dst, int unit, int mode, int combine, int srcRGB[3], int opRGB[3])
{
  // returns a GLSL vec4 expression into dst
  const char *t = "t";
  const char *p = "c";
  const char *k = "uEnvColor0";
  if (unit == 1) k = "uEnvColor1";
  else if (unit == 2) k = "uEnvColor2";
  else if (unit == 3) k = "uEnvColor3";
  char op[3][12];
  char sr[3][12];
  for (int i = 0; i < 3; i++)
  {
    switch (srcRGB[i])
    {
    case 0x8576: strcpy(sr[i], k); break;       // CONSTANT
    case 0x8577: strcpy(sr[i], "vCol"); break;  // PRIMARY_COLOR
    case 0x8578: strcpy(sr[i], "c"); break;     // PREVIOUS
    default: strcpy(sr[i], t); break;           // TEXTURE
    }
    if (opRGB[i] == 0x8591) // ONE_MINUS_SRC_COLOR
    {
      static char buf[3][40];
      snprintf(buf[i], sizeof(buf[i]), "(vec4(1.0)-%s)", sr[i]);
      strcpy(sr[i], buf[i]);
    }
    strcpy(op[i], ".rgb");
  }
  switch (combine)
  {
  case 0x8576: // CONSTANT -> glTexEnvf GL_*_SCALE etc: just texture
    strcpy(dst, t);
    break;
  case 0x8574: // ADD_SIGNED
    snprintf(dst, 192, "vec4(%s.rgb%s+%s.rgb%s-vec3(0.5),%s.a%s*%s.a%s)",
             sr[0], op[0], sr[1], op[1], sr[0], op[0], sr[1], op[1]);
    break;
  case 0x84E7: // SUBTRACT
    snprintf(dst, 192, "vec4(%s.rgb%s-%s.rgb%s,%s.a%s*%s.a%s)",
             sr[0], op[0], sr[1], op[1], sr[0], op[0], sr[1], op[1]);
    break;
  case 0x8575: // INTERPOLATE
    snprintf(dst, 256, "mix(%s.rgb%s,%s.rgb%s,%s.a%s),vec4(0,0,0,%s.a%s*%s.a%s)",
             sr[0], op[0], sr[1], op[1], sr[2], op[2], sr[0], op[0], sr[1], op[1]);
    break;
  case 0x8006: // DOT3 unsupported -> modulate
    WarnOnce(31, "texenv DOT3 not implemented, using modulate");
    // fallthrough
  case 0x2100: // MODULATE
    snprintf(dst, 192, "vec4(%s.rgb%s*%s.rgb%s,%s.a%s*%s.a%s)",
             sr[0], op[0], sr[1], op[1], sr[0], op[0], sr[1], op[1]);
    break;
  case 0x0104: // ADD
    snprintf(dst, 192, "vec4(%s.rgb%s+%s.rgb%s,%s.a%s*%s.a%s)",
             sr[0], op[0], sr[1], op[1], sr[0], op[0], sr[1], op[1]);
    break;
  case 0x2101: // REPLACE
    snprintf(dst, 128, "vec4(%s.rgb%s,%s.a%s)", sr[0], op[0], sr[0], op[0]);
    break;
  default:
    strcpy(dst, p);
    break;
  }
  (void)mode;
}

static bool CompileProgram(GLuint &progOut, const SProgCfg &cfg)
{
  GLuint vs = es_glCreateShader(0x8B31); // GL_VERTEX_SHADER
  GLuint fs = es_glCreateShader(0x8B30); // GL_FRAGMENT_SHADER
  const char *vsS = VS_SRC;
  es_glShaderSource(vs, 1, &vsS, 0);
  es_glCompileShader(vs);
  GLint ok = 0;
  es_glGetShaderiv(vs, 0x8B81, &ok); // COMPILE_STATUS
  if (!ok)
  {
    char log[512];
    es_glGetShaderInfoLog(vs, sizeof(log), 0, log);
    GLog("VS compile failed: %s", log);
    return false;
  }
  // NOTE: never pass &fsrc (address of a char array) as const char** -
  // glShaderSource would dereference the first 8 source bytes as a pointer
  // and crash (this exact bug killed the first menu draw on Android).
  std::string fsrc;
  fsrc +=
    "precision mediump float;\n"
    "varying vec4 vCol;\n"
    "varying vec2 vTC0;\n"
    "varying vec2 vTC1;\n"
    "varying vec2 vTC2;\n"
    "varying vec2 vTC3;\n"
    "varying float vFog;\n"
    "uniform vec4 uFogColor;\n"
    "uniform vec4 uRect0;\n"
    "uniform vec4 uRect1;\n"
    "uniform vec4 uRect2;\n"
    "uniform vec4 uRect3;\n"
    "uniform vec4 uEnvColor0;\n"
    "uniform vec4 uEnvColor1;\n"
    "uniform vec4 uEnvColor2;\n"
    "uniform vec4 uEnvColor3;\n"
    "uniform float uARef;\n";
  if (cfg.texMask & 1) fsrc += "uniform sampler2D uTex0;\n";
  if (cfg.texMask & 2) fsrc += "uniform sampler2D uTex1;\n";
  if (cfg.texMask & 4) fsrc += "uniform sampler2D uTex2;\n";
  if (cfg.texMask & 8) fsrc += "uniform sampler2D uTex3;\n";
  fsrc +=
    "void main(){\n"
    " vec4 c = vCol;\n"
    " float af = c.a;\n";
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    if (!(cfg.texMask & (1 << u)))
      continue;
    char tcname[8];
    snprintf(tcname, sizeof(tcname), "vTC%d", u);
    char rectname[8];
    snprintf(rectname, sizeof(rectname), "uRect%d", u);
    fsrc += " vec4 t" + std::to_string(u) + " = texture2D(uTex" + std::to_string(u) +
            ", " + tcname + " * " + rectname + ".xy);\n";
    char expr[320];
    BuildCombine(expr, u, g_TexUnit[u].env.mode, g_TexUnit[u].env.combineRGB,
                 g_TexUnit[u].env.srcRGB, g_TexUnit[u].env.opRGB);
    fsrc += " c = ";
    fsrc += expr;
    fsrc += ";\n";
    // alpha chain
    if (g_TexUnit[u].env.mode == GL_COMBINE)
    {
      char exprA[320];
      BuildCombine(exprA, u, 0, g_TexUnit[u].env.combineA,
                   g_TexUnit[u].env.srcA, g_TexUnit[u].env.opA);
      fsrc += " af = (";
      fsrc += exprA;
      fsrc += ");\n";
      WarnOnce(32, "combine alpha uses simplified chain");
    }
  }
  if (cfg.alphaOn)
  {
    const char *cmp = ">= ";
    switch (cfg.alphaFunc)
    {
    case GL_NEVER: cmp = 0; break;
    case GL_LESS: cmp = "< "; break;
    case GL_EQUAL: cmp = "=="; break;
    case GL_LEQUAL: cmp = "<="; break;
    case GL_GREATER: cmp = "> "; break;
    case GL_NOTEQUAL: cmp = "!="; break;
    case GL_GEQUAL: cmp = ">="; break;
    }
    if (cfg.alphaFunc == GL_NEVER)
      fsrc += " if (af >= 0.0 && af < 0.0) discard;\n";
    else
    {
      fsrc += " if (!(af ";
      fsrc += cmp;
      fsrc += " uARef)) discard;\n";
    }
  }
  if (cfg.fogOn)
    fsrc += " c.rgb = mix(uFogColor.rgb, c.rgb, vFog);\n";
  fsrc += " gl_FragColor = c;\n}\n";

  const char *fsP = fsrc.c_str();
  es_glShaderSource(fs, 1, &fsP, 0);
  es_glCompileShader(fs);
  ok = 0;
  es_glGetShaderiv(fs, 0x8B81, &ok);
  if (!ok)
  {
    char log[512];
    es_glGetShaderInfoLog(fs, sizeof(log), 0, log);
    GLog("FS compile failed: %s\nsrc:\n%.1200s", log, fsrc.c_str());
    es_glDeleteShader(vs);
    es_glDeleteShader(fs);
    return false;
  }
  GLuint pr = es_glCreateProgram();
  es_glAttachShader(pr, vs);
  es_glAttachShader(pr, fs);
  p_glBindAttribLocation(pr, 0, "aPos");
  p_glBindAttribLocation(pr, 1, "aCol");
  p_glBindAttribLocation(pr, 2, "aTC0");
  p_glBindAttribLocation(pr, 3, "aTC1");
  p_glBindAttribLocation(pr, 4, "aTC2");
  p_glBindAttribLocation(pr, 5, "aTC3");
  p_glBindAttribLocation(pr, 6, "aFog");
  es_glLinkProgram(pr);
  ok = 0;
  es_glGetProgramiv(pr, 0x8B82, &ok); // LINK_STATUS
  if (!ok)
  {
    char log[512];
    es_glGetProgramInfoLog(pr, sizeof(log), 0, log);
    GLog("link failed: %s", log);
    return false;
  }
  es_glDeleteShader(vs);
  es_glDeleteShader(fs);
  progOut = pr;
  return true;
}

static const SProgCfg *CurrentCfg()
{
  static SProgCfg c;
  memset(&c, 0, sizeof(c));
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    if (g_TexUnit[u].bEnabled2D)
    {
      c.texMask |= (1 << u);
      int m = g_TexUnit[u].env.mode;
      if (m == GL_COMBINE)
      {
        switch (u)
        {
        case 0: c.combine0 = 1; break;
        case 1: c.combine1 = 1; break;
        case 2: c.combine2 = 1; break;
        case 3: c.combine3 = 1; break;
        }
        m = GL_MODULATE; // expression built from combine state below
      }
      switch (u)
      {
      case 0: c.texMode0 = (unsigned)m & 15; break;
      case 1: c.texMode1 = (unsigned)m & 15; break;
      case 2: c.texMode2 = (unsigned)m & 15; break;
      case 3: c.texMode3 = (unsigned)m & 15; break;
      }
      STexObj *t = BoundTex(u);
      if (t && t->bRectangle)
        c.rectMask |= (1 << u);
    }
  }
  c.fogOn = g_bFog ? 1 : 0;
  c.alphaOn = g_bAlphaTest ? 1 : 0;
  c.alphaFunc = (unsigned)g_nAlphaFunc & 7;
  return &c;
}

static SProgram *GetProgram()
{
  const SProgCfg *c = CurrentCfg();
  GLuint key = KeyHash(*c);
  std::map<GLuint, SProgram>::iterator it = g_ProgMap.find(key);
  if (it != g_ProgMap.end())
    return &it->second;
  SProgram p;
  memset(&p, 0, sizeof(p));
  if (!CompileProgram(p.prog, *c))
    return 0;
  p.uMVP = es_glGetUniformLocation(p.prog, "uMVP");
  p.uPSize = es_glGetUniformLocation(p.prog, "uPSize");
  p.uARef = es_glGetUniformLocation(p.prog, "uARef");
  p.uFogColor = es_glGetUniformLocation(p.prog, "uFogColor");
  static const char *rectN[GC_MAX_UNITS] = {"uRect0", "uRect1", "uRect2", "uRect3"};
  static const char *envN[GC_MAX_UNITS] = {"uEnvColor0", "uEnvColor1", "uEnvColor2", "uEnvColor3"};
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    p.uRect[u] = es_glGetUniformLocation(p.prog, rectN[u]);
    p.uEnvColor[u] = es_glGetUniformLocation(p.prog, envN[u]);
  }
  g_ProgMap[key] = p;
  return &g_ProgMap[key];
}

// ---------------------------------------------------------------------------
// draw plumbing
// ---------------------------------------------------------------------------
static void EnsureScratch()
{
  if (!g_nStreamVBO)
  {
    es_glGenBuffers(1, &g_nStreamVBO);
    es_glGenBuffers(1, &g_nStreamEBO);
    es_glGenFramebuffers(1, &g_nFBO);
  }
}

static const int kVertFloats = 3 + 1 + GC_MAX_UNITS * 2; // pos,fog,tc
static const int kStreamStride = 52;                     // 12 floats + 4 bytes

static void PushStreamVert(std::vector<SStreamVertex> &out, const SStreamVertex &v)
{
  out.push_back(v);
}

static SStreamVertex XformVert(const float *pos, const float *nrm,
                               const GLfloat *col, const GLfloat tc[GC_MAX_UNITS][2],
                               bool bLit)
{
  SStreamVertex o;
  o.pos[0] = pos[0];
  o.pos[1] = pos[1];
  o.pos[2] = pos[2];
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    o.tc[u][0] = tc[u][0];
    o.tc[u][1] = tc[u][1];
  }
  SLitVert lv;
  lv.rgb[0] = col[0]; lv.rgb[1] = col[1]; lv.rgb[2] = col[2]; lv.a = col[3];
  if (bLit)
    LightVertex(pos, nrm, col, lv);
  o.col[0] = (GLubyte)(lv.rgb[0] * 255.0f + 0.5f);
  o.col[1] = (GLubyte)(lv.rgb[1] * 255.0f + 0.5f);
  o.col[2] = (GLubyte)(lv.rgb[2] * 255.0f + 0.5f);
  o.col[3] = (GLubyte)(lv.a * 255.0f + 0.5f);
  if (g_bFog)
  {
    float pe[4];
    TransformPoint(g_mv.cur, pos, pe);
    o.fog = FogFactor(sqrtf(pe[0] * pe[0] + pe[1] * pe[1] + pe[2] * pe[2]));
  }
  else
    o.fog = 1;
  return o;
}

// conversion of exotic primitives to triangles/lines via index remap
static void BuildConvertedIndices(GLenum mode, int n, std::vector<GLuint> &idx)
{
  idx.clear();
  switch (mode)
  {
  case GL_QUADS:
    for (int q = 0; q + 3 < n; q += 4)
    {
      idx.push_back(q); idx.push_back(q + 1); idx.push_back(q + 2);
      idx.push_back(q); idx.push_back(q + 2); idx.push_back(q + 3);
    }
    break;
  case GL_QUAD_STRIP:
    for (int q = 0; q + 3 < n; q += 2)
    {
      idx.push_back(q); idx.push_back(q + 1); idx.push_back(q + 2);
      idx.push_back(q + 2); idx.push_back(q + 1); idx.push_back(q + 3);
    }
    break;
  case GL_POLYGON:
    for (int q = 1; q + 1 < n; q++)
    {
      idx.push_back(0); idx.push_back(q); idx.push_back(q + 1);
    }
    break;
  case GL_LINE_LOOP:
    for (int q = 0; q < n; q++)
      idx.push_back(q);
    idx.push_back(0);
    break;
  default:
    for (int q = 0; q < n; q++)
      idx.push_back(q);
    break;
  }
}

// stream draw: verts are ready; mode may need conversion
static void StreamDraw(GLenum mode, int n)
{
  if (n <= 0 || g_Stream.empty())
    return;
  EnsureScratch();
  SProgram *p = GetProgram();
  if (!p)
  {
    WarnOnce(40, "no program - draw skipped");
    return;
  }
  es_glUseProgram(p->prog);
  es_glUniformMatrix4fv(p->uMVP, 1, 0, g_mv.cur.m);
  es_glUniform1f(p->uPSize, g_fPointSize);
  if (p->uARef >= 0)
    es_glUniform1f(p->uARef, g_fAlphaRef);
  if (p->uFogColor >= 0)
    es_glUniform4fv(p->uFogColor, 1, g_FogColor);
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    if (p->uRect[u] >= 0)
    {
      GLfloat rc[4] = {1, 1, 0, 0};
      STexObj *t = BoundTex(u);
      if (t && t->bRectangle && t->width > 0)
      {
        rc[0] = 1.0f / t->width;
        rc[1] = 1.0f / t->height;
      }
      es_glUniform4fv(p->uRect[u], 1, rc);
    }
    if (p->uEnvColor[u] >= 0)
      es_glUniform4fv(p->uEnvColor[u], 1, g_TexUnit[u].env.color);
  }
  es_glBindBuffer(0x8892, g_nStreamVBO); // GL_ARRAY_BUFFER
  es_glBufferData(0x8892, (GLsizeiptrARB)(g_Stream.size() * kStreamStride),
                  &g_Stream[0], 0x88E8 /*DYNAMIC_DRAW*/);
  const SProgCfg *cfg = CurrentCfg();
  es_glEnableVertexAttribArray(0);
  es_glVertexAttribPointer(0, 3, GL_FLOAT, 0, kStreamStride, (const void *)0);
  es_glEnableVertexAttribArray(1);
  es_glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, 1, kStreamStride, (const void *)48);
  es_glEnableVertexAttribArray(6);
  es_glVertexAttribPointer(6, 1, GL_FLOAT, 0, kStreamStride, (const void *)12);
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    int a = 2 + u;
    if (cfg->texMask & (1 << u))
    {
      es_glEnableVertexAttribArray(a);
      es_glVertexAttribPointer(a, 2, GL_FLOAT, 0, kStreamStride,
                               (const void *)(size_t)(16 + u * 8));
      if (p->uRect[u] >= -1 && es_glGetUniformLocation(p->prog, "uTex0") >= 0)
      {
      }
    }
    else
      es_glDisableVertexAttribArray(a);
  }
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    if (cfg->texMask & (1 << u))
    {
      es_glActiveTexture(GL_TEXTURE0 + u);
      GLint loc = es_glGetUniformLocation(p->prog, u == 0 ? "uTex0" : u == 1 ? "uTex1" : u == 2 ? "uTex2" : "uTex3");
      if (loc >= 0)
        es_glUniform1i(loc, u);
    }
  }
  es_glActiveTexture(GL_TEXTURE0 + g_nActiveUnit);

  bool bConverted = (mode == GL_QUADS || mode == GL_QUAD_STRIP || mode == GL_POLYGON || mode == GL_LINE_LOOP);
  if (bConverted)
  {
    BuildConvertedIndices(mode, n, g_Idx32);
    es_glBindBuffer(0x8893, g_nStreamEBO); // GL_ELEMENT_ARRAY_BUFFER
    es_glBufferData(0x8893, (GLsizeiptrARB)(g_Idx32.size() * 4), &g_Idx32[0], 0x88E8);
    es_glDrawElements(GL_TRIANGLES, (GLsizei)g_Idx32.size(), GL_UNSIGNED_INT, 0);
    es_glBindBuffer(0x8893, 0);
  }
  else
  {
    es_glDrawArrays(mode, 0, n);
  }
  es_glBindBuffer(0x8892, 0);
}

// ---------------------------------------------------------------------------
// immediate mode (internal impls; exported wrappers record display lists)
// ---------------------------------------------------------------------------
static void ImplVertex(float x, float y, float z)
{
  SImmVert v;
  v.pos[0] = x; v.pos[1] = y; v.pos[2] = z;
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    v.tc[u][0] = g_CurTC[u][0];
    v.tc[u][1] = g_CurTC[u][1];
  }
  v.nrm[0] = g_CurNormal[0]; v.nrm[1] = g_CurNormal[1]; v.nrm[2] = g_CurNormal[2];
  v.col[0] = g_CurColor[0]; v.col[1] = g_CurColor[1]; v.col[2] = g_CurColor[2]; v.col[3] = g_CurColor[3];
  g_Imm.push_back(v);
}

static void ImplEnd()
{
  g_bInBegin = false;
  int n = (int)g_Imm.size();
  g_Stream.clear();
  g_Stream.reserve(n);
  bool bLit = g_bLighting;
  for (int i = 0; i < n; i++)
  {
    const SImmVert &v = g_Imm[i];
    GLfloat tc2[GC_MAX_UNITS][2];
    for (int u = 0; u < GC_MAX_UNITS; u++)
    {
      tc2[u][0] = v.tc[u][0];
      tc2[u][1] = v.tc[u][1];
    }
    g_Stream.push_back(XformVert(v.pos, v.nrm, v.col, tc2, bLit));
  }
  StreamDraw(g_nBeginMode, n);
  g_Imm.clear();
  g_Stream.clear();
}

// ---------------------------------------------------------------------------
// display lists
// ---------------------------------------------------------------------------
static std::map<GLuint, SDisplayList> g_ListsX;
static GLuint g_nCurListX = 0;
bool g_bListRecording = false;
SDisplayList *g_pListCapture = 0;

static GLuint g_nListBase = 0;
static bool g_bReplaying = false;

void ListRecord(const SListCmd &c)
{
  if (g_bListRecording && g_pListCapture && !g_bReplaying)
    g_pListCapture->cmds.push_back(c);
}

// ---------------------------------------------------------------------------
// exported GL implementation
// ---------------------------------------------------------------------------
static void Rec1(EListCmdOp op, float a)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = op;
  c.f[0] = a;
  ListRecord(c);
}
static void Rec3(EListCmdOp op, float a, float b, float cc)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = op;
  c.f[0] = a; c.f[1] = b; c.f[2] = cc;
  ListRecord(c);
}
static void Rec4(EListCmdOp op, float a, float b, float cc, float d)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = op;
  c.f[0] = a; c.f[1] = b; c.f[2] = cc; c.f[3] = d;
  ListRecord(c);
}
static void RecI(EListCmdOp op, int a = 0, int b = 0)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = op;
  c.i[0] = a; c.i[1] = b;
  ListRecord(c);
}
static void RecMat(EListCmdOp op, const Mat4 &m)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = op;
  c.mat = m;
  ListRecord(c);
}

extern "C" {

// forward decls for the display-list replay interpreter
void APIENTRY glBegin(GLenum);
void APIENTRY glEnd(void);
void APIENTRY glVertex3f(GLfloat, GLfloat, GLfloat);
void APIENTRY glColor4f(GLfloat, GLfloat, GLfloat, GLfloat);
void APIENTRY glColor4ub(GLubyte, GLubyte, GLubyte, GLubyte);
void APIENTRY glTexCoord2f(GLfloat, GLfloat);
void APIENTRY glMultiTexCoord2fARB(GLenum, GLfloat, GLfloat);
void APIENTRY glNormal3f(GLfloat, GLfloat, GLfloat);
void APIENTRY glEnable(GLenum);
void APIENTRY glDisable(GLenum);
void APIENTRY glBindTexture(GLenum, GLuint);
void APIENTRY glMatrixMode(GLenum);
void APIENTRY glLoadMatrixf(const GLfloat *);
void APIENTRY glLoadIdentity(void);
void APIENTRY glTranslatef(GLfloat, GLfloat, GLfloat);
void APIENTRY glRotatef(GLfloat, GLfloat, GLfloat, GLfloat);
void APIENTRY glScalef(GLfloat, GLfloat, GLfloat);
void APIENTRY glPushMatrix(void);
void APIENTRY glPopMatrix(void);
void APIENTRY glFrustum(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
void APIENTRY glOrtho(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);

// ---- immediate ----
void APIENTRY glBegin(GLenum mode)
{
  if (g_bInBegin)
    return;
  RecI(LC_BEGIN, (int)mode);
  g_bInBegin = true;
  g_nBeginMode = mode;
  g_Imm.clear();
}
void APIENTRY glEnd(void)
{
  if (!g_bInBegin)
    return;
  RecI(LC_END);
  if (g_bListRecording)
  {
    // verts were already recorded individually; just close
    g_bInBegin = false;
    return;
  }
  ImplEnd();
}
void APIENTRY glVertex2f(GLfloat x, GLfloat y) { Rec3(LC_VERTEX3F, x, y, 0); if (!g_bListRecording) ImplVertex(x, y, 0); }
void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) { Rec3(LC_VERTEX3F, x, y, z); if (!g_bListRecording) ImplVertex(x, y, z); }
void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  Rec4(LC_VERTEX3F, x, y, z, 0);
  if (!g_bListRecording)
    ImplVertex(w != 0 ? x / w : x, w != 0 ? y / w : y, w != 0 ? z / w : z);
}
void APIENTRY glVertex2fv(const GLfloat *v) { glVertex2f(v[0], v[1]); }
void APIENTRY glVertex3fv(const GLfloat *v) { glVertex3f(v[0], v[1], v[2]); }
void APIENTRY glVertex4fv(const GLfloat *v) { glVertex4f(v[0], v[1], v[2], v[3]); }
void APIENTRY glVertex2d(GLdouble x, GLdouble y) { glVertex2f((GLfloat)x, (GLfloat)y); }
void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z) { glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) { glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w); }
void APIENTRY glVertex2dv(const GLdouble *v) { glVertex2f((GLfloat)v[0], (GLfloat)v[1]); }
void APIENTRY glVertex3dv(const GLdouble *v) { glVertex3f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]); }
void APIENTRY glVertex4dv(const GLdouble *v) { glVertex4f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]); }
void APIENTRY glVertex2i(GLint x, GLint y) { glVertex2f((GLfloat)x, (GLfloat)y); }
void APIENTRY glVertex3i(GLint x, GLint y, GLint z) { glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glVertex2s(GLshort x, GLshort y) { glVertex2f((GLfloat)x, (GLfloat)y); }
void APIENTRY glVertex3s(GLshort x, GLshort y, GLshort z) { glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z); }

void APIENTRY glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
  Rec4(LC_COLOR4F, r, g, b, 1);
  g_CurColor[0] = r; g_CurColor[1] = g; g_CurColor[2] = b; g_CurColor[3] = 1;
}
void APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
  Rec4(LC_COLOR4F, r, g, b, a);
  g_CurColor[0] = r; g_CurColor[1] = g; g_CurColor[2] = b; g_CurColor[3] = a;
}
void APIENTRY glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
  glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
void APIENTRY glColor3ub(GLubyte r, GLubyte g, GLubyte b) { glColor4ub(r, g, b, 255); }
void APIENTRY glColor3fv(const GLfloat *v) { glColor3f(v[0], v[1], v[2]); }
void APIENTRY glColor4fv(const GLfloat *v) { glColor4f(v[0], v[1], v[2], v[3]); }
void APIENTRY glColor3ubv(const GLubyte *v) { glColor3ub(v[0], v[1], v[2]); }
void APIENTRY glColor4ubv(const GLubyte *v) { glColor4ub(v[0], v[1], v[2], v[3]); }
void APIENTRY glColor3d(GLdouble r, GLdouble g, GLdouble b) { glColor3f((GLfloat)r, (GLfloat)g, (GLfloat)b); }
void APIENTRY glColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a) { glColor4f((GLfloat)r, (GLfloat)g, (GLfloat)b, (GLfloat)a); }
void APIENTRY glColor3dv(const GLdouble *v) { glColor3d(v[0], v[1], v[2]); }
void APIENTRY glColor4dv(const GLdouble *v) { glColor4d(v[0], v[1], v[2], v[3]); }
void APIENTRY glColor3i(GLint r, GLint g, GLint b) { glColor3f(r / 2147483647.0f, g / 2147483647.0f, b / 2147483647.0f); }
void APIENTRY glColor3s(GLshort r, GLshort g, GLshort b) { glColor3f(r / 32767.0f, g / 32767.0f, b / 32767.0f); }
void APIENTRY glColor4i(GLint r, GLint g, GLint b, GLint a) { glColor4f(r / 2147483647.0f, g / 2147483647.0f, b / 2147483647.0f, a / 2147483647.0f); }
void APIENTRY glColor4s(GLshort r, GLshort g, GLshort b, GLshort a) { glColor4f(r / 32767.0f, g / 32767.0f, b / 32767.0f, a / 32767.0f); }

void APIENTRY glSecondaryColor3fEXT(GLfloat r, GLfloat g, GLfloat b)
{
  g_CurSecColor[0] = r; g_CurSecColor[1] = g; g_CurSecColor[2] = b;
}
void APIENTRY glSecondaryColor3ubEXT(GLubyte r, GLubyte g, GLubyte b)
{
  glSecondaryColor3fEXT(r / 255.0f, g / 255.0f, b / 255.0f);
}
void APIENTRY glSecondaryColor3fvEXT(const GLfloat *v) { glSecondaryColor3fEXT(v[0], v[1], v[2]); }
void APIENTRY glSecondaryColor3ubvEXT(const GLubyte *v) { glSecondaryColor3ubEXT(v[0], v[1], v[2]); }

void APIENTRY glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
  Rec3(LC_NORMAL3F, x, y, z);
  g_CurNormal[0] = x; g_CurNormal[1] = y; g_CurNormal[2] = z;
}
void APIENTRY glNormal3fv(const GLfloat *v) { glNormal3f(v[0], v[1], v[2]); }
void APIENTRY glNormal3d(GLdouble x, GLdouble y, GLdouble z) { glNormal3f((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glNormal3dv(const GLdouble *v) { glNormal3d(v[0], v[1], v[2]); }
void APIENTRY glNormal3b(GLbyte x, GLbyte y, GLbyte z) { glNormal3f(x / 127.0f, y / 127.0f, z / 127.0f); }
void APIENTRY glNormal3i(GLint x, GLint y, GLint z) { glNormal3f(x / 2147483647.0f, y / 2147483647.0f, z / 2147483647.0f); }
void APIENTRY glNormal3s(GLshort x, GLshort y, GLshort z) { glNormal3f(x / 32767.0f, y / 32767.0f, z / 32767.0f); }

void APIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
  Rec3(LC_TEXCOORD2F, s, t, 0);
  g_CurTC[g_nActiveUnit][0] = s;
  g_CurTC[g_nActiveUnit][1] = t;
}
void APIENTRY glTexCoord1f(GLfloat s) { glTexCoord2f(s, 0); }
void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) { glTexCoord2f(s, t); }
void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) { glTexCoord2f(s, t); }
void APIENTRY glTexCoord2fv(const GLfloat *v) { glTexCoord2f(v[0], v[1]); }
void APIENTRY glTexCoord1fv(const GLfloat *v) { glTexCoord1f(v[0]); }
void APIENTRY glTexCoord3fv(const GLfloat *v) { glTexCoord3f(v[0], v[1], v[2]); }
void APIENTRY glTexCoord4fv(const GLfloat *v) { glTexCoord4f(v[0], v[1], v[2], v[3]); }
void APIENTRY glTexCoord2d(GLdouble s, GLdouble t) { glTexCoord2f((GLfloat)s, (GLfloat)t); }
void APIENTRY glTexCoord2dv(const GLdouble *v) { glTexCoord2d(v[0], v[1]); }
void APIENTRY glTexCoord2i(GLint s, GLint t) { glTexCoord2f((GLfloat)s, (GLfloat)t); }
void APIENTRY glTexCoord2s(GLshort s, GLshort t) { glTexCoord2f((GLfloat)s, (GLfloat)t); }

void APIENTRY glMultiTexCoord1fARB(GLenum tex, GLfloat s) { (void)tex; glTexCoord1f(s); }
static void RecMT(int u, float s, float t)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = LC_MULTITEXCOORD2F;
  c.i[0] = u;
  c.f[0] = s; c.f[1] = t;
  ListRecord(c);
}
void APIENTRY glMultiTexCoord2fARB(GLenum tex, GLfloat s, GLfloat t)
{
  int u = tex - GL_TEXTURE0;
  if (u >= 0 && u < GC_MAX_UNITS)
  {
    RecMT(u, s, t);
    g_CurTC[u][0] = s;
    g_CurTC[u][1] = t;
  }
}
void APIENTRY glMultiTexCoord3fARB(GLenum tex, GLfloat s, GLfloat t, GLfloat r) { glMultiTexCoord2fARB(tex, s, t); (void)r; }
void APIENTRY glMultiTexCoord4fARB(GLenum tex, GLfloat s, GLfloat t, GLfloat r, GLfloat q) { glMultiTexCoord2fARB(tex, s, t); (void)r; (void)q; }
void APIENTRY glMultiTexCoord1fvARB(GLenum tex, const GLfloat *v) { glMultiTexCoord1fARB(tex, v[0]); }
void APIENTRY glMultiTexCoord2fvARB(GLenum tex, const GLfloat *v) { glMultiTexCoord2fARB(tex, v[0], v[1]); }
void APIENTRY glMultiTexCoord3fvARB(GLenum tex, const GLfloat *v) { glMultiTexCoord3fARB(tex, v[0], v[1], v[2]); }
void APIENTRY glMultiTexCoord4fvARB(GLenum tex, const GLfloat *v) { glMultiTexCoord4fARB(tex, v[0], v[1], v[2], v[3]); }
void APIENTRY glMultiTexCoord1iARB(GLenum tex, GLint s) { glMultiTexCoord1fARB(tex, (GLfloat)s); }
void APIENTRY glMultiTexCoord2iARB(GLenum tex, GLint s, GLint t) { glMultiTexCoord2fARB(tex, (GLfloat)s, (GLfloat)t); }
void APIENTRY glMultiTexCoord2dARB(GLenum tex, GLdouble s, GLdouble t) { glMultiTexCoord2fARB(tex, (GLfloat)s, (GLfloat)t); }
void APIENTRY glMultiTexCoord2dvARB(GLenum tex, const GLdouble *v) { glMultiTexCoord2dARB(tex, v[0], v[1]); }

void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
  glBegin(GL_QUADS);
  glVertex2f(x1, y1); glVertex2f(x2, y1); glVertex2f(x2, y2); glVertex2f(x1, y2);
  glEnd();
}
void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) { glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2); }
void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2) { glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2); }
void APIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) { glRectf((GLfloat)x1, (GLfloat)y1, (GLfloat)x2, (GLfloat)y2); }
void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2) { glRectf(v1[0], v1[1], v2[0], v2[1]); }

void APIENTRY glArrayElement(GLint i)
{
  (void)i;
  WarnOnce(41, "glArrayElement not supported");
}

// ---- display lists ----
GLuint APIENTRY glGenLists(GLsizei range)
{
  static GLuint next = 1;
  GLuint base = next;
  next += range;
  for (int i = 0; i < range; i++)
    g_ListsX[base + i].cmds.clear();
  return base;
}
GLboolean APIENTRY glIsList(GLuint list) { return g_ListsX.find(list) != g_ListsX.end() ? GL_TRUE : GL_FALSE; }
void APIENTRY glDeleteLists(GLuint list, GLsizei range)
{
  for (int i = 0; i < range; i++)
    g_ListsX.erase(list + i);
}
void APIENTRY glNewList(GLuint list, GLenum mode)
{
  if (g_bListRecording)
    return;
  g_nCurListX = list;
  g_bListRecording = true;
  g_pListCapture = &g_ListsX[list];
  g_pListCapture->cmds.clear();
  (void)mode;
}
void APIENTRY glEndList(void)
{
  g_bListRecording = false;
  g_pListCapture = 0;
  g_nCurListX = 0;
}
void APIENTRY glCallList(GLuint list)
{
  std::map<GLuint, SDisplayList>::iterator it = g_ListsX.find(list);
  if (it == g_ListsX.end())
    return;
  g_bReplaying = true;
  const std::vector<SListCmd> &cs = it->second.cmds;
  for (size_t i = 0; i < cs.size(); i++)
  {
    const SListCmd &c = cs[i];
    switch (c.op)
    {
    case LC_BEGIN: glBegin((GLenum)c.i[0]); break;
    case LC_END: glEnd(); break;
    case LC_VERTEX3F: glVertex3f(c.f[0], c.f[1], c.f[2]); break;
    case LC_COLOR4F: glColor4f(c.f[0], c.f[1], c.f[2], c.f[3]); break;
    case LC_COLOR4UB: glColor4ub((GLubyte)c.i[0], (GLubyte)c.i[1], (GLubyte)c.i[2], (GLubyte)c.i[3]); break;
    case LC_TEXCOORD2F: glTexCoord2f(c.f[0], c.f[1]); break;
    case LC_MULTITEXCOORD2F: glMultiTexCoord2fARB(GL_TEXTURE0 + c.i[0], c.f[0], c.f[1]); break;
    case LC_NORMAL3F: glNormal3f(c.f[0], c.f[1], c.f[2]); break;
    case LC_ENABLE: glEnable((GLenum)c.i[0]); break;
    case LC_DISABLE: glDisable((GLenum)c.i[0]); break;
    case LC_BINDTEXTURE: glBindTexture((GLenum)c.i[0], (GLuint)c.i[1]); break;
    case LC_MATRIXMODE: glMatrixMode((GLenum)c.i[0]); break;
    case LC_LOADMATRIX: glLoadMatrixf(c.mat.m); break;
    case LC_IDENTITY: glLoadIdentity(); break;
    case LC_TRANSLATE: glTranslatef(c.f[0], c.f[1], c.f[2]); break;
    case LC_ROTATE: glRotatef(c.f[0], c.f[1], c.f[2], c.f[3]); break;
    case LC_SCALE: glScalef(c.f[0], c.f[1], c.f[2]); break;
    case LC_PUSHMATRIX: glPushMatrix(); break;
    case LC_POPMATRIX: glPopMatrix(); break;
    case LC_FRUSTUM: glFrustum(c.f[0], c.f[1], c.f[2], c.f[3], c.f[4], c.f[5]); break;
    case LC_ORTHO: glOrtho(c.f[0], c.f[1], c.f[2], c.f[3], c.f[4], c.f[5]); break;
    default: break;
    }
  }
  g_bReplaying = false;
}
void APIENTRY glListBase(GLuint base) { g_nListBase = base; }
void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
  for (int i = 0; i < n; i++)
  {
    GLuint id = 0;
    if (type == GL_UNSIGNED_BYTE) id = ((const GLubyte *)lists)[i];
    else if (type == GL_UNSIGNED_SHORT) id = ((const GLushort *)lists)[i];
    else if (type == GL_UNSIGNED_INT) id = ((const GLuint *)lists)[i];
    glCallList(g_nListBase + id);
  }
}

// ---- matrices ----
static SMatrixSet *CurSet(void)
{
  SMatrixSet *s = ModeSet(g_nMatrixMode);
  if (!s)
    s = &g_mv;
  return s;
}
void APIENTRY glMatrixMode(GLenum mode)
{
  RecI(LC_MATRIXMODE, (int)mode);
  g_nMatrixMode = mode;
}
void APIENTRY glPushMatrix(void)
{
  RecI(LC_PUSHMATRIX);
  SMatrixSet *s = CurSet();
  if (s->depth < s->maxDepth - 1)
  {
    s->stack[s->depth + 1] = s->stack[s->depth];
    s->cur = s->stack[s->depth + 1];
    s->depth++;
  }
  else
    GSetError(GL_STACK_OVERFLOW);
}
void APIENTRY glPopMatrix(void)
{
  RecI(LC_POPMATRIX);
  SMatrixSet *s = CurSet();
  if (s->depth > 0)
  {
    s->depth--;
    s->cur = s->stack[s->depth];
  }
  else
    GSetError(GL_STACK_UNDERFLOW);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glLoadIdentity(void)
{
  RecI(LC_IDENTITY);
  MatIdentity(CurSet()->cur);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glLoadMatrixf(const GLfloat *m)
{
  RecMat(LC_LOADMATRIX, *(const Mat4 *)m);
  memcpy(CurSet()->cur.m, m, sizeof(Mat4));
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glLoadMatrixd(const GLdouble *m)
{
  Mat4 f;
  for (int i = 0; i < 16; i++)
    f.m[i] = (GLfloat)m[i];
  glLoadMatrixf(f.m);
}
void APIENTRY glMultMatrixf(const GLfloat *m)
{
  Mat4 t;
  memcpy(t.m, m, sizeof(Mat4));
  Mat4 &c = CurSet()->cur;
  MatMul(c, c, t);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glMultMatrixd(const GLdouble *m)
{
  Mat4 f;
  for (int i = 0; i < 16; i++)
    f.m[i] = (GLfloat)m[i];
  glMultMatrixf(f.m);
}
void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
  Rec3(LC_TRANSLATE, x, y, z);
  Mat4 t;
  MatIdentity(t);
  t.m[12] = x; t.m[13] = y; t.m[14] = z;
  Mat4 &c = CurSet()->cur;
  MatMul(c, c, t);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z) { glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
  Rec3(LC_SCALE, x, y, z);
  Mat4 t;
  MatIdentity(t);
  t.m[0] = x; t.m[5] = y; t.m[10] = z;
  Mat4 &c = CurSet()->cur;
  MatMul(c, c, t);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z) { glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  Rec4(LC_ROTATE, angle, x, y, z);
  float l = sqrtf(x * x + y * y + z * z);
  if (l < 1e-20f)
    return;
  x /= l; y /= l; z /= l;
  float a = angle * ((float)M_PI / 180.0f);
  float s = sinf(a), c = cosf(a), ic = 1 - c;
  Mat4 r;
  r.m[0] = x * x * ic + c;     r.m[4] = x * y * ic - z * s; r.m[8] = x * z * ic + y * s; r.m[12] = 0;
  r.m[1] = y * x * ic + z * s; r.m[5] = y * y * ic + c;     r.m[9] = y * z * ic - x * s; r.m[13] = 0;
  r.m[2] = x * z * ic - y * s; r.m[6] = y * z * ic + x * s; r.m[10] = z * z * ic + c;    r.m[14] = 0;
  r.m[3] = 0; r.m[7] = 0; r.m[11] = 0; r.m[15] = 1;
  Mat4 &m = CurSet()->cur;
  MatMul(m, m, r);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) { glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z); }
void APIENTRY glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = LC_FRUSTUM;
  float ff[6] = {(float)l, (float)r, (float)b, (float)t, (float)n, (float)f};
  memcpy(c.f, ff, sizeof(ff));
  ListRecord(c);
  Mat4 m;
  m.m[0] = (GLfloat)(2 * n / (r - l));
  m.m[4] = 0; m.m[8] = (GLfloat)((r + l) / (r - l)); m.m[12] = 0;
  m.m[1] = 0;
  m.m[5] = (GLfloat)(2 * n / (t - b));
  m.m[9] = (GLfloat)((t + b) / (t - b)); m.m[13] = 0;
  m.m[2] = 0; m.m[6] = 0;
  m.m[10] = (GLfloat)(-(f + n) / (f - n));
  m.m[14] = (GLfloat)(-2 * f * n / (f - n));
  m.m[3] = 0; m.m[7] = 0; m.m[11] = -1; m.m[15] = 0;
  Mat4 &mm = CurSet()->cur;
  MatMul(mm, mm, m);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f)
{
  SListCmd c;
  memset(&c, 0, sizeof(c));
  c.op = LC_ORTHO;
  float ff[6] = {(float)l, (float)r, (float)b, (float)t, (float)n, (float)f};
  memcpy(c.f, ff, sizeof(ff));
  ListRecord(c);
  Mat4 m;
  MatIdentity(m);
  m.m[0] = (GLfloat)(2 / (r - l));
  m.m[5] = (GLfloat)(2 / (t - b));
  m.m[10] = (GLfloat)(-2 / (f - n));
  m.m[12] = (GLfloat)(-(r + l) / (r - l));
  m.m[13] = (GLfloat)(-(t + b) / (t - b));
  m.m[14] = (GLfloat)(-(f + n) / (f - n));
  Mat4 &mm = CurSet()->cur;
  MatMul(mm, mm, m);
  if (g_nMatrixMode == GL_MODELVIEW)
    g_bNormalDirty = true;
}
void APIENTRY glDepthRange(GLclampd n, GLclampd f)
{
  if (es_glDepthRangef)
    es_glDepthRangef((GLfloat)n, (GLfloat)f);
}

// ---- enables ----
void APIENTRY glEnable(GLenum cap)
{
  RecI(LC_ENABLE, (int)cap);
  switch (cap)
  {
  case GL_FOG: g_bFog = true; return;
  case GL_LIGHTING: g_bLighting = true; return;
  case GL_NORMALIZE: g_bNormalize = true; return;
  case GL_COLOR_MATERIAL: g_bColorMaterial = true; return;
  case GL_ALPHA_TEST: g_bAlphaTest = true; return;
  case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
  case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
    g_Lights[cap - GL_LIGHT0].bOn = true; return;
  case GL_TEXTURE_1D:
  case GL_TEXTURE_2D:
    if (g_nActiveUnit < GC_MAX_UNITS)
      g_TexUnit[g_nActiveUnit].bEnabled2D = true;
    return;
  case GL_TEXTURE_CUBE_MAP:
    if (es_glEnable) es_glEnable(cap);
    return;
  case GL_TEXTURE_GEN_S: case GL_TEXTURE_GEN_T:
  case GL_TEXTURE_GEN_R: case GL_TEXTURE_GEN_Q:
    WarnOnce(51, "texgen requested - ignored");
    return;
  default:
    if (es_glEnable)
      es_glEnable(cap);
    return;
  }
}
void APIENTRY glDisable(GLenum cap)
{
  RecI(LC_DISABLE, (int)cap);
  switch (cap)
  {
  case GL_FOG: g_bFog = false; return;
  case GL_LIGHTING: g_bLighting = false; return;
  case GL_NORMALIZE: g_bNormalize = false; return;
  case GL_COLOR_MATERIAL: g_bColorMaterial = false; return;
  case GL_ALPHA_TEST: g_bAlphaTest = false; return;
  case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
  case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
    g_Lights[cap - GL_LIGHT0].bOn = false; return;
  case GL_TEXTURE_1D:
  case GL_TEXTURE_2D:
    if (g_nActiveUnit < GC_MAX_UNITS)
      g_TexUnit[g_nActiveUnit].bEnabled2D = false;
    return;
  case GL_TEXTURE_CUBE_MAP:
    if (es_glDisable) es_glDisable(cap);
    return;
  default:
    if (es_glDisable)
      es_glDisable(cap);
    return;
  }
}
GLboolean APIENTRY glIsEnabled(GLenum cap)
{
  switch (cap)
  {
  case GL_FOG: return g_bFog ? GL_TRUE : GL_FALSE;
  case GL_LIGHTING: return g_bLighting ? GL_TRUE : GL_FALSE;
  case GL_ALPHA_TEST: return g_bAlphaTest ? GL_TRUE : GL_FALSE;
  case GL_COLOR_MATERIAL: return g_bColorMaterial ? GL_TRUE : GL_FALSE;
  case GL_NORMALIZE: return g_bNormalize ? GL_TRUE : GL_FALSE;
  case GL_TEXTURE_1D:
  case GL_TEXTURE_2D:
    return (g_nActiveUnit < GC_MAX_UNITS && g_TexUnit[g_nActiveUnit].bEnabled2D) ? GL_TRUE : GL_FALSE;
  default:
    return es_glIsEnabled ? es_glIsEnabled(cap) : GL_FALSE;
  }
}

// ---- state setters ----
void APIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
  g_nAlphaFunc = func;
  g_fAlphaRef = (GLfloat)ref;
}
void APIENTRY glFogi(GLenum pname, GLint param)
{
  if (pname == GL_FOG_MODE) g_nFogMode = param;
}
void APIENTRY glFogf(GLenum pname, GLfloat param)
{
  switch (pname)
  {
  case GL_FOG_MODE: g_nFogMode = (GLint)param; break;
  case GL_FOG_DENSITY: g_fFogDensity = param; break;
  case GL_FOG_START: g_fFogStart = param; break;
  case GL_FOG_END: g_fFogEnd = param; break;
  }
}
void APIENTRY glFogfv(GLenum pname, const GLfloat *params)
{
  if (pname == GL_FOG_COLOR)
    memcpy(g_FogColor, params, sizeof(g_FogColor));
  else if (pname == GL_FOG_MODE)
    glFogf(pname, params[0]);
  else if (pname == GL_FOG_DENSITY || pname == GL_FOG_START || pname == GL_FOG_END)
    glFogf(pname, params[0]);
}
void APIENTRY glShadeModel(GLenum mode) { g_nShadeModel = mode; }
void APIENTRY glLightModelf(GLenum pname, GLfloat param) { (void)pname; (void)param; }
void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params)
{
  if (pname == GL_LIGHT_MODEL_AMBIENT)
    memcpy(g_LightModelAmb, params, sizeof(g_LightModelAmb));
}
void APIENTRY glLightModeli(GLenum pname, GLint param) { (void)pname; (void)param; }
void APIENTRY glLightModeliv(GLenum pname, const GLint *params)
{
  if (pname == GL_LIGHT_MODEL_AMBIENT)
    for (int i = 0; i < 4; i++) g_LightModelAmb[i] = (GLfloat)params[i];
}
static SLight *LightByIdx(GLenum light)
{
  int i = light - GL_LIGHT0;
  if (i < 0 || i >= GC_MAX_LIGHTS)
    return 0;
  return &g_Lights[i].l;
}
static SMaterial *MatByFace(GLenum face)
{
  return face == GL_BACK ? &g_MatBack : &g_MatFront;
}
void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
  SLight *L = LightByIdx(light);
  if (!L) return;
  switch (pname)
  {
  case GL_SPOT_EXPONENT: L->spotExp = param; break;
  case GL_SPOT_CUTOFF: L->spotCutoff = param; break;
  case GL_CONSTANT_ATTENUATION: L->attConst = param; break;
  case GL_LINEAR_ATTENUATION: L->attLin = param; break;
  case GL_QUADRATIC_ATTENUATION: L->attQuad = param; break;
  }
}
void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
  SLight *L = LightByIdx(light);
  if (!L) return;
  switch (pname)
  {
  case GL_AMBIENT: memcpy(L->ambient, params, 16); break;
  case GL_DIFFUSE: memcpy(L->diffuse, params, 16); break;
  case GL_SPECULAR: memcpy(L->specular, params, 16); break;
  case GL_POSITION: memcpy(L->position, params, 16); break;
  case GL_SPOT_DIRECTION: memcpy(L->spotDir, params, 12); break;
  default: glLightf(light, pname, params[0]); break;
  }
}
void APIENTRY glLighti(GLenum light, GLenum pname, GLint param) { glLightf(light, pname, (GLfloat)param); }
void APIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params)
{
  GLfloat f[4];
  for (int i = 0; i < 4; i++) f[i] = (GLfloat)params[i];
  glLightfv(light, pname, f);
}
void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
  SMaterial *M = MatByFace(face);
  if (pname == GL_SHININESS)
    M->shininess = param < 0 ? 0 : (param > 128 ? 128 : param);
}
void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
  SMaterial *M = MatByFace(face);
  switch (pname)
  {
  case GL_AMBIENT: memcpy(M->ambient, params, 16); break;
  case GL_DIFFUSE: memcpy(M->diffuse, params, 16); break;
  case GL_SPECULAR: memcpy(M->specular, params, 16); break;
  case GL_EMISSION: memcpy(M->emission, params, 16); break;
  case GL_SHININESS: glMaterialf(face, pname, params[0]); break;
  case GL_AMBIENT_AND_DIFFUSE:
    memcpy(M->ambient, params, 16);
    memcpy(M->diffuse, params, 16);
    break;
  default: break;
  }
  if (face == GL_FRONT_AND_BACK)
  {
    // mirror to back
    SMaterial save = g_MatFront;
    g_MatFront = *MatByFace(GL_FRONT);
    (void)save;
    g_MatBack = g_MatFront;
  }
}
void APIENTRY glMateriali(GLenum face, GLenum pname, GLint param) { glMaterialf(face, pname, (GLfloat)param); }
void APIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
  GLfloat f[4];
  for (int i = 0; i < 4; i++) f[i] = (GLfloat)params[i];
  glMaterialfv(face, pname, f);
}
void APIENTRY glColorMaterial(GLenum face, GLenum mode)
{
  g_nColorMatFace = face;
  g_nColorMatParam = mode;
}
void APIENTRY glPointSize(GLfloat size) { g_fPointSize = size > 0 ? size : 1; }
void APIENTRY glLineWidth(GLfloat w) { if (es_glLineWidth) es_glLineWidth(w > 0 ? w : 1); }
void APIENTRY glPointParameterfARB(GLenum pname, GLfloat param) { (void)pname; (void)param; WarnOnce(52, "point parameter ignored"); }
void APIENTRY glPointParameterfvARB(GLenum pname, const GLfloat *param) { (void)pname; (void)param; WarnOnce(52, "point parameters ignored"); }
void APIENTRY glPointParameterfEXT(GLenum pname, GLfloat param) { glPointParameterfARB(pname, param); }
void APIENTRY glPointParameterfvEXT(GLenum pname, const GLfloat *param) { glPointParameterfvARB(pname, param); }

// ---- clears / viewport / raster state ----
void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
  g_ClearColor[0] = r; g_ClearColor[1] = g; g_ClearColor[2] = b; g_ClearColor[3] = a;
  if (es_glClearColor)
    es_glClearColor(r, g, b, a);
}
void APIENTRY glClearDepth(GLclampd d)
{
  g_fClearDepth = (GLfloat)d;
  if (es_glClearDepthf)
    es_glClearDepthf((GLfloat)d);
}
void APIENTRY glClearStencil(GLint s)
{
  g_nClearStencil = s;
  if (es_glClearStencil)
    es_glClearStencil(s);
}
void APIENTRY glClear(GLbitfield mask)
{
  if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    mask &= (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if (es_glClear)
    es_glClear(mask);
}
void APIENTRY glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { if (es_glViewport) es_glViewport(x, y, w, h); }
void APIENTRY glScissor(GLint x, GLint y, GLsizei w, GLsizei h) { if (es_glScissor) es_glScissor(x, y, w, h); }
void APIENTRY glDepthFunc(GLenum f) { if (es_glDepthFunc) es_glDepthFunc(f); }
void APIENTRY glDepthMask(GLboolean f) { if (es_glDepthMask) es_glDepthMask(f); }
void APIENTRY glCullFace(GLenum m) { if (es_glCullFace) es_glCullFace(m); }
void APIENTRY glFrontFace(GLenum m) { if (es_glFrontFace) es_glFrontFace(m); }
void APIENTRY glStencilFunc(GLenum f, GLint r, GLuint m) { if (es_glStencilFunc) es_glStencilFunc(f, r, m); }
void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { if (es_glStencilOp) es_glStencilOp(fail, zfail, zpass); }
void APIENTRY glStencilMask(GLuint m) { if (es_glStencilMask) es_glStencilMask(m); }
void APIENTRY glBlendFunc(GLenum s, GLenum d) { if (es_glBlendFunc) es_glBlendFunc(s, d); }
void APIENTRY glBlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) { if (es_glBlendColor) es_glBlendColor(r, g, b, a); }
void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
  if (es_glPolygonOffset)
    es_glPolygonOffset(factor, units);
}
void APIENTRY glDrawBuffer(GLenum mode) { (void)mode; }
void APIENTRY glReadBuffer(GLenum mode) { (void)mode; }
void APIENTRY glPixelStorei(GLenum pname, GLint param)
{
  if (es_glPixelStorei && (pname == GL_UNPACK_ALIGNMENT || pname == GL_PACK_ALIGNMENT ||
                           pname == GL_UNPACK_ROW_LENGTH || pname == GL_PACK_ROW_LENGTH ||
                           pname == GL_UNPACK_SKIP_PIXELS || pname == GL_UNPACK_SKIP_ROWS ||
                           pname == GL_PACK_SKIP_PIXELS || pname == GL_PACK_SKIP_ROWS))
    es_glPixelStorei(pname, param);
}
void APIENTRY glPixelTransferf(GLenum pname, GLfloat param) { (void)pname; (void)param; }
void APIENTRY glPixelTransferi(GLenum pname, GLint param) { (void)pname; (void)param; }
void APIENTRY glFinish(void) { if (es_glFinish) es_glFinish(); }
void APIENTRY glFlush(void) { if (es_glFlush) es_glFlush(); }
void APIENTRY glHint(GLenum target, GLenum mode) { if (es_glHint) es_glHint(target, mode); }
void APIENTRY glClipPlane(GLenum plane, const GLdouble *eq)
{
  (void)plane; (void)eq;
  WarnOnce(53, "glClipPlane ignored (no user clip planes on ES3)");
}
void APIENTRY glGetClipPlane(GLenum plane, GLdouble *eq)
{
  (void)plane;
  eq[0] = eq[1] = eq[2] = eq[3] = 0;
}
void APIENTRY glEdgeFlag(GLboolean f) { (void)f; }
void APIENTRY glEdgeFlagv(const GLboolean *f) { (void)f; }
void APIENTRY glIndexi(GLint c) { (void)c; }
void APIENTRY glIndexf(GLfloat c) { (void)c; }
void APIENTRY glLogicOp(GLenum op) { (void)op; }

// ---- errors / strings ----
GLenum APIENTRY glGetError(void)
{
  if (g_lastError != GL_NO_ERROR)
  {
    GLenum e = g_lastError;
    g_lastError = GL_NO_ERROR;
    return e;
  }
  return es_glGetError ? es_glGetError() : GL_NO_ERROR;
}
static std::string g_strExtensions;
const GLubyte APIENTRY *glGetString(GLenum name)
{
  static const GLubyte sVendor = 0;
  (void)sVendor;
  switch (name)
  {
  case GL_VENDOR:
    return (const GLubyte *)"NearChuckle";
  case GL_RENDERER:
    return (const GLubyte *)"GLESCompat (fixed-function over OpenGL ES 3)";
  case GL_VERSION:
    return (const GLubyte *)"1.5.0 GLESCompat 1.0";
  case GL_EXTENSIONS:
  {
    if (g_strExtensions.empty())
    {
      g_strExtensions =
        "GL_ARB_multitexture "
        "GL_ARB_texture_env_combine GL_EXT_texture_env_combine "
        "GL_ARB_texture_env_add GL_EXT_texture_env_add "
        "GL_ARB_texture_compression GL_EXT_texture_compression_s3tc "
        "GL_ARB_vertex_buffer_object GL_EXT_draw_range_elements "
        "GL_EXT_secondary_color GL_EXT_bgra GL_ARB_texture_non_power_of_two "
        "GL_SGIS_generate_mipmap GL_ARB_point_parameters GL_EXT_point_parameters "
        "GL_EXT_texture_filter_anisotropic GL_ARB_texture_mirrored_repeat "
        "GL_EXT_texture_cube_map GL_EXT_texture_lod_bias "
        "GL_ARB_texture_env_crossbar GL_NV_blend_square "
        "GL_NV_fog_distance GL_EXT_clip_volume_hint";
    }
    return (const GLubyte *)g_strExtensions.c_str();
  }
  default:
    break;
  }
  return (const GLubyte *)"";
}

// ---- gets ----
void APIENTRY glGetFloatv(GLenum pname, GLfloat *params)
{
  switch (pname)
  {
  case GL_MODELVIEW_MATRIX: memcpy(params, g_mv.cur.m, 64); return;
  case GL_PROJECTION_MATRIX: memcpy(params, g_pj.cur.m, 64); return;
  case GL_TEXTURE_MATRIX: memcpy(params, g_tx[g_nActiveUnit < GC_MAX_UNITS ? g_nActiveUnit : 0].cur.m, 64); return;
  case GL_FOG_COLOR: memcpy(params, g_FogColor, 16); return;
  case GL_FOG_DENSITY: params[0] = g_fFogDensity; return;
  case GL_FOG_START: params[0] = g_fFogStart; return;
  case GL_FOG_END: params[0] = g_fFogEnd; return;
  case GL_CURRENT_COLOR: memcpy(params, g_CurColor, 16); return;
  case GL_POINT_SIZE: params[0] = g_fPointSize; return;
  case GL_LINE_WIDTH: params[0] = g_fLineWidth; return;
  case GL_COLOR_CLEAR_VALUE: memcpy(params, g_ClearColor, 16); return;
  case GL_DEPTH_CLEAR_VALUE: params[0] = g_fClearDepth; return;
  case GL_LIGHT_MODEL_AMBIENT: memcpy(params, g_LightModelAmb, 16); return;
  case GL_TEXTURE_ENV_COLOR:
    memcpy(params, g_TexUnit[g_nActiveUnit < GC_MAX_UNITS ? g_nActiveUnit : 0].env.color, 16);
    return;
  default:
    if (es_glGetFloatv) { es_glGetFloatv(pname, params); return; }
    params[0] = 0;
    return;
  }
}
void APIENTRY glGetIntegerv(GLenum pname, GLint *params)
{
  switch (pname)
  {
  case GL_MATRIX_MODE: params[0] = g_nMatrixMode; return;
  case GL_MAX_MODELVIEW_STACK_DEPTH: params[0] = MAX_MV; return;
  case GL_MAX_PROJECTION_STACK_DEPTH: params[0] = MAX_PJ; return;
  case GL_MAX_TEXTURE_STACK_DEPTH: params[0] = MAX_TX; return;
  case GL_MAX_TEXTURE_UNITS: params[0] = GC_MAX_UNITS; return;
  case GL_MAX_ACTIVE_TEXTURES: params[0] = GC_MAX_UNITS; return;
  case GL_MAX_LIGHTS: params[0] = GC_MAX_LIGHTS; return;
  case GL_ARRAY_BUFFER_BINDING: params[0] = (GLint)g_nArrayBuffer; return;
  case GL_ELEMENT_ARRAY_BUFFER_BINDING: params[0] = (GLint)g_nElementBuffer; return;
  case GL_TEXTURE_BINDING_2D:
  case GL_TEXTURE_BINDING_1D:
    params[0] = (GLint)(g_nActiveUnit < GC_MAX_UNITS ? g_TexUnit[g_nActiveUnit].id2D : 0);
    return;
  case GL_FOG_MODE: params[0] = g_nFogMode; return;
  case GL_SHADE_MODEL: params[0] = g_nShadeModel; return;
  case GL_ALPHA_TEST_FUNC: params[0] = g_nAlphaFunc; return;
  case GL_NUM_COMPRESSED_TEXTURE_FORMATS: params[0] = 4; return;
  case GL_COMPRESSED_TEXTURE_FORMATS:
    params[0] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    params[1] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    params[2] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    params[3] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    return;
  default:
    if (es_glGetIntegerv)
    {
      es_glGetIntegerv(pname, params);
      return;
    }
    params[0] = 0;
    return;
  }
}
void APIENTRY glGetBooleanv(GLenum pname, GLboolean *params)
{
  GLint i = 0;
  glGetIntegerv(pname, &i);
  params[0] = (GLboolean)i;
}
void APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
  SMaterial *M = MatByFace(face);
  switch (pname)
  {
  case GL_AMBIENT: memcpy(params, M->ambient, 16); break;
  case GL_DIFFUSE: memcpy(params, M->diffuse, 16); break;
  case GL_SPECULAR: memcpy(params, M->specular, 16); break;
  case GL_EMISSION: memcpy(params, M->emission, 16); break;
  case GL_SHININESS: params[0] = M->shininess; break;
  }
}
void APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
  SLight *L = LightByIdx(light);
  if (!L) return;
  switch (pname)
  {
  case GL_AMBIENT: memcpy(params, L->ambient, 16); break;
  case GL_DIFFUSE: memcpy(params, L->diffuse, 16); break;
  case GL_SPECULAR: memcpy(params, L->specular, 16); break;
  case GL_POSITION: memcpy(params, L->position, 16); break;
  case GL_SPOT_DIRECTION: memcpy(params, L->spotDir, 12); break;
  }
}

// ---- buffers ----
void APIENTRY glBindBufferARB(GLenum target, GLuint buf)
{
  if (target == GL_ARRAY_BUFFER)
  {
    g_nArrayBuffer = buf;
  }
  else if (target == GL_ELEMENT_ARRAY_BUFFER)
  {
    g_nElementBuffer = buf;
  }
  if (es_glBindBuffer)
    es_glBindBuffer(target, buf);
}
void APIENTRY glDeleteBuffersARB(GLsizei n, const GLuint *bufs)
{
  for (int i = 0; i < n; i++)
  {
    if (g_nArrayBuffer == bufs[i]) g_nArrayBuffer = 0;
    if (g_nElementBuffer == bufs[i]) g_nElementBuffer = 0;
  }
  if (es_glDeleteBuffers)
    es_glDeleteBuffers(n, bufs);
}
void APIENTRY glGenBuffersARB(GLsizei n, GLuint *bufs) { if (es_glGenBuffers) es_glGenBuffers(n, bufs); }
GLboolean APIENTRY glIsBufferARB(GLuint buf) { return buf != 0; }
void APIENTRY glBufferDataARB(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage)
{
  if (es_glBufferData)
    es_glBufferData(target, size, data, usage);
}
void APIENTRY glBufferSubDataARB(GLenum target, GLintptrARB ofs, GLsizeiptrARB size, const void *data)
{
  if (es_glBufferSubData)
    es_glBufferSubData(target, ofs, size, data);
}
void *APIENTRY glMapBufferARB(GLenum target, GLenum access)
{
  if (!es_glMapBufferRange)
    return 0;
  GLint sz = 0;
  if (es_glGetIntegerv)
  {
    es_glBindBuffer(target, target == GL_ELEMENT_ARRAY_BUFFER ? g_nElementBuffer : g_nArrayBuffer);
    GLint p = 0x8764; // GL_BUFFER_SIZE
    es_glGetIntegerv(p, &sz);
  }
  GLbitfield flags = 0x000A | 0x0040; // READ|WRITE | UNSYNC? keep simple: READ|WRITE|INVALIDATE_BUFFER
  flags = 0x0001 | 0x0002 | 0x0008 | 0x0004; // MAP_READ|MAP_WRITE|INVALIDATE_BUFFER|? -> read+write+invalidate
  return es_glMapBufferRange(target, 0, sz, flags);
}
GLboolean APIENTRY glUnmapBufferARB(GLenum target)
{
  return es_glUnmapBuffer ? es_glUnmapBuffer(target) : GL_FALSE;
}
void APIENTRY glGetBufferParameterivARB(GLenum target, GLenum pname, GLint *params)
{
  if (es_glGetIntegerv)
    es_glGetIntegerv(pname, params);
  (void)target;
}

// ---- client arrays ----
void APIENTRY glClientActiveTextureARB(GLenum tex)
{
  int u = tex - GL_TEXTURE0;
  if (u >= 0 && u < GC_MAX_UNITS)
    g_nClientUnit = u;
}
void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  g_arrVertex.size = size; g_arrVertex.type = type; g_arrVertex.stride = stride;
  g_arrVertex.ptr = ptr; g_arrVertex.vbo = g_nArrayBuffer;
}
void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  g_arrColor.size = size; g_arrColor.type = type; g_arrColor.stride = stride;
  g_arrColor.ptr = ptr; g_arrColor.vbo = g_nArrayBuffer;
}
void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
  g_arrNormal.size = 3; g_arrNormal.type = type; g_arrNormal.stride = stride;
  g_arrNormal.ptr = ptr; g_arrNormal.vbo = g_nArrayBuffer;
}
void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  if (g_nClientUnit >= GC_MAX_UNITS)
    return;
  g_arrTexCoord[g_nClientUnit].size = size; g_arrTexCoord[g_nClientUnit].type = type;
  g_arrTexCoord[g_nClientUnit].stride = stride;
  g_arrTexCoord[g_nClientUnit].ptr = ptr;
  g_arrTexCoord[g_nClientUnit].vbo = g_nArrayBuffer;
}
void APIENTRY glSecondaryColorPointerEXT(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
  g_arrSecondary.size = size; g_arrSecondary.type = type; g_arrSecondary.stride = stride;
  g_arrSecondary.ptr = ptr; g_arrSecondary.vbo = g_nArrayBuffer;
}
void APIENTRY glFogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr)
{
  (void)type; (void)stride; (void)ptr;
  WarnOnce(54, "fog coord array ignored");
}
void APIENTRY glEnableClientState(GLenum cap)
{
  switch (cap)
  {
  case GL_VERTEX_ARRAY: g_arrVertex.enabled = true; break;
  case GL_NORMAL_ARRAY: g_arrNormal.enabled = true; break;
  case GL_COLOR_ARRAY: g_arrColor.enabled = true; break;
  case GL_SECONDARY_COLOR_ARRAY: g_arrSecondary.enabled = true; break;
  case GL_FOG_COORDINATE_ARRAY: break;
  case GL_TEXTURE_COORD_ARRAY:
    if (g_nClientUnit < GC_MAX_UNITS)
      g_arrTexCoord[g_nClientUnit].enabled = true;
    break;
  default: break;
  }
}
void APIENTRY glDisableClientState(GLenum cap)
{
  switch (cap)
  {
  case GL_VERTEX_ARRAY: g_arrVertex.enabled = false; break;
  case GL_NORMAL_ARRAY: g_arrNormal.enabled = false; break;
  case GL_COLOR_ARRAY: g_arrColor.enabled = false; break;
  case GL_SECONDARY_COLOR_ARRAY: g_arrSecondary.enabled = false; break;
  case GL_FOG_COORDINATE_ARRAY: break;
  case GL_TEXTURE_COORD_ARRAY:
    if (g_nClientUnit < GC_MAX_UNITS)
      g_arrTexCoord[g_nClientUnit].enabled = false;
    break;
  default: break;
  }
}

// read one component-set from a client array
static void ReadArr(const SClientArray &a, int i, GLfloat *out, int want)
{
  for (int k = 0; k < want; k++)
    out[k] = 0;
  if (!a.ptr)
    return;
  int compSize = 1;
  switch (a.type)
  {
  case GL_FLOAT: compSize = 4; break;
  case GL_DOUBLE: compSize = 8; break;
  case GL_UNSIGNED_BYTE: compSize = 1; break;
  case GL_BYTE: compSize = 1; break;
  case GL_SHORT: compSize = 2; break;
  case GL_UNSIGNED_SHORT: compSize = 2; break;
  case GL_INT: compSize = 4; break;
  case GL_UNSIGNED_INT: compSize = 4; break;
  }
  int stride = a.stride ? a.stride : a.size * compSize;
  const unsigned char *p = (const unsigned char *)a.ptr + (size_t)i * stride;
  int nc = a.size < want ? a.size : want;
  for (int k = 0; k < nc; k++)
  {
    switch (a.type)
    {
    case GL_FLOAT: out[k] = ((const GLfloat *)p)[k]; break;
    case GL_DOUBLE: out[k] = (GLfloat)((const GLdouble *)p)[k]; break;
    case GL_UNSIGNED_BYTE: out[k] = p[k] / 255.0f; break;
    case GL_BYTE: out[k] = ((const GLbyte *)p)[k] / 127.0f; break;
    case GL_SHORT: out[k] = ((const GLshort *)p)[k] / 32767.0f; break;
    case GL_UNSIGNED_SHORT: out[k] = ((const GLushort *)p)[k] / 65535.0f; break;
    case GL_INT: out[k] = (GLfloat)((const GLint *)p)[k] / 2147483647.0f; break;
    case GL_UNSIGNED_INT: out[k] = (GLfloat)((const GLuint *)p)[k] / 4294967295.0f; break;
    }
  }
}

// convert client buffers into the streamed draw (with staging of VBO data)
struct SStage {
  std::vector<unsigned char> vdata;
  const unsigned char *vertP = 0, *nrmP = 0, *colP = 0, *secP = 0;
  SClientArray v, n, c, tc[GC_MAX_UNITS];
};
static SStage g_Stage;

static void StageArray(const SClientArray &a, GLint first, GLsizei count,
                       std::vector<unsigned char> &staging, const unsigned char *&outP)
{
  outP = 0;
  if (!a.ptr || !a.enabled)
    return;
  if (a.vbo == 0)
  {
    outP = (const unsigned char *)a.ptr;
    return;
  }
  // pull the range out of the VBO
  int compSize = 1;
  switch (a.type)
  {
  case GL_FLOAT: compSize = 4; break;
  case GL_DOUBLE: compSize = 8; break;
  default: compSize = 1; break;
  }
  int stride = a.stride ? a.stride : a.size * compSize;
  size_t need = (size_t)stride * (first + count);
  size_t base = staging.size();
  staging.resize(base + need);
  if (es_glGetBufferSubData)
  {
    GLint prev = 0;
    if (es_glGetIntegerv)
    {
      GLint pb = 0x8894; // GL_ARRAY_BUFFER_BINDING
      es_glGetIntegerv(pb, &prev);
    }
    es_glBindBuffer(GL_ARRAY_BUFFER, a.vbo);
    es_glGetBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptrARB)need, &staging[base]);
    es_glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prev);
  }
  outP = &staging[base];
}

static void StageArrays(GLint first, GLsizei count)
{
  // NOTE: es array buffer binding must point at each staged vbo; we cheat by
  // assuming all client arrays share the engine's single big VBO (they do).
  static std::vector<unsigned char> staging;
  staging.clear();
  SClientArray vb = g_arrVertex; vb.ptr = 0;
  // temporarily re-point at nothing so StageArray copies raw
  SClientArray tmp;
  if (g_arrVertex.enabled)
  {
    tmp = g_arrVertex;
    StageArray(tmp, first, count, staging, g_Stage.vertP);
    if (g_Stage.vertP) { vb.ptr = g_Stage.vertP; vb.vbo = 0; }
    g_Stage.v = vb;
  }
  if (g_arrNormal.enabled)
  {
    tmp = g_arrNormal;
    StageArray(tmp, first, count, staging, g_Stage.nrmP);
    g_Stage.n = g_arrNormal;
    g_Stage.n.ptr = g_Stage.nrmP;
    g_Stage.n.vbo = 0;
  }
  if (g_arrColor.enabled)
  {
    tmp = g_arrColor;
    StageArray(tmp, first, count, staging, g_Stage.colP);
    g_Stage.c = g_arrColor;
    g_Stage.c.ptr = g_Stage.colP;
    g_Stage.c.vbo = 0;
  }
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    g_Stage.tc[u] = g_arrTexCoord[u];
    if (g_arrTexCoord[u].enabled && g_arrTexCoord[u].ptr)
    {
      tmp = g_arrTexCoord[u];
      const unsigned char *p = 0;
      StageArray(tmp, first, count, staging, p);
      g_Stage.tc[u].ptr = p;
      g_Stage.tc[u].vbo = 0;
    }
  }
}

static void DrawFromArrays(GLenum mode, GLint first, GLsizei count)
{
  if (count <= 0)
    return;
  StageArrays(first, count);
  g_Stream.clear();
  g_Stream.reserve(count);
  GLfloat tc[GC_MAX_UNITS][2];
  for (int i = 0; i < count; i++)
  {
    GLfloat pos[4] = {0, 0, 0, 1}, col[4] = {1, 1, 1, 1}, nrm[3] = {0, 0, 1};
    if (g_Stage.vertP)
      ReadArr(g_Stage.v, i, pos, 4);
    else if (g_arrVertex.ptr && g_arrVertex.vbo == 0)
      ReadArr(g_arrVertex, i, pos, 4);
    if (g_bLighting)
    {
      if (g_Stage.nrmP)
        ReadArr(g_Stage.n, i, nrm, 3);
      else if (g_arrNormal.ptr && g_arrNormal.vbo == 0)
        ReadArr(g_arrNormal, i, nrm, 3);
      if (g_Stage.colP)
        ReadArr(g_Stage.c, i, col, 4);
      else if (g_arrColor.ptr && g_arrColor.vbo == 0)
        ReadArr(g_arrColor, i, col, 4);
    }
    else if (g_Stage.colP)
      ReadArr(g_Stage.c, i, col, 4);
    else if (g_arrColor.ptr && g_arrColor.vbo == 0)
      ReadArr(g_arrColor, i, col, 4);
    for (int u = 0; u < GC_MAX_UNITS; u++)
    {
      tc[u][0] = tc[u][1] = 0;
      const SClientArray &ta = g_Stage.tc[u].ptr ? g_Stage.tc[u] : g_arrTexCoord[u];
      if (ta.enabled && ta.ptr)
      {
        GLfloat t[4];
        ReadArr(ta, i, t, 4);
        tc[u][0] = t[0];
        tc[u][1] = t[1];
      }
      else if (!ta.enabled)
      {
        tc[u][0] = g_CurTC[u][0];
        tc[u][1] = g_CurTC[u][1];
      }
    }
    g_Stream.push_back(XformVert(pos, nrm, col, tc, g_bLighting));
  }
  StreamDraw(mode, count);
  g_Stream.clear();
}

void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
  DrawFromArrays(mode, first, count);
}
void APIENTRY glMultiDrawArraysEXT(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount)
{
  for (int i = 0; i < primcount; i++)
    glDrawArrays(mode, first[i], count[i]);
}
void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
  if (count <= 0)
    return;
  // fetch indices to CPU
  std::vector<GLuint> idx;
  idx.resize(count);
  bool bFromEBO = (indices != 0 && g_nElementBuffer != 0);
  if (bFromEBO)
  {
    // indices are offsets into the element buffer
    size_t isz = (type == GL_UNSIGNED_INT) ? 4 : (type == GL_UNSIGNED_SHORT ? 2 : 1);
    static std::vector<unsigned char> ebuf;
    size_t need = (size_t)count * isz + (size_t)(size_t)indices;
    ebuf.resize(need);
    if (es_glGetBufferSubData)
      es_glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (GLsizeiptrARB)need, &ebuf[0]);
    for (int i = 0; i < count; i++)
    {
      const unsigned char *p = &ebuf[(size_t)(size_t)indices + (size_t)i * isz];
      idx[i] = (type == GL_UNSIGNED_INT) ? *(const GLuint *)p : (type == GL_UNSIGNED_SHORT ? *(const GLushort *)p : *p);
    }
  }
  else if (indices)
  {
    for (int i = 0; i < count; i++)
      idx[i] = (type == GL_UNSIGNED_INT) ? ((const GLuint *)indices)[i] :
               (type == GL_UNSIGNED_SHORT ? ((const GLushort *)indices)[i] : ((const GLubyte *)indices)[i]);
  }
  else
    return;
  int mn = 0x7fffffff, mx = 0;
  for (int i = 0; i < count; i++)
  {
    if (idx[i] < (GLuint)mn) mn = (int)idx[i];
    if (idx[i] > (GLuint)mx) mx = (int)idx[i];
  }
  int range = mx - mn + 1;
  StageArrays(mn, range);
  g_Stream.clear();
  g_Stream.reserve(range);
  GLfloat tc[GC_MAX_UNITS][2];
  for (int i = 0; i < range; i++)
  {
    GLfloat pos[4] = {0, 0, 0, 1}, col[4] = {1, 1, 1, 1}, nrm[3] = {0, 0, 1};
    if (g_Stage.vertP)
      ReadArr(g_Stage.v, i, pos, 4);
    else if (g_arrVertex.ptr && g_arrVertex.vbo == 0)
      ReadArr(g_arrVertex, i, pos, 4);
    if (g_bLighting)
    {
      if (g_Stage.nrmP)
        ReadArr(g_Stage.n, i, nrm, 3);
      else if (g_arrNormal.ptr && g_arrNormal.vbo == 0)
        ReadArr(g_arrNormal, i, nrm, 3);
      if (g_Stage.colP)
        ReadArr(g_Stage.c, i, col, 4);
      else if (g_arrColor.ptr && g_arrColor.vbo == 0)
        ReadArr(g_arrColor, i, col, 4);
    }
    else if (g_Stage.colP)
      ReadArr(g_Stage.c, i, col, 4);
    else if (g_arrColor.ptr && g_arrColor.vbo == 0)
      ReadArr(g_arrColor, i, col, 4);
    for (int u = 0; u < GC_MAX_UNITS; u++)
    {
      tc[u][0] = tc[u][1] = 0;
      const SClientArray &ta = g_Stage.tc[u].ptr ? g_Stage.tc[u] : g_arrTexCoord[u];
      if (ta.enabled && ta.ptr)
      {
        GLfloat t[4];
        ReadArr(ta, i, t, 4);
        tc[u][0] = t[0];
        tc[u][1] = t[1];
      }
    }
    g_Stream.push_back(XformVert(pos, nrm, col, tc, g_bLighting));
  }
  // remap indices to the compacted stream
  std::vector<GLuint> remap(count);
  for (int i = 0; i < count; i++)
    remap[i] = idx[i] - (GLuint)mn;
  EnsureScratch();
  g_Idx32.clear();
  // quads in DrawElements are illegal per spec; if engine does it anyway, convert
  if (mode == GL_QUADS || mode == GL_QUAD_STRIP || mode == GL_POLYGON)
  {
    BuildConvertedIndices(mode, count, g_Idx32);
    for (size_t i = 0; i < g_Idx32.size(); i++)
      g_Idx32[i] = remap[g_Idx32[i]];
  }
  else
    g_Idx32 = remap;
  SProgram *p = GetProgram();
  if (!p)
    return;
  es_glUseProgram(p->prog);
  es_glUniformMatrix4fv(p->uMVP, 1, 0, g_mv.cur.m);
  es_glUniform1f(p->uPSize, g_fPointSize);
  if (p->uARef >= 0)
    es_glUniform1f(p->uARef, g_fAlphaRef);
  if (p->uFogColor >= 0)
    es_glUniform4fv(p->uFogColor, 1, g_FogColor);
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    if (p->uRect[u] >= 0)
    {
      GLfloat rc[4] = {1, 1, 0, 0};
      STexObj *t = BoundTex(u);
      if (t && t->bRectangle && t->width > 0)
      {
        rc[0] = 1.0f / t->width;
        rc[1] = 1.0f / t->height;
      }
      es_glUniform4fv(p->uRect[u], 1, rc);
    }
    if (p->uEnvColor[u] >= 0)
      es_glUniform4fv(p->uEnvColor[u], 1, g_TexUnit[u].env.color);
  }
  es_glBindBuffer(0x8892, g_nStreamVBO);
  es_glBufferData(0x8892, (GLsizeiptrARB)(g_Stream.size() * kStreamStride), &g_Stream[0], 0x88E8);
  const SProgCfg *cfg = CurrentCfg();
  es_glEnableVertexAttribArray(0);
  es_glVertexAttribPointer(0, 3, GL_FLOAT, 0, kStreamStride, (const void *)0);
  es_glEnableVertexAttribArray(1);
  es_glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, 1, kStreamStride, (const void *)48);
  es_glEnableVertexAttribArray(6);
  es_glVertexAttribPointer(6, 1, GL_FLOAT, 0, kStreamStride, (const void *)12);
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    int a = 2 + u;
    if (cfg->texMask & (1 << u))
    {
      es_glEnableVertexAttribArray(a);
      es_glVertexAttribPointer(a, 2, GL_FLOAT, 0, kStreamStride, (const void *)(size_t)(16 + u * 8));
      es_glActiveTexture(GL_TEXTURE0 + u);
      GLint loc = es_glGetUniformLocation(p->prog, u == 0 ? "uTex0" : u == 1 ? "uTex1" : u == 2 ? "uTex2" : "uTex3");
      if (loc >= 0)
        es_glUniform1i(loc, u);
    }
    else
      es_glDisableVertexAttribArray(a);
  }
  es_glActiveTexture(GL_TEXTURE0 + g_nActiveUnit);
  es_glBindBuffer(0x8893, g_nStreamEBO);
  es_glBufferData(0x8893, (GLsizeiptrARB)(g_Idx32.size() * 4), &g_Idx32[0], 0x88E8);
  GLenum dt = GL_UNSIGNED_INT;
  if (g_bES32Idx == false && mx - mn < 65536)
  {
    // use 16-bit indices when safe
    g_Idx16.resize(g_Idx32.size());
    for (size_t i = 0; i < g_Idx32.size(); i++)
      g_Idx16[i] = (GLushort)g_Idx32[i];
    es_glBufferData(0x8893, (GLsizeiptrARB)(g_Idx16.size() * 2), &g_Idx16[0], 0x88E8);
    dt = GL_UNSIGNED_SHORT;
  }
  es_glDrawElements(GL_TRIANGLES, (GLsizei)(dt == GL_UNSIGNED_SHORT ? g_Idx16.size() : g_Idx32.size()), dt, 0);
  es_glBindBuffer(0x8893, 0);
  es_glBindBuffer(0x8892, 0);
  g_Stream.clear();
}
void APIENTRY glDrawRangeElementsEXT(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
  glDrawElements(mode, count, type, indices);
  (void)start; (void)end;
}
void APIENTRY glLockArraysEXT(GLint first, GLsizei count) { (void)first; (void)count; }
void APIENTRY glUnlockArraysEXT(void) {}
void APIENTRY glVertexArrayRangeNV(GLsizei size, const void *ptr) { (void)size; (void)ptr; }
void APIENTRY glMultiDrawElementsEXT(GLenum mode, const GLsizei *count, GLenum type, const void *const *indices, GLsizei primcount)
{
  for (int i = 0; i < primcount; i++)
    glDrawElements(mode, count[i], type, indices[i]);
}

// ---- misc stubs the engine may call ----
void APIENTRY glPushAttrib(GLenum mask) { (void)mask; WarnOnce(55, "glPushAttrib no-op"); }
void APIENTRY glPopAttrib(void) { WarnOnce(55, "glPopAttrib no-op"); }
void APIENTRY glPushClientAttrib(GLenum mask) { (void)mask; }
void APIENTRY glPopClientAttrib(void) {}
void APIENTRY glSelectBuffer(GLsizei size, GLuint *buf) { (void)size; (void)buf; }
GLint APIENTRY glRenderMode(GLenum mode) { (void)mode; return 0; }
void APIENTRY glInitNames(void) {}
void APIENTRY glLoadName(GLuint name) { (void)name; }
void APIENTRY glPushName(GLuint name) { (void)name; }
void APIENTRY glPopName(void) {}
void APIENTRY glIndexMask(GLuint mask) { (void)mask; }
void APIENTRY glSampleCoverageARB(GLclampf value, GLboolean invert)
{
  if (p_glSampleCoverage)
    p_glSampleCoverage((GLfloat)value, invert);
}
void APIENTRY glSwapIntervalEXT(GLint interval) { SDL_GL_SetSwapInterval(interval); }
void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param) { (void)coord; (void)pname; (void)param; }
void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param) { (void)coord; (void)pname; (void)param; }
void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *param) { (void)coord; (void)pname; (void)param; }
void APIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param) { (void)coord; (void)pname; (void)param; }
void APIENTRY glPrioritizeTextures(GLsizei n, const GLuint *tex, const GLclampf *pri) { (void)n; (void)tex; (void)pri; }
GLboolean APIENTRY glAreTexturesResident(GLsizei n, const GLuint *tex, GLboolean *res)
{
  for (int i = 0; i < n; i++)
    res[i] = GL_TRUE;
  (void)tex;
  return GL_TRUE;
}
} // extern "C"
} // namespace glescompat

// =============================================================================
//  export glue: name -> function tables + GLESCompat entry points
// =============================================================================
using namespace glescompat;

// generic no-op proc handed out for unimplemented desktop-only functions
static void APIENTRY glescompat_stub_generic(void) {}

// texture module exports (defined in GLESCompatTex.cpp).
// Must have C linkage: GLESCompatTex.cpp defines them inside extern "C",
// and GLESCompat_GetProcAddress hands the addresses out by C name.
extern "C" {
void APIENTRY glGenTextures(GLsizei, const GLuint *);
void APIENTRY glDeleteTextures(GLsizei, const GLuint *);
GLboolean APIENTRY glIsTexture(GLuint);
void APIENTRY glBindTexture(GLenum, GLuint);
void APIENTRY glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
void APIENTRY glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *);
void APIENTRY glCompressedTexImage2DARB(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLsizei, const void *);
void APIENTRY glCompressedTexSubImage2DARB(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void *);
void APIENTRY glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
void APIENTRY glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
void APIENTRY glTexParameteri(GLenum, GLenum, GLint);
void APIENTRY glTexParameterf(GLenum, GLenum, GLfloat);
void APIENTRY glTexParameterfv(GLenum, GLenum, const GLfloat *);
void APIENTRY glTexParameteriv(GLenum, GLenum, const GLint *);
void APIENTRY glTexEnvi(GLenum, GLenum, GLint);
void APIENTRY glTexEnvf(GLenum, GLenum, GLfloat);
void APIENTRY glTexEnvfv(GLenum, GLenum, const GLfloat *);
void APIENTRY glTexEnviv(GLenum, GLenum, const GLint *);
void APIENTRY glActiveTextureARB(GLenum);
void APIENTRY glActiveTexture(GLenum);
void APIENTRY glGetTexImage(GLenum, GLint, GLenum, GLenum, void *);
void APIENTRY glGetTexLevelParameteriv(GLenum, GLint, GLenum, GLint *);
} // extern "C"

static const SNameProc g_CoreProcs[] = {
#define E(fn) {#fn, (void *)&fn}
  E(glBegin), E(glEnd), E(glVertex2f), E(glVertex3f), E(glVertex4f),
  E(glVertex2fv), E(glVertex3fv), E(glVertex4fv), E(glVertex2d), E(glVertex3d),
  E(glVertex4d), E(glVertex2dv), E(glVertex3dv), E(glVertex4dv), E(glVertex2i),
  E(glVertex3i), E(glVertex2s), E(glVertex3s), E(glColor3f), E(glColor4f),
  E(glColor4ub), E(glColor3ub), E(glColor3fv), E(glColor4fv), E(glColor3ubv),
  E(glColor4ubv), E(glColor3d), E(glColor4d), E(glColor3dv), E(glColor4dv),
  E(glColor3i), E(glColor3s), E(glColor4i), E(glColor4s),
  E(glSecondaryColor3fEXT), E(glSecondaryColor3ubEXT), E(glSecondaryColor3fvEXT),
  E(glSecondaryColor3ubvEXT), E(glSecondaryColorPointerEXT),
  E(glNormal3f), E(glNormal3fv), E(glNormal3d), E(glNormal3dv), E(glNormal3b),
  E(glNormal3i), E(glNormal3s), E(glTexCoord1f), E(glTexCoord2f), E(glTexCoord3f),
  E(glTexCoord4f), E(glTexCoord1fv), E(glTexCoord2fv), E(glTexCoord3fv),
  E(glTexCoord4fv), E(glTexCoord2d), E(glTexCoord2dv), E(glTexCoord2i),
  E(glTexCoord2s), E(glMultiTexCoord1fARB), E(glMultiTexCoord2fARB),
  E(glMultiTexCoord3fARB), E(glMultiTexCoord4fARB), E(glMultiTexCoord1fvARB),
  E(glMultiTexCoord2fvARB), E(glMultiTexCoord3fvARB), E(glMultiTexCoord4fvARB),
  E(glMultiTexCoord1iARB), E(glMultiTexCoord2iARB), E(glMultiTexCoord2dARB),
  E(glMultiTexCoord2dvARB), E(glRectf), E(glRectd), E(glRecti), E(glRects),
  E(glRectfv), E(glArrayElement), E(glGenLists), E(glIsList), E(glDeleteLists),
  E(glNewList), E(glEndList), E(glCallList), E(glListBase), E(glCallLists),
  E(glMatrixMode), E(glPushMatrix), E(glPopMatrix), E(glLoadIdentity),
  E(glLoadMatrixf), E(glLoadMatrixd), E(glMultMatrixf), E(glMultMatrixd),
  E(glTranslatef), E(glTranslated), E(glScalef), E(glScaled), E(glRotatef),
  E(glRotated), E(glFrustum), E(glOrtho), E(glDepthRange), E(glEnable),
  E(glDisable), E(glIsEnabled), E(glAlphaFunc), E(glFogi), E(glFogf),
  E(glFogfv), E(glShadeModel), E(glLightModelf), E(glLightModelfv),
  E(glLightModeli), E(glLightModeliv), E(glLightf), E(glLightfv), E(glLighti),
  E(glLightiv), E(glMaterialf), E(glMaterialfv), E(glMateriali),
  E(glMaterialiv), E(glColorMaterial), E(glPointSize), E(glLineWidth),
  E(glPointParameterfARB), E(glPointParameterfvARB), E(glPointParameterfEXT),
  E(glPointParameterfvEXT), E(glClearColor), E(glClearDepth), E(glClearStencil),
  E(glClear), E(glViewport), E(glScissor), E(glDepthFunc), E(glDepthMask),
  E(glCullFace), E(glFrontFace), E(glStencilFunc), E(glStencilOp),
  E(glStencilMask), E(glBlendFunc), E(glBlendColor), E(glPolygonOffset),
  E(glDrawBuffer), E(glReadBuffer), E(glPixelStorei), E(glPixelTransferf),
  E(glPixelTransferi), E(glFinish), E(glFlush), E(glHint), E(glClipPlane),
  E(glGetClipPlane), E(glEdgeFlag), E(glEdgeFlagv), E(glIndexi), E(glIndexf),
  E(glLogicOp), E(glGetError), E(glGetString), E(glGetFloatv), E(glGetIntegerv),
  E(glGetBooleanv), E(glGetMaterialfv), E(glGetLightfv), E(glBindBufferARB),
  E(glDeleteBuffersARB), E(glGenBuffersARB), E(glIsBufferARB),
  E(glBufferDataARB), E(glBufferSubDataARB), E(glMapBufferARB),
  E(glUnmapBufferARB), E(glGetBufferParameterivARB), E(glClientActiveTextureARB),
  E(glVertexPointer), E(glColorPointer), E(glNormalPointer),
  E(glTexCoordPointer), E(glFogCoordPointerEXT), E(glEnableClientState),
  E(glDisableClientState), E(glDrawArrays), E(glMultiDrawArraysEXT),
  E(glDrawElements), E(glDrawRangeElementsEXT), E(glLockArraysEXT),
  E(glUnlockArraysEXT), E(glVertexArrayRangeNV), E(glMultiDrawElementsEXT),
  E(glPushAttrib), E(glPopAttrib), E(glPushClientAttrib),
  E(glPopClientAttrib), E(glSelectBuffer), E(glRenderMode), E(glInitNames),
  E(glLoadName), E(glPushName), E(glPopName), E(glIndexMask),
  E(glSampleCoverageARB), E(glSwapIntervalEXT), E(glTexGeni), E(glTexGenf),
  E(glTexGenfv), E(glTexGend), E(glPrioritizeTextures), E(glAreTexturesResident)
#undef E
};
namespace glescompat {
const SNameProc *CoreProcs(int &count)
{
  count = (int)(sizeof(g_CoreProcs) / sizeof(g_CoreProcs[0]));
  return g_CoreProcs;
}

static const SNameProc g_TexProcs2[] = {
#define E(fn) {#fn, (void *)&fn}
  E(glGenTextures), E(glDeleteTextures), E(glIsTexture), E(glBindTexture),
  E(glTexImage2D), E(glTexSubImage2D), E(glCompressedTexImage2DARB),
  E(glCompressedTexSubImage2DARB), E(glCopyTexImage2D), E(glCopyTexSubImage2D),
  E(glTexParameteri), E(glTexParameterf), E(glTexParameterfv),
  E(glTexParameteriv), E(glTexEnvi), E(glTexEnvf), E(glTexEnvfv),
  E(glTexEnviv), E(glActiveTextureARB), E(glActiveTexture), E(glGetTexImage),
  E(glGetTexLevelParameteriv)
#undef E
};
const SNameProc *TexProcs(int &count)
{
  count = (int)(sizeof(g_TexProcs2) / sizeof(g_TexProcs2[0]));
  return g_TexProcs2;
}
} // namespace glescompat

// matrix stacks + one-time init
static Mat4 s_mvStack[MAX_MV], s_pjStack[MAX_PJ], s_txStack[GC_MAX_UNITS][MAX_TX];
static bool g_bInitialized = false;

void GLESCompat_Init(void)
{
  if (g_bInitialized)
    return;
  g_bInitialized = true;
  if (!LoadESProcs())
  {
    GLog("FATAL: ES procs not resolvable");
    return;
  }
  TexInit();
  g_mv.stack = s_mvStack; g_mv.depth = 0; g_mv.maxDepth = MAX_MV;
  g_pj.stack = s_pjStack; g_pj.depth = 0; g_pj.maxDepth = MAX_PJ;
  for (int u = 0; u < GC_MAX_UNITS; u++)
  {
    g_tx[u].stack = s_txStack[u];
    g_tx[u].depth = 0;
    g_tx[u].maxDepth = MAX_TX;
  }
  g_pColStack = 0; // color matrix not emulated
  MatIdentity(g_mv.cur);
  MatIdentity(g_pj.cur);
  for (int u = 0; u < GC_MAX_UNITS; u++)
    MatIdentity(g_tx[u].cur);
  MatIdentity(g_NormalMat);
  for (int i = 0; i < GC_MAX_LIGHTS; i++)
  {
    memset(&g_Lights[i].l, 0, sizeof(SLight));
    g_Lights[i].l.ambient[3] = 1;
    g_Lights[i].l.diffuse[0] = g_Lights[i].l.diffuse[1] = g_Lights[i].l.diffuse[2] = 1;
    g_Lights[i].l.diffuse[3] = 1;
    g_Lights[i].l.specular[0] = g_Lights[i].l.specular[1] = g_Lights[i].l.specular[2] = 1;
    g_Lights[i].l.specular[3] = 1;
    g_Lights[i].l.spotCutoff = 180;
    g_Lights[i].l.attConst = 1;
    g_Lights[i].l.position[2] = 1; // directional +z per GL default
    g_Lights[i].bOn = false;
  }
  memset(&g_MatFront, 0, sizeof(g_MatFront));
  memset(&g_MatBack, 0, sizeof(g_MatBack));
  g_MatFront.diffuse[0] = g_MatFront.diffuse[1] = g_MatFront.diffuse[2] = 0.8f;
  g_MatFront.diffuse[3] = 1;
  g_MatFront.ambient[0] = g_MatFront.ambient[1] = g_MatFront.ambient[2] = 0.2f;
  g_MatFront.ambient[3] = 1;
  g_MatFront.specular[3] = 1;
  g_MatBack = g_MatFront;
  GLog("initialized (ES3 fixed-function emulation)");
}

void GLESCompat_FrameEnd(void) { FrameTick(); }

extern "C" void *GLESCompat_GetProcAddress(const char *name)
{
  if (!g_bInitialized)
    GLESCompat_Init();
  if (!name || !name[0])
    return 0;
  int n = 0;
  const SNameProc *cp = CoreProcs(n);
  for (int i = 0; i < n; i++)
    if (!strcmp(cp[i].name, name))
      return cp[i].fn;
  const SNameProc *tp = TexProcs(n);
  for (int i = 0; i < n; i++)
    if (!strcmp(tp[i].name, name))
      return tp[i].fn;
  // Never hand out NULL: some desktop-only entry points are called by the
  // engine unconditionally (e.g. glPolygonMode) and a NULL proc is an
  // instant crash. Hand out a logged no-op stub instead - the missing
  // desktop-only feature degrades gracefully. The stub names are logged
  // so the on-device diag log shows exactly what is not implemented.
  {
    static const char *seen[96];
    static int seen_n = 0;
    for (int i = 0; i < seen_n; i++)
      if (!strcmp(seen[i], name))
        return (void *)&glescompat_stub_generic;
    if (seen_n < 96)
    {
      seen[seen_n++] = name;
      GLog("no implementation for '%s' - using a no-op stub", name);
    }
  }
  return (void *)&glescompat_stub_generic;
}
