#include <vector>
#include <d3d9.h>
#include <cmath>
#include <cstdio>
#include "d3dx9.h"

#if 0
#define STUB printf("D3DX9 STUB: %s\n", __func__)
#else
#define STUB
#endif

const char* DXGetErrorStringA(HRESULT hr)
{
	return "STUB";
}

const char* DXGetErrorString(HRESULT hr)
{
	return "STUB";
}

HRESULT D3DXCreateTexture(IDirect3DDevice9 *device, UINT width, UINT height,
        UINT miplevels, DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DTexture9 **texture)
{
	return device->CreateTexture(width, height, miplevels, usage, format, pool, texture, NULL);
}

HRESULT D3DXFilterTexture(IDirect3DBaseTexture9 *texture, const PALETTEENTRY *palette,
	UINT srclevel, DWORD filter)
{
	STUB;
	return -1;
}
HRESULT D3DXCheckTextureRequirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool)
{
	STUB;
	return -1;
}
HRESULT D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
	STUB;
	return -1;

}
HRESULT D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
        const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
	STUB;
	return -1;
}

HRESULT D3DXFillTexture(LPDIRECT3DTEXTURE9 pTexture,
	LPD3DXFILL2D pFunction, void* pData)
{
	return -1;
}

HRESULT D3DXCreateMatrixStack(int unused, D3DXMATRIXSTACK** stack)
{
	*stack = new D3DXMATRIXSTACK();
	(*stack)->Push();
	return D3D_OK;
}

HRESULT D3DXFillVolumeTexture(LPDIRECT3DVOLUMETEXTURE9 pTexture,
	LPD3DXFILL3D pFunction, LPVOID pData)
{
	STUB;
	return -1;
}
HRESULT D3DXCreateSphere(LPDIRECT3DDEVICE9 pDevice, FLOAT Radius, UINT Slices,
	UINT Stacks, LPD3DXMESH *ppMesh, void *ppAdjacency)
{
	STUB;
	return D3D_OK;
}
HRESULT D3DXCreateCubeTexture(LPDIRECT3DDEVICE9 pD3DDev, DWORD dwWidth, DWORD dwMipmaps,
    DWORD usage, D3DFORMAT format, D3DPOOL Pool, LPDIRECT3DCUBETEXTURE9* texture)
{
   return pD3DDev->CreateCubeTexture(dwWidth, dwMipmaps, usage, format, Pool, texture, NULL);
}
HRESULT D3DXCreateVolumeTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width,UINT Height,
    UINT Depth, UINT MipLevels, DWORD Usage,D3DFORMAT Format, D3DPOOL Pool,
	LPDIRECT3DVOLUMETEXTURE9 *ppVolumeTexture)
{
	STUB;
	return -1;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pOut, float l,
	float r, float b, float t, float zn, float zf)
{
	pOut->_11 = 2.0f / (r - l);
	pOut->_12 = 0.0f;
	pOut->_13 = 0.0f;
	pOut->_14 = 0.0f;

	pOut->_21 = 0.0f;
	pOut->_22 = 2.0f / (t - b);
	pOut->_23 = 0.0f;
	pOut->_24 = 0.0f;

	pOut->_31 = 0.0f;
	pOut->_32 = 0.0f;
	pOut->_33 = 1.0f / (zf - zn);
	pOut->_34 = 0.0f;

	pOut->_41 = (l + r) / (l - r);
	pOut->_42 = (t + b) / (b - t);
	pOut->_43 = zn / (zn - zf);
	pOut->_44 = 1.0f;

	return pOut;
}

D3DXMATRIX* D3DXMatrixOrthoOffCenterRH(D3DXMATRIX *pOut, float l, float r,
    float b, float t, float zn, float zf)
{
	pOut->_11 = 2.0f / (r - l);
	pOut->_12 = 0.0f;
	pOut->_13 = 0.0f;
	pOut->_14 = 0.0f;

	pOut->_21 = 0.0f;
	pOut->_22 = 2.0f / (t - b);
	pOut->_23 = 0.0f;
	pOut->_24 = 0.0f;

	pOut->_31 = 0.0f;
	pOut->_32 = 0.0f;
	pOut->_33 = 1.0f / (zn - zf);
	pOut->_34 = 0.0f;

	pOut->_41 = (l + r) / (l - r);
	pOut->_42 = (t + b) / (b - t);
	pOut->_43 = zn / (zn - zf);
	pOut->_44 = 1.0f;

	return pOut;
}

