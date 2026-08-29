// GLESCompat - internal header shared by the implementation modules.
#ifndef _GLES_COMPAT_IMPL_H_
#define _GLES_COMPAT_IMPL_H_

#include "GLESCompat.h"

#include <SDL3/SDL.h>
#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <map>

// ---------------------------------------------------------------------------
// compact ES procedure declarations
// ---------------------------------------------------------------------------
#define EPAPI
#define ESPROC(ret, name, parms) \
  typedef ret(EPAPI *PFN_es_##name) parms; \
  extern PFN_es_##name es_##name;

ESPROC(void, glActiveTexture, (GLenum))
ESPROC(void, glAttachShader, (GLuint, GLuint))
ESPROC(void, glBindBuffer, (GLenum, GLuint))
ESPROC(void, glBindFramebuffer, (GLenum, GLuint))
ESPROC(void, glBindTexture, (GLenum, GLuint))
ESPROC(void, glBlendColor, (GLfloat, GLfloat, GLfloat, GLfloat))
ESPROC(void, glBlendEquationSeparate, (GLenum, GLenum))
ESPROC(void, glBlendFunc, (GLenum, GLenum))
ESPROC(void, glBlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum))
ESPROC(void, glBufferData, (GLenum, GLsizeiptrARB, const void *, GLenum))
ESPROC(void, glBufferSubData, (GLenum, GLintptrARB, GLsizeiptrARB, const void *))
ESPROC(GLenum, glCheckFramebufferStatus, (GLenum))
ESPROC(void, glClear, (GLbitfield))
ESPROC(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat))
ESPROC(void, glClearDepthf, (GLfloat))
ESPROC(void, glClearStencil, (GLint))
ESPROC(void, glColorMask, (GLboolean, GLboolean, GLboolean, GLboolean))
ESPROC(void, glCompileShader, (GLuint))
ESPROC(void, glCompressedTexImage2D, (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void *))
ESPROC(void, glCopyTexImage2D, (GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint))
ESPROC(void, glCopyTexSubImage2D, (GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))
ESPROC(GLuint, glCreateProgram, ())
ESPROC(GLuint, glCreateShader, (GLenum))
ESPROC(void, glCullFace, (GLenum))
ESPROC(void, glDeleteBuffers, (GLsizei, const GLuint *))
ESPROC(void, glDeleteProgram, (GLuint))
ESPROC(void, glDeleteShader, (GLuint))
ESPROC(void, glDeleteTextures, (GLsizei, const GLuint *))
ESPROC(void, glDepthFunc, (GLenum))
ESPROC(void, glDepthMask, (GLboolean))
ESPROC(void, glDepthRangef, (GLfloat, GLfloat))
ESPROC(void, glDisable, (GLenum))
ESPROC(void, glDisableVertexAttribArray, (GLuint))
ESPROC(void, glDrawArrays, (GLenum, GLint, GLsizei))
ESPROC(void, glDrawElements, (GLenum, GLsizei, GLenum, const void *))
ESPROC(void, glEnable, (GLenum))
ESPROC(void, glEnableVertexAttribArray, (GLuint))
ESPROC(void, glFinish, ())
ESPROC(void, glFlush, ())
ESPROC(void, glFramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint))
ESPROC(void, glFrontFace, (GLenum))
ESPROC(void, glGenBuffers, (GLsizei, GLuint *))
ESPROC(void, glGenFramebuffers, (GLsizei, GLuint *))
ESPROC(void, glGenTextures, (GLsizei, GLuint *))
ESPROC(void, glGenerateMipmap, (GLenum))
ESPROC(GLenum, glGetError, ())
ESPROC(void, glGetFloatv, (GLenum, GLfloat *))
ESPROC(void, glGetIntegerv, (GLenum, GLint *))
ESPROC(void, glGetBufferSubData, (GLenum, GLintptrARB, GLsizeiptrARB, void *))
ESPROC(void, glGetProgramInfoLog, (GLuint, GLsizei, GLsizei *, char *))
ESPROC(void, glGetProgramiv, (GLuint, GLenum, GLint *))
ESPROC(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei *, char *))
ESPROC(void, glGetShaderiv, (GLuint, GLenum, GLint *))
ESPROC(const GLubyte *, glGetString, (GLenum))
ESPROC(GLint, glGetUniformLocation, (GLuint, const char *))
ESPROC(void, glHint, (GLenum, GLenum))
ESPROC(GLboolean, glIsEnabled, (GLenum))
ESPROC(void, glLineWidth, (GLfloat))
ESPROC(void, glLinkProgram, (GLuint))
ESPROC(void *, glMapBufferRange, (GLenum, GLintptrARB, GLsizeiptrARB, GLbitfield))
ESPROC(void, glPixelStorei, (GLenum, GLint))
ESPROC(void, glPolygonOffset, (GLfloat, GLfloat))
ESPROC(void, glReadPixels, (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void *))
ESPROC(void, glScissor, (GLint, GLint, GLsizei, GLsizei))
ESPROC(void, glShaderSource, (GLuint, GLsizei, const char *const *, const GLint *))
ESPROC(void, glStencilFunc, (GLenum, GLint, GLuint))
ESPROC(void, glStencilMask, (GLuint))
ESPROC(void, glStencilOp, (GLenum, GLenum, GLenum))
ESPROC(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *))
ESPROC(void, glTexParameterf, (GLenum, GLenum, GLfloat))
ESPROC(void, glTexParameteri, (GLenum, GLenum, GLint))
ESPROC(void, glTexSubImage2D, (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void *))
ESPROC(void, glUniform1f, (GLint, GLfloat))
ESPROC(void, glUniform1i, (GLint, GLint))
ESPROC(void, glUniform2f, (GLint, GLfloat, GLfloat))
ESPROC(void, glUniform4fv, (GLint, GLsizei, const GLfloat *))
ESPROC(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat *))
ESPROC(GLboolean, glUnmapBuffer, (GLenum))
ESPROC(void, glUseProgram, (GLuint))
ESPROC(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void *))
ESPROC(void, glViewport, (GLint, GLint, GLsizei, GLsizei))

