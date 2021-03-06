
//
// Layout.
//

#define For_Layout(layout) \
for(Layout* l = layout->list; l != 0; l = l->next)

struct Layout {
	Vec2i align;
	Rect r;

	Layout* next;
	Layout* list;

	Vec2 minDim;
	Vec2 maxDim;
	Vec2 dim;

	Vec2 finalDim;

	Vec2 padding;
	Vec2 borderPadding;
	bool vAxis;
};

Layout* layoutAlloc(Layout node) {
	Layout* newNode = getTStruct(Layout);
	*newNode = node;

	return newNode;
}

Layout layout(Vec2 dim, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	Layout node = {};
	node.dim = dim;
	node.align = align;
	node.vAxis = vAxis;
	node.borderPadding = borderPadding;
	node.padding = padding;

	return node;
}

Layout layout(Rect region, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	Layout node = {};
	node.r = region;
	node.align = align;
	node.vAxis = vAxis;
	node.borderPadding = borderPadding;
	node.padding = padding;

	return node;
}
void layout(Layout* node, Rect region, bool vAxis = false, Vec2i align = vec2i(0,0), Vec2 padding = vec2(0,0), Vec2 borderPadding = vec2(0,0)) {
	*node = layout(region, vAxis, align, padding, borderPadding);
}

Rect layoutGetRect(Layout* node) {
	return rectExpand(node->r, -node->borderPadding*2);
}

Vec2 layoutGetDim(Layout* node) {
	return rectDim(layoutGetRect(node));
}

Rect layoutInc(Layout** node) {
	if((*node)) {
		Rect r = (*node)->r;
		(*node) = (*node)->next;
		return r;
	}
	return rect(0,0,0,0);
}

void layoutAdd(Layout* node, Layout* newNode, bool addEnd = true) {
	Layout* list = node->list;

	if(list == 0) node->list = newNode;
	else {
		if(addEnd) {
			while(list->next != 0) list = list->next;
			list->next = newNode;
		} else {
			newNode->next = list;
			node->list = newNode;
		}
	}
}

Layout* layoutAdd(Layout* node, Layout nn, bool addEnd = true) {
	Layout* newNode = getTStruct(Layout);
	*newNode = nn;

	layoutAdd(node, newNode, addEnd);
	return newNode;
}

void layoutCalcSizes(Layout* mainNode) {
	Layout* n;
	bool vAxis = mainNode->vAxis;
	Rect mainRect = layoutGetRect(mainNode);

	if(rectZero(mainRect)) return;

	int size = 0;
	n = mainNode->list;
	while(n != 0) {
		size++;
		n = n->next;
	}

	float dim = vAxis==0? rectW(mainRect) : rectH(mainRect);
	float dim2 = vAxis==1? rectW(mainRect) : rectH(mainRect);
	float offset = mainNode->padding.e[vAxis];
	float totalWidth = dim - offset*(size-1);

	float dynamicSum = 0;
	int flowCount = 0;
	float staticSum = 0;
	int staticCount = 0;

	float widthWithoutFloats = 0;

	n = mainNode->list;
	while(n != 0) {
		float val = n->dim.e[vAxis];
		
		if(val < 0) {}
			else if(val == 0) flowCount++;
		else if(val <= 1) widthWithoutFloats += val*totalWidth;
		else widthWithoutFloats += val;

		val = n->dim.e[(vAxis+1)%2];
		if(val == 0) n->dim.e[(vAxis+1)%2] = dim2;
		else if(val <= 1) n->dim.e[(vAxis+1)%2] = val * dim2;

		n = n->next;
	}


	float flowVal = flowCount>0 ? (totalWidth-widthWithoutFloats)/flowCount : 0;
	flowVal = clampMin(flowVal, 0);
	n = mainNode->list;
	while(n != 0) {
		n->finalDim = n->dim;

		float val = n->dim.e[vAxis];

		if(val < 0) n->finalDim.e[vAxis] = 0;
		else if(val == 0) n->finalDim.e[vAxis] = flowVal;
		else if(val <= 1) n->finalDim.e[vAxis] = val*totalWidth;

		clampMin(&n->finalDim.e[vAxis], 0);

		if(n->minDim.x != 0) clampMin(&n->finalDim.x, n->minDim.x);
		if(n->maxDim.x != 0) clampMax(&n->finalDim.x, n->maxDim.x);
		if(n->minDim.y != 0) clampMin(&n->finalDim.y, n->minDim.y);
		if(n->maxDim.y != 0) clampMax(&n->finalDim.y, n->maxDim.y);

		n = n->next;
	}

}

void layoutCalcRects(Layout* mainNode) {
	Rect mainRect = layoutGetRect(mainNode);
	if(rectZero(mainRect)) return;

	bool vAxis = mainNode->vAxis;
	Layout* node;

	Vec2 boundingDim = vec2(0,0);
	node = mainNode->list;
	while(node != 0) {
		boundingDim.e[vAxis] += node->finalDim.e[vAxis] + mainNode->padding.e[vAxis];
		boundingDim.e[(vAxis+1)%2] = max(boundingDim.e[(vAxis+1)%2], node->finalDim.e[(vAxis+1)%2]);

		node = node->next;
	}
	boundingDim.e[vAxis] -= mainNode->padding.e[vAxis];

	Vec2i align = mainNode->align;
	Vec2 currentPos = rectAlign(mainRect, align);

	if(vAxis == false) {
		currentPos.x -= boundingDim.x * (align.x+1)/2;
		currentPos.y += boundingDim.y * (-align.y)/2;

		node = mainNode->list;
		while(node != 0) {
			Vec2 p = currentPos;
			p.y -= node->finalDim.y/2;

			node->r = rectBLDim(p, node->finalDim);
			currentPos.x += node->finalDim.x + mainNode->padding.x;

			node = node->next;
		}
	} else {
		currentPos.y -= boundingDim.y * (align.y-1)/2;
		currentPos.x += boundingDim.x * (-align.x)/2;

		node = mainNode->list;
		while(node != 0) {
			Vec2 p = currentPos;
			p.x -= node->finalDim.x/2;

			node->r = rectTLDim(p, node->finalDim);
			currentPos.y -= node->finalDim.y + mainNode->padding.y;

			node = node->next;
		}
	}

}

void layoutCalc(Layout* mainNode, bool recursive = true) {
	layoutCalcSizes(mainNode);
	layoutCalcRects(mainNode);

	if(recursive) {
		Layout* node = mainNode->list;
		while(node != 0) {
			if(node->list != 0) {
				layoutCalc(node, true);
			}

			node = node->next;
		}
	}
}

Layout* layoutQuickRow(Layout* node, Rect region, float s1, float s2 = -1, float s3 = -1, float s4 = -1) {
	node->r = region;
	node->list = 0;
	layoutAdd(node, layout(!node->vAxis ? vec2(s1,0) : vec2(0,s1)));
	if(s2 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s2,0) : vec2(0,s2)));
	if(s3 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s3,0) : vec2(0,s3)));
	if(s4 != -1) layoutAdd(node, layout(!node->vAxis ? vec2(s4,0) : vec2(0,s4)));

	layoutCalc(node);

	return node->list;
}

Layout* layoutQuickRowArray(Layout* node, Rect region, float* s, float count) {
	node->r = region;
	node->list = 0;

	for(int i = 0; i < count; i++) {
		layoutAdd(node, layout(!node->vAxis ? vec2(s[i],0) : vec2(0,s[i])));
	}

	layoutCalc(node);

	return node->list;
}





enum GuiFocus {
	Gui_Focus_MLeft = 0,
	Gui_Focus_MRight,
	Gui_Focus_MMiddle,
	Gui_Focus_MWheel,
	Gui_Focus_MPos,
	Gui_Focus_ArrowKeys,

	Gui_Focus_Size,
};

struct TextEditVars {
	int cursorIndex;
	int markerIndex;
	Vec2 scrollOffset;

	bool cursorChanged;

	float dt;
	float cursorTimer;

	bool wordSelectionMode;
	int wordSelectionStartIndex;
};

struct BoxSettings {
	Vec4 color;
	float roundedCorner;
	Vec4 borderColor;
	float borderSize; // Not used yet.
	Vec2 padding;

	float vertGradientOffset;
};

BoxSettings boxSettings(Vec4 color, float roundedCorner, Vec4 borderColor) {
	return {color, roundedCorner, borderColor};
}
BoxSettings boxSettings(Vec4 color, float roundedCorner) {
	return {color, roundedCorner, 0};
}
BoxSettings boxSettings(Vec4 color) {
	return {color, 0};
}
BoxSettings boxSettings() {
	return {0};
}

struct CheckBoxSettings {
	BoxSettings boxSettings;
	Vec4 color;
	float sizeMod;
};

CheckBoxSettings checkBoxSettings(BoxSettings boxSettings, Vec4 color, float sizeMod) {
	return {boxSettings, color, sizeMod};
}



struct TextBoxSettings {
	TextSettings textSettings;
	BoxSettings boxSettings;
	float sideAlignPadding;
};

TextBoxSettings textBoxSettings(TextSettings textSettings, BoxSettings boxSettings, float padding) {
	return {textSettings, boxSettings, padding};
}

TextBoxSettings textBoxSettings(TextSettings textSettings, BoxSettings boxSettings) {
	return {textSettings, boxSettings, 0};
}

enum {
	ESETTINGS_WRAPPING = 0,
	ESETTINGS_SINGLE_LINE,
	ESETTINGS_START_RIGHT,
};
struct TextEditSettings {
	TextBoxSettings textBoxSettings;
	Vec4 defaultTextColor;

	char* textBuffer;

	int flags;

	float cursorWidth;
	float cursorHeightMod;

	char* defaultText;

	Vec4 colorSelection;
	Vec4 colorCursor;
	float cursorFlashingSpeed;

	float textOffset;
};

struct SliderSettings {
	TextBoxSettings textBoxSettings;

	float size;
	float minSize;
	float lineWidth;
	float rounding;
	float heightOffset;

	Vec4 color;
	Vec4 lineColor;

	int mouseWheelModInt;
	float mouseWheelModFloat;

	bool useDefaultValue;
	float defaultvalue;
	
	bool applyAfter;

	Vec4 borderColor;
};

SliderSettings sliderSettings(TextBoxSettings textBoxSettings, float size, float minSize, float lineWidth, float rounding, float heightOffset, Vec4 color, Vec4 lineColor) {
	return {textBoxSettings, size, minSize, lineWidth, rounding, heightOffset, color, lineColor, 1,1,false,0};
}

enum {
	SCROLL_BACKGROUND = 0,
	SCROLL_SLIDER,
	SCROLL_MOUSE_WHEEL,
	SCROLL_DYNAMIC_HEIGHT,
};

struct ScrollRegionSettings {
	BoxSettings boxSettings;

	int flags;

	Vec2 border;
	float scrollBarWidth;
	Vec2 sliderMargin;
	float sliderRounding;

	float sliderSize;
	float sliderSizeMin;

	float scrollAmount;

	Vec4 sliderColor;
	Vec4 scrollBarColor;
};

ScrollRegionSettings scrollRegionSettings(BoxSettings boxSettings, int flags, Vec2 border, float scrollBarWidth, Vec2 sliderMargin, float sliderRounding, float sliderSize, float sliderSizeMin, float scrollAmount, Vec4 sliderColor, Vec4 scrollBarColor) {

	ScrollRegionSettings s = {};
	s.boxSettings = boxSettings;
	s.flags = flags;
	s.border = border;
	s.scrollBarWidth = scrollBarWidth;
	s.sliderMargin = sliderMargin;
	s.sliderRounding = sliderRounding;
	s.sliderSize = sliderSize;
	s.sliderSizeMin = sliderSizeMin;
	s.scrollAmount = scrollAmount;
	s.sliderColor = sliderColor;
	s.scrollBarColor = scrollBarColor;
	return s;
}

