//////////////////////////////////////////////////////////////////////
//
//  Minimal GLU compatibility implementation.
//
//  The OpenGL renderer only uses a handful of GLU helpers (quadric
//  spheres for flares, look-at / perspective matrices, error strings).
//  Android has no libGLU, and it is optional even on desktop, so this
//  file provides drop-in replacements built on plain GL calls.
//
//  Compiled instead of linking -lGLU (see CMakeLists.txt).
//
//////////////////////////////////////////////////////////////////////

#include "RenderPCH.h"
#include "GL_Renderer.h" // declares the crygl* GL function pointers

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

// MyGlu.h only forward-declares GLUquadric; this is the full definition.
struct GLUquadric
{
	int drawStyle;	// GLU_FILL / GLU_LINE / ...
	int orientation;
	int normals;
	int texture;
};

extern "C" {

GLUquadric *APIENTRY gluNewQuadric(void)
{
	GLUquadric *q = (GLUquadric *)malloc(sizeof(GLUquadric));
	if (q)
	{
		q->drawStyle = 0x1B02 /*GLU_FILL*/;
		q->orientation = 0x1002 /*GLU_OUTSIDE*/;
		q->normals = 0x1000 /*GLU_SMOOTH*/;
		q->texture = 0;
	}
	return q;
}

void APIENTRY gluDeleteQuadric(GLUquadric *qobj)
{
	if (qobj)
		free(qobj);
}

void APIENTRY gluSphere(GLUquadric *qobj, GLdouble radius, GLint slices, GLint stacks)
{
	if (!qobj || radius <= 0 || slices < 2 || stacks < 1)
		return;

	const int kMaxSeg = 48;
	if (slices > kMaxSeg) slices = kMaxSeg;
	if (stacks > kMaxSeg) stacks = kMaxSeg;

	const GLfloat drho = (GLfloat)(PI / stacks);
	const GLfloat dtheta = (GLfloat)(2.0f * PI / slices);

	glBegin(GL_TRIANGLES);
	for (int i = 0; i < stacks; i++)
	{
		GLfloat rho = (GLfloat)i * drho;
		GLfloat rho2 = (GLfloat)(i + 1) * drho;

		for (int j = 0; j < slices; j++)
		{
			GLfloat theta = (GLfloat)j * dtheta;
			GLfloat theta2 = (GLfloat)(j + 1) * dtheta;

			// four corners of the quad (two collapse at the poles)
			GLfloat x0 = (GLfloat)(-sin(theta) * sin(rho));
			GLfloat y0 = (GLfloat)(cos(theta) * sin(rho));
			GLfloat z0 = (GLfloat)(cos(rho));
			GLfloat x1 = (GLfloat)(-sin(theta) * sin(rho2));
			GLfloat y1 = (GLfloat)(cos(theta) * sin(rho2));
			GLfloat z1 = (GLfloat)(cos(rho2));
			GLfloat x2 = (GLfloat)(-sin(theta2) * sin(rho2));
			GLfloat y2 = (GLfloat)(cos(theta2) * sin(rho2));
			GLfloat z2 = (GLfloat)(cos(rho2));
			GLfloat x3 = (GLfloat)(-sin(theta2) * sin(rho));
			GLfloat y3 = (GLfloat)(cos(theta2) * sin(rho));
			GLfloat z3 = (GLfloat)(cos(rho));

			GLfloat s0 = (GLfloat)j / slices;
			GLfloat s1 = (GLfloat)(j + 1) / slices;
			GLfloat t0 = (GLfloat)i / stacks;
			GLfloat t1 = (GLfloat)(i + 1) / stacks;

			// tri 1
			glTexCoord2f(s0, t0); cryglNormal3f(x0, y0, z0); glVertex3f((GLfloat)(x0 * radius), (GLfloat)(y0 * radius), (GLfloat)(z0 * radius));
			glTexCoord2f(s0, t1); cryglNormal3f(x1, y1, z1); glVertex3f((GLfloat)(x1 * radius), (GLfloat)(y1 * radius), (GLfloat)(z1 * radius));
			glTexCoord2f(s1, t1); cryglNormal3f(x2, y2, z2); glVertex3f((GLfloat)(x2 * radius), (GLfloat)(y2 * radius), (GLfloat)(z2 * radius));
			// tri 2
			glTexCoord2f(s0, t0); cryglNormal3f(x0, y0, z0); glVertex3f((GLfloat)(x0 * radius), (GLfloat)(y0 * radius), (GLfloat)(z0 * radius));
			glTexCoord2f(s1, t1); cryglNormal3f(x2, y2, z2); glVertex3f((GLfloat)(x2 * radius), (GLfloat)(y2 * radius), (GLfloat)(z2 * radius));
			glTexCoord2f(s1, t0); cryglNormal3f(x3, y3, z3); glVertex3f((GLfloat)(x3 * radius), (GLfloat)(y3 * radius), (GLfloat)(z3 * radius));
		}
	}
	glEnd();
}

static void MultMatrix(const GLfloat m[16])
{
	glMultMatrixf(m);
}

void APIENTRY gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez,
		GLdouble centerx, GLdouble centery, GLdouble centerz,
		GLdouble upx, GLdouble upy, GLdouble upz)
{
	GLfloat forward[3], side[3], up[3];
	GLfloat m[16];

	forward[0] = (GLfloat)(centerx - eyex);
	forward[1] = (GLfloat)(centery - eyey);
	forward[2] = (GLfloat)(centerz - eyez);

	up[0] = (GLfloat)upx;
	up[1] = (GLfloat)upy;
	up[2] = (GLfloat)upz;

	// normalize forward
	{
	 GLfloat len = sqrtf(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
	 if (len > 0) { forward[0] /= len; forward[1] /= len; forward[2] /= len; }
	}

	// side = forward x up
	side[0] = forward[1] * up[2] - forward[2] * up[1];
	side[1] = forward[2] * up[0] - forward[0] * up[2];
	side[2] = forward[0] * up[1] - forward[1] * up[0];
	{
	 GLfloat len = sqrtf(side[0] * side[0] + side[1] * side[1] + side[2] * side[2]);
	 if (len > 0) { side[0] /= len; side[1] /= len; side[2] /= len; }
	}

	// up = side x forward
	up[0] = side[1] * forward[2] - side[2] * forward[1];
	up[1] = side[2] * forward[0] - side[0] * forward[2];
	up[2] = side[0] * forward[1] - side[1] * forward[0];

	m[0] = side[0]; m[4] = side[1]; m[8] = side[2]; m[12] = 0.0f;
	m[1] = up[0]; m[5] = up[1]; m[9] = up[2]; m[13] = 0.0f;
	m[2] = -forward[0]; m[6] = -forward[1]; m[10] = -forward[2]; m[14] = 0.0f;
	m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 1.0f;
	MultMatrix(m);

	glTranslatef((GLfloat)-eyex, (GLfloat)-eyey, (GLfloat)-eyez);
}

void APIENTRY gluPerspective(GLdouble fovy, GLdouble aspect,
		GLdouble zNear, GLdouble zFar)
{
	const GLdouble radians = fovy / 2.0 * PI / 180.0;
	const GLdouble delta_z = zFar - zNear;
	const GLdouble sine = sin(radians);
	GLdouble cotangent;

	if (delta_z == 0.0 || sine == 0.0 || aspect == 0.0)
		return;

	cotangent = cos(radians) / sine;

	GLfloat m[16];
	m[0] = (GLfloat)(cotangent / aspect);
	m[4] = 0.0f;
	m[8] = 0.0f;
	m[12] = 0.0f;
	m[1] = 0.0f;
	m[5] = (GLfloat)cotangent;
	m[9] = 0.0f;
	m[13] = 0.0f;
	m[2] = 0.0f;
	m[6] = 0.0f;
	m[10] = (GLfloat)(-(zFar + zNear) / delta_z);
	m[14] = (GLfloat)(-(2.0 * zFar * zNear) / delta_z);
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = -1.0f;
	m[15] = 0.0f;
	MultMatrix(m);
}

const GLubyte *APIENTRY gluErrorString(GLenum errorCode)
{
	switch (errorCode)
	{
	case 0x1001 /*GLU_INVALID_ENUM*/: return (const GLubyte *)"invalid enum";
	case 0x1002 /*GLU_INVALID_VALUE*/: return (const GLubyte *)"invalid value";
	case 0x1003 /*GLU_OUT_OF_MEMORY*/: return (const GLubyte *)"out of memory";
	default: return (const GLubyte *)"unknown GLU error";
	}
}

} // extern "C"
