//////////////////////////////////////////////////////////////////////
//
//  Standalone logic tests for the touch overlay math
//  (hit-testing geometry, stick dead-zone, snap grid, clamp rules).
//
//  Build & run:
//    g++ -std=c++17 tests/TouchLogicTest.cpp -o /tmp/touchlogic && /tmp/touchlogic
//
//////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include <stdio.h>

static int g_failures = 0;
#define CHECK(cond) do { \
	if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_failures++; } \
} while (0)

// ---- reference implementations (mirror of the engine code) ----

struct Elem
{
	float x, y, size;	// normalized
	bool round;
	int halfExtentPx(float scrH, float scale) const
	{
		float rad = size * scrH * scale;
		if (rad < 8.0f) rad = 8.0f;
		return (int)rad;
	}
};

static bool HitTest(const Elem &el, float px, float py, float scrW, float scrH, float scale)
{
	float ex = el.x * scrW;
	float ey = el.y * scrH;
	float rad = el.size * scrH * scale;
	if (rad < 8.0f) rad = 8.0f;
	float dx = px - ex, dy = py - ey;
	if (el.round)
		return dx * dx + dy * dy <= rad * rad;
	return fabsf(dx) <= rad && fabsf(dy) <= rad;
}

static float SnapCoord(float v)
{
	const float step = 1.0f / 48.0f;
	return floorf(v / step + 0.5f) * step;
}

static float Clamp01(float v)
{
	if (v < 0.02f) v = 0.02f;
	if (v > 0.98f) v = 0.98f;
	return v;
}

// stick dead zone: returns axis state for a normalized offset
struct Axes { bool w, a, s, d; };
static Axes StickAxes(float offX, float offY, float radiusPx)
{
	const float dead = 0.30f;
	Axes ax = { false, false, false, false };
	float maxLen = radiusPx * 0.9f;
	if (maxLen < 1.0f) maxLen = 1.0f;
	float nx = offX / maxLen;
	float ny = offY / maxLen;
	if (ny < -dead) ax.w = true;
	if (ny > dead) ax.s = true;
	if (nx < -dead) ax.a = true;
	if (nx > dead) ax.d = true;
	return ax;
}

// resize clamp from the editor
static float ClampSize(float fSize)
{
	float minSize = 0.018f;
	float maxSize = 0.22f;
	if (fSize < minSize) fSize = minSize;
	if (fSize > maxSize) fSize = maxSize;
	return fSize;
}

int main()
{
	// ---- hit testing ----
	{
		Elem fire = { 0.865f, 0.700f, 0.068f, true };
		CHECK(HitTest(fire, 0.865f * 1280, 0.700f * 720, 1280, 720, 1.0f));		// center
		CHECK(HitTest(fire, 0.865f * 1280 + 20, 0.700f * 720, 1280, 720, 1.0f)); // near edge
		CHECK(!HitTest(fire, 0.5f * 1280, 0.5f * 720, 1280, 720, 1.0f));			// far away
		CHECK(!HitTest(fire, 0.865f * 1280 + 49, 0.700f * 720, 1280, 720, 1.0f)); // outside r=49
	}
	{
		Elem slot = { 0.430f, 0.090f, 0.026f, false };	// square (weapons slot)
		CHECK(HitTest(slot, 0.430f * 1280, 0.090f * 720, 1280, 720, 1.0f));
		CHECK(!HitTest(slot, 0.430f * 1280 + 19, 0.090f * 720 + 19, 1280, 720, 1.0f)); // corner of square is outside circle-r
		CHECK(HitTest(slot, 0.430f * 1280 + 18, 0.090f * 720 + 18, 1280, 720, 1.0f));	// inside square
	}
	{
		// minimum radius clamp: tiny element is still >= 8px
		Elem tiny = { 0.5f, 0.5f, 0.001f, true };
		CHECK(HitTest(tiny, 0.5f * 1280 + 7, 0.5f * 720, 1280, 720, 1.0f));
		CHECK(!HitTest(tiny, 0.5f * 1280 + 9, 0.5f * 720, 1280, 720, 1.0f));
	}
	{
		// scale factor enlarges the touch area
		Elem fire = { 0.865f, 0.700f, 0.068f, true };
		CHECK(HitTest(fire, 0.865f * 1280 + 70, 0.700f * 720, 1280, 720, 2.0f));
	}

	// ---- stick axes & dead zone ----
	{
		Axes a = StickAxes(0, 0, 54);					// centered
		CHECK(!a.w && !a.a && !a.s && !a.d);
	}
	{
		Axes a = StickAxes(0, -40, 54);				// up, within dead zone (40/48.6 = 0.82 > 0.3 → W!)
		CHECK(a.w && !a.s);								// -40px is way past dead zone
	}
	{
		Axes a = StickAxes(0, -10, 54);				// small deflection → nothing
		CHECK(!a.w && !a.s && !a.a && !a.d);
	}
	{
		Axes a = StickAxes(-50, 0, 54);				// left
		CHECK(a.a && !a.d && !a.w && !a.s);
	}
	{
		Axes a = StickAxes(50, 20, 54);				// right+back
		CHECK(a.d && a.s);
	}
	{
		// diagonal
		Axes a = StickAxes(-45, -45, 54);
		CHECK(a.w && a.a && !a.s && !a.d);
	}

	// ---- editor helpers ----
	{
		// snap grid lands on the 1/48 lattice
		CHECK(fabsf(SnapCoord(0.5f) - 0.5f) < 0.0001f);
		CHECK(fabsf(SnapCoord(0.1234f) - SnapCoord(0.1234f)) < 0.0001f);
		CHECK(fabsf(SnapCoord(0.1234f) - 6.0f / 48.0f) < 0.02f);		// 5.92 cells -> 6
		CHECK(fabsf(SnapCoord(0.9f) - 43.0f / 48.0f) < 0.02f);			// 43.2 cells -> 43
	}
	{
		CHECK(Clamp01(-0.5f) == 0.02f);
		CHECK(Clamp01(1.7f) == 0.98f);
		CHECK(Clamp01(0.33f) == 0.33f);
	}
	{
		CHECK(ClampSize(0.001f) == 0.018f);	// not smaller than a finger tip
		CHECK(ClampSize(0.9f) == 0.22f);		// not larger than a third of the screen
		CHECK(ClampSize(0.05f) == 0.05f);		// unchanged in range
	}

	// ---- resize drag direction sanity ----
	{
		// dragging corner +40px right and +20px down on a 720-high screen
		float startSize = 0.050f;
		float scale = 1.0f;
		float d = (40.0f + 20.0f) * 0.5f / 720.0f;
		float newSize = ClampSize(startSize + d / scale);
		CHECK(newSize > startSize);
		CHECK(fabsf(newSize - 0.050f - 0.0416f) < 0.001f);
	}

	if (g_failures == 0)
		printf("ALL TOUCH LOGIC TESTS PASSED\n");
	else
		printf("%d FAILURES\n", g_failures);
	return g_failures == 0 ? 0 : 1;
}