#undef ESPROC

namespace glescompat {

// logging ------------------------------------------------------------------
void GLog(const char *fmt, ...);
void WarnOnce(int id, const char *what);
void GSetError(GLenum err);

// ES procs loader ----------------------------------------------------------
bool LoadESProcs(void);
// per-frame cleanup (unused vbo scratch shrink etc.)
void FrameTick(void);

// texture module (GLESCompatTex.cpp) ---------------------------------------
struct STexObj {
  GLuint es;
  GLenum target;         // GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP
  GLint minFilter, magFilter, wrapS, wrapT;
  int maxLevelUploaded;
  int width, height;
  bool bRectangle;       // GL_TEXTURE_RECTANGLE_NV emulation
  bool bProxyTracked;
  STexObj() { Reset(); }
  void Reset() {
    es = 0; target = GL_TEXTURE_2D; minFilter = GL_NEAREST_MIPMAP_LINEAR;
    magFilter = GL_LINEAR; wrapS = wrapT = GL_REPEAT;
    maxLevelUploaded = -1; width = height = 0; bRectangle = false; bProxyTracked = false;
  }
};
void TexInit(void);
void TexShutdown(void);
STexObj *TexGet(GLuint name);
void TexDelete(GLsizei n, const GLuint *names);
// bind our ES texture object for the current active unit; returns the
// effective min filter (possibly downgraded when mips are missing)
void TexBind(GLenum target, GLuint name);
void TexEnableDisable(GLenum cap, bool bOn);
// upload helpers (shared by TexImage/CompressedTexImage paths)
void TexUploadLevel(GLenum target, GLint level, GLint internalFormat,
                    GLsizei w, GLsizei h, GLint border, GLenum format,
                    GLenum type, const void *data);
void TexUploadCompressed(GLenum target, GLint level, GLint internalFormat,
                         GLsizei w, GLsizei h, GLint border, GLsizei sz,
                         const void *data);
GLboolean TexGetImage(GLenum target, GLint level, GLenum format, GLenum type, void *buf);

// fixed pipeline module (GLESCompat.cpp) -----------------------------------
struct Mat4 { GLfloat m[16]; };
void MatIdentity(Mat4 &o);
void MatMul(Mat4 &o, const Mat4 &a, const Mat4 &b);

// name->proc export tables (entries are {name, fn})
struct SNameProc { const char *name; void *fn; };
const SNameProc *CoreProcs(int &count);
const SNameProc *TexProcs(int &count);

// display list ops
enum EListCmdOp {
  LC_NONE = 0, LC_BEGIN, LC_END, LC_VERTEX3F, LC_COLOR4UB, LC_COLOR4F,
  LC_TEXCOORD2F, LC_MULTITEXCOORD2F, LC_NORMAL3F, LC_ENABLE, LC_DISABLE,
  LC_BINDTEXTURE, LC_MATRIXMODE, LC_LOADMATRIX, LC_IDENTITY, LC_TRANSLATE,
  LC_ROTATE, LC_SCALE, LC_PUSHMATRIX, LC_POPMATRIX, LC_FRUSTUM, LC_ORTHO,
  LC_LOADNAME, LC_NOP,
};
struct SListCmd {
  EListCmdOp op;
  float f[16];
  int i[6];
  unsigned char ub[4];
  Mat4 mat;
};
struct SDisplayList { std::vector<SListCmd> cmds; };

// fixed-function state structs ---------------------------------------------
struct SLight {
  GLfloat ambient[4], diffuse[4], specular[4], position[4];
  GLfloat spotDir[3], spotExp, spotCutoff;
  GLfloat attConst, attLin, attQuad;
};

struct SMaterial {
  GLfloat ambient[4], diffuse[4], specular[4], emission[4];
  GLfloat shininess;
};

struct STexEnv {
  GLint mode;          // GL_MODULATE etc.
  GLfloat color[4];
  GLint combineRGB, combineA;
  GLint srcRGB[3], srcA[3];
  GLint opRGB[3], opA[3];
  GLfloat rgbScale, alphaScale;
  void Reset();
};

struct STexUnit {
  bool bEnabled2D;
  GLuint id2D;
  STexEnv env;
};

struct SClientArray {
  GLint size;
  GLenum type;
  GLsizei stride;
  const void *ptr;   // raw pointer OR vbo offset
  GLuint vbo;        // 0 = CPU pointer
  bool enabled;
  SClientArray() { memset(this, 0, sizeof(*this)); }
};

// shared mutable state the tex module needs -------------------------------
extern int g_nActiveUnit;        // glActiveTextureARB selector
extern int g_nClientUnit;        // glClientActiveTextureARB selector
extern bool g_bListRecording;    // capturing a display list
extern SDisplayList *g_pListCapture;
extern STexUnit g_TexUnit[GC_MAX_UNITS];
void ListRecord(const SListCmd &c);

// pipeline entry used by both immediate mode and client arrays
struct SStreamVertex {
  GLfloat pos[3];
  GLfloat fog;
  GLfloat tc[GC_MAX_UNITS][2];
  GLubyte col[4];
};

} // namespace glescompat

#endif // _GLES_COMPAT_IMPL_H_
