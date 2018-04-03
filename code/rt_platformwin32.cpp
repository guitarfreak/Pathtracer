
#define ENABLE_CUSTOM_WINDOW_FRAME


#pragma once

enum Keycode {
	KEYCODE_CTRL = 0,
	KEYCODE_CTRL_RIGHT,
	KEYCODE_SHIFT,
	KEYCODE_SHIFT_RIGHT,
	KEYCODE_ALT,
	KEYCODE_CAPS,
	KEYCODE_TAB,
	KEYCODE_SPACE,
	KEYCODE_RETURN,
	KEYCODE_ESCAPE,
	KEYCODE_BACKSPACE,
	KEYCODE_DEL,
	KEYCODE_HOME,
	KEYCODE_END,
	KEYCODE_PAGEUP,
	KEYCODE_PAGEDOWN,
	KEYCODE_UP,
	KEYCODE_DOWN,
	KEYCODE_LEFT,
	KEYCODE_RIGHT,

	KEYCODE_A,
	KEYCODE_B,
	KEYCODE_C,
	KEYCODE_D,
	KEYCODE_E,
	KEYCODE_F,
	KEYCODE_G,
	KEYCODE_H,
	KEYCODE_I,
	KEYCODE_J,
	KEYCODE_K,
	KEYCODE_L,
	KEYCODE_M,
	KEYCODE_N,
	KEYCODE_O,
	KEYCODE_P,
	KEYCODE_Q,
	KEYCODE_R,
	KEYCODE_S,
	KEYCODE_T,
	KEYCODE_U,
	KEYCODE_V,
	KEYCODE_W,
	KEYCODE_X,
	KEYCODE_Y,
	KEYCODE_Z,

	KEYCODE_0,
	KEYCODE_1,
	KEYCODE_2,
	KEYCODE_3,
	KEYCODE_4,
	KEYCODE_5,
	KEYCODE_6,
	KEYCODE_7,
	KEYCODE_8,
	KEYCODE_9,

	KEYCODE_F1,
	KEYCODE_F2,
	KEYCODE_F3,
	KEYCODE_F4,
	KEYCODE_F5,
	KEYCODE_F6,
	KEYCODE_F7,
	KEYCODE_F8,
	KEYCODE_F9,
	KEYCODE_F10,
	KEYCODE_F11,
	KEYCODE_F12,

	KEYCODE_COUNT,
};

struct Input {
	bool firstFrame;
	Vec2 mousePos;
	Vec2 mousePosNegative;
	Vec2 mousePosScreen;
	Vec2 mousePosNegativeScreen;

	Vec2 mousePosWindow;

	Vec2 mouseDelta;
	int mouseWheel;
	bool mouseButtonPressed[8];
	bool mouseButtonDown[8];
	bool mouseButtonReleased[8];
	bool doubleClick;
	Vec2 doubleClickPos;

	Vec2 lastMousePos;

	bool keysDown[KEYCODE_COUNT];
	bool keysPressed[KEYCODE_COUNT];
	char inputCharacters[32];
	int inputCharacterCount;

	bool mShift, mCtrl, mAlt;

	bool anyKey;

	bool closeWindow;
	bool maximizeWindow;
	bool minimizeWindow;
	bool resize;
};



#define WIN_KEY_NUMERIC_START 0x30
#define WIN_KEY_NUMERIC_END 0x39
#define WIN_KEY_LETTERS_START 0x41
#define WIN_KEY_LETTERS_END 0x5a
#define WIN_KEY_F_START 0x70
#define WIN_KEY_F_END 0x7B

int vkToKeycode(int vk) {

	switch(vk) {
		case VK_CONTROL: return KEYCODE_CTRL;
		case VK_RCONTROL: return KEYCODE_CTRL_RIGHT;
		case VK_SHIFT: return KEYCODE_SHIFT;
		case VK_RSHIFT: return KEYCODE_SHIFT_RIGHT;
		case VK_MENU: return KEYCODE_ALT;
		case VK_CAPITAL: return KEYCODE_CAPS;
		case VK_TAB: return KEYCODE_TAB;
		case VK_SPACE: return KEYCODE_SPACE;
		case VK_RETURN: return KEYCODE_RETURN;
		case VK_ESCAPE: return KEYCODE_ESCAPE;
		case VK_BACK: return KEYCODE_BACKSPACE;
		case VK_DELETE: return KEYCODE_DEL;
		case VK_HOME: return KEYCODE_HOME;
		case VK_END: return KEYCODE_END;
		case VK_PRIOR: return KEYCODE_PAGEUP;
		case VK_NEXT: return KEYCODE_PAGEDOWN;
		case VK_UP: return KEYCODE_UP;
		case VK_DOWN: return KEYCODE_DOWN;
		case VK_LEFT: return KEYCODE_LEFT;
		case VK_RIGHT: return KEYCODE_RIGHT;

		default: {
				 if(vk >= WIN_KEY_NUMERIC_START && vk <= WIN_KEY_NUMERIC_END) return KEYCODE_0 + vk - WIN_KEY_NUMERIC_START;
			else if(vk >= WIN_KEY_LETTERS_START && vk <= WIN_KEY_LETTERS_END) return KEYCODE_A + vk - WIN_KEY_LETTERS_START;
			else if(vk >= WIN_KEY_F_START 		&& vk <= WIN_KEY_F_END) 	  return KEYCODE_F1 + vk - WIN_KEY_F_START;
		}
	}

	return -1;
}

void initInput(Input* input) {
    *input = {};

    input->firstFrame = true;
}

enum RequestType {
	REQUEST_TYPE_GET = 0,
	REQUEST_TYPE_POST,
	REQUEST_TYPE_COUNT,
};