enum LayoutMod {
	LAYOUT_MOD_SMALL = 0,
	LAYOUT_MOD_MEDIUM,
	LAYOUT_MOD_LARGE,
	LAYOUT_MOD_SIZE,

	LAYOUT_MOD_CUSTOM,
};

#define LAYOUT_COLUMN_COUNT_MAX 10

struct LayoutData {
	Rect region;
	Vec2 pos;

	float lineHeight;
	Vec2 padding;

	int columnCount;
	int columnIndex;
	float columns[LAYOUT_COLUMN_COUNT_MAX];

	union {
		struct {
			float lineHeightSmall;
			float lineHeightMedium;
			float lineHeightLarge;

			Vec2 paddingSmall;
			Vec2 paddingMedium;
			Vec2 paddingLarge;
		};
		
		struct {
			float lineHeights[LAYOUT_MOD_SIZE];
			Vec2 paddings[LAYOUT_MOD_SIZE];
		};
	};
};

enum Popup_Type {
	POPUP_TYPE_COMBO_BOX = 0,
	POPUP_TYPE_OTHER,
};

struct PopupData {
	int type;
	int id;
	char name[30];
	Rect rSource;
	Rect r;
	Rect rCheck;

	Vec2 p;
	float width;

	BoxSettings settings;
	Vec2 border;
};

struct ComboBoxData {
	int index;
	char** strings;
	int count;
	bool finished;
};


struct NewGui {
	int id;

	int activeId;
	int gotActiveId;
	int wasActiveId;
	
	int hotId[Gui_Focus_Size];
	int contenderId[Gui_Focus_Size];
	int contenderIdZ[Gui_Focus_Size];

	// Used for text edits right now.
	int activeSignalId;

	int storedIds[20];
	int storedIdIndex;
	int storedIdCount;

	Input* input;
	WindowSettings* windowSettings;
	float dt;

	Vec4 colorModHot;
	Vec4 colorModActive;

	int menuId;
	bool menuActive;

	bool forceNoClear;

	bool disable;
	bool setInactive;

	bool mouseInClient;

	int test;

	// Temp vars for convenience.

	Vec2 mouseAnchor, mouseAnchor2;
	int mode;

	char editText[100];
	int editInt;
	float editFloat;
	int editMaxSize;

	int sliderId;

	TextEditVars editVars;

	PopupData popupStack[10];
	int popupStackCount;
	int popupStartId;

	int* comboBoxIndex;
	ComboBoxData comboBoxData;

	//

	TextSettings textSettings;
	BoxSettings boxSettings;
	TextBoxSettings textBoxSettings;
	TextBoxSettings buttonSettings;
	TextEditSettings editSettings;
	SliderSettings sliderSettings;
	ScrollRegionSettings scrollSettings;
	BoxSettings popupSettings;
	TextBoxSettings comboBoxSettings;
	CheckBoxSettings checkBoxSettings;

	int zLevel;

	Rect scissor;
	Rect scissorStack[10];
	int scissorStackIndex;

	LayoutData* ld;
	LayoutData layoutStack[10];
	int layoutStackIndex;

	LPCSTR currentCursor;
};


LayoutData* newGuiLayoutPush(NewGui* gui, LayoutData ld);
LayoutData* newGuiLayoutPop(NewGui* gui, bool updateY = true);
void newGuiBegin(NewGui* gui, Input* input, WindowSettings* ws, float dt = 0, bool mouseInClient = true) {
	int voidId = 0;

	gui->id = 1;
	if(!gui->forceNoClear) {
		gui->gotActiveId = voidId;
		gui->wasActiveId = voidId;
	}

	gui->forceNoClear = false;

	for(int i = 0; i < Gui_Focus_Size; i++) {
		gui->contenderId[i] = voidId;
		gui->contenderIdZ[i] = voidId;
	}

	if(gui->disable) {
		for(int i = 0; i < Gui_Focus_Size; i++) gui->hotId[i] = voidId;
	}

	// if(gui->activeId != 0) {
	// 	for(int i = 0; i < Gui_Focus_Size; i++) gui->hotId[i] = voidId;
	// }

	gui->zLevel = 0;

	gui->colorModHot = vec4(0.08f, 0);
	gui->colorModActive = vec4(0.17f, 0);

	gui->input = input;
	gui->windowSettings = ws;
	gui->dt = dt;

	gui->mouseInClient = mouseInClient;

	gui->currentCursor = IDC_ARROW;

	gui->scissor = rectCenDim(0,0,10000000,10000000);
	gui->scissorStack[0] = gui->scissor;
	gui->scissorStackIndex = 0;
	gui->layoutStackIndex = 0;

	scissorState();

	// LayoutData ld = {rect(0,0,0,0), 0, 0, 0};
	// newGuiLayoutPush(gui, ld);
}

void newGuiPopupSetup(NewGui* gui);
void newGuiUpdateComboBoxPopups(NewGui* gui);
void newGuiEnd(NewGui* gui) {

	if(gui->currentCursor != IDC_ARROW && gui->mouseInClient) {
		setCursor(gui->windowSettings, gui->currentCursor);
	}

	// newGuiLayoutPop(gui);

	for(int i = 0; i < Gui_Focus_Size; i++) {
		gui->hotId[i] = gui->contenderId[i];
	}

	if(!gui->mouseInClient) {
		for(int i = 0; i < Gui_Focus_Size; i++) {
			gui->hotId[i] = 0;
		}
	}

	scissorState(false);
}

int newGuiAdvanceId(NewGui* gui) {
	return gui->id++;
}

// "Increment" doesn't make sense anymore, but too lazy to change the name.
int newGuiIncrementId(NewGui* gui) {
	if(gui->storedIdIndex < gui->storedIdCount) {
		return gui->storedIds[gui->storedIdIndex++];

	} else {
		return newGuiAdvanceId(gui);
	}
}

void newGuiStoreIds(NewGui* gui, int count) {
	gui->storedIdIndex = 0;
	gui->storedIdCount = count;
	for(int i = 0; i < count; i++) {
		gui->storedIds[i] = newGuiAdvanceId(gui);
	}
}

void newGuiClearStoredIds(NewGui* gui) {
	gui->storedIdIndex = 0;
	gui->storedIdCount = 0;
}

int newGuiCurrentId(NewGui* gui) {
	if(gui->storedIdIndex < gui->storedIdCount) {
		return gui->storedIds[gui->storedIdIndex-1];
	} else {
		return gui->id-1;
	}
}

bool newGuiIsHot(NewGui* gui, int id = 0, int focus = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->hotId[focus];
}

bool newGuiIsActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->activeId;
}

bool newGuiIsHotOrActive(NewGui* gui, int id = 0, int focus = 0) {
	return newGuiIsHot(gui, id, focus) || newGuiIsActive(gui, id);
}

bool newGuiGotActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->gotActiveId;
}

bool newGuiWasActive(NewGui* gui, int id = 0) {
	return (id==0?newGuiCurrentId(gui):id) == gui->wasActiveId;
}

bool newGuiIsWasHotOrActive(NewGui* gui, int id = 0, int focus = 0) {
	return newGuiIsHot(gui, id, focus) || newGuiIsActive(gui, id) || newGuiWasActive(gui, id);
}

void newGuiClearActive(NewGui* gui) {
	gui->activeId = 0;
}

void newGuiSetNotActive(NewGui* gui, int id) {
	newGuiClearActive(gui);
	gui->wasActiveId = id;
}

bool newGuiSomeoneActive(NewGui* gui) {
	return gui->activeId != 0;
}

void newGuiSetNotActiveWhenActive(NewGui* gui, int id) {
	if(newGuiIsActive(gui, id)) newGuiClearActive(gui);
}

void newGuiSetActive(NewGui* gui, int id, bool input, int focus = 0) {
	bool setActive = false;

	if(id == gui->activeSignalId) {
		setActive = true;
		gui->activeSignalId = 0;

	} else {
		if(!newGuiIsActive(gui, id)) {
			if(input && newGuiIsHot(gui, id, focus)) {
				if(newGuiSomeoneActive(gui)) { 
					gui->activeSignalId = id;
					
				} else {
					setActive = true;
				}
			}
		}
	}

	if(setActive) {
		gui->activeId = id;
		gui->gotActiveId = id;
	}
}

bool newGuiFocusCanBeHot(NewGui* gui, int focus) {
	// Input* input = gui->input;
	// bool result = true;
	// switch(focus) {
	// 	case Gui_Focus_MLeft:   if(input->mouseButtonDown[0]) result = false; break;
	// 	case Gui_Focus_MRight:  if(input->mouseButtonDown[1]) result = false; break;
	// 	case Gui_Focus_MMiddle: if(input->mouseButtonDown[2]) result = false; break;
	// }
	
	bool result = true;
	return result;
}

void newGuiSetHot(NewGui* gui, int id, float z, int focus = 0) {
	// if(!newGuiSomeoneActive(gui) && newGuiFocusCanBeHot(gui, focus)) {
	if(newGuiFocusCanBeHot(gui, focus) && !gui->setInactive) {
		if(z > gui->contenderIdZ[focus]) {
			gui->contenderId[focus] = id;
			gui->contenderIdZ[focus] = z;
		} else {
			if(z == gui->contenderIdZ[focus]) {
				gui->contenderId[focus] = max(id, gui->contenderId[focus]);
			}
		}
	}
}
void newGuiSetHot(NewGui* gui, int id, int focus = 0) {
	return newGuiSetHot(gui, id, gui->zLevel, focus);
}

void newGuiSetHotAll(NewGui* gui, int id, float z) {
	for(int i = 0; i < Gui_Focus_Size; i++) {
		newGuiSetHot(gui, id, z, i);
	}
}

void newGuiSetHotAll(NewGui* gui, float z) {
	return newGuiSetHotAll(gui, newGuiIncrementId(gui), z);
}

void newGuiSetHotAllMouseOver(NewGui* gui, int id, Rect r, float z) {
	if(pointInRectEx(gui->input->mousePosNegative, r)) return newGuiSetHotAll(gui, id, z);
}

void newGuiSetHotAllMouseOver(NewGui* gui, Rect r, float z) {
	return newGuiSetHotAllMouseOver(gui, newGuiIncrementId(gui), r, z);
}

void newGuiSetHotMouseOver(NewGui* gui, int id, Vec2 mousePos, Rect r, float z, int focus = 0) {
	if(pointInRectEx(mousePos, r) && !rectEmpty(r)) {
		newGuiSetHot(gui, id, z, focus);
	}
}

void newGuiForceActive(NewGui* gui, int id) {
	gui->activeId = gui->menuId;
	gui->gotActiveId = gui->menuId;
	gui->forceNoClear = true;
}

int newGuiInputFromFocus(Input* input, int focus, bool press = true) {
	switch(focus) {
		case Gui_Focus_MLeft: return press?input->mouseButtonPressed[0]:input->mouseButtonReleased[0];
		case Gui_Focus_MRight: return press?input->mouseButtonPressed[1]:input->mouseButtonReleased[1];
		case Gui_Focus_MMiddle: return press?input->mouseButtonPressed[2]:input->mouseButtonReleased[2];

		case Gui_Focus_MWheel: return input->mouseWheel != 0;
		default: return -1;
	}
}

void newGuiSetCursor(NewGui* gui, LPCSTR cursorType) {
	gui->currentCursor = cursorType;
}


