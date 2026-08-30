//////////////////////////////////////////////////////////////////////
//
//  Crytek CryENGINE Source code
//
//  File: TouchControls.cpp
//  Description: On-screen touch controls (virtual joystick + buttons)
//               with a full layout editor for mobile ports.
//
//  Features:
//   - virtual analog stick (dynamic or fixed) with WASD emulation
//   - on-screen buttons that inject real engine key events
//   - EDIT button: move / resize / hide / show / add / delete elements,
//     grid snapping, pinch resize, save & reset of the layout
//   - layout persistence as JSON next to the game data
//   - every aspect tunable via console variables
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(_XBOX) && !defined(PS2)

#include "TouchControls.h"

// on-device log sharing (defined in AndroidApp/DiagLog.cpp)
#if defined(__ANDROID__)
extern "C" void AndroidSendLogs(void) __attribute__((weak));
#endif
#include "Game.h"

#include <ISystem.h>
#include <IRenderer.h>
#include <IConsole.h>
#include <ILog.h>
#include <ITimer.h>
#include <IFont.h>
#include <Cry_Math.h>

#include <stdarg.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Optional platform haptic feedback (defined in the Android launcher
// library; weak, so ports without it simply get no vibration).
#if defined(__GNUC__)
extern "C" void AndroidTouchVibrate(int ms) __attribute__((weak));
#endif

#define TOUCH_LAYOUT_FILE "touch_layout.json"
#define TOUCH_LAYOUT_VERSION 1

// icon glyphs
#define TOUCH_ICON_HIDE  "x"
#define TOUCH_ICON_GRIP  "="

//////////////////////////////////////////////////////////////////////////
// Key name <-> XKey mapping (subset used by touch controls)
//////////////////////////////////////////////////////////////////////////
struct SKeyMapEntry { const char *szName; int nXKey; };

static const SKeyMapEntry g_KeyMap[] =
{
	{ "mouse1", XKEY_MOUSE1 }, { "mouse2", XKEY_MOUSE2 }, { "mouse3", XKEY_MOUSE3 },
	{ "mouse4", XKEY_MOUSE4 }, { "mouse5", XKEY_MOUSE5 },
	{ "wheel_up", XKEY_MWHEEL_UP }, { "wheel_down", XKEY_MWHEEL_DOWN },
	{ "space", XKEY_SPACE }, { "return", XKEY_RETURN }, { "tab", XKEY_TAB },
	{ "escape", XKEY_ESCAPE }, { "backspace", XKEY_BACKSPACE },
	{ "up", XKEY_UP }, { "down", XKEY_DOWN }, { "left", XKEY_LEFT }, { "right", XKEY_RIGHT },
	{ "lctrl", XKEY_LCONTROL }, { "rctrl", XKEY_RCONTROL },
	{ "lalt", XKEY_LALT }, { "ralt", XKEY_RALT },
	{ "lshift", XKEY_LSHIFT }, { "rshift", XKEY_RSHIFT },
	{ "0", XKEY_0 }, { "1", XKEY_1 }, { "2", XKEY_2 }, { "3", XKEY_3 }, { "4", XKEY_4 },
	{ "5", XKEY_5 }, { "6", XKEY_6 }, { "7", XKEY_7 }, { "8", XKEY_8 }, { "9", XKEY_9 },
	{ "a", XKEY_A }, { "b", XKEY_B }, { "c", XKEY_C }, { "d", XKEY_D }, { "e", XKEY_E },
	{ "f", XKEY_F }, { "g", XKEY_G }, { "h", XKEY_H }, { "i", XKEY_I }, { "j", XKEY_J },
	{ "k", XKEY_K }, { "l", XKEY_L }, { "m", XKEY_M }, { "n", XKEY_N }, { "o", XKEY_O },
	{ "p", XKEY_P }, { "q", XKEY_Q }, { "r", XKEY_R }, { "s", XKEY_S }, { "t", XKEY_T },
	{ "u", XKEY_U }, { "v", XKEY_V }, { "w", XKEY_W }, { "x", XKEY_X }, { "y", XKEY_Y },
	{ "z", XKEY_Z },
	{ "f1", XKEY_F1 }, { "f2", XKEY_F2 }, { "f3", XKEY_F3 }, { "f4", XKEY_F4 },
	{ "f5", XKEY_F5 }, { "f6", XKEY_F6 }, { "f7", XKEY_F7 }, { "f8", XKEY_F8 },
	{ "f9", XKEY_F9 }, { "f10", XKEY_F10 }, { "f11", XKEY_F11 }, { "f12", XKEY_F12 },
	{ "tilde", XKEY_TILDE }, { "insert", XKEY_INSERT }, { "delete", XKEY_DELETE },
	{ "home", XKEY_HOME }, { "end", XKEY_END },
	{ "page_up", XKEY_PAGE_UP }, { "page_down", XKEY_PAGE_DOWN },
	{ "none", 0 },
};
#define KEYMAP_COUNT (sizeof(g_KeyMap) / sizeof(g_KeyMap[0]))

const char *CTouchControls::XKeyToName(int nXKey)
{
	for (unsigned i = 0; i < KEYMAP_COUNT; i++)
		if (g_KeyMap[i].nXKey == nXKey)
			return g_KeyMap[i].szName;
	return "none";
}