struct HttpRequest {
	char* link;
	RequestType type;
	char* additionalBodyContent;

	char* contentBuffer;
	char* contentFile;
	char* contentFilePath;
	char* headerResponseFile;

	bool finished;
	int size;
	float progress;

	bool stopProcess;
};

enum ShellCommand {
	SHELLCOMMAND_FILE,
	SHELLCOMMAND_PATH,
	SHELLCOMMAND_URL,
	SHELLCOMMAND_COUNT,
};


// void atomicAdd(unsigned int* n) {
void atomicAdd(volatile unsigned int* n) {
	InterlockedIncrement(n);
}

void atomicSub(volatile unsigned int* n) {
	InterlockedDecrement(n);
}

struct ThreadQueue;

int getThreadId() {
	int tId = GetCurrentThreadId();
	return tId;
}

struct SystemData {
	WindowsData windowsData;
	HINSTANCE instance;
	HDC deviceContext;
	HWND windowHandle;

	HANDLE folderHandles[10];
	int folderHandleCount;

	WNDCLASS windowClass;

	HGLRC openglContext;

	Vec2i minWindowDim;
	Vec2i maxWindowDim;

	void* mainFiber;
	void* messageFiber;
	Input* input;

	int coreCount;

	int fontHeight;

	// For nchittest

	int titleHeight;
	int borderSize;
	int visualBorderSize;
	int buttonMargin;
	Rect rMinimize, rMaximize, rClose;
	int cornerGrabSize;

	int normalTitleHeight;
	int normalBorderSize;
	int normalVisualBorderSize;

	bool vsyncTempTurnOff;

	bool maximized;
	bool killedFocus;
	bool setFocus;
	bool windowIsFocused;

	bool mouseInClient;

	int ncTestRegion;

	// Duplication with WindowSettings.
	int styleBorderSize;
};

void systemDataInit(SystemData* sd, HINSTANCE instance) {
	sd->instance = instance;
}

struct MonitorData {
	Rect fullRect;
	Rect workRect;
	HMONITOR handle;
};

struct WindowSettings {
	Vec2i res;
	// Vec2i fullRes;
	bool fullscreen;
	uint style;
	WINDOWPLACEMENT g_wpPrev;
	Rect previousWindowRect;

	MonitorData monitors[3];
	int monitorCount;
	Vec2i biggestMonitorSize;
	int refreshRate;

	bool vsync;
	int frameRate;


	Vec2i currentRes;

	Vec2i clientRes;
	Vec2i windowRes;

	Rect clientRect;
	Rect windowRect;
	Rect titleRect;

	Rect windowRectBL; // vec2(0,0) is bottom left
	Rect clientRectBL; // vec2(0,0) is bottom left

	float aspectRatio;	

	bool dontUpdateCursor;
	bool customCursor;
	POINT lastMousePosition;

	int styleBorderSize;

	bool windowHasFocus;
};


void updateCursor(WindowSettings* ws) {
	if(!ws->customCursor) {
		SetCursor(LoadCursor(0, IDC_ARROW));
	}
	ws->customCursor = false;
}

void setCursor(WindowSettings* ws, LPCSTR type) {
	SetCursor(LoadCursor(0, type));
	ws->customCursor = true;
}

BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {

	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMonitor, &mi);

	WindowSettings* ws = (WindowSettings*)(dwData);
	MonitorData* md = ws->monitors + ws->monitorCount;
	md->fullRect = rect(mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
	md->workRect = rect(mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);
	md->handle = hMonitor;
	ws->monitorCount++;

	return true;
}


Vec2 getMousePos(HWND windowHandle, bool yInverted = true) {
	POINT point;    
	GetCursorPos(&point);
	ScreenToClient(windowHandle, &point);
	Vec2 mousePos = vec2(0,0);
	mousePos.x = point.x;
	mousePos.y = point.y;
	if(yInverted) mousePos.y = -mousePos.y;

	return mousePos;
}

Vec2 getMousePosS(bool yInverted = true) {
	POINT point;    
	GetCursorPos(&point);
	Vec2 mousePos = vec2(0,0);
	mousePos.x = point.x;
	mousePos.y = point.y;
	if(yInverted) mousePos.y = -mousePos.y;

	return mousePos;
}

bool mouseInClientArea(HWND windowHandle) {
	POINT point;    
	GetCursorPos(&point);
	ScreenToClient(windowHandle, &point);

	Vec2i mp = vec2i(point.x, point.y);

	RECT cr; 
	GetClientRect(windowHandle, &cr);
	bool result = (mp.x >= cr.left && mp.x < cr.right && 
				   mp.y >= cr.top  && mp.y < cr.bottom);

	return result;
}

#include <Windowsx.h>