int newGuiButtonAction(NewGui* gui, int id, Rect r, float z, Vec2 mousePos, bool input, int focus = 0) {
	newGuiSetActive(gui, id, input, focus);
	if(newGuiIsActive(gui, id)) newGuiSetNotActive(gui, id);
	newGuiSetHotMouseOver(gui, id, mousePos, r, z, focus);

	return id;
}
int newGuiButtonAction(NewGui* gui, Rect r, float z, Vec2 mousePos, bool input, int focus = 0) {
	return newGuiButtonAction(gui, newGuiIncrementId(gui), r, z, mousePos, input, focus);
}
int newGuiButtonAction(NewGui* gui, int id, Rect r, float z, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	return newGuiButtonAction(gui, id, r, z, gui->input->mousePosNegative, input, focus);
}
int newGuiButtonAction(NewGui* gui, Rect r, float z, int focus = 0) {
	return newGuiButtonAction(gui, newGuiIncrementId(gui), r, z, focus);
}
int newGuiButtonAction(NewGui* gui, Rect r, float z, bool input, int focus = 0) {
	return newGuiButtonAction(gui, newGuiIncrementId(gui), r, z, gui->input->mousePosNegative, input, focus);
}

bool newGuiGoButtonAction(NewGui* gui, int id, Rect r, float z, int focus = 0) {
	id = newGuiButtonAction(gui, id, r, z, focus);
	bool gotActive = newGuiGotActive(gui, id);
	return gotActive;
}
bool newGuiGoButtonAction(NewGui* gui, Rect r, float z, int focus = 0) {
	return newGuiGoButtonAction(gui, newGuiIncrementId(gui), r, z, focus);
}
bool newGuiGoButtonAction(NewGui* gui, Rect r, float z, bool input, int focus = 0) {
	int id = newGuiButtonAction(gui, r, z, input, focus);
	bool gotActive = newGuiGotActive(gui, id);
	return gotActive;
}
bool newGuiGoButtonAction(NewGui* gui, Rect r) {
	return newGuiGoButtonAction(gui, newGuiIncrementId(gui), r, gui->zLevel, 0);
}

int newGuiDragAction(NewGui* gui, int id, Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0) {
	newGuiSetActive(gui, id, input, focus);
	if(newGuiIsActive(gui, id) && (inputRelease || gui->activeSignalId)) {
		newGuiSetNotActive(gui, id);
	}
	newGuiSetHotMouseOver(gui, id, mousePos, r, z, focus);

	return id;
}
int newGuiDragAction(NewGui* gui, Rect r, float z, Vec2 mousePos, bool input, bool inputRelease, int focus = 0) {
	return newGuiDragAction(gui, newGuiIncrementId(gui), r, z, mousePos, input, inputRelease, focus);
}
int newGuiDragAction(NewGui* gui, Rect r, float z, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);
	return newGuiDragAction(gui, newGuiIncrementId(gui), r, z, gui->input->mousePosNegative, input, inputRelease, focus);
}
int newGuiDragAction(NewGui* gui, bool isHot, int focus = 0) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);

	int id = newGuiIncrementId(gui);
	Vec2 mousePos = gui->input->mousePosNegative;

	newGuiSetActive(gui, id, input);
	if(newGuiIsActive(gui, id) && inputRelease) newGuiSetNotActive(gui, id);

	if(isHot) newGuiSetHot(gui, id, focus);
	
	return id;
}

int newGuiGoDragAction(NewGui* gui, Rect r, float z, bool input, bool inputRelease, int focus = 0, bool screenMouse = false) {
	Vec2 mousePos = screenMouse ? gui->input->mousePosNegativeScreen : gui->input->mousePosNegative;
	int id = newGuiDragAction(gui, r, z, mousePos, input, inputRelease, focus);
	if(newGuiGotActive(gui, id)) return 1;
	if(newGuiIsActive(gui, id)) return 2;
	if(newGuiWasActive(gui, id)) return 3;
	
	return 0;
}
int newGuiGoDragAction(NewGui* gui, Rect r, float z, int focus = 0, bool screenMouse = false) {
	bool input = newGuiInputFromFocus(gui->input, focus, true);
	bool inputRelease = newGuiInputFromFocus(gui->input, focus, false);
	return newGuiGoDragAction(gui, r, z, input, inputRelease, focus, screenMouse);
}

bool newGuiGoMousePosAction(NewGui* gui, Rect r, float z) {
	newGuiSetHotMouseOver(gui, newGuiIncrementId(gui), gui->input->mousePosNegative, r, z, Gui_Focus_MPos);
	bool hot = newGuiIsHot(gui, newGuiCurrentId(gui), Gui_Focus_MPos);
	return hot;
}


Vec4 newGuiHotActiveColorMod(bool isHot, bool isActive) {
	Vec4 colorMod = vec4(0,0,0,0);
	if(isHot) colorMod = vec4(0.08f,0); 
	if(isActive) colorMod = vec4(0.17f,0); 

	return colorMod;
}
Vec4 newGuiInactiveColorMod(bool inactive) {
	Vec4 colorMod = !inactive ? vec4(0,0) : vec4(-0.1f,0);
	return colorMod;
}
// We assume you got an id first before calling this.
Vec4 newGuiColorModId(NewGui* gui, int id, int focus = 0) {
	return newGuiHotActiveColorMod(newGuiIsHot(gui, id, focus), newGuiIsActive(gui, id));
}
Vec4 newGuiColorModBId(NewGui* gui, int id, int focus = 0) {
	return newGuiHotActiveColorMod(newGuiIsHot(gui, id, focus), newGuiGotActive(gui, id));
}
// Vec4 newGuiColorModPId(NewGui* gui, int id, int focus = 0) {
// 	return newGuiHotActiveColorMod(newGuiIsHot(gui, id, focus), newGuiGotActive(gui, id));
// }

Vec4 newGuiColorMod(NewGui* gui, int focus = 0) {
	return newGuiColorModId(gui, newGuiCurrentId(gui), focus);
}
Vec4 newGuiColorModB(NewGui* gui, int focus = 0) {
	return newGuiColorModBId(gui, newGuiCurrentId(gui), focus);
}
// Vec4 newGuiColorModP(NewGui* gui, int focus = 0) {
// 	return newGuiColorModPId(gui, newGuiCurrentId(gui), focus);
// }



// @GuiAutomation

LayoutData layoutData(Rect region, float lineHeightMedium, Vec2 paddingMedium) {
	LayoutData ld = {};
	ld.region = region;
	ld.pos = rectTL(region);
	ld.lineHeight = lineHeightMedium;
	ld.padding = paddingMedium;

	ld.lineHeightSmall = lineHeightMedium*0.75f;
	ld.lineHeightMedium = lineHeightMedium;
	ld.lineHeightLarge = lineHeightMedium*2.0f;

	ld.paddingSmall = vec2(0,0);
	ld.paddingMedium = paddingMedium;
	ld.paddingLarge = paddingMedium * 2.5f;

	return ld;
}

LayoutData* newGuiLayoutPush(NewGui* gui, LayoutData ld) {
	gui->layoutStackIndex++;
	gui->layoutStack[gui->layoutStackIndex] = ld;
	gui->ld = &gui->layoutStack[gui->layoutStackIndex];

	return gui->ld;
}
LayoutData* newGuiLayoutPush(NewGui* gui, Rect region) {
	LayoutData newLd = *gui->ld;
	newLd.region = region;
	newLd.pos = rectTL(newLd.region);

	return newGuiLayoutPush(gui, newLd);
}
LayoutData* newGuiLayoutPush(NewGui* gui) {
	return newGuiLayoutPush(gui, *gui->ld);
}

LayoutData* newGuiLayoutPop(NewGui* gui, bool updateY) {

	float lastRegionHeight = rectH(gui->ld->region);
	gui->layoutStackIndex--;
	gui->ld = &gui->layoutStack[gui->layoutStackIndex];

	if(updateY) {
		// gui->ld->pos.y -= lastRegionHeight + gui->ld->padding.y*2;
	}
	return gui->ld;
}

void newGuiLayoutNewLine(NewGui* gui, int flag = -1, float height = 0) {
	LayoutData* ld = gui->ld;

	float padding = ld->padding.y;
	if(flag != -1) {
		if(flag == LAYOUT_MOD_CUSTOM) padding = height;
		else padding = ld->paddings[flag].y;
	}

	ld->pos.y -= ld->lineHeight + padding;
	ld->pos.x = ld->region.left;

	ld->columnCount = 0;
}

void newGuiLayoutRowArray(NewGui* gui, float* columns, int count) {
	assert(count <= LAYOUT_COLUMN_COUNT_MAX);

	LayoutData* ld = gui->ld;
	ld->columnCount = count;
	ld->columnIndex = 0;

	Layout lay = layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(ld->padding.x, 0));

	Rect r = rectTLDim(ld->pos, vec2(rectW(ld->region), ld->lineHeight));
	Layout* node = layoutQuickRowArray(&lay, r, columns, count);

	for(int i = 0; i < count; i++) {
		ld->columns[i] = rectW(layoutInc(&node));
	}
}

void newGuiLayoutRow(NewGui* gui, float s0, float s1 = -1, float s2 = -1, float s3 = -1) {
	float cols[4];
	cols[0] = s0;
	int count = 1;
	if(s1 != -1) { cols[1] = s1; count = 2; }
	if(s2 != -1) { cols[2] = s2; count = 3; }
	if(s3 != -1) { cols[3] = s3; count = 4; }

	newGuiLayoutRowArray(gui, cols, count);
}

Rect newGuiLayoutGet(NewGui* gui, float width) {
	LayoutData* ld = gui->ld;
	Rect r = rectTLDim(ld->pos, vec2(width, ld->lineHeight));
	ld->pos.x += width + ld->padding.w;

	return r;
}
Rect newGuiLayoutGet(NewGui* gui) {
	LayoutData* ld = gui->ld;

	if(ld->columnCount > 0) {
		float width = ld->columns[ld->columnIndex++];

		Rect r = newGuiLayoutGet(gui, width);

		if(ld->columnIndex > ld->columnCount-1) {
			ld->columnCount = 0;
			newGuiLayoutNewLine(gui);
		}

		return r;
	}

	Rect r = newGuiLayoutGet(gui, rectW(gui->ld->region));
	newGuiLayoutNewLine(gui);

	return r;
}
Rect newGuiLayoutGetAll(NewGui* gui, float height) {
	LayoutData* ld = gui->ld;
	float oldLineHeight = ld->lineHeight;
	ld->lineHeight = height;

	Rect r = newGuiLayoutGet(gui);

	ld->lineHeight = oldLineHeight;
	return r;
}


void newGuiLayoutSetLineHeight(NewGui* gui, int flag, float height = 0) {
	LayoutData* ld = gui->ld;
	if(flag == LAYOUT_MOD_CUSTOM) ld->lineHeight = height;
	else ld->lineHeight = gui->ld->lineHeights[flag];
}	
void newGuiLayoutSetXPadding(NewGui* gui, int flag, float size = 0) {
	LayoutData* ld = gui->ld;
	if(flag == LAYOUT_MOD_CUSTOM) ld->padding.x = size;
	else ld->padding.x = gui->ld->paddings[flag].x;
}
void newGuiLayoutSetYPadding(NewGui* gui, int flag, float size = 0) {
	LayoutData* ld = gui->ld;
	if(flag == LAYOUT_MOD_CUSTOM) ld->padding.y = size;
	else ld->padding.y = gui->ld->paddings[flag].y;
}



void newGuiScissorPush(NewGui* gui, Rect scissor) {
	gui->scissor = rectIntersect(gui->scissor, scissor);
	gui->scissorStack[gui->scissorStackIndex+1] = gui->scissor;
	gui->scissorStackIndex++;

	scissorTestScreen(gui->scissor);
}
void newGuiScissorPop(NewGui* gui) {
	gui->scissorStackIndex--;
	gui->scissor = gui->scissorStack[gui->scissorStackIndex];

	scissorTestScreen(gui->scissor);
}

