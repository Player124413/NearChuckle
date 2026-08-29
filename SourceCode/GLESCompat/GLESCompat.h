// =============================================================================
//   GLESCompat - desktop OpenGL 1.x fixed-function emulation over OpenGL ES 3
//
//   Far Cry's XRenderOGL resolves every GL entry point at runtime through
//   CGLRenderer::FindProc(). When built with GFX_GLES3 (Android), FindProc
//   asks GLESCompat_GetProcAddress() first, so this module *is* the driver:
//   it implements the desktop-GL subset the engine actually uses on top of
//   ES3 (matrices, T&L, texenv, fog, immediate mode, VBOs, textures).
//
//   Feature policy: only advertise what we implement. Anything unimplemented
//   resolves to NULL, FindProc flags the extension unsupported and the engine
//   falls back to simpler paths - exactly like on a weak 2004 GPU.
// =============================================================================
#ifndef _GLES_COMPAT_H_
#define _GLES_COMPAT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// emulation limits
#define GC_MAX_UNITS 4
#define GC_MAX_LIGHTS 8
#define GC_MAX_CLIP 2

// Resolve the address of an emulated desktop-GL function (NULL if we do not
// implement it). `name` is the plain desktop name ("glBlendFunc", ...).
void *GLESCompat_GetProcAddress(const char *name);
// Must be called once right after the ES context became current.
void  GLESCompat_Init(void);
// Per-frame housekeeping (called from the renderer's end-of-frame).
void  GLESCompat_FrameEnd(void);

// ---------------------------------------------------------------------------
// Minimal GL type re-declarations (self-contained on purpose - this file must
// compile against *any* GL/GLES backend without including GL headers).
// ---------------------------------------------------------------------------
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef void           GLvoid;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef intptr_t       GLsizeiptrARB;
typedef intptr_t       GLintptrARB;
#ifndef APIENTRY
#define APIENTRY
#endif

// --- constants (desktop GL values; shared enums match ES3 values 1:1) -------
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_ZERO 0
#define GL_ONE  1
#define GL_NONE 0
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

#define GL_POINTS 0
#define GL_LINES 1
#define GL_LINE_LOOP 2
#define GL_LINE_STRIP 3
#define GL_TRIANGLES 4
#define GL_TRIANGLE_STRIP 5
#define GL_TRIANGLE_FAN 6
#define GL_QUADS 7
#define GL_QUAD_STRIP 8
#define GL_POLYGON 9

#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000

#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207

#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004

#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_CW 0x0900
#define GL_CCW 0x0901

#define GL_CURRENT_COLOR 0x0B01
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_MAX_LIGHTS 0x0D31
#define GL_CURRENT_BIT 0x00000001
#define GL_POINT_BIT 0x00000002
#define GL_LINE_BIT 0x00000004
#define GL_POLYGON_BIT 0x00000008
#define GL_POLYGON_STIPPLE_BIT 0x00000010
#define GL_PIXEL_MODE_BIT 0x00000020
#define GL_LIGHTING_BIT 0x00000040
#define GL_FOG_BIT 0x00000080
#define GL_DEPTH_TEST 0x00000B71
#define GL_CULL_FACE 0x00000B44
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_LIGHT2 0x4002
#define GL_LIGHT3 0x4003
#define GL_LIGHT4 0x4004
#define GL_LIGHT5 0x4005
#define GL_LIGHT6 0x4006
#define GL_LIGHT7 0x4007
#define GL_COLOR_LOGIC_OP 0x00000BF2
#define GL_INDEX_LOGIC_OP 0x00000BF1
#define GL_LOGIC_OP GL_INDEX_LOGIC_OP
#define GL_SCISSOR_TEST 0x00000C11
#define GL_STENCIL_TEST 0x00000B90
#define GL_NORMALIZE 0x00000BA1
#define GL_RESCALE_NORMAL 0x00002A52
#define GL_POLYGON_OFFSET_POINT 0x00002A01
#define GL_POLYGON_OFFSET_LINE 0x00002A02
#define GL_POLYGON_OFFSET_FILL 0x00008037
#define GL_VERTEX_ARRAY 0x00008074
#define GL_NORMAL_ARRAY 0x00008075
#define GL_COLOR_ARRAY 0x00008076
#define GL_INDEX_ARRAY 0x00008077
#define GL_TEXTURE_COORD_ARRAY 0x00008078
#define GL_EDGE_FLAG_ARRAY 0x00008079
#define GL_FOG 0x00000B60
#define GL_FOG_INDEX 0x00000B62
#define GL_FOG_DENSITY 0x00000B62
#define GL_FOG_START 0x00000B63
#define GL_FOG_END 0x00000B64
#define GL_FOG_MODE 0x00000B65
#define GL_FOG_COLOR 0x00000B66
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
#define GL_LINEAR 0x2601
#define GL_FOG_HINT 0x00000C54
#define GL_POINT_SMOOTH 0x00000B13
#define GL_LINE_SMOOTH 0x00000B20
#define GL_POLYGON_SMOOTH 0x00000B41
#define GL_POINT_SIZE 0x00000B11
#define GL_LINE_WIDTH 0x00000B21
#define GL_ALPHA_TEST 0x00000BC0
#define GL_ALPHA_TEST_REF 0x00000BC2
#define GL_ALPHA_TEST_FUNC 0x00000BC1
#define GL_LIGHTING 0x00000B50
#define GL_LIGHT_MODEL_LOCAL_VIEWER 0x00001B51
#define GL_LIGHT_MODEL_TWO_SIDE 0x00001B52
#define GL_LIGHT_MODEL_AMBIENT 0x00001B53
#define GL_COLOR_MATERIAL 0x00000B57
#define GL_SHADE_MODEL 0x00000B54
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01
#define GL_COLOR_MATERIAL_FACE 0x00001B55
#define GL_COLOR_MATERIAL_PARAMETER 0x00001B56
#define GL_EMISSION 0x1600
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_SHININESS 0x1601
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_POSITION 0x1203
#define GL_SPOT_DIRECTION 0x1204
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_FRONT_LEFT 0x0400
#define GL_FRONT_RIGHT 0x0401
#define GL_BACK_LEFT 0x0402
#define GL_BACK_RIGHT 0x0403
#define GL_AUX0 0x0409