LRESULT CALLBACK mainWindowCallBack(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {

	SystemData* sd = (SystemData*)GetWindowLongPtrA(window, GWLP_USERDATA);

    switch(message) {
        case WM_DESTROY: {
            PostMessage(window, message, wParam, lParam);
        } break;

        case WM_CLOSE: {
            PostMessage(window, message, wParam, lParam);
        } break;

        case WM_QUIT: {
            PostMessage(window, message, wParam, lParam);
        } break;

        #ifdef ENABLE_CUSTOM_WINDOW_FRAME
        case WM_NCACTIVATE: {
        	sd->vsyncTempTurnOff = true;
        	SwitchToFiber(sd->mainFiber);
        } break;
        #endif

        case WM_SIZE: {
        	if(wParam == SIZE_MAXIMIZED) sd->maximized = true;
        	else if(wParam == SIZE_RESTORED) sd->maximized = false;

        	// sd->vsyncTempTurnOff = true;
        	sd->input->resize = true;
        } break;

        #ifdef ENABLE_CUSTOM_WINDOW_FRAME
        case WM_NCHITTEST: {
        	sd->mouseInClient = false;

        	int x = GET_X_LPARAM(lParam);
        	int y = GET_Y_LPARAM(lParam);

        	if(sd->maximized) {
        		x -= sd->styleBorderSize;
        		y -= sd->styleBorderSize;
        	}

        	RECT wr; 
        	GetWindowRect(window, &wr);

        	x = x - wr.left;
        	y = y - wr.top;
        	y *= -1;

        	Rect re = rectTLDim(vec2(0,0), vec2(wr.right - wr.left, wr.bottom - wr.top));
        	Recti r = rectiRound(re);
        	int b = sd->borderSize;
        	int t = sd->titleHeight;

        	r.left += b;
        	r.bottom += b+1;
        	r.right -= b+1;
        	r.top -= b;

        	// Border.
        	if((x < r.left) || (x > r.right) || (y < r.bottom) || (y > r.top)) {
        		// printf("%i %i, %i %i %i %i, %i\n", x,y, PRECT(r), t);

        		float cg = sd->cornerGrabSize;

        		     if(x < r.left     && y < r.bottom+cg ||
        		        x < r.left+cg  && y < r.bottom) return sd->ncTestRegion = HTBOTTOMLEFT;
        		else if(x > r.right    && y < r.bottom+cg ||
        		        x > r.right-cg && y < r.bottom) return sd->ncTestRegion = HTBOTTOMRIGHT;
        		else if(x < r.left     && y > r.top-cg ||
        		        x < r.left+cg  && y > r.top) return sd->ncTestRegion = HTTOPLEFT;
        		else if(x > r.right    && y > r.top-cg ||
        		        x > r.right-cg && y > r.top) return sd->ncTestRegion = HTTOPRIGHT;
        		else if(y < r.bottom)  return sd->ncTestRegion = HTBOTTOM;
        		else if(x > r.right)   return sd->ncTestRegion = HTRIGHT;
        		else if(y > r.top)     return sd->ncTestRegion = HTTOP;
        		else if(x < r.left)    return sd->ncTestRegion = HTLEFT;
        	}

        	// BottomRight corner grab.
        	if(b)
        	{
        		Vec2 cornerPoint = vec2(r.right,r.bottom) - vec2(sd->cornerGrabSize,0);
        		int result = dot(vec2(x,y) - cornerPoint, normVec2(vec2(1,-1)));
        		if(result > 0) return sd->ncTestRegion = HTBOTTOMRIGHT;
        	}

        	Vec2 off = vec2(0,0);

        	Vec2 p = vec2(x,y);
        	if(pointInRect(p+off, sd->rMinimize)) return sd->ncTestRegion = HTMINBUTTON;
        	if(pointInRect(p+off, sd->rMaximize)) return sd->ncTestRegion = HTMAXBUTTON;
        	if(pointInRect(p+off, sd->rClose)) return sd->ncTestRegion = HTCLOSE;

        	if(p.y+off.y > -t-sd->visualBorderSize) return sd->ncTestRegion = HTCAPTION;

        	sd->mouseInClient = true;

        	return sd->ncTestRegion = HTCLIENT;
        } break;

        // case WM_NCPAINT: {
	       //  HDC hdc;
	       //  hdc = GetDCEx(window, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
	       //  // Paint into this DC 
	       //  ReleaseDC(window, hdc);

	       //  // sd->vsyncTempTurnOff = true;
	       //  // SwitchToFiber(sd->mainFiber);

        // 	return 0;
        // } break;

        case WM_NCLBUTTONDOWN: {
        	int test = wParam;
        	if(test == HTMINBUTTON) SendMessage(window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        	else if(test == HTMAXBUTTON) {
	        	if(!sd->maximized) SendMessage(window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
	        	else SendMessage(window, WM_SYSCOMMAND, SC_RESTORE, 0);
        	}
        	else if(test == HTCLOSE) SendMessage(window, WM_SYSCOMMAND, SC_CLOSE, 0);

            else return DefWindowProc(window, message, wParam, lParam);
        } break;

        case WM_NCCALCSIZE: {
        	return 0;
        } break;
		#endif

        case WM_PAINT: {
        	PAINTSTRUCT ps;
        	HDC hdc = BeginPaint(window, &ps); 
        	EndPaint(window, &ps);

        	sd->vsyncTempTurnOff = true;
        	SwitchToFiber(sd->mainFiber);

        	return 0;
        } break;

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
            if(sd) {
	            lpMMI->ptMinTrackSize.x = sd->minWindowDim.w;
	            lpMMI->ptMinTrackSize.y = sd->minWindowDim.h;
	            lpMMI->ptMaxTrackSize.x = sd->maxWindowDim.w;
	            lpMMI->ptMaxTrackSize.y = sd->maxWindowDim.h;
            } else {
            	lpMMI->ptMinTrackSize.x = 200;
            	lpMMI->ptMinTrackSize.y = 200;
            	lpMMI->ptMaxTrackSize.x = 300;
            	lpMMI->ptMaxTrackSize.y = 300;
            }
        } break;

        case WM_SETFOCUS: {
        	sd->setFocus = true;
        	sd->windowIsFocused = true;
        	sd->vsyncTempTurnOff = true;
        	SwitchToFiber(sd->mainFiber);
        } break;

        case WM_KILLFOCUS: {
		    // PostMessage(window, message, wParam, lParam);
		    sd->killedFocus = true;
        	sd->windowIsFocused = false;

        	sd->vsyncTempTurnOff = true;
        	SwitchToFiber(sd->mainFiber);
        } break;

        case WM_TIMER: {
        	sd->vsyncTempTurnOff = true;
        	SwitchToFiber(sd->mainFiber);
        } break;

        default: {
            return DefWindowProc(window, message, wParam, lParam);
        } break;
    }

    return 1;
}

void inputPrepare(Input* input) {
	input->anyKey = false;
    input->mouseWheel = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonPressed); i++) input->mouseButtonPressed[i] = 0;
    for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) input->mouseButtonReleased[i] = 0;
    for(int i = 0; i < arrayCount(input->keysPressed); i++) input->keysPressed[i] = 0;
    input->mShift = 0;
    input->mCtrl = 0;
    input->mAlt = 0;
    input->inputCharacterCount = 0;
    input->mouseDelta = vec2(0,0);

    input->doubleClick = false;

    input->closeWindow = false;
	input->maximizeWindow = false;
	input->minimizeWindow = false;
}