D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX *pOut,
	float l, float r, float b, float t, float zn, float zf)
{
	pOut->_11 = (2.0f * zn) / (r - l);
	pOut->_12 = 0.0f;
	pOut->_13 = 0.0f;
	pOut->_14 = 0.0f;

	pOut->_21 = 0.0f;
	pOut->_22 = (2.0f * zn) / (t - b);
	pOut->_23 = 0.0f;
	pOut->_24 = 0.0f;

	pOut->_31 = (l + r) / (r - l);
	pOut->_32 = (t + b) / (t - b);
	pOut->_33 = zf / (zn - zf);
	pOut->_34 = -1.0f;

	pOut->_41 = 0.0f;
	pOut->_42 = 0.0f;
	pOut->_43 = (zn * zf) / (zn - zf);
	pOut->_44 = 0.0f;

	return pOut;
}

void D3DXMatrixMultiply(D3DMATRIX* out, D3DMATRIX* m1, D3DMATRIX* m2)
{
	memset(out, 0, sizeof(D3DMATRIX));
	out->_11 = (m1->_11 * m2->_11) + (m1->_12 * m2->_21) + (m1->_13 * m2->_31) + (m1->_14 * m2->_41);
	out->_12 = (m1->_11 * m2->_12) + (m1->_12 * m2->_22) + (m1->_13 * m2->_32) + (m1->_14 * m2->_42);
	out->_13 = (m1->_11 * m2->_13) + (m1->_12 * m2->_23) + (m1->_13 * m2->_33) + (m1->_14 * m2->_43);
	out->_14 = (m1->_11 * m2->_14) + (m1->_12 * m2->_24) + (m1->_13 * m2->_34) + (m1->_14 * m2->_44);

	out->_21 = (m1->_21 * m2->_11) + (m1->_22 * m2->_21) + (m1->_23 * m2->_31) + (m1->_24 * m2->_41);
	out->_22 = (m1->_21 * m2->_12) + (m1->_22 * m2->_22) + (m1->_23 * m2->_32) + (m1->_24 * m2->_42);
	out->_23 = (m1->_21 * m2->_13) + (m1->_22 * m2->_23) + (m1->_23 * m2->_33) + (m1->_24 * m2->_43);
	out->_24 = (m1->_21 * m2->_14) + (m1->_22 * m2->_24) + (m1->_23 * m2->_34) + (m1->_24 * m2->_44);

	out->_31 = (m1->_31 * m2->_11) + (m1->_32 * m2->_21) + (m1->_33 * m2->_31) + (m1->_34 * m2->_41);
	out->_32 = (m1->_31 * m2->_12) + (m1->_32 * m2->_22) + (m1->_33 * m2->_32) + (m1->_34 * m2->_42);
	out->_33 = (m1->_31 * m2->_13) + (m1->_32 * m2->_23) + (m1->_33 * m2->_33) + (m1->_34 * m2->_43);
	out->_34 = (m1->_31 * m2->_14) + (m1->_32 * m2->_24) + (m1->_33 * m2->_34) + (m1->_34 * m2->_44);

	out->_41 = (m1->_41 * m2->_11) + (m1->_42 * m2->_21) + (m1->_43 * m2->_31) + (m1->_44 * m2->_41);
	out->_42 = (m1->_41 * m2->_12) + (m1->_42 * m2->_22) + (m1->_43 * m2->_32) + (m1->_44 * m2->_42);
	out->_43 = (m1->_41 * m2->_13) + (m1->_42 * m2->_23) + (m1->_43 * m2->_33) + (m1->_44 * m2->_43);
	out->_44 = (m1->_41 * m2->_14) + (m1->_42 * m2->_24) + (m1->_43 * m2->_34) + (m1->_44 * m2->_44);
}