#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702
#define GL_COLOR 0x1800  // note: real GL_COLOR is 0x1800 for Get; matrix mode is 0x1703
#define GL_MATRIX_MODE 0x1703
#define GL_COLOR_MATRIX 0x180B
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_TEXTURE_MATRIX 0x0BA8
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#define GL_TEXTURE_STACK_DEPTH 0x0BA5
#define GL_MAX_MODELVIEW_STACK_DEPTH 0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH 0x0D39
#define GL_MATRIX_MODE_QUERY GL_MATRIX_MODE

#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_RED_SCALE 0x0D14
#define GL_GREEN_SCALE 0x0D18
#define GL_BLUE_SCALE 0x0D1C
#define GL_ALPHA_SCALE 0x0D1C
#define GL_BLEND_SRC 0x0BE1
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_EQUATION 0x8009
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_MIN 0x8007
#define GL_MAX 0x8008

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_1D 0x0DE0
#define GL_TEXTURE_3D 0x806F
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_PROXY_TEXTURE_1D 0x8063
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_BINDING_1D 0x8068
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_TEXTURE_LUMINANCE_SIZE 0x8060
#define GL_TEXTURE_INTENSITY_SIZE 0x8061
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_TEXTURE_PRIORITY 0x8066
#define GL_TEXTURE_RESIDENT 0x8067
#define GL_NEAREST 0x2600
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_REPEAT 0x2901
#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_MIRRORED_REPEAT 0x8370
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TEXTURE_ENV_COLOR 0x2201
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
#define GL_ADD 0x0104
#define GL_ADD_SIGNED 0x8574
#define GL_SUBTRACT 0x84E7
#define GL_INTERPOLATE 0x8575
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_COMBINE_ALPHA 0x8572
#define GL_RGB_SCALE 0x8573
#define GL_SRC0_RGB 0x8580
#define GL_SRC1_RGB 0x8581
#define GL_SRC2_RGB 0x8582
#define GL_SRC0_ALPHA 0x8588
#define GL_SRC1_ALPHA 0x8589
#define GL_SRC2_ALPHA 0x858A
#define GL_OPERAND0_RGB 0x8590
#define GL_OPERAND1_RGB 0x8591
#define GL_OPERAND2_RGB 0x8592
#define GL_OPERAND0_ALPHA 0x8598
#define GL_OPERAND1_ALPHA 0x8599
#define GL_OPERAND2_ALPHA 0x859A
#define GL_PREVIOUS 0x8578
#define GL_PRIMARY_COLOR 0x8577
#define GL_CONSTANT 0x8576
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_MAX_TEXTURE_UNITS 0x84E2
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_ACTIVE_TEXTURES 0x84E1