void CALLBACK updateInput(SystemData* sd) {
	for(;;) {

		Input* input = sd->input;
		HWND windowHandle = sd->windowHandle;

		SetTimer(windowHandle, 1, 1, 0);

	    bool mouseInClient = mouseInClientArea(windowHandle);

        #ifdef ENABLE_CUSTOM_WINDOW_FRAME
        {
        	mouseInClient = sd->mouseInClient && mouseInClientArea(windowHandle);
        }
        #endif

	    MSG message;
	    while(PeekMessage(&message, windowHandle, 0, 0, PM_REMOVE)) {
	        switch(message.message) {
		        case WM_LBUTTONDBLCLK: {
		        	input->doubleClick = true;
					input->doubleClickPos = getMousePos(windowHandle, true);
		        } break;

	            case WM_KEYDOWN:
	            case WM_KEYUP: {
	                uint vk = uint(message.wParam);

	                bool keyDown = (message.message == WM_KEYDOWN);
	                int keycode = vkToKeycode(vk);
	                input->keysDown[keycode] = keyDown;
	                input->keysPressed[keycode] = keyDown;
	                input->mShift = ((GetKeyState(VK_SHIFT) & 0x80) != 0);
	                input->mCtrl = ((GetKeyState(VK_CONTROL) & 0x80) != 0);
	                input->mAlt = ((GetKeyState(VK_MENU) & 0x80) != 0);

	                if(keyDown) {
	                	input->anyKey = true;
	                }

	                TranslateMessage(&message); 
	                DispatchMessage(&message); 
	            } break;

	            case WM_CHAR: {
	                // input->inputCharacters[input->inputCharacterCount] = (char)uint(message.wParam);
	            	uint charIndex = uint(message.wParam);
	            	if(charIndex < ' ' || charIndex > '~') break;
	            	char c = (char)charIndex;
	                input->inputCharacters[input->inputCharacterCount] = c;
	                input->inputCharacterCount++;
	            } break;

	            case WM_INPUT: {
	            	RAWINPUT inputBuffer;
	            	UINT rawInputSize = sizeof(inputBuffer);
	            	GetRawInputData((HRAWINPUT)(message.lParam), RID_INPUT, &inputBuffer, &rawInputSize, sizeof(RAWINPUTHEADER));
	            	RAWINPUT* raw = (RAWINPUT*)(&inputBuffer);
	            	
	            	if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {

	            	    input->mouseDelta += vec2(raw->data.mouse.lLastX, raw->data.mouse.lLastY);

	            	    USHORT buttonFlags = raw->data.mouse.usButtonFlags;

	            	    if(mouseInClient) {
							if(buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[0] = true; 
								input->mouseButtonDown[0] = true; 
							} else if(buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[1] = true; 
								input->mouseButtonDown[1] = true; 
							} else if(buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
								// SetCapture(windowHandle);
								input->mouseButtonPressed[2] = true; 
								input->mouseButtonDown[2] = true; 
							} else if(buttonFlags & RI_MOUSE_WHEEL) {
								input->mouseWheel += ((SHORT)raw->data.mouse.usButtonData) / WHEEL_DELTA;
							}
	            	    }

	            	    if(buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[0] = false; 
	            	    	input->mouseButtonReleased[0] = true; 
	            	    } else if(buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[1] = false; 
	            	    	input->mouseButtonReleased[1] = true; 
	            	    } else if(buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
	            	    	// ReleaseCapture();
	            	    	input->mouseButtonDown[2] = false; 
	            	    	input->mouseButtonReleased[2] = true; 
	            	    }

	            	} break;

	            	TranslateMessage(&message);
	            	DispatchMessage(&message);
	            } break;

	            case WM_DESTROY: 
	            case WM_CLOSE: 
	            case WM_QUIT: 
	            	input->closeWindow = true;
	            	break;

	            default: {
	                TranslateMessage(&message); 
	                DispatchMessage(&message); 
	            } break;
	        }
	    }

	    if(!sd->windowIsFocused) {
	    	for(int i = 0; i < arrayCount(input->mouseButtonPressed); i++) input->mouseButtonPressed[i] = 0;
	    	for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) input->mouseButtonReleased[i] = 0;
	    	input->mouseWheel = 0;
	    }
	    sd->setFocus = false;

	    // In case we clear because of focus.
	    bool closeWindowTemp = input->closeWindow;

	    if(sd->killedFocus) {
	    	for(int i = 0; i < KEYCODE_COUNT; i++) {
	    		input->keysDown[i] = false;
	    	}
	    	*input = {};

	    	for(int i = 0; i < arrayCount(input->mouseButtonReleased); i++) {
		    	input->mouseButtonReleased[i] = true;
	    	}

	    	sd->killedFocus = false;
	    }

	    input->closeWindow = closeWindowTemp;

	    input->mousePos = getMousePos(windowHandle, false);
	    input->mousePosNegative = getMousePos(windowHandle, true);

	    input->mousePosScreen = getMousePosS(false);
	    input->mousePosNegativeScreen = getMousePosS(true);

	    input->lastMousePos = input->mousePos;

	    input->firstFrame = false;

	    #ifdef ENABLE_CUSTOM_WINDOW_FRAME
	    {
	    	float xOff = -sd->visualBorderSize;
	    	float yOff = sd->visualBorderSize+sd->titleHeight;

	    	int frameSize = sd->styleBorderSize;
		    if(sd->maximized) {
		    	xOff -= frameSize;
		    	yOff += frameSize;
		    }

		    input->mousePosWindow = input->mousePosNegative;
		    if(sd->maximized) input->mousePosWindow += vec2(-frameSize,frameSize);

	    	input->mousePos += vec2(xOff, -yOff);
	    	input->mousePosNegative += vec2(xOff, yOff);
	    	input->mousePosScreen += vec2(xOff, -yOff);
	    	input->mousePosNegativeScreen += vec2(xOff, yOff);
		    input->lastMousePos += vec2(xOff, -yOff);
	    }
	    #endif

	    SwitchToFiber(sd->mainFiber);
	}
}

