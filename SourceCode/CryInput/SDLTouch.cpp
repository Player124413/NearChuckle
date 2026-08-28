//////////////////////////////////////////////////////////////////////
//
//  Touch Input Device (SDL3) - Android / touch-screen support.
//
//  Raw finger events are routed to the game-side touch overlay sink
//  first (virtual joystick, on-screen buttons, layout editor). Fingers
//  the overlay does not claim become mouse-look deltas.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SDLTouch.h"
#include "Input.h"
#include "SDLMouse.h"

#include <ILog.h>
#include <ISystem.h>
#include <IConsole.h>
#include <IGame.h>

CSDLTouch::CSDLTouch()
{
	m_pInput = 0;
	m_pSystem = 0;
	m_pLog = 0;
	m_pWindow = 0;
	m_nWinWidth = 1280;
	m_nWinHeight = 720;
	m_fLookDX = m_fLookDY = 0;
	m_pTouchLookSens = 0;
	m_pTouchInvertX = 0;
	m_pTouchInvertY = 0;
	for (int i = 0; i < TOUCH_MAX_FINGERS; i++)
	{
		m_fingers[i].m_id = 0;
		m_fingers[i].m_bDown = false;
		m_fingers[i].m_bClaimed = false;
		m_fingers[i].m_fX = m_fingers[i].m_fY = 0;
	}
}

CSDLTouch::~CSDLTouch()
{
}

bool CSDLTouch::Init(CInput *pInput, ISystem *pSystem, SDL_Window *window)
{
	m_pInput = pInput;
	m_pSystem = pSystem;
	m_pLog = pSystem->GetILog();
	m_pWindow = window;

	m_pTouchLookSens = pSystem->GetIConsole()->CreateVariable("touch_look_sens", "1.0", VF_DUMPTODISK,
			"Touch look sensitivity multiplier.\n"
			"Usage: touch_look_sens [0.1 .. 5.0]\n"
			"Default is 1.0");

	m_pTouchInvertX = pSystem->GetIConsole()->CreateVariable("touch_look_invert_x", "0", VF_DUMPTODISK,
			"Invert horizontal touch look.\n"
			"Usage: touch_look_invert_x [0/1]");

	m_pTouchInvertY = pSystem->GetIConsole()->CreateVariable("touch_look_invert_y", "0", VF_DUMPTODISK,
			"Invert vertical touch look.\n"
			"Usage: touch_look_invert_y [0/1]");

	if (window)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(window, &w, &h);
		if (w > 0 && h > 0)
		{
			m_nWinWidth = w;
			m_nWinHeight = h;
		}
	}

	m_pLog->Log("Initializing touch device\n");
	return true;
}

void CSDLTouch::ShutDown()
{
	for (int i = 0; i < TOUCH_MAX_FINGERS; i++)
		m_fingers[i].m_bDown = false;
}

ITouchOverlaySink *CSDLTouch::GetSink()
{
	if (!m_pInput)
		return 0;
	return m_pInput->GetTouchOverlaySink();
}

int CSDLTouch::FindFinger(SDL_FingerID id)
{
	for (int i = 0; i < TOUCH_MAX_FINGERS; i++)
		if (m_fingers[i].m_bDown && m_fingers[i].m_id == id)
			return i;
	return -1;
}

int CSDLTouch::AllocFinger(SDL_FingerID id)
{
	// reuse existing slot first
	int slot = FindFinger(id);
	if (slot >= 0)
		return slot;
	for (int i = 0; i < TOUCH_MAX_FINGERS; i++)
	{
		if (!m_fingers[i].m_bDown)
		{
			m_fingers[i].m_id = id;
			m_fingers[i].m_bDown = true;
			m_fingers[i].m_bClaimed = false;
			return i;
		}
	}
	return -1;
}

void CSDLTouch::RemoveFinger(int slot)
{
	if (slot >= 0 && slot < TOUCH_MAX_FINGERS)
		m_fingers[slot].m_bDown = false;
}

void CSDLTouch::AddLookDelta(float dx, float dy)
{
	m_fLookDX += dx;
	m_fLookDY += dy;
}

float CSDLTouch::ConsumeLookDeltaX()
{
	float d = m_fLookDX;
	m_fLookDX = 0;
	return d;
}

float CSDLTouch::ConsumeLookDeltaY()
{
	float d = m_fLookDY;
	m_fLookDY = 0;
	return d;
}