void D3DXMatrixInverse(D3DMATRIX* out_mat, float* out_det, D3DMATRIX* in)
{
	float inv[16], det;
	int i = 0;
	float m[16];
	m[i++] = in->_11;
	m[i++] = in->_12;
	m[i++] = in->_13;
	m[i++] = in->_14;

	m[i++] = in->_21;
	m[i++] = in->_22;
	m[i++] = in->_23;
	m[i++] = in->_24;

	m[i++] = in->_31;
	m[i++] = in->_32;
	m[i++] = in->_33;
	m[i++] = in->_34;

	m[i++] = in->_41;
	m[i++] = in->_42;
	m[i++] = in->_43;
	m[i++] = in->_44;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
#if 0
	if (fabs(det) < 0.0001f)
	{
		D3DMatrixIdentity(out_mat);
		*out_det = det;
		return;
	}
#endif
	det = 1.0f / det;

	i = 0;
	out_mat->_11 = inv[i++] * det;
	out_mat->_12 = inv[i++] * det;
	out_mat->_13 = inv[i++] * det;
	out_mat->_14 = inv[i++] * det;

	out_mat->_21 = inv[i++] * det;
	out_mat->_22 = inv[i++] * det;
	out_mat->_23 = inv[i++] * det;
	out_mat->_24 = inv[i++] * det;

	out_mat->_31 = inv[i++] * det;
	out_mat->_32 = inv[i++] * det;
	out_mat->_33 = inv[i++] * det;
	out_mat->_34 = inv[i++] * det;

	out_mat->_41 = inv[i++] * det;
	out_mat->_42 = inv[i++] * det;
	out_mat->_43 = inv[i++] * det;
	out_mat->_44 = inv[i++] * det;

	if (out_det != NULL)
	{
		*out_det = det;
	}
}

void D3DXMatrixTranspose(D3DMATRIX* dst, D3DMATRIX* src)
{
	dst->_11 = src->_11;
	dst->_12 = src->_21;
	dst->_13 = src->_31;
	dst->_14 = src->_41;

	dst->_21 = src->_12;
	dst->_22 = src->_22;
	dst->_23 = src->_32;
	dst->_24 = src->_42;

	dst->_31 = src->_13;
	dst->_32 = src->_23;
	dst->_33 = src->_33;
	dst->_34 = src->_43;

	dst->_41 = src->_14;
	dst->_42 = src->_24;
	dst->_43 = src->_34;
	dst->_44 = src->_44;
}

void D3DXMatrixIdentity(D3DMATRIX* mat)
{
	D3DXMatrixScaling(mat, 1.0f, 1.0f, 1.0f);
}

void D3DXMatrixScaling(D3DMATRIX* mat, float sx, float sy, float sz)
{
	memset(mat, 0, sizeof(D3DMATRIX));
	mat->_11 = sx;
	mat->_22 = sy;
	mat->_33 = sz;
	mat->_44 = 1.0f;
}

void D3DXMatrixTranslation(D3DMATRIX* mat, float x, float y, float z)
{
	D3DXMatrixIdentity(mat);
	mat->_41 = x;
	mat->_42 = y;
	mat->_43 = z;
}

void D3DXMatrixRotationX(D3DMATRIX* mat, float angle)
{
	D3DXVECTOR3 vec = { 1.0f, 0.0f, 0.0f };
	D3DXMatrixRotationAxis(mat, &vec, angle);
}

void D3DXMatrixRotationY(D3DMATRIX* mat, float angle)
{
	D3DXVECTOR3 vec = { 0.0f, 1.0f, 0.0f };
	D3DXMatrixRotationAxis(mat, &vec, angle);
}

void D3DXMatrixRotationZ(D3DMATRIX* mat, float angle)
{
	D3DXVECTOR3 vec = { 0.0f, 0.0f, 1.0f };
	D3DXMatrixRotationAxis(mat, &vec, angle);
}