#include <Mmsystem.h>



int getSystemFontHeight(HWND windowHandle);
void initSystem(SystemData* systemData, WindowSettings* ws, WindowsData wData, Vec2i res, int style, int , int monitor = 0) {
	systemData->windowsData = wData;

	EnumDisplayMonitors(0, 0, monitorEnumProc, ((LPARAM)ws));

	ws->biggestMonitorSize = vec2i(0,0);
	for(int i = 0; i < arrayCount(ws->monitors); i++) {
		Rect r = ws->monitors[i].fullRect;
		ws->biggestMonitorSize.w = maxInt(ws->biggestMonitorSize.w, rectW(r));
		ws->biggestMonitorSize.h = maxInt(ws->biggestMonitorSize.h, rectH(r));
	}

	DEVMODE devMode;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
	ws->refreshRate = devMode.dmDisplayFrequency;


	ws->currentRes = res;
	ws->fullscreen = false;
	ws->aspectRatio = (float)res.w / (float)res.h;

	ws->style = style;

	RECT cr = {0, 0, res.w, res.h};
	AdjustWindowRectEx(&cr, ws->style, 0, 0);

	int ww = cr.right - cr.left;
	int wh = cr.bottom - cr.top;
	int wx, wy;
	{
		MonitorData* md = ws->monitors + monitor;
		wx = rectCen(md->workRect).x - res.w/2;
		wy = rectCen(md->workRect).y - res.h/2;
	}
	ws->res = vec2i(ww, wh);

    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
     
    windowClass.lpfnWndProc = mainWindowCallBack;
    windowClass.hInstance = systemData->instance;
    windowClass.lpszClassName = "App";

    if(!RegisterClass(&windowClass)) {
        DWORD errorCode = GetLastError();
        int dummy = 2;   
    }

    systemData->windowClass = windowClass;
    systemData->windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "", ws->style, wx,wy,ww,wh, 0, 0, systemData->instance, 0);

    HWND windowHandle = systemData->windowHandle;
    if(!windowHandle) {
        DWORD errorCode = GetLastError();
    }

    SetWindowLongPtr(windowHandle, GWLP_USERDATA, (LONG_PTR)systemData);

    PIXELFORMATDESCRIPTOR pixelFormatDescriptor =
    {
        +    sizeof(PIXELFORMATDESCRIPTOR),
        1,
        /*PFD_SUPPORT_COMPOSITION |*/ PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        24,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0, //8
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        // 32,                        //Number of bits for the depthbuffer
        // 0,                        //Number of bits for the depthbuffer
        // 8,                        //Number of bits for the stencilbuffer
        0,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };    
    
    HDC deviceContext = GetDC(windowHandle);
    systemData->deviceContext = deviceContext;
    int pixelFormat;
    pixelFormat = ChoosePixelFormat(deviceContext, &pixelFormatDescriptor);
	SetPixelFormat(deviceContext, pixelFormat, &pixelFormatDescriptor);
	
    HGLRC openglContext = wglCreateContext(systemData->deviceContext);
    bool result = wglMakeCurrent(systemData->deviceContext, openglContext);
    if(!result) { printf("Could not set Opengl Context.\n"); }

    systemData->openglContext = openglContext;

    #ifndef HID_USAGE_PAGE_GENERIC
    #define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
    #endif
    #ifndef HID_USAGE_GENERIC_MOUSE
    #define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
    #endif

    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
    Rid[0].hwndTarget = windowHandle;
    Rid[0].dwFlags = RIDEV_INPUTSINK;   
    // Rid[0].dwFlags = 0;   
    bool r = RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
    assert(r);

    systemData->mainFiber = ConvertThreadToFiber(0);
    systemData->messageFiber = CreateFiber(0, (PFIBER_START_ROUTINE)updateInput, systemData);

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    systemData->coreCount = sysinfo.dwNumberOfProcessors;

    ws->styleBorderSize = GetSystemMetrics(SM_CXSIZEFRAME);


    // Set icon.
    {
    	char* rs = MAKEINTRESOURCE(1);

    	HANDLE hbicon = LoadImage(GetModuleHandle(0), rs, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    	if(hbicon) SendMessage(windowHandle, WM_SETICON, ICON_BIG, (LPARAM)hbicon);

    	HANDLE hsicon = LoadImage(GetModuleHandle(0), rs, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    	if(hsicon) SendMessage(windowHandle, WM_SETICON, ICON_SMALL, (LPARAM)hsicon);
    }

    // Set minimal sleep timer resolution.
    {
    	TIMECAPS timecaps;
    	timeGetDevCaps(&timecaps, sizeof(TIMECAPS));
    	int error = timeBeginPeriod(timecaps.wPeriodMin);
    	if(error != TIMERR_NOERROR) printf("Timer error.\n");
    }

	systemData->fontHeight = getSystemFontHeight(windowHandle);

	// SetWindowLong(windowHandle, GWL_EXSTYLE,
		// GetWindowLong(windowHandle, GWL_EXSTYLE) | WS_EX_LAYERED);
	// SetLayeredWindowAttributes(windowHandle, 0, (255 * 70) / 100, LWA_ALPHA);
	// SetLayeredWindowAttributes(windowHandle, RGB(255,255,255), 255, LWA_COLORKEY);
	
	SetFocus(windowHandle);
	systemData->windowIsFocused = true;
}

void makeWindowTopmost(SystemData* sd) {
    SetWindowPos(sd->windowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void showWindow(HWND windowHandle) {
    ShowWindow(windowHandle, SW_SHOW);
}

float getScalingFactor(HWND windowHandle) {
    HDC deviceContext = GetDC(windowHandle);
    int LogicalScreenHeight = GetDeviceCaps(deviceContext, VERTRES);
    int PhysicalScreenHeight = GetDeviceCaps(deviceContext, DESKTOPVERTRES); 

    float ScreenScalingFactor = (float)PhysicalScreenHeight / (float)LogicalScreenHeight;

    return ScreenScalingFactor;
}

// MetaPlatformFunction();
// const char* getClipboard(MemoryBlock* memory) {
//     BOOL result = OpenClipboard(0);
//     HANDLE clipBoardData = GetClipboardData(CF_TEXT);

//     return (char*)clipBoardData;
// }

char* getClipboard() {
    BOOL result = OpenClipboard(0);
    HANDLE clipboardHandle = GetClipboardData(CF_TEXT);
    // char* data = GlobalLock(clipboardHandle);
    char* data = (char*)clipboardHandle;

    return data;
}

void closeClipboard() {
    CloseClipboard();
}

// MetaPlatformFunction();
void setClipboard(char* text) {
    int textSize = strLen(text) + 1;
    HANDLE clipHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, textSize);
    char* pointer = (char*)GlobalLock(clipHandle);
    memCpy(pointer, (char*)text, textSize);
    GlobalUnlock(clipHandle);

    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clipHandle);
    CloseClipboard();
}

Rect getWindowWindowRect(HWND windowHandle) {
	RECT r; 
	GetWindowRect(windowHandle, &r);
	Rect windowRect = rect(r.left, r.bottom, r.right, r.top);
	
	return windowRect;
}

void getWindowProperties(HWND windowHandle, int* viewWidth, int* viewHeight, int* width, int* height, int* x, int* y) {
	if(viewWidth && viewHeight) {
	    RECT cr; 
	    GetClientRect(windowHandle, &cr);
	    *viewWidth = cr.right - cr.left;
	    *viewHeight = cr.bottom - cr.top;
	}

    if(width && height) {
    	RECT wr; 
    	GetWindowRect(windowHandle, &wr);
    	*width = wr.right - wr.left;
    	*height = wr.bottom - wr.top;
    }

    if(x && y) {
    	WINDOWPLACEMENT placement;
    	GetWindowPlacement(windowHandle, &placement);
    	RECT r; 
    	r = placement.rcNormalPosition; 
    	*x = r.left;
    	*y = r.top;    	
    }
}

// MetaPlatformFunction();
void setWindowProperties(HWND windowHandle, int width, int height, int x, int y) {
    WINDOWPLACEMENT placement;
    GetWindowPlacement(windowHandle, &placement);
    RECT r = placement.rcNormalPosition;

    if(width != -1) r.right = r.left + width;
    if(height != -1) r.bottom = r.top + height;
    if(x != -1) {
        int width = r.right - r.left;
        r.left = x;
        r.right = x + width;
    }
    if(y != -1) {
        int height = r.bottom - r.top;
        r.top = y;
        r.bottom = y + height;
    }

    placement.rcNormalPosition = r;
    SetWindowPlacement(windowHandle, &placement);
}

enum WindowMode {
	WINDOW_MODE_WINDOWED = 0,
	WINDOW_MODE_FULLBORDERLESS,

	WINDOW_MODE_COUNT,
};

void setWindowStyle(HWND hwnd, DWORD dwStyle) {
	SetWindowLong(hwnd, GWL_STYLE, dwStyle);
}

DWORD getWindowStyle(HWND hwnd) {
	return GetWindowLong(hwnd, GWL_STYLE);
}

void updateResolution(HWND windowHandle, SystemData* sd, WindowSettings* ws) {
	getWindowProperties(windowHandle, &ws->currentRes.x, &ws->currentRes.y, 0,0,0,0);

	#ifdef ENABLE_CUSTOM_WINDOW_FRAME
	if(sd->maximized) {
		// Windows returns us the wrong client resolution so we have to do some workaround.

		HMONITOR monitorHandle = MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		bool result = GetMonitorInfo(monitorHandle, &monitorInfo);

		Vec2i windowsClientRes = ws->currentRes;

		RECT rWork = monitorInfo.rcWork;
		Vec2i workRes = vec2i(rWork.right - rWork.left, rWork.bottom - rWork.top);

		ws->styleBorderSize = (windowsClientRes.w - workRes.w) / 2;
		sd->styleBorderSize = ws->styleBorderSize;

		ws->currentRes = workRes;
	}
	#endif ENABLE_CUSTOM_WINDOW_FRAME

	ws->aspectRatio = ws->currentRes.x / (float)ws->currentRes.y;
}

Rect getWindowRect(WindowSettings* ws);
void setWindowMode(HWND hwnd, WindowSettings* wSettings, int mode) {
	if(mode == WINDOW_MODE_FULLBORDERLESS && !wSettings->fullscreen) {
		wSettings->previousWindowRect = getWindowWindowRect(hwnd);
		
		wSettings->g_wpPrev = {};

		DWORD dwStyle = getWindowStyle(hwnd);
		if (dwStyle & WS_OVERLAPPEDWINDOW) {
		  MONITORINFO mi = { sizeof(mi) };
		  if (GetWindowPlacement(hwnd, &wSettings->g_wpPrev) &&
		      GetMonitorInfo(MonitorFromWindow(hwnd,
		                     MONITOR_DEFAULTTOPRIMARY), &mi)) {
		    SetWindowLong(hwnd, GWL_STYLE,
		                  dwStyle & ~WS_OVERLAPPEDWINDOW);
			setWindowStyle(hwnd, dwStyle & ~WS_OVERLAPPEDWINDOW);

		    SetWindowPos(hwnd, HWND_TOP,
		                 mi.rcMonitor.left, mi.rcMonitor.top,
		                 mi.rcMonitor.right - mi.rcMonitor.left,
		                 mi.rcMonitor.bottom - mi.rcMonitor.top,
		                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		  }
		}

		wSettings->fullscreen = true;

	} else if(mode == WINDOW_MODE_WINDOWED && wSettings->fullscreen) {
		setWindowStyle(hwnd, wSettings->style);
		SetWindowPlacement(hwnd, &wSettings->g_wpPrev);

		wSettings->fullscreen = false;

		InvalidateRect(NULL, NULL, FALSE);
	}
}

void swapBuffers(SystemData* systemData) {
    SwapBuffers(systemData->deviceContext); }

Rect getWindowRect(WindowSettings* ws) {
	return ws->windowRect;
}

Rect getClientRect(WindowSettings* ws) {
	return ws->clientRect;
}

Rect getScreenRect(WindowSettings* ws) {
	return rect(0, -ws->clientRes.h, ws->clientRes.w, 0);
}
// Rect getScreenRect(WindowSettings* ws) {
// 	return rect(0, -ws->currentRes.h, ws->currentRes.w, 0);
// }

// void captureMouse(HWND windowHandle, bool t, Input* input) {
// 	if(t) {
// 		int w,h;
// 		Vec2i wPos;
// 		getWindowProperties(windowHandle, &w, &h, 0, 0, &wPos.x, &wPos.y);

// 		SetCursorPos(wPos.x + w/2, wPos.y + h/2);
// 		input->lastMousePos = getMousePos(windowHandle,false);
// 	}
// }

bool windowHasFocus(HWND windowHandle) {
	bool result = GetFocus() == windowHandle;
	return result;
}

bool windowSizeChanged(HWND windowHandle, WindowSettings* ws) {
	Vec2i cr;
	getWindowProperties(windowHandle, &cr.x, &cr.y, 0, 0, 0, 0);

	bool result = cr != ws->currentRes;
	return result;
}

int getSystemFontHeight(HWND windowHandle) {
	HDC dc = GetDC(windowHandle);

	TEXTMETRIC textMetric;
	GetTextMetrics(dc, &textMetric);

	return textMetric.tmHeight;
}

// MetaPlatformFunction();
uint getTicks() {
    uint result = GetTickCount();

    return result;
}

inline __int64 getCycleStamp() {
	return __rdtsc();
}


struct Timer {
	double frequency;
	LARGE_INTEGER timeStamp;

	double dt;
};

void timerInit(Timer* timer) {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency); 

	timer->frequency = (double)frequency.QuadPart;
}

void timerStart(Timer* timer) {
	QueryPerformanceCounter(&timer->timeStamp);
}

double timerStop(Timer* timer) {	
	LARGE_INTEGER newTimeStamp;
	QueryPerformanceCounter(&newTimeStamp);

	timer->dt = newTimeStamp.QuadPart - timer->timeStamp.QuadPart;

	// In seconds.
	timer->dt /= timer->frequency;

	return timer->dt;
}

double timerUpdate(Timer* timer) {	
	double time = timerStop(timer);
	timerStart(timer);

	return time;
}


void shellExecute(char* command) {
	system(command);
}

bool windowIsMinimized(HWND windowHandle) {	
	return IsIconic(windowHandle);
}

bool windowIsMaximized(HWND windowHandle) {	
	WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT)};
	GetWindowPlacement(windowHandle, &placement);
	if(placement.showCmd == SW_SHOWMAXIMIZED) return true;
	else return false;
}