void newGuiLayoutScissorPush(NewGui* gui, LayoutData ld, Rect scissor) {
	newGuiScissorPush(gui, scissor);
	newGuiLayoutPush(gui, ld);
}
void newGuiLayoutScissorPush(NewGui* gui, Rect region, Rect scissor) {
	newGuiScissorPush(gui, scissor);
	newGuiLayoutPush(gui, region);
}

void newGuiLayoutScissorPop(NewGui* gui, bool updateY = true) {
	newGuiScissorPop(gui);
	newGuiLayoutPop(gui);
}

void newGuiPopupPush(NewGui* gui, PopupData data) {
	gui->popupStack[gui->popupStackCount++] = data;
}
void newGuiPopupPop(NewGui* gui) {
	gui->popupStackCount--;
}



enum {
	EDIT_MODE_CHAR = 0,
	EDIT_MODE_INT,
	EDIT_MODE_FLOAT,
};

void textEditBox(char* text, int textMaxSize, Font* font, Rect textRect, Input* input, Vec2i align, TextEditSettings tes, TextEditVars* tev);
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, void* var, int mode, TextEditSettings editSettings, TextEditVars* editVars, int maxTextSize, bool doubleClick = false) {
	Input* input = gui->input;

	if(maxTextSize == 0) maxTextSize = arrayCount(gui->editText);
	maxTextSize = min(maxTextSize, arrayCount(gui->editText));

	bool leftMouse = input->mouseButtonPressed[0] && !pointInRectEx(input->mousePosNegative, textRect);
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool escape = input->keysPressed[KEYCODE_ESCAPE];

	bool releaseEvent = leftMouse || enter || escape;

	int event = newGuiGoDragAction(gui, textRect, z, doubleClick?input->doubleClick:input->mouseButtonPressed[0], releaseEvent, Gui_Focus_MLeft);

	if(event == 1) {
		gui->editVars.scrollOffset = vec2(0,0);
		if(mode == EDIT_MODE_CHAR)      strCpy(gui->editText, (char*)var);
		else if(mode == EDIT_MODE_INT) strCpy(gui->editText, fillString("%i", *((int*)var)));
		else if(mode == EDIT_MODE_FLOAT) strCpy(gui->editText, fillString("%f", *((float*)var)));
		if(flagGet(editSettings.flags, ESETTINGS_START_RIGHT)) {
			editVars->cursorIndex = editVars->markerIndex = strlen(gui->editText);
		}
	}

	if(event == 3 && (leftMouse || enter)) {
		if(mode == 0)      strCpy((char*)var, gui->editText);
		else if(mode == 1) *((int*)var) = strToInt(gui->editText);
		else if(mode == 2) *((float*)var) = strToFloat(gui->editText);
	}

	if(event == 1 || event == 2) {
		if(event == 1) editVars->cursorTimer = 0;
		textEditBox(gui->editText, maxTextSize, editSettings.textBoxSettings.textSettings.font, textRect, input, vec2i(-1,1), editSettings, editVars);
	}

	if(event == 3 && (escape)) event = 4;

	return event;
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, char* text, TextEditSettings editSettings, int maxTextSize) {
	return newGuiGoTextEdit(gui, textRect, z, text, EDIT_MODE_CHAR, editSettings, &gui->editVars, maxTextSize);
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, int* number, TextEditSettings editSettings) {
	return newGuiGoTextEdit(gui, textRect, z, number, EDIT_MODE_INT, editSettings, &gui->editVars, 0);
}
int newGuiGoTextEdit(NewGui* gui, Rect textRect, float z, float* number, TextEditSettings editSettings) {
	return newGuiGoTextEdit(gui, textRect, z, number, EDIT_MODE_FLOAT, editSettings, &gui->editVars, 0);
}


Rect newGuiCalcSlider(float value, Rect br, float size, float min, float max, bool vertical) {
	if(vertical) {
		float sliderPos = mapRange(value, min, max, br.min.x + size/2, br.max.x - size/2);
		Rect slider = rectCenDim(sliderPos, rectCen(br).y, size, rectH(br));
		return slider;
	} else {
		float sliderPos = mapRange(value, min, max, br.min.y + size/2, br.max.y - size/2);
		Rect slider = rectCenDim(rectCen(br).x, sliderPos, rectW(br), size);
		return slider;
	}
}

float newGuiSliderGetValue(Vec2 sliderCenter, Rect br, float size, float min, float max, bool vertical) {
	if(vertical) {
		float sliderValue = mapRangeClamp(sliderCenter.x, br.min.x + size/2, br.max.x - size/2, min, max);
		return sliderValue;
	} else {
		float sliderValue = mapRangeClamp(sliderCenter.y, br.min.y + size/2, br.max.y - size/2, min, max);
		return sliderValue;
	}
}

void newGuiDiv(float width, float* c, int size, float offset = 0) {
	float dynamicSum = 0;
	int flowCount = 0;
	float staticSum = 0;
	int staticCount = 0;
	for(int i = 0; i < size; i++) {
		float val = c[i];
		
		if(val == 0) flowCount++; 			 // float element
		else if(val <= 1) dynamicSum += val; // dynamic element
		else { 								 // static element
			staticSum += val;
			staticCount++;
		}
	}

	if(flowCount) {
		float flowVal = abs(dynamicSum-1)/(float)flowCount;
		for(int i = 0; i < size; i++)
			if(c[i] == 0) c[i] = flowVal;
	}

	float totalWidth = width - staticSum - offset*(size-1);
	float sum = 0;
	for(int i = 0; i < size; i++) {
		if(sum > width) {
			c[i] = 0;
			continue;
		}

		float val = c[i];
		if(val <= 1) {
			if(totalWidth > 0) c[i] = val * totalWidth;
			else c[i] = 0;
		} else c[i] = val;

		float added = c[i] + offset;
		if(sum + added > width) {
			c[i] = width - sum;
			sum += added;
		}
		else sum += added;
	}
}

void newGuiRectsFromWidths(Rect r, float* widths, int size, Rect* rects, float offset = 0) {
	float yPos = rectCen(r).y;
	float h = rectH(r);
	float sum = 0;
	for(int i = 0; i < size; i++) {
		float xPos = r.left + sum + widths[i]/2;
		rects[i] = rectCenDim(xPos, yPos, widths[i], h);

		sum += widths[i] + offset;
	}
}

//

bool getRectScissor(Rect* scissor, Rect r) {
	*scissor = rectIntersect(*scissor, r);
	if(rectEmpty(*scissor) || 
	   rectW(*scissor) == 0 || rectH(*scissor) == 0) return false;
	return true;
}

bool testRectScissor(Rect scissor, Rect r) {
	bool result = getRectScissor(&scissor, r);
	return result;
}

Rect getRectScissor(Rect scissor, Rect r) {
	return rectIntersect(scissor, r);
}

Rect scissorTestIntersect(Rect scissor, Rect r) {
	scissorTestScreen(rectIntersect(scissor, r));
	return r;
}


void drawText(Rect r, char* text, Vec2i align, Rect scissor, TextSettings settings, float borderSize = 0) {
	Vec2 pos = rectCen(r) + (rectDim(r)/2) * vec2(align);

	scissorTestScreen(rectExpand(getRectScissor(r,scissor), -vec2(borderSize*2)));
	drawText(text, pos, align, settings);
	scissorTestScreen(scissor);
}

void drawBox(Rect r, Rect scissor, BoxSettings settings, bool round = true) {
	scissorTestScreen(scissor);

	if(round) r = rectRound(r);

	bool hasBorder = settings.borderColor.a != 0 ? true : false;
	int borderSize = 1;

	if(settings.color.a != 0) {
		Rect br = r;
		if(hasBorder) br = rectExpand(br, -vec2(borderSize*2));

		if(settings.vertGradientOffset) {
			glEnable(GL_STENCIL_TEST);

			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilMask(0xFF);

			drawRectRounded(br, settings.color, settings.roundedCorner);

			glStencilFunc(GL_EQUAL, 1, 0xFF);
			glStencilMask(0x00); 

			setSRGB(false);
			Vec4 off = vec4(settings.vertGradientOffset, 0);
			drawRectNewColoredH(r, settings.color - off, settings.color + off);
			setSRGB(true);

			glStencilMask(0xFF); 
			glDisable(GL_STENCIL_TEST);
		} else {
			drawRectRounded(br, settings.color, settings.roundedCorner);
		}
	}

	if(hasBorder) {
		glLineWidth(1);
		drawRectRoundedOutline(r, settings.borderColor, settings.roundedCorner);
	}
}

void drawTextBox(Rect r, char* text, Vec2i align, Rect scissor, TextBoxSettings settings) {
	// scissorTestScreen(scissor);
	drawBox(r, scissor, settings.boxSettings);

	if(align.x == -1) r.left += settings.sideAlignPadding;
	if(align.x == 1) r.right -= settings.sideAlignPadding;

	float borderSize = 0;
	if(settings.boxSettings.borderColor.a != 0) borderSize = 1;

	drawText(r, text, align, scissor, settings.textSettings, borderSize);
}

