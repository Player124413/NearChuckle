struct D3DXVECTOR3
{
	D3DXVECTOR3(void)
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	D3DXVECTOR3(float inx, float iny, float inz)
	{
		x = inx;
		y = iny;
		z = inz;
	}

	D3DXVECTOR3 operator+(const D3DXVECTOR3& a) const
	{
		return D3DXVECTOR3(a.x + x, a.y + y, a.z + z);
	}

	D3DXVECTOR3 operator+=(const D3DXVECTOR3& a)
	{
		x += a.x;
		y += a.y;
		z += a.z;
		return *this;
	}

	D3DXVECTOR3 operator-(const D3DXVECTOR3& a) const
	{
		return D3DXVECTOR3(x - a.x, y - a.y, z - a.z);
	}

	D3DXVECTOR3 operator-=(const D3DXVECTOR3& a) const
	{
		return D3DXVECTOR3(x - a.x, y - a.y, z - a.z);
	}

	D3DXVECTOR3 operator*(const float scale) const
	{
		return D3DXVECTOR3(x * scale, y * scale, z * scale);
	}

	D3DXVECTOR3 operator*=(const float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;

		return *this;
	}

	D3DXVECTOR3 operator/=(const float scale) const
	{
		return D3DXVECTOR3(x / scale, y / scale, z / scale);
	}
	float& operator[](size_t index)
	{
		switch (index)
		{
			case 0:
				return x;
			case 1:
				return y;
			case 2:
				return z;
		}
		__builtin_trap();
		return x;
	}

	float x;
	float y;
	float z;
};

struct D3DXVECTOR4
{
	D3DXVECTOR4()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	}

	D3DXVECTOR4(float* in)
	{
		x = in[0];
		y = in[1];
		z = in[2];
		w = in[3];
	}

	D3DXVECTOR4(float inx, float iny, float inz, float inw)
	{
		x = inx;
		y = iny;
		z = inz;
		w = inw;
	}

	D3DXVECTOR4 operator*(const float scale) const
	{
		return D3DXVECTOR4(x * scale, y * scale, z * scale, w * scale);
	}

	D3DXVECTOR4 operator/(const float scale) const
	{
		return D3DXVECTOR4(x / scale, y / scale, z / scale, w / scale);
	}

	D3DXVECTOR4 operator*=(const float scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
		w *= scale;
		return *this;
	}

	D3DXVECTOR4 operator/=(const float scale)
	{
		x /= scale;
		y /= scale;
		z /= scale;
		w /= scale;
		return *this;
	}

	float x;
	float y;
	float z;
	float w;
};

struct D3DXVECTOR2
{
	D3DXVECTOR2(void)
	{
		x = 0.0f;
		y = 0.0f;
	}

	D3DXVECTOR2(float inx, float iny)
	{
		x = inx;
		y = iny;
	}

	D3DXVECTOR2 operator*=(const int scale)
	{
		x *= (float)scale;
		y *= (float)scale;

		return *this;
	}

	D3DXVECTOR2 operator/=(const float scale)
	{
		x /= scale;
		y /= scale;
		return *this;
	}

	float x;
	float y;
};

struct D3DXCOLOR
{
	D3DXCOLOR(void)
	{
		r = 0.0f;
		g = 0.0f;
		b = 0.0f;
		a = 0.0f;
	}

	D3DXCOLOR(float inr, float ing, float inb, float ina)
	{
		r = inr;
		g = ing;
		b = inb;
		a = ina;
	}

	D3DXCOLOR operator/=(const float scale) const
	{
		return D3DXCOLOR(r / scale, g / scale, b / scale, a / scale);
	}

	float r;
	float g;
	float b;
	float a;
};

typedef D3DMATRIX D3DXMATRIX;
typedef D3DMATRIX D3DXMATRIXA16;