// Command line execution: "cmd.exe /c start"
void shellExecuteNoWindow(char* command, bool wait = true) {
	STARTUPINFO si = {};
	PROCESS_INFORMATION pi = {};
	si.cb = sizeof(si);

	if (CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		if(wait) {
		    WaitForSingleObject(pi.hProcess, INFINITE);
		}
	    CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);
	}
}

// MetaPlatformFunction();
void sleep(int milliseconds) {
    Sleep(milliseconds);
}

void folderExistsCreate(char* path) {
	bool folderExists = PathFileExists(path);
	if(!folderExists) {
		CreateDirectory(path, 0);
	}
}

int getFolderFileCount(char* path) {
	int fileCount = 0;

	WIN32_FIND_DATA findData; 
	HANDLE folderHandle = FindFirstFile(path, &findData);
	if(INVALID_HANDLE_VALUE != folderHandle) {
		while(FindNextFile(folderHandle, &findData)) {
			char* fileName = findData.cFileName;
			if(strLen(fileName) <= 2) continue; // Skip "..".

			fileCount++;
		}
	}

	return fileCount;
}



struct FolderSearchData {
	WIN32_FIND_DATA findData;
	HANDLE folderHandle;

	char* fileName;
};

bool folderSearchStart(FolderSearchData* fd, char* folder) {	
	// Remember, for searching folder add "*" at the end of path

	fd->folderHandle = FindFirstFile(folder, &fd->findData);

	if(fd->folderHandle != INVALID_HANDLE_VALUE) return true;
	else return false;
}