void D3DXMatrixRotationAxis(D3DMATRIX* mat, D3DXVECTOR3* axis, float angle)
{
	float xy, xz, yz;
	float one_minus_cos = 1.0f - cosf(angle);

	D3DXVec3Normalize(axis, axis);

	xy = axis->x * axis->y;
	xz = axis->x * axis->z;
	yz = axis->y * axis->z;

	mat->_11 = cosf(angle) + ((axis->x * axis->x) * one_minus_cos);
	mat->_21 = (xy * one_minus_cos) - (axis->z * sinf(angle));
	mat->_31 = (xz * one_minus_cos) + (axis->y * sinf(angle));
	mat->_41 = 0.0f;

	mat->_12 = (xy * one_minus_cos) + (axis->z * sinf(angle));
	mat->_22 = cosf(angle) + ((axis->y * axis->y) * one_minus_cos);
	mat->_32 = (yz * one_minus_cos) - (axis->x * sinf(angle));
	mat->_42 = 0.0f;

	mat->_13 = (xz * one_minus_cos) - (axis->y * sinf(angle));
	mat->_23 = (yz * one_minus_cos) + (axis->x * sinf(angle));
	mat->_33 = cosf(angle) + ((axis->z * axis->z) * one_minus_cos);
	mat->_43 = 0.0f;

	mat->_14 = 0.0f;
	mat->_24 = 0.0f;
	mat->_34 = 0.0f;
	mat->_44 = 1.0f;
}

D3DXMATRIX* D3DXMatrixPerspectiveFovRH(D3DXMATRIX *pOut, FLOAT mfovy,
    FLOAT Aspect, FLOAT zn, FLOAT zf)
{
	D3DXMatrixIdentity(pOut);

	float yScale = cos(mfovy / 2.0f) / sin(mfovy / 2.0f);
	float xScale = yScale / Aspect;

    pOut->m[0][0] = xScale;
    pOut->m[1][1] =yScale;
    pOut->m[2][2] = zf / (zn - zf);
    pOut->m[2][3] = -1.0f;
    pOut->m[3][2] = (zf * zn) / (zn - zf);
    pOut->m[3][3] = 0.0f;
    return pOut;
}

D3DXMATRIX* D3DXMatrixOrthoRH(D3DXMATRIX *pOut, FLOAT w,
	FLOAT h, FLOAT zn, FLOAT zf)
{
	D3DXMatrixIdentity(pOut);
    pOut->m[0][0] = 2.0f / w;
    pOut->m[1][1] = 2.0f / h;
    pOut->m[2][2] = 1.0f / (zn - zf);
    pOut->m[3][2] = zn / (zn - zf);
    return pOut;
}

D3DXCOLOR* D3DXColorLerp(D3DXCOLOR *pOut, const D3DXCOLOR *pC1,
    const D3DXCOLOR *pC2, FLOAT s)
{
	STUB;
    return NULL;
}

static void D3DXVec3TransformCoord(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM)
{
   D3DXVECTOR3 out;
   FLOAT norm;
   norm = pM->m[0][3] * pV->x + pM->m[1][3] * pV->y + pM->m[2][3] *pV->z + pM->m[3][3];

   out.x = (pM->m[0][0] * pV->x + pM->m[1][0] * pV->y + pM->m[2][0] * pV->z + pM->m[3][0]) / norm;
   out.y = (pM->m[0][1] * pV->x + pM->m[1][1] * pV->y + pM->m[2][1] * pV->z + pM->m[3][1]) / norm;
   out.z = (pM->m[0][2] * pV->x + pM->m[1][2] * pV->y + pM->m[2][2] * pV->z + pM->m[3][2]) / norm;

   *pOut = out;
}

D3DXVECTOR3* D3DXVec3Project(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV,
	const D3DVIEWPORT9 *pViewport, const D3DXMATRIX *pProjection,
	const D3DXMATRIX *pView, const D3DXMATRIX *pWorld)
{
   D3DXMATRIX m;

   D3DXMatrixIdentity(&m);
   if (pWorld) D3DXMatrixMultiply(&m, &m, pWorld);
   if (pView) D3DXMatrixMultiply(&m, &m, pView);
   if (pProjection) D3DXMatrixMultiply(&m, &m, pProjection);

   D3DXVec3TransformCoord(pOut, pV, &m);

   if (pViewport)
   {
      pOut->x = pViewport->X +  ( 1.0f + pOut->x ) * pViewport->Width / 2.0f;
      pOut->y = pViewport->Y +  ( 1.0f - pOut->y ) * pViewport->Height / 2.0f;
      pOut->z = pViewport->MinZ + pOut->z * ( pViewport->MaxZ - pViewport->MinZ );
   }
   return pOut;
}