#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGB5 0x8050
#define GL_RGB8 0x8051
#define GL_RGBA8 0x8058
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE8 0x8040
#define GL_LUMINANCE4 0x803F
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_LUMINANCE8_ALPHA8 0x8045
#define GL_INTENSITY 0x8049
#define GL_INTENSITY8 0x804B
#define GL_ALPHA 0x1906
#define GL_ALPHA8 0x803C
#define GL_R3_G3_B2 0x2A10
#define GL_RGB4 0x804F
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_BYTE 0x1400
#define GL_SHORT 0x1402
#define GL_INT 0x1404
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_COMPRESSED_RGB 0x84ED
#define GL_COMPRESSED_RGBA 0x84EE
#define GL_TEXTURE_COMPRESSED 0x86A0
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE 0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_TEXTURE_COMPRESSION_HINT 0x84EF

#define GL_VERTEX_ARRAY_SIZE 0x807A
#define GL_VERTEX_ARRAY_TYPE 0x807B
#define GL_VERTEX_ARRAY_STRIDE 0x807C
#define GL_VERTEX_ARRAY_POINTER 0x807E
#define GL_NORMAL_ARRAY_TYPE 0x807E
#define GL_COLOR_ARRAY_SIZE 0x8081

#define GL_ARRAY_BUFFER 0x8892
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

#define GL_QUAD_PRIORITY GL_TEXTURE_PRIORITY

// extension-name advertisement flags used by the engine's GL_EXT list
#define GL_ARB_multitexture 1
#define GL_ARB_texture_env_combine 1
#define GL_ARB_texture_env_add 1
#define GL_EXT_texture_env_combine 1
#define GL_EXT_texture_env_add 1
#define GL_ARB_texture_compression 1
#define GL_EXT_texture_compression_s3tc 1
#define GL_ARB_vertex_buffer_object 1
#define GL_EXT_draw_range_elements 1
#define GL_EXT_secondary_color 1
#define GL_EXT_bgra 1
#define GL_ARB_texture_non_power_of_two 1
#define GL_SGIS_generate_mipmap 1
#define GL_ARB_point_parameters 1
#define GL_EXT_point_parameters 1
#define GL_EXT_texture_filter_anisotropic 1
#define GL_ARB_texture_mirrored_repeat 1
#define GL_EXT_texture_cube_map 1
#define GL_ARB_multisample 1
#define GL_EXT_texture_lod_bias 1
#define GL_ARB_texture_env_crossbar 1
#define GL_NV_fog_distance 1
#define GL_EXT_clip_volume_hint 1
#define GL_NV_blend_square 1
#define GL_ARB_shadow 1
#define GL_ARB_depth_texture 1

#define GL_FOG_COORDINATE_SOURCE 0x8450
#define GL_FOG_COORD GL_FOG_COORDINATE_SOURCE
#define GL_FRAGMENT_DEPTH 0x8452
#define GL_FOG_COORDINATE 0x8451
#define GL_COORD_REPLACE 0x8862
#define GL_POINT_SPRITE 0x8861
#define GL_POINT_DISTANCE_ATTENUATION 0x8129
#define GL_POINT_SIZE_MIN 0x8126
#define GL_POINT_SIZE_MAX 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE 0x8128
#define GL_SECONDARY_COLOR_ARRAY 0x845E
#define GL_FOG_COORDINATE_ARRAY 0x8457
#define GL_TEXTURE_GEN_S 0x0C60
#define GL_TEXTURE_GEN_T 0x0C61
#define GL_TEXTURE_GEN_R 0x0C62
#define GL_TEXTURE_GEN_Q 0x0C63
#define GL_OBJECT_LINEAR 0x2401
#define GL_EYE_LINEAR 0x2400
#define GL_SPHERE_MAP 0x2402
#define GL_NORMAL_MAP 0x8511
#define GL_REFLECTION_MAP 0x8512
#define GL_S 0x2000
#define GL_T 0x2001
#define GL_R 0x2002
#define GL_Q 0x2003
#define GL_EYE_PLANE 0x2502
#define GL_OBJECT_PLANE 0x2501

#define GL_LIST_MODE 0x0B30
#define GL_LIST_INDEX 0x0B33
#define GL_COMPILE 0x1300
#define GL_COMPILE_AND_EXECUTE 0x1301

#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_PACK_LSB_FIRST 0x0D01
#define GL_UNPACK_LSB_FIRST 0x0CF1
#define GL_PACK_SWAP_BYTES 0x0D00
#define GL_UNPACK_SWAP_BYTES 0x0CF0
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_FRAMEBUFFER 0x8D40

#ifdef __cplusplus
}
#endif
#endif // _GLES_COMPAT_H_