bool folderSearchNextFile(FolderSearchData* fd) {
	if(FindNextFile(fd->folderHandle, &fd->findData) == 0) return false;

	fd->fileName = fd->findData.cFileName;
	
	return true;
}

#include "psapi.h"

void getMemoryUsage() {
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

	SIZE_T commited = pmc.PrivateUsage;
	SIZE_T workingSet = pmc.WorkingSetSize;

	printf("%f %f\n", commited/1024.0f/1024.0f, workingSet/1024.0f/1024.0f);
}


void updateWindowFrameData(SystemData* sd, WindowSettings* ws) {
	#ifdef ENABLE_CUSTOM_WINDOW_FRAME

	Vec2 bDim = vec2(sd->titleHeight);

	sd->titleHeight = sd->normalTitleHeight;
	sd->borderSize = sd->normalBorderSize;
	sd->visualBorderSize = sd->normalVisualBorderSize;

	if(sd->maximized) {
		sd->borderSize = 0;
		sd->visualBorderSize = 0;
	}

	if(ws->fullscreen) {
		sd->borderSize = 0;
		sd->visualBorderSize = 0;
		sd->titleHeight = 0;
	}

	Vec2i res = ws->currentRes;

	ws->windowRes = res;
	ws->windowRect = rect(0, -ws->windowRes.h, ws->windowRes.w, 0);

	Rect wr = ws->windowRect;
	ws->clientRect = rect(wr.min + vec2(sd->visualBorderSize), wr.max - vec2(sd->visualBorderSize, sd->titleHeight+sd->visualBorderSize));
	ws->clientRes = vec2i(roundInt(rectW(ws->clientRect)), roundInt(rectH(ws->clientRect)));

	float vbs = sd->visualBorderSize;
	ws->titleRect = rect(vbs, -sd->titleHeight - vbs, rectW(ws->windowRect)-vbs, -vbs);

	float bm = sd->buttonMargin*2;

	Rect titleRectWithSepLine = ws->titleRect;
	titleRectWithSepLine.bottom += 1;

	Vec2 p = vec2(titleRectWithSepLine.right - bDim.w/2, rectCen(titleRectWithSepLine).y);
	sd->rClose = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;
	sd->rMaximize = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;
	sd->rMinimize = rectCenDim(p, bDim - bm); p.x -= bDim.w - bm/2;

	if(ws->fullscreen) {
		sd->rClose    = rect(-1,-1,-1,-1);
		sd->rMaximize = rect(-1,-1,-1,-1);
		sd->rMinimize = rect(-1,-1,-1,-1);
	}

	ws->windowRectBL = rect(0, 0, res.w, res.h);

	ws->clientRectBL = ws->windowRectBL;
	ws->clientRectBL.min += vec2(sd->visualBorderSize);
	ws->clientRectBL.max -= vec2(sd->visualBorderSize,sd->visualBorderSize+sd->titleHeight);

	if(sd->maximized) {
		float sbs = ws->styleBorderSize;
		ws->windowRectBL = rect(sbs, sbs, sbs+res.w, sbs+res.h);
	}

	#else 

	Vec2i res = ws->currentRes;
	ws->clientRes = res;
	ws->windowRes = res;

	#endif
}