void drawTextEditBox(char* text, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {

	BoxSettings* boSettings = &editSettings.textBoxSettings.boxSettings;
	TextSettings* textSettings = &editSettings.textBoxSettings.textSettings;

	Vec2 startPos = rectL(textRect) + vec2(editSettings.textOffset,0);
	if(active) startPos += editVars.scrollOffset;

	drawBox(textRect, scissor, *boSettings);

	// scissorTestScreen(rectExpand(scissor, vec2(-1,-1)));
	scissorTestScreen(rectExpand(getRectScissor(textRect, scissor), vec2(-2,-2)));
	// scissorTestScreen(getRectScissor(textRect, scissor));

	if(active) text = editSettings.textBuffer;

	Vec2i align = vec2i(-1,0);
	if(active) {
		// Selection.
		drawTextSelection(text, textSettings->font, startPos, editVars.cursorIndex, editVars.markerIndex, editSettings.colorSelection, align);
	}

	if(!strEmpty(editSettings.defaultText) && strEmpty(text) && !active) {
		TextSettings defaultTextSettings = *textSettings;
		defaultTextSettings.color = editSettings.defaultTextColor;
		drawText(editSettings.defaultText, rectCen(textRect), vec2i(0,0), defaultTextSettings);
	}
	else 
		drawText(text, startPos, align, *textSettings);

	if(active) {
		// Cursor.
		Vec2 cPos = textIndexToPos(text, textSettings->font, startPos, editVars.cursorIndex, align);
		Rect cRect = rectCenDim(cPos, vec2(editSettings.cursorWidth, textSettings->font->height* editSettings.cursorHeightMod));

		if(editVars.cursorIndex == strLen(text) && editVars.cursorIndex == editVars.markerIndex) {
			cRect = rectTrans(cRect, vec2(2,0)); // Small offset for looks.
		}

		Vec4 cCursor = editSettings.colorCursor;
		cCursor.a = (cos(editVars.cursorTimer) + 1)/2.0f;

		drawBox(cRect, scissor, boxSettings(cCursor));
	}

	scissorTestScreen(scissor);
}

void drawTextEditBox(void* val, int mode, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(mode == EDIT_MODE_INT ? fillString("%i", Void_Dref(int, val)) : fillString("%f", Void_Dref(float, val)), textRect, active, scissor, editVars, editSettings);
}
void drawTextEditBox(int number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(fillString("%i", number), textRect, active, scissor, editVars, editSettings);
}
void drawTextEditBox(float number, Rect textRect, bool active, Rect scissor, TextEditVars editVars, TextEditSettings editSettings) {
	return drawTextEditBox(fillString("%f", number), textRect, active, scissor, editVars, editSettings);
}


enum {
	SLIDER_TYPE_INT = 0,
	SLIDER_TYPE_FLOAT,
};

void drawSlider(void* val, bool type, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	scissor = getRectScissor(br, scissor);

	BoxSettings* boSettings = &settings.textBoxSettings.boxSettings;
	TextSettings* textSettings = &settings.textBoxSettings.textSettings;

	// rectExpand(&sr, vec2(0,-settings.heightOffset*2));
	rectExpand(&sr, vec2(-settings.heightOffset*2,-settings.heightOffset*2));

	drawBox(br, scissor, *boSettings);
	if(settings.lineColor.a > 0 && settings.lineWidth > 0) {
		glLineWidth(settings.lineWidth);
		drawLine(rectL(br), rectR(br), settings.lineColor);
	}

	BoxSettings sliderBoxSettings = boxSettings(settings.color, settings.rounding, settings.borderColor);
	int border = 1;
	scissor = rectExpand(scissor, vec2(-border*2));

	drawBox(sr, scissor, sliderBoxSettings);

	char* text = type == SLIDER_TYPE_FLOAT ? fillString("%f", *((float*)val)) : fillString("%i", *((int*)val)) ;

	// Position text outside of slider rect.
	{
		int border = 1;
		br = rectExpand(br, -vec2(border*2));
		Vec2 tp = rectCen(br);

		float off = settings.heightOffset;
		Vec2 tDim = getTextDim(text, textSettings->font);
		float l = tp.x - tDim.w/2 - off;
		float r = tp.x + tDim.w/2 + off;

		Vec2i align = vec2i(0,0);

		// Test doesn't fit.
		if(tDim.w > rectDim(br).w-off*2) {
			tp = rectCen(br);
			align = vec2i(0,0);
		} else {
			if(sr.right < l || sr.left > r) {
				// Text fits in middle.
				tp = rectCen(br);
				align = vec2i(0,0);
			} else {
				Vec2 brRight = rectR(br) - vec2(off,0);
				Vec2 brLeft = rectL(br) + vec2(off,0);

				if(rectCen(sr).x+0.1f < rectCen(br).x) {
					Vec2 nextToSlider = rectR(sr) + vec2(off,0);
					// Clamp.
					if(brRight.x - nextToSlider.x < tDim.w) {
						tp = brRight;
						align = vec2i(1,0);
					} else {
						// Slider left of center.
						tp = nextToSlider;
						align = vec2i(-1,0);
					}
				} else {
					Vec2 nextToSlider = rectL(sr) - vec2(off,0);
					// Clamp.
					if(nextToSlider.x - brLeft.x < tDim.w) {
						tp = brLeft;
						align = vec2i(-1,0);
					} else {
						// Slider right of center.
						tp = nextToSlider;
						align = vec2i(1,0);
					}
				}
			}
		}

		drawText(text, tp, align, *textSettings);
	}
}
void drawSlider(float val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, SLIDER_TYPE_FLOAT, br, sr, scissor, settings);
}
void drawSlider(int val, Rect br, Rect sr, Rect scissor, SliderSettings settings) {
	return drawSlider(&val, SLIDER_TYPE_INT, br, sr, scissor, settings);
}

//

void newGuiQuickText(NewGui* gui, Rect r, char* t, Vec2i align, TextSettings* settings = 0) {
	if(rectEmpty(getRectScissor(gui->scissor, r))) return;

	TextSettings set = settings == 0 ? gui->textSettings : *settings;
	drawText(r, t, align, gui->scissor, set);
}
void newGuiQuickText(NewGui* gui, Rect r, char* t, TextSettings* settings = 0) {
	newGuiQuickText(gui, r, t, vec2i(0,0), settings);
}

void newGuiQuickBox(NewGui* gui, Rect r, BoxSettings* settings = 0) {
	if(rectEmpty(getRectScissor(gui->scissor, r))) return;

	BoxSettings set = settings == 0 ? gui->boxSettings : *settings;
	drawBox(r, gui->scissor, set);
}

void newGuiQuickTextBox(NewGui* gui, Rect r, char* t, Vec2i align, TextBoxSettings* settings = 0) {
	if(rectEmpty(getRectScissor(gui->scissor, r))) return;

	TextBoxSettings set = settings == 0 ? gui->textBoxSettings : *settings;
	drawTextBox(r, t, align, gui->scissor, set);
}
void newGuiQuickTextBox(NewGui* gui, Rect r, char* t, TextBoxSettings* settings = 0) {
	return newGuiQuickTextBox(gui, r, t, vec2i(0,0), settings);
}

bool _newGuiQuickButton(NewGui* gui, Rect r, char* text, Vec2i align, TextBoxSettings* settings, bool highlightOnActive) {
	r = rectRound(r);

	Rect intersection = getRectScissor(gui->scissor, r);
	bool active = newGuiGoButtonAction(gui, intersection, gui->zLevel);
	if(rectEmpty(intersection)) return false;

	TextBoxSettings set = settings == 0 ? gui->buttonSettings : *settings;
	set.boxSettings.color += highlightOnActive?newGuiColorModB(gui):newGuiColorMod(gui);

	// set.boxSettings.color += newGuiInactiveColorMod(gui->setInactive);

	drawTextBox(r, text, align, gui->scissor, set);

	if(newGuiIsHot(gui)) newGuiSetCursor(gui, IDC_HAND);

	if(gui->setInactive) gui->setInactive = false;

	return active;
}

bool newGuiQuickButton(NewGui* gui, Rect r, char* text, Vec2i align, TextBoxSettings* settings = 0) {
	return _newGuiQuickButton(gui, r, text, align, settings == 0 ? &gui->buttonSettings : settings, true);
}
bool newGuiQuickButton(NewGui* gui, Rect r, char* text, TextBoxSettings* settings = 0) {
	return newGuiQuickButton(gui, r, text, vec2i(0,0), settings);
}
bool newGuiQuickButton(NewGui* gui, Rect r, TextBoxSettings* settings = 0) {
	return newGuiQuickButton(gui, r, "", vec2i(0,0), settings);
}

bool newGuiQuickPButton(NewGui* gui, Rect r, char* text, Vec2i align, TextBoxSettings* settings = 0) {
	return _newGuiQuickButton(gui, r, text, align, settings == 0 ? &gui->buttonSettings : settings, false);
}
bool newGuiQuickPButton(NewGui* gui, Rect r, char* text, TextBoxSettings* settings = 0) {
	return newGuiQuickPButton(gui, r, text, vec2i(0,0), settings);
}
bool newGuiQuickPButton(NewGui* gui, Rect r, TextBoxSettings* settings = 0) {
	return newGuiQuickPButton(gui, r, "", vec2i(0,0), settings);
}

bool newGuiQuickCheckBox(NewGui* gui, Rect r, bool* value, CheckBoxSettings* settings = 0) {
	Rect intersection = getRectScissor(gui->scissor, r);
	bool active = newGuiGoButtonAction(gui, intersection, gui->zLevel);
	if(rectEmpty(intersection)) return false;

	if(active) {
		*value = !(*value);
	}

	CheckBoxSettings set = settings == 0 ? gui->checkBoxSettings : *settings;
	set.boxSettings.color += newGuiColorMod(gui);

	Rect cr = rectCenDim(rectCen(r), vec2(min(rectW(r), rectH(r))));
	drawBox(cr, gui->scissor, set.boxSettings);

	if(*value) {
		cr = rectRound(cr);
		drawBox(rectExpand(cr, vec2(-rectW(cr)*(1 - set.sizeMod))), gui->scissor, boxSettings(set.color, set.boxSettings.roundedCorner/2));
	}

	if(newGuiIsHot(gui)) newGuiSetCursor(gui, IDC_HAND);

	return active;
}

bool newGuiQuickTextEditAllVars(NewGui* gui, Rect r, void* data, int varType, int maxSize, TextEditSettings* editSettings = 0) {
	Rect intersect = getRectScissor(gui->scissor, r);

	TextEditSettings set = editSettings == 0 ? gui->editSettings : *editSettings;
	TextSettings* textSettings = &set.textBoxSettings.textSettings;

	char* charData = (char*)data;
	int* intData = (int*)data;
	float* floatData = (float*)data;

	int event;
	if(varType == 0) event = newGuiGoTextEdit(gui, intersect, gui->zLevel, charData, set, maxSize);
	else if(varType == 1) event = newGuiGoTextEdit(gui, intersect, gui->zLevel, intData, set);
	else event = newGuiGoTextEdit(gui, intersect, gui->zLevel, floatData, set);

	if(rectEmpty(intersect)) return false;

	if(event == 0) set.textBoxSettings.boxSettings.color += newGuiColorMod(gui);

	bool active = event == 1 || event == 2;
	gui->editVars.dt = gui->dt;
	if(varType == 0) drawTextEditBox(charData, r, active, gui->scissor, gui->editVars, set);
	else if(varType == 1) drawTextEditBox(*intData, r, active, gui->scissor, gui->editVars, set);
	else drawTextEditBox(*floatData, r, active, gui->scissor, gui->editVars, set);

	if(newGuiIsHot(gui) || (newGuiIsActive(gui) && pointInRectEx(gui->input->mousePosNegative, intersect))) newGuiSetCursor(gui, IDC_IBEAM);

	if(event == 3) return true;

	return false;
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, char* data, int maxSize, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, EDIT_MODE_CHAR, maxSize, editSettings);
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, int* data, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, EDIT_MODE_INT, 0, editSettings);
}
bool newGuiQuickTextEdit(NewGui* gui, Rect r, float* data, TextEditSettings* editSettings = 0) {
	return newGuiQuickTextEditAllVars(gui, r, data, EDIT_MODE_FLOAT, 0, editSettings);
}


