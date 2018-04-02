
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

#define Windows_Font_Folder "\\Fonts\\"
#define Windows_Font_Path_Variable "windir"

#define Fallback_Font "arial.ttf"
#define Fallback_Font_Italic "ariali.ttf"
#define Fallback_Font_Bold "arialbd.ttf"

#ifdef SHIPPING_MODE
#define App_Font_Folder ".\\data\\Fonts\\"
#define App_Image_Folder ".\\data\\Images\\"
#else
#define App_Font_Folder "..\\data\\Fonts\\"
#define App_Image_Folder "..\\data\\Images\\"
#endif

#define App_Session_File ".\\session.tmp"

#define Screenshot_Folder ".\\Screenshots"
#define Scenes_Folder ".\\Scenes"


#define App_Name "Pathtracer"

//

enum SamplerType {
	SAMPLER_NORMAL = 0,
	SAMPLER_NEAREST,
	SAMPLER_SIZE,
};

enum MeshType {
	MESH_CUBE = 0,
	MESH_SPHERE,
	MESH_CYLINDER,
	MESH_CONE,
	MESH_PLANE,

	MESH_SIZE,
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




struct AppSessionSettings {
	Rect windowRect;
	float mouseSpeed;
	float fontScale;
	float panelWidthLeft;
	float panelWidthRight;

	char sceneFile[200];
};

void appWriteSessionSettings(char* filePath, AppSessionSettings* at) {
	writeDataToFile((char*)at, sizeof(AppSessionSettings), filePath);
}

void appReadSessionSettings(char* filePath, AppSessionSettings* at) {
	readDataFile((char*)at, filePath);
}

void saveAppSettings(AppSessionSettings at) {
	if(fileExists(App_Session_File)) {
		appWriteSessionSettings(App_Session_File, &at);
	}
}
