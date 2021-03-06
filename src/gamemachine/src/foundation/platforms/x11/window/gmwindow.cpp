#include "stdafx.h"
#include "gmengine/ui/gmwindow.h"
#include <X11/Xlib.h>
#include <GL/glx.h>
#include "gminput.h"
#include "gmxrendercontext.h"
#include "gmwindowhelper.h"
#include <gamemachine.h>
#include "gmengine/ui/gmwindow_p.h"

BEGIN_NS

namespace
{
	constexpr GMshort MOUSE_DELTA = 120;

	GMMouseButton translateButton(GMuint32 state)
	{
		GMMouseButton button = GMMouseButton_None;
		if (state & Button1Mask)
			button |= GMMouseButton_Left;
		if (state & Button2Mask)
			button |= GMMouseButton_Middle;
		if (state & Button3Mask)
			button |= GMMouseButton_Right;
		return button;
	}

	GMModifier translateModifier(GMuint32 state)
	{
		GMModifier modifier = GMModifier_None;
		if (state & ControlMask)
			modifier |= GMModifier_Ctrl;
		if (state & ShiftMask)
			modifier |= GMModifier_Shift;
		return modifier;
	}

	GMKey translateKey(XKeyEvent* xkey)
	{
		XComposeStatus composeStatus;
		char asciiCode[32];
		KeySym keySym;
		GMint32 len = 0;
		len = XLookupString(xkey, asciiCode, sizeof(asciiCode), &keySym, &composeStatus);
		if (len > 0)
		{
			return GM_ASCIIToKey(asciiCode[0]);
		}
		else
		{
			switch (keySym)
			{
				case XK_F1:
					return GMKey_F1;
				case XK_F2:
					return GMKey_F2;
				case XK_F3:
					return GMKey_F3;
				case XK_F4:
					return GMKey_F4;
				case XK_F5:
					return GMKey_F5;
				case XK_F6:
					return GMKey_F6;
				case XK_F7:
					return GMKey_F7;
				case XK_F8:
					return GMKey_F8;
				case XK_F9:
					return GMKey_F9;
				case XK_F10:
					return GMKey_F10;
				case XK_F11:
					return GMKey_F11;
				case XK_F12:
					return GMKey_F12;
				case XK_KP_Left:
				case XK_Left:
					return GMKey_Left;
				case XK_KP_Right:
				case XK_Right:
					return GMKey_Right;
				case XK_KP_Up:
				case XK_Up:
					return GMKey_Up;
				case XK_KP_Down:
				case XK_Down:
					return GMKey_Down;
				case XK_KP_Prior:
				case XK_Prior:
					return GMKey_Prior;
				case XK_KP_Next:
				case XK_Next:
					return GMKey_Next;
				case XK_KP_Home:
				case XK_Home:
					return GMKey_Home;
				case XK_KP_End:
				case XK_End:
					return GMKey_End;
				case XK_KP_Insert:
				case XK_Insert:
					return GMKey_Insert;
				case XK_Num_Lock:
					return GMKey_Numlock;
				case XK_KP_Delete:
					return GMKey_Delete;
				case XK_Shift_L:
				case XK_Shift_R:
					return GMKey_Shift;
				case XK_Control_L:
				case XK_Control_R:
					return GMKey_Control;
				//TODO Key Begin, Key Alt
			}
		}
		return GMKey_Unknown;
	}

	void sendCharEvents(GMWindow* window, GMKey key, GMModifier m, XKeyEvent* xkey)
	{
		static GMwchar s_wc[128];
		static GMLResult s_result;

		const GMXRenderContext* context = gm_cast<const GMXRenderContext*>(window->getContext());
		Status s;
		KeySym keySym;
		GMint32 len = 0;
		len = XwcLookupString(context->getIC(), xkey, s_wc, sizeof(s_wc), &keySym, &s);
		for (GMint32 i = 0; i < len; ++i)
		{
			GMSystemCharEvent e(GMSystemEventType::Char, key, s_wc[i], m);
			window->handleSystemEvent(&e, s_result);
		}
	}

