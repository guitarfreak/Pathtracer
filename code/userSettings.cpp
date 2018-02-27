
#define USE_SRGB 1

#if USE_SRGB == 1
#define INTERNAL_TEXTURE_FORMAT GL_SRGB8_ALPHA8
#else 
#define INTERNAL_TEXTURE_FORMAT GL_RGBA8
#endif

#define COLOR_SRGB(color) \
	(globalGraphicsState->useSRGB ? colorSRGB(color) : color);


#define APP_FONT_COUNT 4



#define Window_Min_Size_X 300
#define Window_Min_Size_Y 400

#define App_Font_Folder "Fonts\\"
#define Windows_Font_Folder "\\Fonts\\"
#define Windows_Font_Path_Variable "windir"

#define App_Image_Folder "Images\\"

#define App_Save_File ".\\temp"

#define Screenshot_Folder ".\\Screenshots"
#define Scenes_Folder ".\\Scenes"

#define Fallback_Font "arial.ttf"
#define Fallback_Font_Italic "ariali.ttf"
#define Fallback_Font_Bold "arialbd.ttf"



//

enum TextureId {
	// TEXTURE_RECT,
	// TEXTURE_CIRCLE,
	// TEXTURE_TEST,
	// TEXTURE_WHITE = 0,
	TEXTURE_ROUNDED_SQUARE = 0,
	TEXTURE_SIZE,
};

char* texturePaths[] = {
	"..\\data\\Textures\\Misc\\white.png",
};

//

enum SamplerType {
	SAMPLER_NORMAL = 0,
	SAMPLER_NEAREST,
	SAMPLER_SIZE,
};

//

enum FrameBufferType {
	FRAMEBUFFER_2dMsaa = 0,
	FRAMEBUFFER_2dNoMsaa,

	FRAMEBUFFER_2dPanels,

	FRAMEBUFFER_ScreenShot,

	FRAMEBUFFER_SIZE,
};

enum WatchFolder {
	WATCH_FOLDER_APP = 0,
	WATCH_FOLDER_PLAYLISTS,
	
	WATCH_FOLDER_SIZE,
};




struct AppTempSettings {
	Rect windowRect;
};

void appWriteTempSettings(char* filePath, AppTempSettings* at) {
	writeDataToFile((char*)at, sizeof(AppTempSettings), filePath);
}

void appReadTempSettings(char* filePath, AppTempSettings* at) {
	readDataFile((char*)at, filePath);
}

void appTempDefault(AppTempSettings* at, Rect monitor) {
	Rect r = monitor;
	Vec2 center = vec2(rectCenX(r), (r.top - r.bottom)/2);
	Vec2 dim = vec2(rectW(r), -rectH(r));
	at->windowRect = rectCenDim(center, dim*0.85f);
}

void saveAppSettings(SystemData* sd) {
	if(fileExists(App_Save_File)) {
		AppTempSettings at = {};

		at.windowRect = getWindowWindowRect(sd->windowHandle);

		appWriteTempSettings(App_Save_File, &at);
	}
}