float sliderGetMod(NewGui* gui, int type, SliderSettings* settings, int mod) {
	Input* input = gui->input;
	float modifier = mod;
	if(input->keysDown[KEYCODE_SHIFT] && input->keysDown[KEYCODE_CTRL]) modifier *= 100;
	else if(input->keysDown[KEYCODE_SHIFT]) modifier *= 10;
	else if(input->keysDown[KEYCODE_CTRL]) modifier /= 10;
	
	if(type == SLIDER_TYPE_INT) {
		int mod = settings->mouseWheelModInt * modifier;
		clampMinInt(&mod, 1);
		return mod;
	} else {
		float mod = settings->mouseWheelModFloat * modifier;
		return mod;
	}
}
bool newGuiQuickSlider(NewGui* gui, Rect r, int type, void* val, void* min, void* max, SliderSettings* settings = 0) {

	bool typeIsInt = type == SLIDER_TYPE_INT;
	SliderSettings set = settings == 0 ? gui->sliderSettings : *settings;

	bool result = false;
	bool editMode = false;

	newGuiStoreIds(gui, 6);

	// Double click text edit mode.
	{
		Rect intersect = getRectScissor(gui->scissor, r);

		TextEditSettings edSet = gui->editSettings;

		int event;
		flagSet(&edSet.flags, ESETTINGS_START_RIGHT);
		event = newGuiGoTextEdit(gui, intersect, gui->zLevel, val, typeIsInt?EDIT_MODE_INT:EDIT_MODE_FLOAT, edSet, &gui->editVars, 0, true);
		if(event == 1 || event == 2) {
			drawTextEditBox(val, typeIsInt?EDIT_MODE_INT:EDIT_MODE_FLOAT, r, event > 0, gui->scissor, gui->editVars, edSet);
			editMode = true;

			newGuiClearStoredIds(gui);
			return false;

		} else if(event == 3) {
			result = true;
		}
	}

	// Mouse Wheel / Arrow Keys.
	{
		Input* input = gui->input;
		int mod = 0;
		if(newGuiGoButtonAction(gui, r, gui->zLevel, Gui_Focus_MWheel)) {
			mod = input->mouseWheel;
		}
		if(newGuiGoButtonAction(gui, r, gui->zLevel, input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_RIGHT], Gui_Focus_ArrowKeys)) {
			mod = input->keysPressed[KEYCODE_LEFT]?-1:1;
		}

		if(mod) {
			float modValue = sliderGetMod(gui, type, &set, mod);
			if(typeIsInt) Void_Dref(int, val) += roundInt(modValue);
			else Void_Dref(float, val) += modValue;
			result = true;
		}
	}

	// For apply after.
	void* valuePointer = val;
	int sliderId = newGuiIncrementId(gui);

	if(set.applyAfter && gui->sliderId == sliderId) {
		if(typeIsInt) val = &gui->editInt;
		else val = &gui->editFloat;
	}

	// Right now we just handle int as if it was a float.

	if(typeIsInt) clampInt((int*)val, Void_Dref(int, min), Void_Dref(int, max));
	else clamp((float*)val, Void_Dref(float, min), Void_Dref(float, max));

	float floatVal = typeIsInt ? Void_Dref(int, val) : Void_Dref(float, val);
	float floatMin = typeIsInt ? Void_Dref(int, min) : Void_Dref(float, min);
	float floatMax = typeIsInt ? Void_Dref(int, max) : Void_Dref(float, max);

	float sliderSize;
	if(typeIsInt) {
		sliderSize = (rectW(r) / (float)(Void_Dref(int, max) - Void_Dref(int, min) + 1));
		sliderSize = clampMin(sliderSize, set.minSize);
	} else {
		sliderSize = set.size;
	}

	Rect slider = newGuiCalcSlider(floatVal, r, sliderSize, floatMin, floatMax, true);

	int event = newGuiGoDragAction(gui, getRectScissor(r, slider), gui->zLevel);
	if(!testRectScissor(gui->scissor, r)) {
		newGuiClearStoredIds(gui);
		return false;
	}

	if(event == 1 && set.useDefaultValue && gui->input->doubleClick) {
		newGuiSetNotActive(gui, newGuiCurrentId(gui));
		result = true;

		if(typeIsInt) Void_Dref(int, val) = roundInt(set.defaultvalue);
		else Void_Dref(float, val) = set.defaultvalue;

	} else {
		if(event == 1) {
			if(set.applyAfter) {
				if(typeIsInt) gui->editInt = Void_Dref(int, val);
				else gui->editFloat = Void_Dref(float, val);

				gui->sliderId = sliderId;
			}

			gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
		}
		if(event > 0) {
			Vec2 pos = gui->input->mousePosNegative - gui->mouseAnchor;
			floatVal = newGuiSliderGetValue(pos, r, sliderSize, floatMin, floatMax, true);

			if(typeIsInt) floatVal = roundInt(floatVal);
			slider = newGuiCalcSlider(floatVal, r, sliderSize, floatMin, floatMax, true);

			if(gui->input->keysDown[KEYCODE_SHIFT]) floatVal = roundFloat(floatVal);
		}

		if(typeIsInt) Void_Dref(int, val) = floatVal;
		else Void_Dref(float, val) = floatVal;
	}

	set.color += newGuiColorMod(gui);
	if(!editMode) {
		newGuiScissorPush(gui, r);
		drawSlider(val, type, r, slider, gui->scissor, set);
		newGuiScissorPop(gui);
	}

	if(event == 3) {
		if(set.applyAfter) {
			if(typeIsInt) Void_Dref(int, valuePointer) = gui->editInt;
			else Void_Dref(float, valuePointer) = gui->editFloat;

			gui->sliderId = 0;
		}
	
		result = true;
	}

	newGuiClearStoredIds(gui);

	return result;
}

bool newGuiQuickSlider(NewGui* gui, Rect r, float* val, float min, float max, SliderSettings* settings = 0) {
	return newGuiQuickSlider(gui, r, SLIDER_TYPE_FLOAT, val, &min, &max, settings);
}
bool newGuiQuickSlider(NewGui* gui, Rect r, int* val, int min, int max, SliderSettings* settings = 0) {
	return newGuiQuickSlider(gui, r, SLIDER_TYPE_INT, val, &min, &max, settings);
}



void newGuiQuickScroll(NewGui* gui, Rect r, float height, float* scrollValue, ScrollRegionSettings* settings = 0) {
	ScrollRegionSettings set = settings == 0 ? gui->scrollSettings : *settings;
	int flags = set.flags;

	clampMin(&height, 0);

	float scrollRegionHeight = rectH(r);

	float itemsHeight = height - scrollRegionHeight;
	clampMin(&itemsHeight, 0);
	bool hasScrollbar = itemsHeight > 0;

	Rect scrollRegion = r;
	if(!hasScrollbar && flagGet(flags, SCROLL_DYNAMIC_HEIGHT)) rectSetB(&scrollRegion, scrollRegion.top - height);
	float scrollBarWidth = hasScrollbar?set.scrollBarWidth:0;

	Rect itemRegion = rectSetR(scrollRegion, scrollRegion.right - scrollBarWidth);
	if(hasScrollbar) rectAddR(&itemRegion, set.sliderMargin.x);
	Rect scrollBarRegion = rectSetL(scrollRegion, scrollRegion.right - scrollBarWidth);

	scissorTestScreen(rectExpand(gui->scissor, vec2(-3,-3)));

	// if(!hasScrollbar) rectSetB(&scrollRegion, scrollRegion.top-height);
	drawRect(scrollRegion, set.boxSettings.color);
	if(set.scrollBarColor.a != 0) drawRect(scrollBarRegion, set.scrollBarColor);
	if(set.boxSettings.borderColor.a != 0) {
		glLineWidth(1);
		drawRectOutline(scrollRegion, set.boxSettings.borderColor);
	}

	float sliderSize = set.sliderSize;
	if(sliderSize == 0) sliderSize = (rectH(scrollBarRegion) / (rectH(scrollRegion)+itemsHeight)) * rectH(scrollBarRegion);
	clampMin(&sliderSize, set.sliderSizeMin);

	newGuiStoreIds(gui, 3);

	if(hasScrollbar) {
		// Scroll with mousewheel.
		{
			if(newGuiGoButtonAction(gui, getRectScissor(gui->scissor, scrollRegion), gui->zLevel, Gui_Focus_MWheel)) {
				float scrollAmount = set.scrollAmount;
				if(gui->input->keysDown[KEYCODE_SHIFT]) scrollAmount *= 2;

				(*scrollValue) = clamp((*scrollValue) + -gui->input->mouseWheel*(scrollAmount/itemsHeight), 0, 1);
			}
		}

		Rect slider;
		int sliderId;
		// Scroll with handle.
		{
			slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			int event = newGuiGoDragAction(gui, getRectScissor(gui->scissor, slider), gui->zLevel);
			sliderId = newGuiCurrentId(gui);
			if(event == 1) gui->mouseAnchor = gui->input->mousePosNegative - rectCen(slider);
			if(event > 0) {
				(*scrollValue) = 1-newGuiSliderGetValue(gui->input->mousePosNegative - gui->mouseAnchor, scrollBarRegion, sliderSize, 0, 1, false);
				slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			}
		}

		// Scroll with background drag.
		if(flagGet(flags, SCROLL_BACKGROUND))
		{
			int event = newGuiGoDragAction(gui, getRectScissor(gui->scissor, itemRegion), gui->zLevel);
			if(event == 1) gui->mouseAnchor.y = gui->input->mousePosNegative.y - ((*scrollValue)*itemsHeight);
			if(event > 0) {
				(*scrollValue) = (gui->input->mousePosNegative.y - gui->mouseAnchor.y)/itemsHeight;
				clamp(&(*scrollValue), 0, 1);

				slider = newGuiCalcSlider(1-(*scrollValue), scrollBarRegion, sliderSize, 0, 1, false);
			}
		}

		rectExpand(&slider, -set.sliderMargin*2);
		if(hasScrollbar) drawRectRounded(slider, set.sliderColor + newGuiColorModId(gui, sliderId), set.sliderRounding);
	}

	newGuiClearStoredIds(gui);

	set.border.x = clampMin(set.border.x, 1);
	set.border.y = clampMin(set.border.y, 1);

	itemRegion = rectExpand(itemRegion, -set.border*2);

	Rect scissor = getRectScissor(gui->scissor, itemRegion);
	Vec2 layoutStartPos = vec2(itemRegion.left, itemRegion.top + (*scrollValue)*itemsHeight);

	newGuiLayoutScissorPush(gui, itemRegion, scissor);
	gui->ld->pos = layoutStartPos;
}

void newGuiQuickScrollEnd(NewGui* gui) {
	newGuiLayoutScissorPop(gui);
}

#define COMBO_BOX_TRIANGLE_SIZE_MOD (1.0f/4.0f)
#define COMBO_BOX_TRIANGLE_OFFSET_MOD 0.25f

float newGuiGetComboBoxWidth(char* text, TextSettings settings) {
	float textWidth = getTextDim(text, settings.font).w;
	float triangleWidth = settings.font->height * COMBO_BOX_TRIANGLE_SIZE_MOD;
	float triangleOffset = settings.font->height * COMBO_BOX_TRIANGLE_OFFSET_MOD;

	float result = textWidth + triangleOffset + triangleWidth;
	return result;
}

bool newGuiQuickComboBox(NewGui* gui, Rect r, int* index, char** strings, int stringCount, TextBoxSettings* settings = 0) {

	Rect intersection = getRectScissor(gui->scissor, r);
	bool active = newGuiGoButtonAction(gui, intersection, gui->zLevel);
	if(rectEmpty(intersection)) return false;

	if(newGuiIsHot(gui)) newGuiSetCursor(gui, IDC_HAND);

	TextBoxSettings set = settings == 0 ? gui->comboBoxSettings : *settings;
	set.boxSettings.color += newGuiColorMod(gui);

	bool updated = false;

	// Mouse Wheel / Arrow Keys.
	{
		Input* input = gui->input;
		int mod = 0;
		if(newGuiGoButtonAction(gui, r, gui->zLevel, Gui_Focus_MWheel)) {
			mod = -input->mouseWheel;
		}
		bool keys = input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_RIGHT] || input->keysPressed[KEYCODE_UP] || input->keysPressed[KEYCODE_DOWN];
		if(newGuiGoButtonAction(gui, r, gui->zLevel, keys, Gui_Focus_ArrowKeys)) {
			mod = (input->keysPressed[KEYCODE_LEFT] || input->keysPressed[KEYCODE_UP])?-1:1;
		}
		if(mod) {
			*index += mod;
			if(*index < 0 || *index > stringCount-1) updated = false;
			else updated = true;

			clampInt(index, 0, stringCount-1);
			gui->comboBoxData.index = *index;
		}
	}

	if(active) {
		PopupData pd = {};
		pd.type = POPUP_TYPE_COMBO_BOX;
		pd.id = newGuiCurrentId(gui);
		// pd.r = rectTLDim(rectBL(r), vec2(rectW(r), set.textSettings.font->height*cData.count));
		pd.r = rectTLDim(rectBL(r), vec2(rectW(r), 0));
		pd.settings = gui->boxSettings;
		pd.border = vec2(5,5);

		newGuiPopupPush(gui, pd);

		gui->comboBoxIndex = index;
		gui->comboBoxData = {*index, strings, stringCount};
	}

	// Check if popup is active.
	bool popupActive = false;
	for(int i = 0; i < gui->popupStackCount; i++) {
		if(newGuiCurrentId(gui) == gui->popupStack[i].id) {
			popupActive = true;
			break;
		}
	}

	if(gui->comboBoxData.finished && popupActive) {
		gui->comboBoxData.finished = false;
		newGuiPopupPop(gui);
		updated = true;
	}


	{
		drawBox(r, gui->scissor, set.boxSettings);

		Rect mainRect = rectExpand(r, vec2(-set.sideAlignPadding*2,0));
		float fontHeight = set.textSettings.font->height;

		float triangleSize = fontHeight * COMBO_BOX_TRIANGLE_SIZE_MOD;
		float triangleOffset = fontHeight * COMBO_BOX_TRIANGLE_OFFSET_MOD;
		float triangleRectWidth = fontHeight * 0.5f;

		char* text = strings[*index];

		#if 0
		// float textWidth = getTextDim(text, set.textSettings.font).w+4;
		// float fullWidth = textWidth + triangleRectWidth + triangleOffset;
		// if(rectW(mainRect) > fullWidth) rectSetW(&mainRect, fullWidth);
		#endif 

		float textRectWidth = rectW(mainRect)-triangleRectWidth-triangleOffset;

		Rect textRect = rectRSetR(mainRect, textRectWidth);
		Rect triangleRect = rectRSetL(mainRect, triangleRectWidth);

		newGuiScissorPush(gui, rectExpand(r,vec2(-2)));

		drawText(textRect, text, vec2i(-1,0), gui->scissor, set.textSettings);

		// Vec2 dir = popupActive ? vec2(0,-1) : vec2(1,0);
		Vec2 dir = vec2(0,-1);
		drawTriangle(rectCen(triangleRect), triangleSize, dir, set.textSettings.color);

		newGuiScissorPop(gui);
	}

	return updated;
}