	void translateSystemEvent(GMuint32 uMsg, GMWParam wParam, GMLParam lParam, OUT GMSystemEvent** event)
	{
		GM_ASSERT(event);
		GMXEventContext* c = reinterpret_cast<GMXEventContext*>(lParam);
		GMWindow* window = gm_cast<GMWindow*>(c->window);
		XEvent* xevent = c->event;

		GMSystemEvent* newSystemEvent = nullptr;
		GMKey key;

		switch (xevent->type)
		{
			case ClientMessage:
			{
				const GMXRenderContext* context = gm_cast<const GMXRenderContext*>(window->getContext());
				if ((Atom)xevent->xclient.data.l[0] == context->getAtomDeleteWindow())
					newSystemEvent = new GMSystemEvent(GMSystemEventType::WindowAboutToClose);
				break;
			}
			case KeymapNotify:
				XRefreshKeyboardMapping(&xevent->xmapping);
				gm_debug("XRefreshKeyboardMapping");
				break;
			case KeyPress:
			{
				key = translateKey(&xevent->xkey);
				GMModifier m = translateModifier(xevent->xkey.state);
				newSystemEvent = new GMSystemKeyEvent(GMSystemEventType::KeyDown, key, m);
				sendCharEvents(window, key, m, &xevent->xkey);
				break;
			}
			case KeyRelease:
			{
				key = translateKey(&xevent->xkey);
				newSystemEvent = new GMSystemKeyEvent(GMSystemEventType::KeyUp, key, translateModifier(xevent->xkey.state));
				break;
			}
			case ButtonPress:
			case ButtonRelease:
			{
				GMPoint mousePoint = { xevent->xbutton.x, xevent->xbutton.y };
				GMSystemEventType type = GMSystemEventType::Unknown;
				if (xevent->type == ButtonPress)
				{
					type = GMSystemEventType::MouseDown;
					// gm_debug(gm_dbg_wrap("Mouse down detected."));
				}
				else if (xevent->type == ButtonRelease)
				{
					type = GMSystemEventType::MouseUp;
					// gm_debug(gm_dbg_wrap("Mouse up detected."));
				}

				GMMouseButton triggeredButton = GMMouseButton_None;
				if (xevent->xbutton.button == Button1)
					triggeredButton = GMMouseButton_Left;
				else if (xevent->xbutton.button == Button2)
					triggeredButton = GMMouseButton_Middle;
				else if (xevent->xbutton.button == Button3)
					triggeredButton = GMMouseButton_Right;

				switch (xevent->xbutton.button)
				{
					case Button1:
					case Button2:
					case Button3:
					{
						GM_ASSERT(type != GMSystemEventType::Unknown);
						newSystemEvent = new GMSystemMouseEvent(type, mousePoint, triggeredButton, translateButton(xevent->xbutton.state), translateModifier(xevent->xbutton.state));
						break;
					}
					case Button4:
					case Button5:
					{
						newSystemEvent = new GMSystemMouseWheelEvent(
							mousePoint,
							GMMouseButton_None,
							GMMouseButton_None,
							translateModifier(xevent->xbutton.state),
							xevent->xbutton.button == Button4 ? MOUSE_DELTA : -MOUSE_DELTA
						);
						break;
					}
				}
				break;
			}
			case MotionNotify:
			{
				GMPoint mousePoint = { xevent->xmotion.x, xevent->xmotion.y };
				GMSystemEventType type = GMSystemEventType::MouseMove;
				GMMouseButton triggeredButton = GMMouseButton_None;
				if (xevent->xbutton.button == Button1)
					triggeredButton = GMMouseButton_Left;
				else if (xevent->xbutton.button == Button2)
					triggeredButton = GMMouseButton_Middle;
				else if (xevent->xbutton.button == Button3)
					triggeredButton = GMMouseButton_Right;
				newSystemEvent = new GMSystemMouseEvent(type, mousePoint, triggeredButton, translateButton(xevent->xbutton.state), translateModifier(xevent->xbutton.state));
				break;
			}
			case ResizeRequest:
			{
				newSystemEvent = new GMSystemEvent(GMSystemEventType::WindowSizeChanged);
				break;
			}
			default:
				newSystemEvent = new GMSystemEvent(); // Create an empty event
		}

		*event = newSystemEvent;
	}

	GMLResult GM_SYSTEM_CALLBACK WndProc(GMWindowHandle hWnd, GMuint32 uMsg, GMWParam wParam, GMLParam lParam)
	{
		GMXEventContext* c = reinterpret_cast<GMXEventContext*>(lParam);
		GMWindow* window = gm_cast<GMWindow*>(c->window);
		GMSystemEvent* sysEvent = nullptr;
		translateSystemEvent(uMsg, wParam, lParam, &sysEvent);
		GMScopedPtr<GMSystemEvent> guard(sysEvent);
		GMLResult lRes = 0;
		window->handleSystemEvent(sysEvent, lRes);
		if (sysEvent->getType() == GMSystemEventType::WindowAboutToClose && !lRes) // lRes为0表示保持原样
		{
			GMSystemEvent* destroyEvent = new GMSystemEvent(GMSystemEventType::WindowAboutToDestroy);
			GMScopedPtr<GMSystemEvent> g(destroyEvent);
			lRes = window->handleSystemEvent(destroyEvent, lRes);
		}
		return lRes;
	}
}

GMWindow::~GMWindow()
{

}

void GMWindow::showWindow()
{
	const GMXRenderContext* context = gm_cast<const GMXRenderContext*>(getContext());
	XMapWindow(context->getDisplay(), getWindowHandle());
	XFlush(context->getDisplay());
}

GMWindowHandle GMWindow::create(const GMWindowDesc& desc)
{
	D(d);
	onWindowCreated(desc);
	d->windowStates.renderRect = getRenderRect();
	d->windowStates.framebufferRect = d->windowStates.renderRect;
	return getWindowHandle();
}

IInput* GMWindow::getInputManager()
{
	D(d);
	if (!d->input)
		d->input = gm_makeOwnedPtr<GMInput>(this);
	return d->input.get();
}

GMRect GMWindow::getWindowRect()
{
	// TODO WindowRect contains border
	return getRenderRect();
}

GMRect GMWindow::getRenderRect()
{
	const GMXRenderContext* context = gm_cast<const GMXRenderContext*>(getContext());
	GMRect rc = GMWindowHelper::getWindowRect(context->getDisplay(), getWindowHandle(), context->getRootWindow()); // Retrieve coords from root
	return rc;
}

void GMWindow::centerWindow()
{
	D(d);
	const auto& windowRect = getWindowRect();
	const GMXRenderContext* context = gm_cast<const GMXRenderContext*>(getContext());

	GMint32 x = (context->getScreenWidth() - windowRect.width) / 2;
	GMint32 y = (context->getScreenHeight() - windowRect.height) / 2;
	XMoveWindow(context->getDisplay(), getWindowHandle(), x, y);
}

bool GMWindow::isWindowActivate()
{
	return true;
}

void GMWindow::setWindowCapture(bool)
{
}

void GMWindow::changeCursor()
{
}

void GMWindow::onWindowDestroyed()
{
}

GMWindowProcHandler GMWindow::getProcHandler()
{
	return &WndProc;
}

void GMWindow::setMultithreadRenderingFlag(GMMultithreadRenderingFlag)
{
}

END_NS