struct DialogData {
	char* type;

	bool active;
	bool finished;

	char* filePath;
	int filePathSize;

	char* initialDir;
	char* filter;
	int filterIndex;
	char* defaultExtension;

	bool saveMode;

	HWND windowHandle;

	char* result;
	bool error;
};

DWORD WINAPI openDialogProc(LPVOID data) {
	DialogData* dd = (DialogData*)data;

	dd->active = true;

	bool saveMode = dd->saveMode;

	OPENFILENAME ofn = {};

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = dd->windowHandle;
	ofn.lpstrFile = dd->filePath;
	ofn.nMaxFile = dd->filePathSize;

	ofn.lpstrInitialDir = dd->initialDir;

	ofn.lpstrFilter = dd->filter;
	ofn.nFilterIndex = dd->filterIndex;
	ofn.lpstrDefExt = dd->defaultExtension;

	ofn.Flags = OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;

	if(saveMode) ofn.Flags |= OFN_OVERWRITEPROMPT;
	else ofn.Flags |= OFN_FILEMUSTEXIST;

	int result;
	if(saveMode) result = GetSaveFileName(&ofn);	
	else result = GetOpenFileName(&ofn);


	if(result > 0) {
		dd->error = false;
		dd->result = ofn.lpstrFile;
	} else {
		dd->error = true;
	}

	dd->active = false;
	dd->finished = true;

	return 0;
}