TextEditSettings textEditSettings(TextBoxSettings textSettings, Vec4 defaultTextColor, char* textBuffer, int flags, float cursorWidth, float cursorHeightMod, Vec4 colorSelection, Vec4 colorCursor, float cursorFlashingSpeed, float textOffset) {
	return {textSettings, defaultTextColor, textBuffer, flags, cursorWidth, cursorHeightMod, "", colorSelection, colorCursor, cursorFlashingSpeed, textOffset};
}

char* textSelectionToString(char* text, int index1, int index2) {
	myAssert(index1 >= 0 && index2 >= 0);

	int range = abs(index1 - index2);
	char* str = getTString(range + 1);
	strCpy(str, text + minInt(index1, index2), range);
	return str;
}


int textWordSearch(char* text, int startIndex, bool left) {
	if(left) {
		int index = startIndex;
		if(text[index] == ' ' && index != 0) index--;
		while(text[index] != ' ' && index != 0) index--;
		if(text[index] == ' ') index++;

		return index;
	} else {
		int index = startIndex;
		if(text[index] == ' ' && index != 0) index--;
		while(text[index] != ' ' && text[index] != '\0') index++;

		return index;
	}
}

void textEditBox(char* text, int textMaxSize, Font* font, Rect textRect, Input* input, Vec2i align, TextEditSettings tes, TextEditVars* tev) {

	bool wrapping = flagGet(tes.flags, ESETTINGS_WRAPPING);
	bool singleLine = flagGet(tes.flags, ESETTINGS_SINGLE_LINE);

	if(singleLine) wrapping = false;

	// Vec2 startPos = rectTL(textRect) + tev->scrollOffset;
	Vec2 startPos = rectTL(textRect) + tev->scrollOffset + vec2(tes.textOffset,0);
	int wrapWidth = wrapping ? rectDim(textRect).w : 0;

	int cursorIndex = tev->cursorIndex;
	int markerIndex = tev->markerIndex;

	// Bug.
	Vec2 off = vec2(2,0);
	Vec2 mp = input->mousePosNegative + off;
	if(singleLine) mp.y = rectCenY(textRect); // Lock mouse y axis.

	int mouseIndex = textMouseToIndex(text, font, startPos, mp, align, wrapWidth);

	if(input->doubleClick) {
		tev->wordSelectionMode = true;
		cursorIndex = 0;
		markerIndex = strLen(text);

		tev->wordSelectionStartIndex = mouseIndex;
	}

	if(input->mouseButtonReleased[0]) {
		tev->wordSelectionMode = false;
	}

	if(!tev->wordSelectionMode) {
		if(input->mouseButtonPressed[0]) {
			if(pointInRect(input->mousePosNegative, textRect)) {
				markerIndex = mouseIndex;
			}
		}

		if(input->mouseButtonDown[0]) {
			cursorIndex = mouseIndex;
		}

	} else {
		if(input->mouseButtonDown[0]) {
			if(tev->wordSelectionStartIndex != mouseIndex) {
				if(mouseIndex < tev->wordSelectionStartIndex) {
					markerIndex = textWordSearch(text, tev->wordSelectionStartIndex, false);
					cursorIndex = textWordSearch(text, mouseIndex, true);
				} else {
					markerIndex = textWordSearch(text, tev->wordSelectionStartIndex, true);
					cursorIndex = textWordSearch(text, mouseIndex, false);
				}
			} else {
				markerIndex = textWordSearch(text, mouseIndex, true);
				cursorIndex = textWordSearch(text, mouseIndex, false);
			}
		}
	}

	bool left = input->keysPressed[KEYCODE_LEFT];
	bool right = input->keysPressed[KEYCODE_RIGHT];
	bool up = input->keysPressed[KEYCODE_UP];
	bool down = input->keysPressed[KEYCODE_DOWN];

	bool a = input->keysPressed[KEYCODE_A];
	bool x = input->keysPressed[KEYCODE_X];
	bool c = input->keysPressed[KEYCODE_C];
	bool v = input->keysPressed[KEYCODE_V];
	
	bool home = input->keysPressed[KEYCODE_HOME];
	bool end = input->keysPressed[KEYCODE_END];
	bool backspace = input->keysPressed[KEYCODE_BACKSPACE];
	bool del = input->keysPressed[KEYCODE_DEL];
	bool enter = input->keysPressed[KEYCODE_RETURN];
	bool tab = input->keysPressed[KEYCODE_TAB];

	bool ctrl = input->keysDown[KEYCODE_CTRL];
	bool shift = input->keysDown[KEYCODE_SHIFT];

	// Main navigation and things.

	int startCursorIndex = cursorIndex;

	if(ctrl && backspace) {
		shift = true;
		left = true;
	}

	if(ctrl && del) {
		shift = true;
		right = true;
	}

	if(!singleLine) {
		if(up || down) {
			float cursorYOffset;
			if(up) cursorYOffset = font->height;
			else if(down) cursorYOffset = -font->height;

			Vec2 cPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);
			cPos.y += cursorYOffset;
			int newCursorIndex = textMouseToIndex(text, font, startPos, cPos, align, wrapWidth);
			cursorIndex = newCursorIndex;
		}
	}


	if(left) {
		if(ctrl) {
			if(cursorIndex > 0) {
				while(text[cursorIndex-1] == ' ') cursorIndex--;

				if(cursorIndex > 0)
					cursorIndex = strFindBackwards(text, ' ', cursorIndex-1);
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex < markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex > 0) cursorIndex--;
			}
		}
	}

	if(right) {
		if(ctrl) {
			while(text[cursorIndex] == ' ') cursorIndex++;
			if(cursorIndex <= strLen(text)) {
				cursorIndex = strFindOrEnd(text, ' ', cursorIndex+1);
				if(cursorIndex != strLen(text)) cursorIndex--;
			}
		} else {
			bool isSelected = cursorIndex != markerIndex;
			if(isSelected && !shift) {
				if(cursorIndex > markerIndex) {
					markerIndex = cursorIndex;
				} else {
					cursorIndex = markerIndex;
				}
			} else {
				if(cursorIndex < strLen(text)) cursorIndex++;
			}
		}
	}

	if(singleLine) {
		if(home) {
			cursorIndex = 0;
		}

		if(end) {
			cursorIndex = strLen(text);
		}
	}

	if((startCursorIndex != cursorIndex) && !shift) {
		markerIndex = cursorIndex;
	}

	if(ctrl && a) {
		cursorIndex = 0;
		markerIndex = strLen(text);
	}

	bool isSelected = cursorIndex != markerIndex;

	if((ctrl && x) && isSelected) {
		c = true;
		del = true;
	}

	if((ctrl && c) && isSelected) {
		char* selection = textSelectionToString(text, cursorIndex, markerIndex);
		setClipboard(selection);
	}

	if(enter) {
		if(singleLine) {
			// strClear(text);
			// cursorIndex = 0;
			// markerIndex = 0;
		} else {
			input->inputCharacters[input->inputCharacterCount++] = '\n';
		}
	}

	if(backspace || del || (input->inputCharacterCount > 0) || (ctrl && v)) {
		if(isSelected) {
			int delIndex = min(cursorIndex, markerIndex);
			int delAmount = abs(cursorIndex - markerIndex);
			strRemoveX(text, delIndex, delAmount);
			cursorIndex = delIndex;
		}

		markerIndex = cursorIndex;
	}

	if(ctrl && v) {
		char* clipboard = (char*)getClipboard();
		if(clipboard) {
			int clipboardSize = strLen(clipboard);
			if(clipboardSize + strLen(text) < textMaxSize) {
				strInsert(text, cursorIndex, clipboard);
				cursorIndex += clipboardSize;
				markerIndex = cursorIndex;
			}
			closeClipboard();
		}
	}

	// Add input characters to input buffer.
	if(input->inputCharacterCount > 0) {
		if(input->inputCharacterCount + strLen(text) < textMaxSize) {
			strInsert(text, cursorIndex, input->inputCharacters, input->inputCharacterCount);
			cursorIndex += input->inputCharacterCount;
			markerIndex = cursorIndex;
		}
	}

	if(backspace && !isSelected) {
		if(cursorIndex > 0) {
			strRemove(text, cursorIndex);
			cursorIndex--;
		}
		markerIndex = cursorIndex;
	}

	if(del && !isSelected) {
		if(cursorIndex+1 <= strLen(text)) {
			strRemove(text, cursorIndex+1);
		}
		markerIndex = cursorIndex;
	}

	// Scrolling.
	{
		Vec2 cursorPos = textIndexToPos(text, font, startPos, cursorIndex, align, wrapWidth);

		float leftEnd = textRect.left + tes.textOffset;
		float rightEnd = textRect.right - tes.textOffset;
		if(		cursorPos.x < leftEnd) tev->scrollOffset.x += leftEnd - cursorPos.x;
		else if(cursorPos.x > rightEnd) tev->scrollOffset.x += rightEnd - cursorPos.x;

		clampMax(&tev->scrollOffset.x, 0);
		
		float ch = font->height;
		if(!singleLine) {
			if(		cursorPos.y - ch/2 < textRect.min.y) tev->scrollOffset.y += textRect.min.y - (cursorPos.y - ch/2);
			else if(cursorPos.y + ch/2 > textRect.max.y) tev->scrollOffset.y += textRect.max.y - (cursorPos.y + ch/2);

			clampMin(&tev->scrollOffset.y, 0);
		}
	}

	tev->cursorChanged = (tev->cursorIndex != cursorIndex || tev->markerIndex != markerIndex);

	tev->cursorIndex = cursorIndex;
	tev->markerIndex = markerIndex;

	tev->cursorTimer += tev->dt*tes.cursorFlashingSpeed;
	if(tev->cursorChanged || input->mouseButtonPressed[0]) tev->cursorTimer = 0;
}

struct GuiWindowSettings {
	float borderSize;
	float cornerSize;
	Vec2 minDim;
	Vec2 maxDim;
	Rect insideRect;

	bool movable;
	bool resizableX;
	bool resizableY;

	bool mouseScreenCoordinates;
};

