// Touch input device for SDL3 (Android / touch-screens).
// Routes raw finger events to the game-side touch overlay (virtual
// joystick / buttons / edit mode) and turns unclaimed drag into
// mouse-look deltas, exactly like a relative mouse would.
#ifndef SDL_TOUCH
#define SDL_TOUCH

#include <IInput.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL3/SDL.h>
#endif

struct ICVar;

#define TOUCH_MAX_FINGERS 16

class CInput;
class CSDLMouse;

class CSDLTouch
{
public:
	CSDLTouch();
	~CSDLTouch();

	bool Init(CInput *pInput, ISystem *pSystem, SDL_Window *window);
	void ShutDown();
	void Update(bool bFocus);
	void ClearKeyState();

	// pending look delta gathered from unclaimed finger drags (pixels)
	void AddLookDelta(float dx, float dy);
	float ConsumeLookDeltaX();
	float ConsumeLookDeltaY();

private:
	struct SFinger
	{
		SDL_FingerID m_id;
		bool m_bDown;
		bool m_bClaimed; // consumed by the overlay sink
		float m_fX, m_fY;
	};
	SFinger m_fingers[TOUCH_MAX_FINGERS];

	CInput *m_pInput;
	ISystem *m_pSystem;
	ILog *m_pLog;
	SDL_Window *m_pWindow;
	int m_nWinWidth;
	int m_nWinHeight;
	float m_fLookDX, m_fLookDY; // accumulated, not yet consumed
	ICVar *m_pTouchLookSens;
	ICVar *m_pTouchInvertY;
	ICVar *m_pTouchInvertX;

	ITouchOverlaySink *GetSink();
	int FindFinger(SDL_FingerID id);
	int AllocFinger(SDL_FingerID id);
	void RemoveFinger(int slot);
};

#endif // SDL_TOUCH