int CTouchControls::NameToXKey(const char *szName)
{
	if (!szName || !szName[0])
		return 0;
	for (unsigned i = 0; i < KEYMAP_COUNT; i++)
		if (_stricmp(g_KeyMap[i].szName, szName) == 0)
			return g_KeyMap[i].nXKey;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Global instance
//////////////////////////////////////////////////////////////////////////
static CTouchControls *g_pTouchControls = 0;

CTouchControls *GetTouchControls()
{
	return g_pTouchControls;
}

void ShutDownTouchControls()
{
	if (g_pTouchControls)
	{
		g_pTouchControls->ShutDown();
		delete g_pTouchControls;
		g_pTouchControls = 0;
	}
}

void TouchVibrate(int ms)
{
#if defined(__GNUC__)
	if (AndroidTouchVibrate)
		AndroidTouchVibrate(ms);
#endif
}

//////////////////////////////////////////////////////////////////////////
// Default layout (Far Cry 1 default PC bindings)
//////////////////////////////////////////////////////////////////////////
struct SDefaultElement { const char *name, *label, *key; float x, y, size; bool round, tapOnly; };

static const SDefaultElement g_DefaultElements[] =
{
	// right thumb cluster
	{ "fire",      "FIRE",   "mouse1",    0.865f, 0.700f, 0.068f, true,  false },
	{ "aim",       "AIM",    "mouse2",    0.745f, 0.780f, 0.052f, true,  false },
	{ "reload",    "REL",    "r",         0.770f, 0.620f, 0.040f, true,  false },
	{ "use",       "USE",    "f",         0.640f, 0.660f, 0.040f, true,  false },
	{ "jump",      "JMP",    "space",     0.870f, 0.860f, 0.046f, true,  false },
	{ "crouch",    "CRCH",   "lctrl",     0.750f, 0.900f, 0.040f, true,  false },
	{ "grenade",   "GRN",    "g",         0.660f, 0.540f, 0.036f, true,  false },
	{ "sprint",    "SPR",    "lshift",    0.680f, 0.800f, 0.040f, true,  false },
	// left thumb column (above stick)
	{ "wpncycle",  "WPN",    "q",         0.220f, 0.560f, 0.038f, true,  true  },
	{ "flashlight","LIGHT",  "l",         0.095f, 0.560f, 0.034f, true,  true  },
	{ "binoc",     "BINOC",  "b",         0.090f, 0.400f, 0.034f, true,  true  },
	{ "map",       "MAP",    "tab",       0.220f, 0.430f, 0.034f, true,  true  },
	// extra weapons slots
	{ "slot1",     "1",      "1",         0.430f, 0.090f, 0.026f, false, true  },
	{ "slot2",     "2",      "2",         0.475f, 0.090f, 0.026f, false, true  },
	{ "slot3",     "3",      "3",         0.520f, 0.090f, 0.026f, false, true  },
	{ "slot4",     "4",      "4",         0.565f, 0.090f, 0.026f, false, true  },
};
#define DEFAULT_ELEMENT_COUNT (sizeof(g_DefaultElements) / sizeof(g_DefaultElements[0]))

// add-palette (cycles when pressing ADD in the editor)
static const SDefaultElement g_PaletteExtras[] =
{
	{ "heal",      "HEAL",   "h",         0.560f, 0.660f, 0.036f, true,  true  },
	{ "save",      "QSAVE",  "f6",        0.060f, 0.090f, 0.026f, false, true  },
	{ "load",      "QLOAD",  "f9",        0.115f, 0.090f, 0.026f, false, true  },
	{ "screenshot","PRTSC",  "f12",       0.170f, 0.090f, 0.026f, false, true  },
};
#define PALETTE_EXTRA_COUNT (sizeof(g_PaletteExtras) / sizeof(g_PaletteExtras[0]))

//////////////////////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////////////////////
CTouchControls::CTouchControls()
{
	m_pGame = 0;
	m_pSystem = 0;
	m_pRenderer = 0;
	m_pConsole = 0;
	m_pFont = 0;
	m_nStickIdx = -1;
	m_nEditIdx = -1;
	m_nSelected = -1;
	m_nLastTouched = -1;
	m_nStickFinger = -1;
	m_fStickOffX = m_fStickOffY = 0;
	m_bStickKey[0] = m_bStickKey[1] = m_bStickKey[2] = m_bStickKey[3] = false;
	m_bEditMode = false;
	m_nEditFinger = -1;
	m_nEditDragMode = 0;
	m_fEditLastX = m_fEditLastY = 0;
	m_bPinchActive = false;
	m_fPinchStartDist = m_fPinchStartSize = 0;
	m_nPinchFingerA = m_nPinchFingerB = -1;
	m_bSnapGrid = true;
	m_nAddedCount = 0;
	m_bToolbarDirty = false;
	m_nTexWhite = -1;
	m_bTexOk = false;
	m_pCVarTouchEnabled = 0;
	m_pCVarOpacity = 0;
	m_pCVarScale = 0;
	m_pCVarEdit = 0;
	m_pCVarHaptics = 0;
	m_pCVarStickDynamic = 0;
	m_fScreenWidth = 1280;
	m_fScreenHeight = 720;
	m_dwLastFrameDrawn = 0;
}

CTouchControls::~CTouchControls()
{
	ShutDown();
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::ResolveElementKey(STouchElement &el)
{
	el.nXKey = NameToXKey(el.sKeyName.c_str());
}

//////////////////////////////////////////////////////////////////////////
bool CTouchControls::Init(CXGame *pGame)
{
	m_pGame = pGame;
	m_pSystem = pGame->GetSystem();
	m_pRenderer = m_pSystem->GetIRenderer();
	m_pConsole = m_pSystem->GetIConsole();

	g_pTouchControls = this;

	// cvars
	m_pCVarTouchEnabled = m_pConsole->CreateVariable("touch_enabled", "1", VF_DUMPTODISK,
			"Enable on-screen touch controls.\n"
			"Usage: touch_enabled [0/1]\n"
			"Default is 1 on touch devices");
	m_pCVarOpacity = m_pConsole->CreateVariable("touch_opacity", "0.35", VF_DUMPTODISK,
			"Opacity of the on-screen controls (0.0 - 1.0).\n"
			"Usage: touch_opacity [float]");
	m_pCVarScale = m_pConsole->CreateVariable("touch_scale", "1.0", VF_DUMPTODISK,
			"Global size multiplier of the on-screen controls.\n"
			"Usage: touch_scale [0.5 - 2.0]");
	m_pCVarEdit = m_pConsole->CreateVariable("touch_edit", "0", 0,
			"Layout editor mode. While 1, elements can be moved, resized,\n"
			"hidden and added. Touch the EDIT on-screen button to toggle.\n"
			"Usage: touch_edit [0/1]");
	m_pCVarHaptics = m_pConsole->CreateVariable("touch_vibrate", "1", VF_DUMPTODISK,
			"Vibrate shortly when a touch control is pressed.\n"
			"Usage: touch_vibrate [0/1]");
	m_pCVarStickDynamic = m_pConsole->CreateVariable("touch_stick_dynamic", "1", VF_DUMPTODISK,
			"Dynamic joystick: the stick moves under the finger anywhere\n"
			"in the lower left quadrant. 0 = fixed position stick.\n"
			"Usage: touch_stick_dynamic [0/1]");

	// create default elements
	CreateDefaultElements();

	// try to load saved layout
	m_sLayoutFile = TOUCH_LAYOUT_FILE;
	if (!LoadLayout(m_sLayoutFile.c_str()))
		SaveLayout(m_sLayoutFile.c_str()); // write defaults so users can hand-edit

	if (m_pSystem->GetICryFont())
	{
		m_pFont = m_pSystem->GetICryFont()->GetFont("default");
		if (!m_pFont)
			m_pFont = m_pSystem->GetICryFont()->GetFont("Default");
	}

	LogToConsole("Touch controls initialized (%d elements). Touch EDIT to customize the layout.", (int)m_Elements.size());

	// register with the input system
	IInput *pInput = m_pSystem->GetIInput();
	if (pInput)
		pInput->SetTouchOverlaySink(this);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::ShutDown()
{
	IInput *pInput = m_pSystem ? m_pSystem->GetIInput() : 0;
	if (pInput && pInput->GetTouchOverlaySink() == this)
		pInput->SetTouchOverlaySink(0);
	if (g_pTouchControls == this)
		g_pTouchControls = 0;
	m_Elements.clear();
}

//////////////////////////////////////////////////////////////////////////
STouchElement *CTouchControls::CreateDefaultElements()
{
	m_Elements.clear();

	// joystick first
	STouchElement stick;
	stick.sName = "stick";
	stick.sLabel = "";
	stick.sKeyName = "none";
	stick.fX = 0.120f;
	stick.fY = 0.760f;
	stick.fSize = 0.075f;
	stick.bRound = true;
	stick.bCanHide = true;
	stick.bTapOnly = false;
	m_Elements.push_back(stick);

	for (unsigned i = 0; i < DEFAULT_ELEMENT_COUNT; i++)
	{
		STouchElement el;
		el.sName = g_DefaultElements[i].name;
		el.sLabel = g_DefaultElements[i].label;
		el.sKeyName = g_DefaultElements[i].key;
		el.fX = g_DefaultElements[i].x;
		el.fY = g_DefaultElements[i].y;
		el.fSize = g_DefaultElements[i].size;
		el.bRound = g_DefaultElements[i].round;
		el.bTapOnly = g_DefaultElements[i].tapOnly;
		el.bCanHide = true;
		ResolveElementKey(el);
		m_Elements.push_back(el);
	}

	// the EDIT button: small, top right corner
	STouchElement edit;
	edit.sName = "edit";
	edit.sLabel = "EDIT";
	edit.sKeyName = "none";
	edit.fX = 0.965f;
	edit.fY = 0.048f;
	edit.fSize = 0.036f;
	edit.bRound = false;
	edit.bTapOnly = true;
	edit.bCanHide = false;
	ResolveElementKey(edit);
	m_Elements.push_back(edit);
	m_nEditIdx = (int)m_Elements.size() - 1;

	m_nStickIdx = 0;
	m_nSelected = -1;
	m_nLastTouched = -1;
	return &m_Elements[0];
}

//////////////////////////////////////////////////////////////////////////
int CTouchControls::FindElement(const char *szName)
{
	for (unsigned i = 0; i < m_Elements.size(); i++)
		if (m_Elements[i].sName == szName)
			return (int)i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////
// JSON mini-parser lives in TouchJson.h (shared with the unit tests)
//////////////////////////////////////////////////////////////////////////
#include "TouchJson.h"

//////////////////////////////////////////////////////////////////////////
// Layout load/save
//////////////////////////////////////////////////////////////////////////
bool CTouchControls::LoadLayout(const char *szFile)
{
	FILE *f = fopen(szFile, "rb");
	if (!f)
		return false;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size <= 0 || size > 256 * 1024)
	{
		fclose(f);
		return false;
	}
	char *buf = new char[size + 1];
	size_t rd = fread(buf, 1, size, f);
	buf[rd] = 0;
	fclose(f);

	TouchJson::Parser parser(buf);
	float fVersion = 0;

	// top level object
	if (!parser.Consume('{')) { delete[] buf; return false; }
	while (!parser.Peek('}') && parser.ok)
	{
		string key = parser.ParseString();
		if (!parser.Consume(':')) break;

		if (key == "version")
		{
			fVersion = parser.ParseNumber();
		}
		else if (key == "stick_dynamic")
		{
			bool b = true;
			parser.ParseBool(b);
			if (m_pCVarStickDynamic)
				m_pCVarStickDynamic->ForceSet(b ? "1" : "0");
		}
		else if (key == "opacity")
		{
			float v = parser.ParseNumber();
			char tmp[32];
			sprintf(tmp, "%g", v);
			if (m_pCVarOpacity)
				m_pCVarOpacity->ForceSet(tmp);
		}
		else if (key == "elements" && parser.Peek('['))
		{
			parser.Consume('[');
			while (!parser.Peek(']') && parser.ok)
			{
				// one element object
				if (!parser.Consume('{')) break;
				string ename, elabel, ekey;
				float ex = -1, ey = -1, esize = -1;
				bool evis = true, eround = true, etap = false;
				while (!parser.Peek('}') && parser.ok)
				{
					string ek = parser.ParseString();
					if (!parser.Consume(':')) break;
					if (ek == "name") ename = parser.ParseString();
					else if (ek == "label") elabel = parser.ParseString();
					else if (ek == "key") ekey = parser.ParseString();
					else if (ek == "x") ex = parser.ParseNumber();
					else if (ek == "y") ey = parser.ParseNumber();
					else if (ek == "size") esize = parser.ParseNumber();
					else if (ek == "visible") parser.ParseBool(evis);
					else if (ek == "round") parser.ParseBool(eround);
					else if (ek == "tap") parser.ParseBool(etap);
					else if (ek == "null") ; // skip unknown value
					if (parser.Peek(','))
						parser.Consume(',');
					else
						break;
				}
				parser.Consume('}');

				if (!ename.empty())
				{
					int idx = FindElement(ename.c_str());
					if (idx < 0 && ename != "stick" && ename != "edit")
					{
						// user-added element, create it
						STouchElement el;
						el.sName = ename;
						el.sLabel = elabel.empty() ? ename : elabel;
						el.sKeyName = ekey.empty() ? "none" : ekey;
						el.fX = ex < 0 ? 0.5f : ex;
						el.fY = ey < 0 ? 0.5f : ey;
						el.fSize = esize < 0 ? 0.04f : esize;
						el.bRound = eround;
						el.bTapOnly = etap;
						el.bVisible = evis;
						el.bCanHide = true;
						ResolveElementKey(el);
						m_Elements.push_back(el);
					}
					else if (idx >= 0)
					{
						STouchElement &el = m_Elements[idx];
						if (!elabel.empty()) el.sLabel = elabel;
						if (!ekey.empty()) el.sKeyName = ekey;
						if (ex >= 0) el.fX = ex;
						if (ey >= 0) el.fY = ey;
						if (esize >= 0) el.fSize = esize;
						el.bVisible = evis;
						el.bRound = eround;
						el.bTapOnly = etap;
						ResolveElementKey(el);
					}
				}

				if (parser.Peek(','))
					parser.Consume(',');
				else
					break;
			}
			parser.Consume(']');
		}
		else
		{
			// skip unknown value (string/number/bool/array/object)
			if (parser.Peek('"')) parser.ParseString();
			else if (parser.Peek('['))
			{
				int depth = 0;
				while (*parser.p)
				{
					if (*parser.p == '[') depth++;
					else if (*parser.p == ']') { depth--; if (!depth) { parser.p++; break; } }
					else if (*parser.p == '"') { parser.ParseString(); continue; }
					parser.p++;
				}
			}
			else if (parser.Peek('{'))
			{
				int depth = 0;
				while (*parser.p)
				{
					if (*parser.p == '{') depth++;
					else if (*parser.p == '}') { depth--; if (!depth) { parser.p++; break; } }
					else if (*parser.p == '"') { parser.ParseString(); continue; }
					parser.p++;
				}
			}
			else if (parser.Peek('t') || parser.Peek('f'))
			{
				bool b;
				parser.ParseBool(b);
			}
			else
				parser.ParseNumber();
		}

		if (parser.Peek(','))
			parser.Consume(',');
		else
			break;
	}
	parser.Consume('}');

	delete[] buf;
	return parser.ok && fVersion >= 1;
}

//////////////////////////////////////////////////////////////////////////
bool CTouchControls::SaveLayout(const char *szFile)
{
	FILE *f = fopen(szFile, "wb");
	if (!f)
		return false;

	char num[64];
	string out = "{\n\t\"version\": 1,\n";
	out += "\t\"stick_dynamic\": ";
	out += (m_pCVarStickDynamic && m_pCVarStickDynamic->GetIVal()) ? "true" : "false";
	out += ",\n\t\"opacity\": ";
	sprintf(num, "%g", m_pCVarOpacity ? m_pCVarOpacity->GetFVal() : 0.35f);
	out += num;
	out += ",\n\t\"elements\": [\n";

	for (unsigned i = 0; i < m_Elements.size(); i++)
	{
		const STouchElement &el = m_Elements[i];
		out += "\t\t{ \"name\": \"";
		out += TouchJson::Escape(el.sName);
		out += "\", \"label\": \"";
		out += TouchJson::Escape(el.sLabel);
		out += "\", \"key\": \"";
		out += el.sKeyName;
		out += "\", \"x\": ";
		sprintf(num, "%.4f", el.fX);
		out += num;
		out += ", \"y\": ";
		sprintf(num, "%.4f", el.fY);
		out += num;
		out += ", \"size\": ";
		sprintf(num, "%.4f", el.fSize);
		out += num;
		out += ", \"visible\": ";
		out += el.bVisible ? "true" : "false";
		out += ", \"round\": ";
		out += el.bRound ? "true" : "false";
		out += ", \"tap\": ";
		out += el.bTapOnly ? "true" : "false";
		out += " }";
		if (i + 1 < m_Elements.size())
			out += ",";
		out += "\n";
	}
	out += "\t]\n}\n";

	bool bOk = fwrite(out.c_str(), 1, out.size(), f) == out.size();
	fclose(f);
	return bOk;
}

//////////////////////////////////////////////////////////////////////////
// Console-driven configuration is done via the CVars (touch_enabled,
// touch_edit, ...). Layout is edited with the on-screen EDIT button.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Geometry helpers
//////////////////////////////////////////////////////////////////////////
static inline float clamp01(float v)
{
	if (v < 0.02f) v = 0.02f;
	if (v > 0.98f) v = 0.98f;
	return v;
}

float CTouchControls::SnapCoord(float v)
{
	if (!m_bSnapGrid)
		return v;
	const float step = 1.0f / 48.0f;
	return floorf(v / step + 0.5f) * step;
}

STouchElement *CTouchControls::HitTest(float fX, float fY, bool bIncludeHidden)
{
	// fX/fY in pixels; iterate topmost first (reverse draw order)
	for (int i = (int)m_Elements.size() - 1; i >= 0; i--)
	{
		STouchElement &el = m_Elements[i];
		if (!el.bVisible && !bIncludeHidden)
			continue;
		float px = el.fX * m_fScreenWidth;
		float py = el.fY * m_fScreenHeight;
		float rad = el.fSize * m_fScreenHeight * (m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f);
		if (rad < 8.0f) rad = 8.0f;
		float dx = fX - px, dy = fY - py;
		if (el.bRound)
		{
			if (dx * dx + dy * dy <= rad * rad)
				return &el;
		}
		else
		{
			if (fabsf(dx) <= rad && fabsf(dy) <= rad)
				return &el;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Key injection
//////////////////////////////////////////////////////////////////////////
void CTouchControls::SetElementPressed(STouchElement *pEl, bool bDown)
{
	if (!pEl || pEl->bPressed == bDown)
		return;
	pEl->bPressed = bDown;
	IInput *pInput = m_pSystem->GetIInput();
	if (pInput && pEl->nXKey != 0)
	{
		if (pEl->bTapOnly && bDown)
		{
			// tap-only buttons emit a short click on release, handled in OnTouchUp
			return;
		}
		pInput->PostVirtualKeyEvent(pEl->nXKey, bDown);
	}
}

//////////////////////////////////////////////////////////////////////////
// Touch routing
//////////////////////////////////////////////////////////////////////////
bool CTouchControls::OnTouchDown(int idFinger, float fX, float fY)
{
	m_fScreenWidth = (float)m_pRenderer->GetWidth();
	m_fScreenHeight = (float)m_pRenderer->GetHeight();

	// ---------- EDIT MODE: all fingers belong to the editor ----------
	if (m_bEditMode)
	{
		// toolbar?
		int nTool = HitTestToolbar(fX, fY);
		if (nTool >= 0)
		{
			switch (nTool)
			{
			case 0: SaveLayout(m_sLayoutFile.c_str()); m_bToolbarDirty = true; break; // SAVE
			case 1: AddElement(); break;																										// ADD
			case 2: DeleteElement(m_nLastTouched); break;											// DEL
			case 3: m_bSnapGrid = !m_bSnapGrid; break;											// GRID
			case 4: // TCH - master toggle for the whole touch overlay
				if (m_pCVarTouchEnabled)
				{
					if (m_pCVarTouchEnabled->GetIVal() != 0)
					{
						// disabling from the editor: keep the user's layout edits
						SaveLayout(m_sLayoutFile.c_str());
						m_pCVarTouchEnabled->Set(0);
					}
					else
					{
						m_pCVarTouchEnabled->Set(1);
					}
				}
				break;
#if defined(__ANDROID__)
			case 5: // LOG - share + save the diagnostics log (no PC needed)
				if (AndroidSendLogs)
					AndroidSendLogs();
				break;
			case 6: LeaveEditMode(true); return true;										// EXIT
#else
			case 5: LeaveEditMode(true); return true;												// EXIT
#endif
			}
			m_nEditFinger = -1;
			return true;
		}

		STouchElement *pEl = HitTest(fX, fY, true);
		if (pEl)
		{
			int idx = (int)(pEl - &m_Elements[0]);
			m_nSelected = idx;
			m_nLastTouched = idx;

			// resize grip: bottom-right quarter of the element
			float px = pEl->fX * m_fScreenWidth;
			float py = pEl->fY * m_fScreenHeight;
			float rad = pEl->fSize * m_fScreenHeight * (m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f);
			float grip = rad * 0.45f;
			if (!pEl->bVisible)
				grip = 0; // hidden elements: tap toggles visibility back
			bool bGrip = (fX > px + rad - grip * 1.6f) && (fY > py + rad - grip * 1.6f) &&
					(pEl->sName != "edit");

			if (!pEl->bVisible)
			{
				pEl->bVisible = true; // tap ghost -> restore
				SaveLayout(m_sLayoutFile.c_str());
				TouchVibrate(20);
			}
			else if (bGrip && !pEl->bTapOnly)
			{
				m_nEditDragMode = 2;
				m_fEditLastX = fX;
				m_fEditLastY = fY;
				m_nEditFinger = idFinger;
				TouchVibrate(15);
			}
			else
			{
				m_nEditDragMode = 1;
				m_fEditLastX = fX;
				m_fEditLastY = fY;
				m_nEditFinger = idFinger;
				TouchVibrate(15);
			}
		}
		else
		{
			m_nSelected = -1;
			m_nEditFinger = -1;
			m_nEditDragMode = 0;
		}

		// pinch setup
		if (m_nPinchFingerA < 0)
		{
			m_nPinchFingerA = idFinger;
			m_fPinchStartDist = 0;
		}
		else if (m_nPinchFingerB < 0 && idFinger != m_nPinchFingerA)
		{
			m_nPinchFingerB = idFinger;
			m_bPinchActive = false;
		}
		return true;
	}

	// ---------- GAME MODE ----------
	// EDIT button first (works even in menus)
	{
		STouchElement *pEl = HitTest(fX, fY, false);
		if (pEl && pEl->sName == "edit")
		{
			EnterEditMode();
			TouchVibrate(40);
			return true;
		}
	}

	// no overlay interaction in menus (SDL mouse emulation drives the UI)
	if (m_pGame && m_pGame->IsInMenu())
		return false;

	bool bClaimed = false;

	STouchElement *pEl = HitTest(fX, fY, false);
	if (pEl)
	{
		if (pEl->sName == "stick")
		{
			m_nStickFinger = idFinger;
			if (m_pCVarStickDynamic && m_pCVarStickDynamic->GetIVal())
			{
				// dynamic stick re-centers under the finger
				float half = pEl->fSize * m_fScreenHeight * (m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f);
				float maxOff = half * 2.5f;
				if (fX < m_fScreenWidth * 0.55f && fY > m_fScreenHeight * 0.35f)
				{
					pEl->fX = clamp01(fX / m_fScreenWidth);
					pEl->fY = clamp01(fY / m_fScreenHeight);
					(void)maxOff;
				}
			}
			m_fStickOffX = 0;
			m_fStickOffY = 0;
			bClaimed = true;
			TouchVibrate(12);
		}
		else
		{
			pEl->nFinger = idFinger;
			SetElementPressed(pEl, true);
			bClaimed = true;
			if (m_pCVarHaptics && m_pCVarHaptics->GetIVal())
				TouchVibrate(10);
		}
	}

	return bClaimed;
}

//////////////////////////////////////////////////////////////////////////
bool CTouchControls::OnTouchMove(int idFinger, float fX, float fY, float fDX, float fDY)
{
	m_fScreenWidth = (float)m_pRenderer->GetWidth();
	m_fScreenHeight = (float)m_pRenderer->GetHeight();

	if (m_bEditMode)
	{
		// pinch resizing
		if (idFinger == m_nPinchFingerA || idFinger == m_nPinchFingerB)
		{
			if (m_nPinchFingerA >= 0 && m_nPinchFingerB >= 0)
			{
				// we only know one finger's position here; approximate using
				// movement magnitude: pinch = both fingers active & dragging
				if (!m_bPinchActive && m_nSelected >= 0)
				{
					STouchElement &el = m_Elements[m_nSelected];
					m_fPinchStartSize = el.fSize;
					m_fPinchStartDist = 0;
				}
			}
		}

		if (idFinger == m_nEditFinger && m_nEditDragMode != 0 && m_nSelected >= 0)
		{
			STouchElement &el = m_Elements[m_nSelected];
			float scale = m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f;
			if (m_nEditDragMode == 1)
			{
				el.fX = clamp01(SnapCoord(el.fX + fDX / m_fScreenWidth));
				el.fY = clamp01(SnapCoord(el.fY + fDY / m_fScreenHeight));
			}
			else // resize
			{
				float d = (fDX + fDY) * 0.5f / m_fScreenHeight;
				el.fSize += d / scale;
				float minSize = 0.018f;
				float maxSize = 0.22f;
				if (el.fSize < minSize) el.fSize = minSize;
				if (el.fSize > maxSize) el.fSize = maxSize;
			}
			m_fEditLastX = fX;
			m_fEditLastY = fY;
			return true;
		}
		return true; // all fingers consumed in edit mode
	}

	if (m_pGame && m_pGame->IsInMenu())
		return false;

	// joystick drag
	if (idFinger == m_nStickFinger && m_nStickIdx >= 0)
	{
		STouchElement &stick = m_Elements[m_nStickIdx];
		float px = stick.fX * m_fScreenWidth;
		float py = stick.fY * m_fScreenHeight;
		float rad = stick.fSize * m_fScreenHeight * (m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f);
		float dx = fX - px, dy = fY - py;
		float len = sqrtf(dx * dx + dy * dy);
		float maxLen = rad * 0.9f;
		if (len > maxLen && len > 0.001f)
		{
			dx = dx / len * maxLen;
			dy = dy / len * maxLen;
		}
		m_fStickOffX = dx;
		m_fStickOffY = dy;
		return true;
	}

	// button hold (finger may slide slightly off the button, keep it pressed)
	for (unsigned i = 0; i < m_Elements.size(); i++)
	{
		if (m_Elements[i].nFinger == idFinger)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CTouchControls::OnTouchUp(int idFinger, float fX, float fY)
{
	if (m_bEditMode)
	{
		if (idFinger == m_nEditFinger)
		{
			m_nEditFinger = -1;
			m_nEditDragMode = 0;
		}
		if (idFinger == m_nPinchFingerA) m_nPinchFingerA = -1;
		if (idFinger == m_nPinchFingerB) m_nPinchFingerB = -1;
		return true;
	}

	if (idFinger == m_nStickFinger)
	{
		m_nStickFinger = -1;
		m_fStickOffX = m_fStickOffY = 0;
	}

	for (unsigned i = 0; i < m_Elements.size(); i++)
	{
		STouchElement &el = m_Elements[i];
		if (el.nFinger == idFinger)
		{
			el.nFinger = -1;
			if (el.bTapOnly && el.sName != "edit")
			{
				// tap: short press+release burst
				SetElementPressed(&el, true);
				SetElementPressed(&el, false);
			}
			else
				SetElementPressed(&el, false);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// Per-frame: synthesize movement keys from the stick position
//////////////////////////////////////////////////////////////////////////
void CTouchControls::OnTouchUpdate()
{
	if (!m_pSystem)
		return;
	IInput *pInput = m_pSystem->GetIInput();
	if (!pInput)
		return;

	// release everything when the overlay gets disabled globally
	if (!IsTouchEnabled() && !m_bEditMode)
	{
		for (unsigned i = 0; i < m_Elements.size(); i++)
		{
			SetElementPressed(&m_Elements[i], false);
			m_Elements[i].nFinger = -1;
		}
		m_nStickFinger = -1;
		m_fStickOffX = m_fStickOffY = 0;
	}

	// stick keys: W/A/S/D by offset direction
	const float dead = 0.30f; // fraction of knob travel
	bool bKey[4] = { false, false, false, false };
	if (m_nStickFinger >= 0 && m_nStickIdx >= 0 && !m_bEditMode && !(m_pGame && m_pGame->IsInMenu()))
	{
		STouchElement &stick = m_Elements[m_nStickIdx];
		float rad = stick.fSize * m_fScreenHeight * (m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f);
		if (rad < 1.0f) rad = 1.0f;
		float nx = m_fStickOffX / (rad * 0.9f);
		float ny = m_fStickOffY / (rad * 0.9f);
		if (ny < -dead) bKey[0] = true; // W
		if (ny > dead) bKey[2] = true;	// S
		if (nx < -dead) bKey[1] = true; // A
		if (nx > dead) bKey[3] = true;	// D
	}

	static const int nStickKeys[4] = { XKEY_W, XKEY_A, XKEY_S, XKEY_D };
	for (int i = 0; i < 4; i++)
	{
		if (bKey[i] != m_bStickKey[i])
		{
			m_bStickKey[i] = bKey[i];
			pInput->PostVirtualKeyEvent(nStickKeys[i], bKey[i]);
		}
	}

	// keep cvar and editor state in sync
	if (m_pCVarEdit)
	{
		bool bCvarEdit = m_pCVarEdit->GetIVal() != 0;
		if (bCvarEdit != m_bEditMode)
		{
			if (bCvarEdit) EnterEditMode();
			else LeaveEditMode(true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTouchControls::IsUiActive() const
{
	// menu / pause overlay owns the screen: unclaimed taps become UI cursor
	return m_pGame ? m_pGame->IsInMenu() : false;
}

bool CTouchControls::IsTouchEnabled() const
{
	if (!m_pCVarTouchEnabled)
		return false;
	return m_pCVarTouchEnabled->GetIVal() != 0;
}

//////////////////////////////////////////////////////////////////////////
// Tap on the tiny "TOUCH OFF" recovery button (drawn top-right while the
// overlay is globally disabled) re-enables touch controls.
bool CTouchControls::OnDisabledTap(float fX, float fY)
{
	if (!m_pRenderer)
		return false;
	m_fScreenWidth = (float)m_pRenderer->GetWidth();
	m_fScreenHeight = (float)m_pRenderer->GetHeight();

	float bw = m_fScreenWidth * 0.05f, bh = m_fScreenHeight * 0.035f;
	float bx = m_fScreenWidth - bw - m_fScreenHeight * 0.01f;
	float by = m_fScreenHeight * 0.01f;
	// finger-friendly margin around the drawn rect
	float m = bh * 0.75f;
	if (fX < bx - m || fX > bx + bw + m || fY < by - m || fY > by + bh + m)
		return false;

	if (m_pCVarTouchEnabled)
		m_pCVarTouchEnabled->Set(1);
	TouchVibrate(30);
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::EnterEditMode()
{
	if (m_bEditMode)
		return;
	m_bEditMode = true;
	m_nSelected = -1;
	m_nEditFinger = -1;
	m_nEditDragMode = 0;
	m_nPinchFingerA = m_nPinchFingerB = -1;
	m_bToolbarDirty = true;
	if (m_pCVarEdit)
		m_pCVarEdit->ForceSet("1");
	// release all gameplay keys
	for (unsigned i = 0; i < m_Elements.size(); i++)
		SetElementPressed(&m_Elements[i], false);
	for (int i = 0; i < 4; i++)
	{
		static const int nStickKeys[4] = { XKEY_W, XKEY_A, XKEY_S, XKEY_D };
		if (m_bStickKey[i]) { m_bStickKey[i] = false; IInput *pInput = m_pSystem->GetIInput(); if (pInput) pInput->PostVirtualKeyEvent(nStickKeys[i], false); }
	}
	if (m_pConsole)
		LogToConsole("TOUCH EDITOR: drag = move, corner grip = resize, x-badge = hide, toolbar = save/add/del/grid/exit");
}

void CTouchControls::LeaveEditMode(bool bSave)
{
	if (!m_bEditMode)
		return;
	m_bEditMode = false;
	m_nEditFinger = -1;
	m_nEditDragMode = 0;
	if (m_pCVarEdit)
		m_pCVarEdit->ForceSet("0");
	if (bSave)
	{
		if (SaveLayout(m_sLayoutFile.c_str()))
			LogToConsole("Touch layout saved to %s", m_sLayoutFile.c_str());
		else
			LogToConsole("Touch layout SAVE FAILED (%s)", m_sLayoutFile.c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::AddElement()
{
	// add the next palette element that does not exist yet
	for (unsigned i = 0; i < PALETTE_EXTRA_COUNT; i++)
	{
		const SDefaultElement &def = g_PaletteExtras[i];
		if (FindElement(def.name) < 0)
		{
			STouchElement el;
			el.sName = def.name;
			el.sLabel = def.label;
			el.sKeyName = def.key;
			el.fX = def.x;
			el.fY = def.y;
			el.fSize = def.size;
			el.bRound = def.round;
			el.bTapOnly = def.tapOnly;
			el.bCanHide = true;
			ResolveElementKey(el);
			m_Elements.push_back(el);
			m_nSelected = (int)m_Elements.size() - 1;
			m_nLastTouched = m_nSelected;
			m_bToolbarDirty = true;
			return;
		}
	}
	// palette exhausted: duplicate last touched as a custom button
	if (m_nLastTouched >= 0 && m_nLastTouched < (int)m_Elements.size())
	{
		STouchElement el = m_Elements[m_nLastTouched];
		char tmp[32];
		static int nCopy = 0;
		sprintf(tmp, "btn%d", ++nCopy);
		el.sName = tmp;
		el.sKeyName = "none";
		el.nXKey = 0;
		el.fX = clamp01(el.fX + 0.06f);
		m_Elements.push_back(el);
		m_nSelected = (int)m_Elements.size() - 1;
		m_nLastTouched = m_nSelected;
		m_bToolbarDirty = true;
	}
}

void CTouchControls::DeleteElement(int idx)
{
	if (idx < 0 || idx >= (int)m_Elements.size())
		return;
	STouchElement &el = m_Elements[idx];
	if (el.sName == "edit" || el.sName == "stick")
		return; // system elements cannot be deleted
	SetElementPressed(&el, false);
	m_Elements.erase(m_Elements.begin() + idx);
	if (m_nStickIdx >= idx) m_nStickIdx--;
	if (m_nEditIdx >= idx) m_nEditIdx--;
	m_nSelected = -1;
	m_nLastTouched = -1;
	m_bToolbarDirty = true;
	SaveLayout(m_sLayoutFile.c_str());
}

//////////////////////////////////////////////////////////////////////////
// Drawing
// Draw2dLine uses the current material color, so every helper sets it.
//////////////////////////////////////////////////////////////////////////
void CTouchControls::DrawFillRect(float x, float y, float w, float h, float r, float g, float b, float a)
{
	m_pRenderer->SetMaterialColor(r, g, b, a);
	const float step = 2.0f;
	for (float yy = y + step * 0.5f; yy < y + h; yy += step)
		m_pRenderer->Draw2dLine(x, yy, x + w, yy);
}

void CTouchControls::DrawFrameRect(float x, float y, float w, float h, float r, float g, float b, float a)
{
	m_pRenderer->SetMaterialColor(r, g, b, a);
	m_pRenderer->Draw2dLine(x, y, x + w, y);
	m_pRenderer->Draw2dLine(x + w, y, x + w, y + h);
	m_pRenderer->Draw2dLine(x + w, y + h, x, y + h);
	m_pRenderer->Draw2dLine(x, y + h, x, y);
}

void CTouchControls::DrawFillCircle(float cx, float cy, float radius, float r, float g, float b, float a)
{
	m_pRenderer->SetMaterialColor(r, g, b, a);
	const float step = 2.5f;
	for (float yy = -radius + step * 0.5f; yy < radius; yy += step)
	{
		float half = sqrtf(radius * radius - yy * yy);
		if (half < 1.0f) continue;
		m_pRenderer->Draw2dLine(cx - half, cy + yy, cx + half, cy + yy);
	}
}

void CTouchControls::DrawFrameCircle(float cx, float cy, float radius, float r, float g, float b, float a)
{
	m_pRenderer->SetMaterialColor(r, g, b, a);
	const int seg = 24;
	float px = cx + radius, py = cy;
	for (int i = 1; i <= seg; i++)
	{
		float ang = (float)i / seg * 2.0f * 3.14159265f;
		float nx = cx + cosf(ang) * radius;
		float ny = cy + sinf(ang) * radius;
		m_pRenderer->Draw2dLine(px, py, nx, ny);
		px = nx;
		py = ny;
	}
}

void CTouchControls::DrawText(float cx, float cy, const char *szText, float fScale, float r, float g, float b, float a)
{
	if (!szText || !szText[0])
		return;
	if (m_pFont)
	{
		// font sizes are normalized to the screen height (1.0 = full screen)
		float hs = 0.024f * fScale;
		vector2f size(hs, hs);
		color4f col(r, g, b, a);
		m_pFont->Reset();
		m_pFont->SetSize(size);
		m_pFont->SetColor(col);
		// rough width estimate for centering (font has no text metrics here)
		float w = (float)strlen(szText) * m_fScreenHeight * hs * 0.42f;
		m_pFont->DrawString(cx - w * 0.5f, cy - m_fScreenHeight * hs * 0.5f, szText);
	}
	else
	{
		float fColor[4] = { r, g, b, a };
		m_pRenderer->Draw2dLabel(cx, cy, 1.1f * fScale, fColor, true, "%s", szText);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::LogToConsole(const char *szFormat, ...)
{
	if (!m_pSystem || !m_pSystem->GetILog())
		return;
	char buffer[1024];
	va_list args;
	va_start(args, szFormat);
	vsnprintf(buffer, sizeof(buffer), szFormat, args);
	va_end(args);
	m_pSystem->GetILog()->LogToConsole("%s", buffer);
}

//////////////////////////////////////////////////////////////////////////
int CTouchControls::HitTestToolbar(float fX, float fY)
{
	// toolbar: SAVE ADD DEL GRID TCH [LOG] EXIT - top center row
#if defined(__ANDROID__)
	int count = 7;
#else
	int count = 6;
#endif
	float bh = m_fScreenHeight * 0.045f;
	float bw = m_fScreenWidth * 0.085f;
	float gap = m_fScreenWidth * 0.012f;
	float total = count * bw + (count - 1) * gap;
	float x0 = (m_fScreenWidth - total) * 0.5f;
	float y0 = m_fScreenHeight * 0.012f;
	if (fY < y0 || fY > y0 + bh)
		return -1;
	for (int i = 0; i < count; i++)
	{
		float bx = x0 + i * (bw + gap);
		if (fX >= bx && fX <= bx + bw)
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////
void CTouchControls::OnTouchRender()
{
	if (!m_pRenderer)
		return;

	m_fScreenWidth = (float)m_pRenderer->GetWidth();
	m_fScreenHeight = (float)m_pRenderer->GetHeight();
	if (m_fScreenWidth < 16 || m_fScreenHeight < 16)
		return;

	bool bEnabled = IsTouchEnabled();
	bool bInMenu = m_pGame ? m_pGame->IsInMenu() : false;
	float fOpacity = m_pCVarOpacity ? m_pCVarOpacity->GetFVal() : 0.35f;
	if (fOpacity < 0.02f) fOpacity = 0.02f;
	if (fOpacity > 1.0f) fOpacity = 1.0f;
	float scale = m_pCVarScale ? m_pCVarScale->GetFVal() : 1.0f;

	float fColor[4];

	// master switch OFF: show only a tiny recovery button
	if (!bEnabled)
	{
		if (m_bEditMode)
			LeaveEditMode(false);
		float bw = m_fScreenWidth * 0.05f, bh = m_fScreenHeight * 0.035f;
		float bx = m_fScreenWidth - bw - m_fScreenHeight * 0.01f;
		float by = m_fScreenHeight * 0.01f;
		m_pRenderer->SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
		DrawFrameRect(bx, by, bw, bh, 0.3f, 0.9f, 0.3f, 0.7f);
		fColor[0] = 0.4f; fColor[1] = 1.0f; fColor[2] = 0.4f; fColor[3] = 0.9f;
		m_pRenderer->Draw2dLabel(bx + bw * 0.5f, by + bh * 0.5f, 1.0f, fColor, true, "TOUCH OFF");
		return;
	}

	m_pRenderer->SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

	if (m_bEditMode)
	{
		// dim the screen slightly
		for (float yy = 0; yy < m_fScreenHeight; yy += 4.0f)
			m_pRenderer->Draw2dLine(0, yy, m_fScreenWidth, yy);

		// toolbar
#if defined(__ANDROID__)
		static const char *labels[7] = { "SAVE", "ADD", "DEL", "GRID", "TCH", "LOG", "EXIT" };
		const int nTB = 7;
#else
		static const char *labels[6] = { "SAVE", "ADD", "DEL", "GRID", "TCH", "EXIT" };
		const int nTB = 6;
#endif
		float bh = m_fScreenHeight * 0.045f;
		float bw = m_fScreenWidth * 0.085f;
		float gap = m_fScreenWidth * 0.012f;
		float total = nTB * bw + (nTB - 1) * gap;
		float x0 = (m_fScreenWidth - total) * 0.5f;
		float y0 = m_fScreenHeight * 0.012f;
		for (int i = 0; i < nTB; i++)
		{
			float bx = x0 + i * (bw + gap);
			DrawFillRect(bx, y0, bw, bh, 0.1f, 0.1f, 0.2f, 0.35f);
			// TCH reflects the current master-switch state (green=on, red=off)
			if (i == 4)
			{
				bool on = IsTouchEnabled();
				DrawFrameRect(bx, y0, bw, bh, on ? 0.3f : 1.0f, on ? 1.0f : 0.35f, on ? 0.3f : 0.35f, 0.9f);
			}
			else
			{
				DrawFrameRect(bx, y0, bw, bh, 0.5f, 0.8f, 1.0f, 0.9f);
			}
			DrawText(bx + bw * 0.5f, y0 + bh * 0.55f, labels[i], 0.8f, 0.9f, 0.95f, 1.0f, 1.0f);
		}

		// help text
		float hy = y0 + bh + m_fScreenHeight * 0.02f;
		DrawText(m_fScreenWidth * 0.5f, hy, m_bSnapGrid ? "EDIT - grid ON - drag=move grip=resize x=hide TCH=on/off" : "EDIT - drag=move grip=resize x=hide TCH=on/off", 0.75f, 1.0f, 1.0f, 0.6f, 0.9f);

		// elements (including hidden, ghosted)
		for (unsigned i = 0; i < m_Elements.size(); i++)
		{
			STouchElement &el = m_Elements[i];
			float px = el.fX * m_fScreenWidth;
			float py = el.fY * m_fScreenHeight;
			float rad = el.fSize * m_fScreenHeight * scale;
			if (rad < 8.0f) rad = 8.0f;
			bool bSel = ((int)i == m_nSelected);

			float alpha = el.bVisible ? fOpacity : fOpacity * 0.25f;
			if (el.bRound)
			{
				DrawFillCircle(px, py, rad, 0.2f, 0.4f, 0.8f, alpha * 0.8f);
				DrawFrameCircle(px, py, rad, bSel ? 1.0f : 0.4f, bSel ? 1.0f : 0.7f, 1.0f, bSel ? 1.0f : 0.8f);
			}
			else
			{
				DrawFillRect(px - rad, py - rad, rad * 2, rad * 2, 0.2f, 0.4f, 0.8f, alpha * 0.8f);
				DrawFrameRect(px - rad, py - rad, rad * 2, rad * 2, bSel ? 1.0f : 0.4f, bSel ? 1.0f : 0.7f, 1.0f, bSel ? 1.0f : 0.8f);
			}

			// label + key name
			string label = el.sLabel;
			if (el.sName != "edit" && el.sName != "stick")
			{
				label += el.bTapOnly ? " [t]" : "";
				DrawText(px, py, label.c_str(), 0.62f, 1, 1, 1, 1);
				string key = el.sKeyName;
				DrawText(px, py + rad * 0.55f, key.c_str(), 0.45f, 0.7f, 0.8f, 1.0f, 0.85f);
			}
			else if (el.sName == "stick")
			{
				DrawText(px, py, "STICK", 0.55f, 1, 1, 1, 0.9f);
			}
			else
				DrawText(px, py, "EDIT", 0.6f, 1, 1, 1, 1);

			// hide badge (top-right)
			if (el.bCanHide && el.bVisible && el.sName != "edit")
			{
				float br = rad * 0.28f;
				float bx = px + rad - br * 0.4f;
				float by = py - rad - br * 0.4f;
				DrawFrameCircle(bx, by, br, 1.0f, 0.4f, 0.4f, 0.95f);
				DrawText(bx, by, "x", 0.5f, 1.0f, 0.5f, 0.5f, 1.0f);
			}

			// resize grip (bottom-right)
			if (el.bVisible && !el.bTapOnly && el.sName != "edit")
			{
				float gr = rad * 0.24f;
				float gx = px + rad - gr;
				float gy = py + rad - gr;
				DrawFrameRect(gx, gy, gr * 2, gr * 2, 1.0f, 0.8f, 0.2f, 0.95f);
				DrawText(gx + gr, gy + gr, "=", 0.5f, 1.0f, 0.9f, 0.4f, 1.0f);
			}
		}

		// stick knob preview
		if (m_nStickIdx >= 0)
		{
			STouchElement &stick = m_Elements[m_nStickIdx];
			float px = stick.fX * m_fScreenWidth;
			float py = stick.fY * m_fScreenHeight;
			float rad = stick.fSize * m_fScreenHeight * scale;
			DrawFillCircle(px + m_fStickOffX, py + m_fStickOffY, rad * 0.42f, 0.8f, 0.9f, 1.0f, fOpacity + 0.1f);
		}
		return;
	}

	// ---------- normal in-game overlay ----------
	if (bInMenu)
	{
		// in menus only the EDIT button stays (small, dim)
		if (m_nEditIdx >= 0)
		{
			STouchElement &el = m_Elements[m_nEditIdx];
			float px = el.fX * m_fScreenWidth;
			float py = el.fY * m_fScreenHeight;
			float rad = el.fSize * m_fScreenHeight * scale;
			DrawFrameRect(px - rad, py - rad, rad * 2, rad * 2, 0.6f, 0.8f, 1.0f, 0.5f);
			DrawText(px, py, "EDIT", 0.5f, 1, 1, 1, 0.8f);
		}
		return;
	}

	bool bStickActive = (m_nStickFinger >= 0);

	for (unsigned i = 0; i < m_Elements.size(); i++)
	{
		STouchElement &el = m_Elements[i];
		if (!el.bVisible)
			continue;
		float px = el.fX * m_fScreenWidth;
		float py = el.fY * m_fScreenHeight;
		float rad = el.fSize * m_fScreenHeight * scale;
		if (rad < 4.0f) rad = 4.0f;

		bool bHeld = el.bPressed || el.nFinger >= 0;

		if (el.sName == "stick")
		{
			// outer ring + inner knob
			float a = fOpacity * (bStickActive ? 1.6f : 1.0f);
			if (a > 1.0f) a = 1.0f;
			DrawFrameCircle(px, py, rad, 0.8f, 0.9f, 1.0f, a);
			DrawFillCircle(px, py, rad, 0.5f, 0.6f, 0.8f, a * 0.25f);
			float knx = px + m_fStickOffX, kny = py + m_fStickOffY;
			DrawFillCircle(knx, kny, rad * 0.42f, 0.85f, 0.92f, 1.0f, a * 0.8f + 0.1f);
			DrawFrameCircle(knx, kny, rad * 0.42f, 1.0f, 1.0f, 1.0f, a);
		}
		else if (el.sName == "edit")
		{
			DrawFillRect(px - rad, py - rad, rad * 2, rad * 2, 0.15f, 0.2f, 0.35f, fOpacity * 0.6f);
			DrawFrameRect(px - rad, py - rad, rad * 2, rad * 2, 0.55f, 0.75f, 1.0f, 0.7f);
			DrawText(px, py, el.sLabel.c_str(), 0.55f, 1, 1, 1, 0.9f);
		}
		else
		{
			float a = fOpacity;
			if (bHeld) a = 1.0f;
			if (el.bRound)
			{
				DrawFillCircle(px, py, rad, 0.9f, 0.95f, 1.0f, bHeld ? 0.5f : a * 0.35f);
				DrawFrameCircle(px, py, rad, 0.85f, 0.92f, 1.0f, bHeld ? 1.0f : a + 0.25f);
			}
			else
			{
				DrawFillRect(px - rad, py - rad, rad * 2, rad * 2, 0.9f, 0.95f, 1.0f, bHeld ? 0.5f : a * 0.35f);
				DrawFrameRect(px - rad, py - rad, rad * 2, rad * 2, 0.85f, 0.92f, 1.0f, bHeld ? 1.0f : a + 0.25f);
			}
			DrawText(px, py, el.sLabel.c_str(), 0.55f, 1, 1, 1, bHeld ? 1.0f : 0.85f);
		}
	}
}

#endif // !defined(_XBOX) && !defined(PS2)