void D3DXMatrixMultiply(D3DMATRIX* out, D3DMATRIX* m1, D3DMATRIX* m2);
void D3DXMatrixInverse(D3DMATRIX* out_mat, float* out_det, D3DMATRIX* in);
void D3DXMatrixTranspose(D3DMATRIX* dst, D3DMATRIX* src);
void D3DXMatrixIdentity(D3DMATRIX* mat);
void D3DXMatrixScaling(D3DMATRIX* mat, float sx, float sy, float sz);
void D3DXMatrixTranslation(D3DMATRIX* mat, float x, float y, float z);
void D3DXMatrixRotationX(D3DMATRIX* mat, float angle);
void D3DXMatrixRotationY(D3DMATRIX* mat, float angle);
void D3DXMatrixRotationZ(D3DMATRIX* mat, float angle);
void D3DXMatrixRotationAxis(D3DMATRIX* mat, D3DXVECTOR3* axis, float angle);

struct D3DXMATRIXSTACK
{
	void Push()
	{
		D3DMATRIX tmp;
		D3DXMatrixIdentity(&tmp);
		matrices.push_back(tmp);
	}
	HRESULT Pop()
	{
		if (matrices.empty())
		{
			return -1;
		}

		matrices.pop_back();
		return 1;
	}
	void LoadIdentity()
	{
		if (matrices.empty())
		{
			__builtin_trap();
			return;
		}
		D3DXMatrixIdentity(&matrices.back());
	}
	void LoadMatrix(D3DMATRIX* mat)
	{
		matrices.pop_back();
		matrices.push_back(*mat);
	}
	HRESULT TranslateLocal(float x, float y, float z)
	{
		D3DMATRIX tmp;
		if (matrices.empty())
		{
			return -1;
		}
		D3DXMatrixTranslation( &tmp, x, y, z );
		D3DXMatrixMultiply(&matrices.back(), &tmp, &matrices.back());
		return 1;
	}

	HRESULT RotateAxisLocal(D3DXVECTOR3 *pV, float angle)
	{
		D3DMATRIX tmp;
		if (matrices.empty())
		{
			return -1;
		}
		D3DXMatrixRotationAxis( &tmp, pV, angle );
		D3DXMatrixMultiply(&matrices.back(), &tmp, &matrices.back());
		return 1;
	}

	HRESULT Scale(float x, float y, float z)
	{
		D3DMATRIX tmp;
		if (matrices.empty())
		{
			return -1;
		}
		D3DXMatrixScaling(&tmp, x, y, z);
		D3DXMatrixMultiply(&matrices.back(), &matrices.back(), &tmp);
		return 1;
	}

	HRESULT ScaleLocal(float x, float y, float z)
	{
		D3DMATRIX tmp;
		if (matrices.empty())
		{
			return -1;
		}
		D3DXMatrixScaling(&tmp, x, y, z);
		D3DXMatrixMultiply(&matrices.back(), &tmp, &matrices.back());
		return 1;
	}

	HRESULT MultMatrixLocal(D3DXMATRIX *pMat)
	{
		D3DXMatrixMultiply(&matrices.back(), pMat, &matrices.back());
		return 1;
	}

	D3DMATRIX* GetTop()
	{
		if (matrices.empty())
		{
			return NULL;
		}
		return &matrices.back();
	}

	std::vector<D3DMATRIX> matrices;
};

typedef D3DXMATRIXSTACK* LPD3DXMATRIXSTACK;
typedef char TCHAR;
typedef char CHAR;

struct D3DXMESH
{
	void Release() {}
	void DrawSubset(int unused) {}
	int lmao;
};

typedef D3DXMESH* LPD3DXMESH;

#define D3DX_FILTER_NONE                 0x00000001
#define D3DX_FILTER_POINT                0x00000002
#define D3DX_FILTER_LINEAR               0x00000003
#define D3DX_FILTER_TRIANGLE             0x00000004
#define D3DX_FILTER_BOX                  0x00000005

typedef void (*LPD3DXFILL2D)(
    D3DXVECTOR4*,
    const D3DXVECTOR2*,
    const D3DXVECTOR2*,
    void*
);

typedef void (*LPD3DXFILL3D)(
    D3DXVECTOR4*,
    const D3DXVECTOR3*,
    const D3DXVECTOR3*,
    void*
);

const char* DXGetErrorStringA(HRESULT hr);
const char* DXGetErrorString(HRESULT hr);

HRESULT D3DXFilterTexture(IDirect3DBaseTexture9 *texture, const PALETTEENTRY *palette,
	UINT srclevel, DWORD filter);