D3DXMATRIX* D3DXMatrixLookAtRH(D3DXMATRIX *pOut, D3DXVECTOR3 *pEye,
	D3DXVECTOR3 *pAt, D3DXVECTOR3 *pUp)
{
	D3DXVECTOR3 right, upn, vec;
	vec = *pAt - *pEye;
    D3DXVec3Normalize(&vec, &vec);
    D3DXVec3Cross(&right, pUp, &vec);
    D3DXVec3Cross(&upn, &vec, &right);
    D3DXVec3Normalize(&right, &right);
    D3DXVec3Normalize(&upn, &upn);

    pOut->m[0][0] = -right.x;
    pOut->m[1][0] = -right.y;
    pOut->m[2][0] = -right.z;
    pOut->m[3][0] = D3DXVec3Dot(&right, pEye);
    pOut->m[0][1] = upn.x;
    pOut->m[1][1] = upn.y;
    pOut->m[2][1] = upn.z;
    pOut->m[3][1] = -D3DXVec3Dot(&upn, pEye);
    pOut->m[0][2] = -vec.x;
    pOut->m[1][2] = -vec.y;
    pOut->m[2][2] = -vec.z;
    pOut->m[3][2] = D3DXVec3Dot(&vec, pEye);
    pOut->m[0][3] = 0.0f;
    pOut->m[1][3] = 0.0f;
    pOut->m[2][3] = 0.0f;
    pOut->m[3][3] = 1.0f;

	return pOut;
}

float D3DXVec2Dot(D3DXVECTOR2* A, D3DXVECTOR2* B)
{
	return (A->x * B->x) + (A->y * B->y);
}

float D3DXVec3Dot(D3DXVECTOR3* A, D3DXVECTOR3* B)
{
	return (A->x * B->x) + (A->y * B->y) + (A->z * B->z);
}

float D3DXVec2Length(D3DXVECTOR2* v)
{
	return sqrtf(D3DXVec2Dot(v, v));
}

float D3DXVec3Length(D3DXVECTOR3* v)
{
	return sqrtf(D3DXVec3Dot(v, v));
}

D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3 *pOut, D3DXVECTOR3 *pv)
{
	float length = D3DXVec3Length(pv);

	if (length == 0.0f)
	{
		length = 1.0f;
	}

	pOut->x = pv->x / length;
	pOut->y = pv->y / length;
	pOut->z = pv->z / length;

	return pOut;
}

void D3DXVec3Cross(D3DXVECTOR3* out, D3DXVECTOR3* A, D3DXVECTOR3* B)
{
	out->x = (A->y * B->z) - (A->z * B->y);
	out->y = (A->z * B->x) - (A->x * B->z);
	out->z = (A->x * B->y) - (A->y * B->x);
}

void CopyD3DMatrixToFloats(D3DMATRIX* mat, float* buf)
{
	size_t i = 0;
	buf[i++] = mat->_11;
	buf[i++] = mat->_12;
	buf[i++] = mat->_13;
	buf[i++] = mat->_14;

	buf[i++] = mat->_21;
	buf[i++] = mat->_22;
	buf[i++] = mat->_23;
	buf[i++] = mat->_24;

	buf[i++] = mat->_31;
	buf[i++] = mat->_32;
	buf[i++] = mat->_33;
	buf[i++] = mat->_34;

	buf[i++] = mat->_31;
	buf[i++] = mat->_32;
	buf[i++] = mat->_33;
	buf[i++] = mat->_34;
}

void CopyFloatsToD3DMatrix(float* buf, D3DMATRIX* mat)
{
	size_t i = 0;

	mat->_11 = buf[i++];
	mat->_12 = buf[i++];
	mat->_13 = buf[i++];
	mat->_14 = buf[i++];

	mat->_21 = buf[i++];
	mat->_22 = buf[i++];
	mat->_23 = buf[i++];
	mat->_24 = buf[i++];

	mat->_31 = buf[i++];
	mat->_32 = buf[i++];
	mat->_33 = buf[i++];
	mat->_34 = buf[i++];

	mat->_41 = buf[i++];
	mat->_42 = buf[i++];
	mat->_43 = buf[i++];
	mat->_44 = buf[i++];
}

BOOL InflateRect(RECT* lprc, int dx, int dy)
{
    lprc->left   -= dx;
    lprc->top    -= dy;
    lprc->right  += dx;
    lprc->bottom += dy;
    return 1;
}