//////////////////////////////////////////////////////////////////////
//
//  Crytek CryENGINE Source code
//
//  File: TouchControls.h
//  Description: On-screen touch controls (virtual joystick, buttons)
//               with a built-in layout editor for mobile ports.
//
//////////////////////////////////////////////////////////////////////

#ifndef _TOUCHCONTROLS_H_
#define _TOUCHCONTROLS_H_

#if !defined(_XBOX) && !defined(PS2)

#include <IInput.h>
#include <vector>
#include <string>

class CXGame;
struct ISystem;
struct IRenderer;
struct IConsole;
struct IFFont;
struct ICVar;

//////////////////////////////////////////////////////////////////////////
// One on-screen control element.
// Positions are stored normalized (x: 0..1 of screen width, y: 0..1 of
// screen height, size: 0..1 of screen height) so the layout survives
// resolution changes.
//////////////////////////////////////////////////////////////////////////
struct STouchElement
{
	string	sName;								// stable identifier, e.g. "fire"
	string	sLabel;								// text shown inside the button
	string	sKeyName;							// XKey name, e.g. "mouse1" ("none" = tap only / no key)
	int			nXKey;								// resolved XKEY_* code
	float		fX, fY;								// center, normalized
	float		fSize;								// half-extent as fraction of screen height
	bool		bVisible;							// user visibility switch
	bool		bRound;								// circle (true) or rounded square (false)
	bool		bTapOnly;							// click on touch-up instead of hold
	bool		bCanHide;							// false for system elements (edit button)
	int			nFinger;							// finger currently holding it (-1)
	bool		bPressed;							// virtual key currently down

	STouchElement() { nXKey = 0; fX = 0.5f; fY = 0.5f; fSize = 0.05f; bVisible = true; bRound = true; bTapOnly = false; bCanHide = true; nFinger = -1; bPressed = false; }
};

//////////////////////////////////////////////////////////////////////////
class CTouchControls : public ITouchOverlaySink
{
public:
	CTouchControls();
	~CTouchControls();

	bool Init(CXGame *pGame);
	void ShutDown();

	// ITouchOverlaySink
	virtual bool OnTouchDown(int idFinger, float fX, float fY);
	virtual bool OnTouchMove(int idFinger, float fX, float fY, float fDX, float fDY);
	virtual bool OnTouchUp(int idFinger, float fX, float fY);
	virtual void OnTouchUpdate();
	virtual void OnTouchRender();
	virtual bool IsTouchEditMode() const { return m_bEditMode; }
	virtual bool IsTouchEnabled() const;

	bool LoadLayout(const char *szFile);
	bool SaveLayout(const char *szFile);

private:
	// drawing helpers
	void DrawFillRect(float x, float y, float w, float h, float r, float g, float b, float a);
	void DrawFrameRect(float x, float y, float w, float h, float r, float g, float b, float a);
	void DrawFillCircle(float cx, float cy, float radius, float r, float g, float b, float a);
	void DrawFrameCircle(float cx, float cy, float radius, float r, float g, float b, float a);
	void DrawText(float cx, float cy, const char *szText, float fScale, float r, float g, float b, float a);

	// interaction helpers
	STouchElement *HitTest(float fX, float fY, bool bIncludeHidden);
	int HitTestToolbar(float fX, float fY);
	void SetElementPressed(STouchElement *pEl, bool bDown);
	void EnterEditMode();
	void LeaveEditMode(bool bSave);
	void AddElement();
	void DeleteElement(int idx);
	float SnapCoord(float v);

	// element key mapping
	void ResolveElementKey(STouchElement &el);
	static const char *XKeyToName(int nXKey);
	static int NameToXKey(const char *szName);

	// internal element creation
	STouchElement *CreateDefaultElements();
	int FindElement(const char *szName);

	CXGame *m_pGame;
	ISystem *m_pSystem;
	IRenderer *m_pRenderer;
	IConsole *m_pConsole;
	IFFont *m_pFont;

	typedef std::vector<STouchElement> TVecElements;
	TVecElements m_Elements;
	int m_nStickIdx;			// index of joystick element
	int m_nEditIdx;				// index of the edit button
	int m_nSelected;			// selected element in edit mode
	int m_nLastTouched;		// last touched element (for DEL)

	// joystick state
	int m_nStickFinger;
	float m_fStickOffX, m_fStickOffY;		// knob offset (pixels)
	bool m_bStickKey[4];										// W A S D currently injected

	// edit mode
	bool m_bEditMode;
	int m_nEditFinger;										// finger used for dragging
	int m_nEditDragMode;									// 0 none, 1 move, 2 resize
	float m_fEditLastX, m_fEditLastY;
	bool m_bPinchActive;
	float m_fPinchStartDist, m_fPinchStartSize;
	int m_nPinchFingerA, m_nPinchFingerB;
	bool m_bSnapGrid;
	int m_nAddedCount;										// palette cycling for ADD
	bool m_bToolbarDirty;

	// textures
	int m_nTexWhite;											// white texture id (-1 if unavailable)
	bool m_bTexOk;

	// cvars
	ICVar *m_pCVarTouchEnabled;
	ICVar *m_pCVarOpacity;
	ICVar *m_pCVarScale;
	ICVar *m_pCVarEdit;
	ICVar *m_pCVarHaptics;
	ICVar *m_pCVarStickDynamic;

	// helper
	void LogToConsole(const char *szFormat, ...);

	string m_sLayoutFile;
	float m_fScreenWidth, m_fScreenHeight;
	unsigned int m_dwLastFrameDrawn;
};

//! Global instance accessor (created by CXGame, used by console commands)
CTouchControls *GetTouchControls();

//! Destroys the global instance (called by CXGame destructor)
void ShutDownTouchControls();

//! Haptic feedback helper (implemented per-platform, no-op where unsupported)
void TouchVibrate(int ms);

#endif // !defined(_XBOX) && !defined(PS2)

#endif // _TOUCHCONTROLS_H_