HRESULT D3DXCheckTextureRequirements(struct IDirect3DDevice9 *device, UINT *width, UINT *height,
        UINT *miplevels, DWORD usage, D3DFORMAT *format, D3DPOOL pool);
HRESULT D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key);
HRESULT D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
        const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key);

HRESULT D3DXFillTexture(LPDIRECT3DTEXTURE9 pTexture,
	LPD3DXFILL2D pFunction, void* pData);

HRESULT D3DXCreateMatrixStack(int unused, D3DXMATRIXSTACK** stack);

HRESULT D3DXCreateTexture(IDirect3DDevice9 *device, UINT width, UINT height,
        UINT miplevels, DWORD usage, D3DFORMAT format, D3DPOOL pool, struct IDirect3DTexture9 **texture);

HRESULT D3DXFillVolumeTexture(LPDIRECT3DVOLUMETEXTURE9 pTexture,
	LPD3DXFILL3D pFunction, LPVOID pData);
HRESULT D3DXCreateSphere(LPDIRECT3DDEVICE9 pDevice, FLOAT Radius, UINT Slices,
	UINT Stacks, LPD3DXMESH *ppMesh, void *ppAdjacency);
HRESULT D3DXCreateCubeTexture(LPDIRECT3DDEVICE9 pD3DDev, DWORD dwWidth, DWORD dwMipmaps,
    DWORD usage, D3DFORMAT format, D3DPOOL Pool, LPDIRECT3DCUBETEXTURE9* texture);
HRESULT D3DXCreateVolumeTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width,UINT Height,
    UINT Depth, UINT MipLevels, DWORD Usage,D3DFORMAT Format, D3DPOOL Pool,
	LPDIRECT3DVOLUMETEXTURE9 *ppVolumeTexture);

D3DXMATRIX* D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pOut, float l,
	float r, float b, float t, float zn, float zf);
D3DXMATRIX* D3DXMatrixOrthoOffCenterRH(D3DXMATRIX *pOut, float l, float r,
    float b, float t, float zn, float zf);

D3DXMATRIX* D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX *pOut,
	float l, float r, float b, float t, float zn, float zf);
D3DXMATRIX* D3DXMatrixPerspectiveFovRH(D3DXMATRIX *pOut, FLOAT mfovy,
    FLOAT Aspect, FLOAT zn, FLOAT zf);

D3DXMATRIX* D3DXMatrixOrthoRH(D3DXMATRIX *pOut, FLOAT w,
	FLOAT h, FLOAT zn, FLOAT zf);

D3DXCOLOR* D3DXColorLerp(D3DXCOLOR *pOut, const D3DXCOLOR *pC1,
    const D3DXCOLOR *pC2, FLOAT s);

D3DXVECTOR3* D3DXVec3Project(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV,
	const D3DVIEWPORT9 *pViewport, const D3DXMATRIX *pProjection,
	const D3DXMATRIX *pView, const D3DXMATRIX *pWorld);

D3DXMATRIX* D3DXMatrixLookAtRH(D3DXMATRIX  *pOut, D3DXVECTOR3 *pEye,
	D3DXVECTOR3 *pAt, D3DXVECTOR3 *pUp);


float D3DXVec2Dot(D3DXVECTOR2* A, D3DXVECTOR2* B);
float D3DXVec3Dot(D3DXVECTOR3* A, D3DXVECTOR3* B);

float D3DXVec2Length(D3DXVECTOR2* v);
float D3DXVec3Length(D3DXVECTOR3* v);

D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3 *pOut, D3DXVECTOR3 *pv);
void D3DXVec3Cross(D3DXVECTOR3* out, D3DXVECTOR3* A, D3DXVECTOR3* B);

void D3DVec3Normalize(D3DXVECTOR3* out, D3DXVECTOR3* in);

void CopyD3DMatrixToFloats(D3DMATRIX* mat, float* buf);
void CopyFloatsToD3DMatrix(float* buf, D3DMATRIX* mat);

BOOL InflateRect(RECT* lprc, int dx, int dy);

#define D3DX_DEFAULT         ((UINT)-1)
#define D3DX_PI    ((FLOAT)3.141592654)
#define D3DXToRadian(degree) ((degree) * (D3DX_PI / 180.0f))
#define lstrcpyn strncpy