void CSDLTouch::ClearKeyState()
{
	for (int i = 0; i < TOUCH_MAX_FINGERS; i++)
		m_fingers[i].m_bDown = false;
	m_fLookDX = m_fLookDY = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSDLTouch::Update(bool bFocus)
{
	SDL_Event event;
	std::vector<SDL_Event> events;
	ITouchOverlaySink *pSink = GetSink();
	bool bTouchEnabled = (pSink != 0) && pSink->IsTouchEnabled();
	bool bEditMode = (pSink != 0) && pSink->IsTouchEditMode();
	bool bClaimAll = bTouchEnabled && bEditMode; // editor swallows everything

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
		{
			int w, h;
			SDL_GetWindowSizeInPixels(m_pWindow, &w, &h);
			if (w > 0 && h > 0)
			{
				m_nWinWidth = w;
				m_nWinHeight = h;
			}
			events.push_back(event);
		}
		break;

		case SDL_EVENT_FINGER_DOWN:
		case SDL_EVENT_FINGER_MOTION:
		case SDL_EVENT_FINGER_UP:
		case SDL_EVENT_FINGER_CANCELED:
		{
			if (!bTouchEnabled || !bFocus)
			{
				// without focus never route to the overlay
				if (!bFocus)
				{
					events.push_back(event); // let SDL's mouse emulation feed the menus
					break;
				}
				// overlay disabled: only the small "TOUCH OFF" recovery button
				// reacts, so the user can re-enable touch from the UI
				if (event.type == SDL_EVENT_FINGER_DOWN && pSink)
				{
					float px = event.tfinger.x * (float)m_nWinWidth;
					float py = event.tfinger.y * (float)m_nWinHeight;
					if (pSink->OnDisabledTap(px, py))
						break; // consumed - no mouse emulation for this tap
				}
				events.push_back(event); // let SDL's mouse emulation feed the menus
				break;
			}

			SDL_FingerID id = event.tfinger.fingerID;
			// normalized [0..1] -> framebuffer pixels
			float px = event.tfinger.x * (float)m_nWinWidth;
			float py = event.tfinger.y * (float)m_nWinHeight;
			float dpx = event.tfinger.dx * (float)m_nWinWidth;
			float dpy = event.tfinger.dy * (float)m_nWinHeight;

			bool bConsumed = false;

			if (event.type == SDL_EVENT_FINGER_DOWN)
			{
				int slot = AllocFinger(id);
				if (slot >= 0)
				{
					m_fingers[slot].m_fX = px;
					m_fingers[slot].m_fY = py;
				}
				if (pSink)
					bConsumed = pSink->OnTouchDown((int)id, px, py);
				if (bClaimAll)
					bConsumed = true;
				if (slot >= 0)
					m_fingers[slot].m_bClaimed = bConsumed;
			}
			else if (event.type == SDL_EVENT_FINGER_MOTION)
			{
				int slot = FindFinger(id);
				if (pSink)
					bConsumed = pSink->OnTouchMove((int)id, px, py, dpx, dpy);
				if (bClaimAll)
					bConsumed = true;
				if (slot >= 0)
				{
					if (m_fingers[slot].m_bClaimed)
						bConsumed = true;
					m_fingers[slot].m_fX = px;
					m_fingers[slot].m_fY = py;
				}
			}
			else // UP / CANCELED
			{
				int slot = FindFinger(id);
				if (pSink)
					bConsumed = pSink->OnTouchUp((int)id, px, py);
				if (bClaimAll)
					bConsumed = true;
				if (slot >= 0)
				{
					if (m_fingers[slot].m_bClaimed)
						bConsumed = true;
					RemoveFinger(slot);
				}
			}

			if (!bConsumed)
			{
				// unclaimed finger drag == look
				float sens = m_pTouchLookSens ? m_pTouchLookSens->GetFVal() : 1.0f;
				if (sens <= 0.01f)
					sens = 1.0f;
				float dx = dpx * sens;
				float dy = dpy * sens;
				if (m_pTouchInvertX && m_pTouchInvertX->GetIVal())
					dx = -dx;
				if (m_pTouchInvertY && m_pTouchInvertY->GetIVal())
					dy = -dy;
				m_fLookDX += dx;
				m_fLookDY += dy;
			}
		}
		break;

		default:
			events.push_back(event);
			break;
		}
	}

	for (SDL_Event ev : events)
	{
		SDL_PushEvent(&ev);
	}
	events.clear();

	// per-frame sink maintenance (stick centering etc.)
	if (pSink && bTouchEnabled)
		pSink->OnTouchUpdate();
	else if (pSink)
		pSink->OnTouchUpdate(); // let sink release held keys when disabled
}