bool newGuiWindowUpdate(NewGui* gui, Rect* r, float z, GuiWindowSettings settings) {
	Rect region = *r;
	Rect sr = settings.insideRect;
	bool insideClamp = !rectEmpty(sr);
	if(!insideClamp) sr = rectCenDim(vec2(0,0), vec2(FLT_MAX, FLT_MAX)); // So we don't clamp.

	if(settings.maxDim == vec2(0,0)) settings.maxDim = vec2(FLT_MAX,FLT_MAX);

	bool screenMouse = settings.mouseScreenCoordinates;

	float w = settings.borderSize;
	Vec2 p = screenMouse?gui->input->mousePosNegativeScreen:gui->input->mousePosNegative;

	int uiEvent = 0;
	bool changeCursor = false;
	Vec2i resizeAlign;

	bool move = false;


	if((settings.resizableY || settings.resizableX) || settings.movable) {
		int event = newGuiGoDragAction(gui, region, z, Gui_Focus_MLeft, screenMouse);
		int eventRightClick = newGuiGoDragAction(gui, region, z, Gui_Focus_MRight, screenMouse);
		event = max(event, eventRightClick);
		if(event == 1) {
			gui->mode = gui->input->keysDown[KEYCODE_CTRL];

			if(!gui->mode) {
				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				// gui->mouseAnchor = input->mousePosNegative - rectTL(region);
				gui->mouseAnchor = mp - rectTL(region);
				gui->mouseAnchor2 = rectDim(region);
			}
		}

		if(event > 0) {
			if(gui->mode) {
				uiEvent = event;
				resizeAlign = vec2i(1,-1);
			} else {
				move = true;

				POINT p; 
				GetCursorPos(&p);
				Vec2 mp = vec2(p.x, -p.y);

				// Vec2 pos = input->mousePosNegative - gui->mouseAnchor;
				Vec2 pos = mp - gui->mouseAnchor;

				if(insideClamp) {
					clamp(&pos.x, sr.left, sr.right - gui->mouseAnchor2.w);
					clamp(&pos.y, sr.bottom + gui->mouseAnchor2.h, sr.top);
				}
				region = rectTLDim(pos, gui->mouseAnchor2);
			}
		}
	}

	float cornerSize = settings.cornerSize;
	for(int x = -1; x < 2; x++) {
		for(int y = -1; y < 2; y++) {
			if(x == 0 && y == 0) continue;

			Vec2i align = vec2i(x,y);
			Vec2 dim = vec2(align.x==0?rectW(region)-cornerSize*2+2:w+1, align.y==0?rectH(region)-cornerSize*2+2:w+1);
			Rect r = rectAlignDim(region, align, dim);

			int event;
			bool corner = abs(x) == 1 && abs(y) == 1;
			if(corner) {
				r = rectAlignDim(region, align, vec2(w+1,cornerSize));
				event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
				r = rectAlignDim(region, align, vec2(cornerSize,w+1));
				gui->id--;
				event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
			} else {
				event = newGuiGoDragAction(gui, r, z, Gui_Focus_MLeft, screenMouse);
			}

			if(event > 0) {
				uiEvent = event;
				resizeAlign = align;
			}
			if(newGuiIsWasHotOrActive(gui)) {
				changeCursor = true;
				resizeAlign = align;
			}
		}
	}

	if(!move) {
		if(uiEvent == 1) {
			if(resizeAlign.x == -1) gui->mouseAnchor.x = p.x - region.left;
			if(resizeAlign.x ==  1) gui->mouseAnchor.x = p.x - region.right;
			if(resizeAlign.y ==  1) gui->mouseAnchor.y = p.y - region.top;
			if(resizeAlign.y == -1) gui->mouseAnchor.y = p.y - region.bottom;
		}

		if(uiEvent > 0) {
			if(resizeAlign.x == -1) region.left = (p - gui->mouseAnchor).x;
			else if(resizeAlign.x == 1) region.right = (p - gui->mouseAnchor).x;

			if(settings.resizableY) {
				if(resizeAlign.y == -1) region.bottom = (p - gui->mouseAnchor).y;
				else if(resizeAlign.y == 1) region.top = (p - gui->mouseAnchor).y;
			}
		}

		if(changeCursor) {
			if(resizeAlign == vec2i(-1,-1) || resizeAlign == vec2i(1,1)) newGuiSetCursor(gui, IDC_SIZENESW);
			if(resizeAlign == vec2i(-1,1) || resizeAlign == vec2i(1,-1)) newGuiSetCursor(gui, IDC_SIZENWSE);
			if(resizeAlign == vec2i(-1,0) || resizeAlign == vec2i(1, 0)) newGuiSetCursor(gui, IDC_SIZEWE);
			if(resizeAlign == vec2i(0,-1) || resizeAlign == vec2i(0, 1)) newGuiSetCursor(gui, IDC_SIZENS);
		}

    	// if(uiEvent > 0 && insideClamp) {
		if(uiEvent > 0) {
			Vec2 minDim = settings.minDim;
			Vec2 maxDim = settings.maxDim;

			if(settings.resizableX) {
				if(resizeAlign.x == -1) region.left = clamp(region.left, max(sr.left, region.right - maxDim.x), region.right - minDim.x);
				if(resizeAlign.x ==  1) region.right = clamp(region.right, region.left + minDim.x, min(sr.right, region.left + maxDim.x));
			}

			if(settings.resizableY) {
				if(resizeAlign.y == -1) region.bottom = clamp(region.bottom, max(sr.bottom, region.top - maxDim.y), region.top - minDim.y);
				if(resizeAlign.y ==  1) region.top = clamp(region.top, region.bottom + minDim.y, min(sr.top, region.bottom + maxDim.y));
			}
		}
	}


    // If window is resizing clamp panel.
	if(insideClamp)
	{
		Vec2 dim = rectDim(region);
		if(region.left < sr.left) rectTrans(&region, vec2(sr.left - region.left, 0));
		if(region.right > sr.right) rectTrans(&region, vec2(sr.right - region.right, 0));
		if(region.bottom < sr.bottom) rectTrans(&region, vec2(0, sr.bottom - region.bottom));
		if(region.top > sr.top) rectTrans(&region, vec2(0, sr.top - region.top));

		if(rectH(region) > rectH(sr)) {
			region.top = sr.top;
			region.bottom = sr.bottom;
		}
		if(rectW(region) > rectW(sr)) {
			region.left = sr.left;
			region.right = sr.right;
		}
	}

	*r = region;

	if(move || uiEvent) return true;

	return false;
}


#define POPUP_MIN_WIDTH 80
#define POPUP_MAX_WIDTH 300

void newGuiPopupSetup(NewGui* gui) {

	gui->zLevel = 5;

	if(gui->popupStackCount > 0) {

		TextBoxSettings s = gui->textBoxSettings;
		s.boxSettings.color.a = 0;
		s.boxSettings.borderColor.a = 0;

		// Hack. We ignore the mouseclick that probably created the popup.
		if(gui->test == 1 && gui->input->mouseButtonPressed[0])
		{
			bool overPopup = false;
			for(int i = 0; i < gui->popupStackCount; i++) {
				if(pointInRectEx(gui->input->mousePosNegative, gui->popupStack[i].rCheck)) {
					overPopup = true;
					break;
				}
			}
			if(!overPopup) {
				gui->popupStackCount = 0;
			}
		}
		if(gui->input->mouseButtonPressed[0]) gui->test = 1;


		// if(newGuiGoButtonAction(gui, getScreenRect(gui->windowSettings))) {
		// 	gui->popupStackCount = 0;
		// }

		// // Capture all mouse clicks.
		// int id = newGuiIncrementId(gui);
		// newGuiSetHotAll(gui, id, gui->zLevel);
		// int focus[] = {Gui_Focus_MLeft, Gui_Focus_MRight, Gui_Focus_MMiddle};
		// for(int i = 0; i < arrayCount(focus); i++) {
		// 	newGuiSetActive(gui, id, newGuiInputFromFocus(gui->input, focus[i]), focus[i]);
		// 	newGuiSetNotActiveWhenActive(gui, id);			
		// }

		// if(newGuiGotActive(gui, id)) gui->popupStackCount = 0;
	} else {
		gui->menuActive = false;
		gui->test = 0;
	}

	newGuiUpdateComboBoxPopups(gui);
}

void newGuiUpdateComboBoxPopups(NewGui* gui) {
	for(int i = 0; i < gui->popupStackCount; i++) {
		PopupData pd = gui->popupStack[i];
		int popupIndex = i;

		if(pd.type == POPUP_TYPE_COMBO_BOX) {
			ComboBoxData cData = gui->comboBoxData;
			BoxSettings ps = gui->popupSettings;

			float comboboxPopupTopOffset = 2;

			Font* font = gui->textSettings.font;
			float fontHeight = font->height;
			float padding = gui->comboBoxSettings.sideAlignPadding;
			// float topBottomPadding = padding*0.3f;
			float topBottomPadding = 0;

			float eh = fontHeight * 1.2f;

			float border = 0;
			if(ps.borderColor.a > 0) border = 1;

			float maxWidth = 0;
			for(int i = 0; i < cData.count; i++) {
				float w = getTextDim(cData.strings[i], font).w;
				maxWidth = max(maxWidth, w);
			}
			maxWidth += padding*2 + 4;
			clampMin(&maxWidth, POPUP_MIN_WIDTH);

			float popupWidth = max(maxWidth, rectW(pd.r));
			float popupHeight = eh * cData.count + border*2 + topBottomPadding*2;

			Rect r = rectTDim(rectT(pd.r)-vec2(0,comboboxPopupTopOffset), vec2(popupWidth, popupHeight));
			gui->popupStack[popupIndex].rCheck = r;

			// Shadow.
			Rect shadowRect = rectTrans(r, vec2(1,-1)*padding*0.5f);
			drawRect(shadowRect, vec4(0,0.8f));

			// Background.
			drawBox(r, gui->scissor, gui->popupSettings);


			Rect lr = rectExpand(r, vec2(-border*2));
			Vec2 p = rectTL(lr) + vec2(0,-topBottomPadding);

			float ew = rectW(lr);

			Vec4 cButton = gui->popupSettings.color;
			TextBoxSettings tbs = textBoxSettings(gui->comboBoxSettings.textSettings, boxSettings());
			tbs.sideAlignPadding = padding - border;

			BoxSettings bs = boxSettings(cButton);
			BoxSettings bsSelected = gui->editSettings.textBoxSettings.boxSettings;

			for(int i = 0; i < cData.count; i++) {
				bool selected = cData.index == i;
				tbs.boxSettings = selected ? bsSelected : bs;

				Rect br = rectTLDim(p, vec2(ew, eh)); p.y -= eh;
				// if(selected) br = rectExpand(br, vec2(-padding*0.5f,0));

				if(newGuiQuickButton(gui, br, cData.strings[i], vec2i(-1,0), &tbs)) {
					gui->comboBoxData.index = i;
					gui->comboBoxData.finished = true;

					*gui->comboBoxIndex = gui->comboBoxData.index;
				}
			}
		}
	}
}







struct QuickRow {
	Rect* rects;
	int index;
	int count;
};

Rect quickRowNext(QuickRow* qr) {
	if(qr->index >= qr->count) return rect(0,0,0,0);
	else return qr->rects[qr->index++];
}

QuickRow quickRow(Rect r, float padding, float* columns, int count) {
	Layout lay = layout(rect(0,0,0,0), false, vec2i(-1,0), vec2(padding, 0));
	Layout* node = layoutQuickRowArray(&lay, r, columns, count);
	
	QuickRow qr = {};
	qr.rects = getTArray(Rect, count);
	qr.index = 0;
	qr.count = count;

	for(int i = 0; i < count; i++) qr.rects[i] = layoutInc(&node);

	return qr;
}

QuickRow quickRow(Rect r, float padding, float s0, float s1 = -1, float s2 = -1, float s3 = -1) {
	float columns[4] = {};
	int count = 0;
	columns[count++] = s0;
	if(s1 != -1) columns[count++] = s1;
	if(s2 != -1) columns[count++] = s2;
	if(s3 != -1) columns[count++] = s3;

	return quickRow(r, padding, columns, count);
}
