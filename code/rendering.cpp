
struct DrawCommandList;
extern DrawCommandList* globalCommandList;
struct GraphicsState;
extern GraphicsState* globalGraphicsState;


// 
// Misc.
// 

char* fillString(char* text, ...) {
	va_list vl;
	va_start(vl, text);

	int length = strLen(text);
	char* buffer = getTString(length+1);

	char valueBuffer[20] = {};

	int ti = 0;
	int bi = 0;
	while(true) {
		char t = text[ti];

		if(text[ti] == '%' && text[ti+1] == '.') {
			float v = va_arg(vl, double);
			floatToStr(valueBuffer, v, charToInt(text[ti+2]));
			int sLen = strLen(valueBuffer);
			memCpy(buffer + bi, valueBuffer, sLen);

			ti += 4;
			bi += sLen;
			getTString(sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'f') {
			float v = va_arg(vl, double);
			floatToStr(valueBuffer, v, 2);
			int sLen = strLen(valueBuffer);
			memCpy(buffer + bi, valueBuffer, sLen);

		ti += 2;
			bi += sLen;
			getTString(sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'i') {
			if(text[ti+2] == '6') {
				// 64 bit signed integer.

				myAssert(text[ti+3] == '4');

				i64 v = va_arg(vl, i64);
				intToStr(valueBuffer, v);
				int sLen = strLen(valueBuffer);

				if(text[ti+4] == '.') {
					ti += 1;

					int digitCount = intDigits(v);
					int commaCount = digitCount/3;
					if(digitCount%3 == 0) commaCount--;
					for(int i = 0; i < commaCount; i++) {
						strInsert(valueBuffer, sLen - (i+1)*3 - i, ',');
						sLen++;
					}
				}

				memCpy(buffer + bi, valueBuffer, sLen);
				ti += 4;
				bi += sLen;
				getTString(sLen);
			} else {
				// 32 bit signed integer.
				int v = va_arg(vl, int);
				intToStr(valueBuffer, v);
				int sLen = strLen(valueBuffer);

				if(text[ti+2] == '.') {
					ti += 1;

					int digitCount = intDigits(v);
					int commaCount = digitCount/3;
					if(digitCount%3 == 0) commaCount--;
					for(int i = 0; i < commaCount; i++) {
						strInsert(valueBuffer, sLen - (i+1)*3 - i, ',');
						sLen++;
					}
				}

				memCpy(buffer + bi, valueBuffer, sLen);

				ti += 2;
				bi += sLen;
				getTString(sLen);
			}
		} else if(text[ti] == '%' && text[ti+1] == 's') {
			char* str = va_arg(vl, char*);
			int sLen = strLen(str);
			memCpy(buffer + bi, str, sLen);

			ti += 2;
			bi += sLen;
			getTString(sLen);
		} else if(text[ti] == '%' && text[ti+1] == 'b') {
			bool str = va_arg(vl, bool);
			char* s = str == 1 ? "true" : "false";
			int sLen = strLen(s);
			memCpy(buffer + bi, s, sLen);

			ti += 2;
			bi += sLen;
			getTString(sLen);
		} else if(text[ti] == '%' && text[ti+1] == '%') {
			buffer[bi++] = '%';
			ti += 2;
			getTString(1);
		} else {
			buffer[bi++] = text[ti++];
			getTString(1);

			if(buffer[bi-1] == '\0') break;
		}
	}

	return buffer;
}

//
// Textures.
// 

struct Texture {
	bool isCreated;
	char* name;
	uint id;

	Vec2i dim;
	int channels;
	int levels;
	int internalFormat;
	int channelType;
	int channelFormat;

	bool isRenderBuffer;
	int msaa;
};

int getMaximumMipmapsFromSize(int w, int h) {
	int size = min(w, h);

	int mipLevels = 1;
	while(size >= 2) {
		size /= 2;
		mipLevels++;
	}

	return mipLevels;
}

void loadTexture(Texture* texture, unsigned char* buffer, int w, int h, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {

	if(!reload) {
		glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
		glTextureStorage2D(texture->id, mipLevels, internalFormat, w, h);

		texture->dim = vec2i(w,h);
		texture->channels = 4;
		texture->levels = mipLevels;
	}	

	glTextureSubImage2D(texture->id, 0, 0, 0, w, h, channelType, channelFormat, buffer);

	glTextureParameteri(texture->id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(texture->id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glGenerateTextureMipmap(texture->id);
}

void loadTextureFromFile(Texture* texture, char* path, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {
	int x,y,n;
	unsigned char* stbData = stbi_load(path, &x, &y, &n, 0);

	if(mipLevels == -1) mipLevels = getMaximumMipmapsFromSize(x,y);
	
	loadTexture(texture, stbData, x, y, mipLevels, internalFormat, channelType, channelFormat, reload);

	stbi_image_free(stbData);
}

void loadTextureFromMemory(Texture* texture, char* buffer, int length, int mipLevels, int internalFormat, int channelType, int channelFormat, bool reload = false) {
	int x,y,n;
	unsigned char* stbData = stbi_load_from_memory((uchar*)buffer, length, &x, &y, &n, 0);

	if(mipLevels == -1) mipLevels = getMaximumMipmapsFromSize(x,y);
	
	loadTexture(texture, stbData, x, y, mipLevels, internalFormat, channelType, channelFormat, reload);

	stbi_image_free(stbData);
}

void createTexture(Texture* texture, bool isRenderBuffer = false) {	
	if(!isRenderBuffer) glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
	else glCreateRenderbuffers(1, &texture->id);
}

void recreateTexture(Texture* t) {
	glDeleteTextures(1, &t->id);
	glCreateTextures(GL_TEXTURE_2D, 1, &t->id);

	glTextureStorage2D(t->id, 1, t->internalFormat, t->dim.w, t->dim.h);
}

void deleteTexture(Texture* t) {
	glDeleteTextures(1, &t->id);
}

void initTexture(Texture* texture, int mipLevels, int internalFormat, Vec2i dim, int channels, int filterMode, int wrapMode) {
	*texture = {};

	if(mipLevels == -1) mipLevels = getMaximumMipmapsFromSize(dim.w, dim.h);

	glCreateTextures(GL_TEXTURE_2D, 1, &texture->id);
	glTextureStorage2D(texture->id, mipLevels, internalFormat, dim.w, dim.h);

	texture->dim = vec2i(dim.w,dim.h);
	texture->channels = channels;
	texture->levels = mipLevels;

	glTextureParameteri(texture->id, GL_TEXTURE_MIN_FILTER, filterMode);
	glTextureParameteri(texture->id, GL_TEXTURE_MAG_FILTER, filterMode);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_S, wrapMode);
	glTextureParameteri(texture->id, GL_TEXTURE_WRAP_T, wrapMode);

	glGenerateTextureMipmap(texture->id);

	texture->isCreated = true;
}

//
// Fonts.
//

struct PackedChar;
struct Font {
	char* file;
	int id;
	float heightIndex;

	FT_Library library;
	FT_Face face;

	float pixelScale;

	char* fileBuffer;
	Texture tex;
	Vec2i glyphRanges[5];
	int glyphRangeCount;
	int totalGlyphCount;

	PackedChar* cData;
	int height;
	float baseOffset;
	float lineSpacing;

	Font* boldFont;
	Font* italicFont;

	bool pixelAlign;
};

struct PackedChar {
   unsigned short x0,y0,x1,y1;
   float xBearing, yBearing;
   float width, height;
   float xadvance; // yBearing + h-yBearing
};
//
// Framebuffers.
//

enum FrameBufferSlot {
	FRAMEBUFFER_SLOT_COLOR,
	FRAMEBUFFER_SLOT_DEPTH,
	FRAMEBUFFER_SLOT_STENCIL,
	FRAMEBUFFER_SLOT_DEPTH_STENCIL,
};

struct FrameBuffer {
	uint id;

	union {
		struct {
			Texture* colorSlot[4];
			Texture* depthSlot[4];
			Texture* stencilSlot[4];
			Texture* depthStencilSlot[4];
		};

		struct {
			Texture* slots[16];
		};
	};
};

FrameBuffer* getFrameBuffer(int id);
Texture* addTexture(Texture tex);

void initFrameBuffer(FrameBuffer* fb) {
	glCreateFramebuffers(1, &fb->id);

	for(int i = 0; i < arrayCount(fb->slots); i++) {
		fb->slots[i] = 0;
	}
}

void attachToFrameBuffer(FrameBuffer* fb, int slot, int internalFormat, int w, int h, int msaa = 0) {
	bool isRenderBuffer = msaa > 0;

	Texture t;
	createTexture(&t, isRenderBuffer);
	t.internalFormat = internalFormat;
	t.dim.w = w;
	t.dim.h = h;
	t.isRenderBuffer = isRenderBuffer;
	t.msaa = msaa;

	t.name = getPStringCpy("");
	Texture* tex = addTexture(t);

	Vec2i indexRange;
	if(slot == FRAMEBUFFER_SLOT_COLOR) indexRange = vec2i(0,4);
	else if(slot == FRAMEBUFFER_SLOT_DEPTH) indexRange = vec2i(4,8);
	else if(slot == FRAMEBUFFER_SLOT_STENCIL) indexRange = vec2i(8,12);
	else if(slot == FRAMEBUFFER_SLOT_DEPTH_STENCIL) indexRange = vec2i(12,16);

	for(int i = indexRange.x; i <= indexRange.y; i++) {
		if(fb->slots[i] == 0) {
			fb->slots[i] = tex;
			break;
		}
	}

}

void attachToFrameBuffer(int id, int slot, int internalFormat, int w, int h, int msaa = 0) {
	return attachToFrameBuffer(getFrameBuffer(id), slot, internalFormat, w, h, msaa);
}

void reloadFrameBuffer(FrameBuffer* fb) {

	for(int i = 0; i < arrayCount(fb->slots); i++) {
		if(!fb->slots[i]) continue;
		Texture* t = fb->slots[i];

		int slot;
		if(valueBetween(i, 0, 3)) slot = GL_COLOR_ATTACHMENT0 + i;
		else if(valueBetween(i, 4, 7)) slot = GL_DEPTH_ATTACHMENT;
		else if(valueBetween(i, 8, 11)) slot = GL_STENCIL_ATTACHMENT;
		else if(valueBetween(i, 12, 15)) slot = GL_DEPTH_STENCIL_ATTACHMENT;

		if(t->isRenderBuffer) {
			glNamedRenderbufferStorageMultisample(t->id, t->msaa, t->internalFormat, t->dim.w, t->dim.h);
			glNamedFramebufferRenderbuffer(fb->id, slot, GL_RENDERBUFFER, t->id);
		} else {
			glDeleteTextures(1, &t->id);
			glCreateTextures(GL_TEXTURE_2D, 1, &t->id);
			glTextureStorage2D(t->id, 1, t->internalFormat, t->dim.w, t->dim.h);
			glNamedFramebufferTexture(fb->id, slot, t->id, 0);
		}
	}
}

void reloadFrameBuffer(int id) {
	return reloadFrameBuffer(getFrameBuffer(id));
}

void blitFrameBuffers(int id1, int id2, Vec2i dim, int bufferBit, int filter) {
	FrameBuffer* fb1 = getFrameBuffer(id1);
	FrameBuffer* fb2 = getFrameBuffer(id2);

	glBlitNamedFramebuffer (fb1->id, fb2->id, 0,0, dim.x, dim.y, 0,0, dim.x, dim.y, bufferBit, filter);
}

void bindFrameBuffer(FrameBuffer* fb, int slot = GL_FRAMEBUFFER) {
	glBindFramebuffer(slot, fb->id);
}

void bindFrameBuffer(int id, int slot = GL_FRAMEBUFFER) {
	return bindFrameBuffer(getFrameBuffer(id), slot);
}

void setDimForFrameBufferAttachmentsAndUpdate(FrameBuffer* fb, int w, int h) {
	for(int i = 0; i < arrayCount(fb->slots); i++) {
		if(!fb->slots[i]) continue;
		Texture* t = fb->slots[i];

		t->dim = vec2i(w, h);
	}

	reloadFrameBuffer(fb);
}

void setDimForFrameBufferAttachmentsAndUpdate(int id, int w, int h) {
	return setDimForFrameBufferAttachmentsAndUpdate(getFrameBuffer(id), w, h);
}

uint checkStatusFrameBuffer(int id) {
	FrameBuffer* fb = getFrameBuffer(id);
	GLenum result = glCheckNamedFramebufferStatus(fb->id, GL_FRAMEBUFFER);
	return result;
}

//
// Data.
//

#pragma pack(push,1)
struct MeshVertex {
	Vec3 p;
	Vec3 n;
};
#pragma pack(pop)

struct Mesh {
	MeshVertex* vertices;
	int vertexCount;
};

struct GraphicsState {
	bool useSRGB;

	Mesh* meshes;
	int meshCount;
	int meshCountMax;

	Texture* textures;
	int textureCount;
	int textureCountMax;

	Texture textures3d[2];
	GLuint samplers[SAMPLER_SIZE];

	Font fonts[10][20];
	int fontsCount;
	char* fontFolders[10];
	int fontFolderCount;
	char* fallbackFont, *fallbackFontBold, *fallbackFontItalic;

	GLuint textureUnits[16];
	GLuint samplerUnits[16];

	FrameBuffer frameBuffers[FRAMEBUFFER_SIZE];

	float zOrder;
	Vec2i screenRes;
	Vec2 clientRectOffset;
};

void setSRGB(bool enable = true) {
	if(enable) glEnable(GL_FRAMEBUFFER_SRGB);
	else glDisable(GL_FRAMEBUFFER_SRGB);

	globalGraphicsState->useSRGB = enable;
}

Texture* getTexture(int textureId) {
	Texture* t = globalGraphicsState->textures + textureId;
	return t;
}

Texture* getTexture(char* name) {
	for(int i = 0; i < globalGraphicsState->textureCount; i++) {
		Texture* t = globalGraphicsState->textures + i;

		if(strCompare(t->name, name)) {
			return t;
		}
	}

	return (&globalGraphicsState->textures[0]);
}

Texture* addTexture(Texture tex) {
	GraphicsState* gs = globalGraphicsState;

	assert(gs->textureCount < gs->textureCountMax);

	gs->textures[gs->textureCount++] = tex;
	return gs->textures + (gs->textureCount - 1);
}

FrameBuffer* getFrameBuffer(int id) {
	FrameBuffer* fb = globalGraphicsState->frameBuffers + id;
	return fb;
}

#define Font_Error_Glyph (int)0x20-1

Font* fontInit(Font* fontSlot, char* file, float height, bool enableHinting = false) {
	char* fontFolder = 0;
	for(int i = 0; i < globalGraphicsState->fontFolderCount; i++) {
		if(fileExists(fillString("%s%s", globalGraphicsState->fontFolders[i], file))) {
			fontFolder = globalGraphicsState->fontFolders[i];
			break;
		}
	}
	if(!fontFolder) return 0;

	char* path = fillString("%s%s", fontFolder, file);



	Font font;

	// Settings.
	
	bool stemDarkening = true;
	bool pixelAlign = true;
	
	// FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_FORCE_AUTOHINT
	// FT_LOAD_TARGET_NORMAL | FT_LOAD_TARGET_LIGHT | FT_LOAD_TARGET_MONO
	int loadFlags = FT_LOAD_DEFAULT | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL;

	// FT_RENDER_MODE_NORMAL, FT_RENDER_MODE_LIGHT, FT_RENDER_MODE_MONO, FT_RENDER_MODE_LCD, FT_RENDER_MODE_LCD_V,
	FT_Render_Mode renderFlags = FT_RENDER_MODE_NORMAL;


	font.glyphRangeCount = 0;
	#define setupRange(a,b) vec2i(a, b - a + 1)
	font.glyphRanges[font.glyphRangeCount++] = setupRange(0x20, 0x7F);
	font.glyphRanges[font.glyphRangeCount++] = setupRange(0xA1, 0xFF);
	font.glyphRanges[font.glyphRangeCount++] = setupRange(0x25BA, 0x25C4);
	#undef setupRange

	font.totalGlyphCount = 0;
	for(int i = 0; i < font.glyphRangeCount; i++) font.totalGlyphCount += font.glyphRanges[i].y;



	font.file = getPString(strLen(file)+1);
	strCpy(font.file, file);
	font.heightIndex = height;

	int error;
	error = FT_Init_FreeType(&font.library); assert(error == 0);
	error = FT_New_Face(font.library, path, 0, &font.face); assert(error == 0);
	FT_Face face = font.face;

	FT_Parameter parameter;
	FT_Bool darkenBool = stemDarkening;
	parameter.tag = FT_PARAM_TAG_STEM_DARKENING;
	parameter.data = &darkenBool;
	error = FT_Face_Properties(face, 1, &parameter); assert(error == 0);

	int pointFraction = 64;
	font.pixelScale = (float)1/pointFraction;
	float fullHeightToAscend = (float)face->ascender / (float)(face->ascender + abs(face->descender));

	// Height < 0 means use point size instead of pixel size
	if(height > 0) {
		error = FT_Set_Pixel_Sizes(font.face, 0, roundInt(height) * fullHeightToAscend); assert(error == 0);
	} else {
		error = FT_Set_Char_Size(font.face, 0, (roundInt(-height) * fullHeightToAscend) * pointFraction, 0, 0); assert(error == 0);
	}

	// Get true height from freetype.
	font.height = (face->size->metrics.ascender + abs(face->size->metrics.descender)) / pointFraction;
	font.baseOffset = (face->size->metrics.ascender / pointFraction);

	// We calculate the scaling ourselves because Freetype doesn't offer it??
	float scale = (float)face->size->metrics.ascender / (float)face->ascender;
	font.lineSpacing = roundInt(((face->height * scale) / pointFraction));
	font.pixelAlign = pixelAlign;



	int gridSize = (sqrt(font.totalGlyphCount) + 1);
	Vec2i texSize = vec2i(gridSize * font.height);
	uchar* fontBitmapBuffer = mallocArray(unsigned char, texSize.x*texSize.y);
	memSet(fontBitmapBuffer, 0, texSize.x*texSize.y);

	{
		font.cData = mallocArray(PackedChar, font.totalGlyphCount);
		int glyphIndex = 0;
		for(int rangeIndex = 0; rangeIndex < font.glyphRangeCount; rangeIndex++) {
			for(int i = 0; i < font.glyphRanges[rangeIndex].y; i++) {
				int unicode = font.glyphRanges[rangeIndex].x + i;

				FT_Load_Char(face, unicode, loadFlags);
				FT_Render_Glyph(face->glyph, renderFlags);

				FT_Bitmap* bitmap = &face->glyph->bitmap;
				Vec2i coordinate = vec2i(glyphIndex%gridSize, glyphIndex/gridSize);
				Vec2i startPixel = coordinate * font.height;

				font.cData[glyphIndex].x0 = startPixel.x;
				font.cData[glyphIndex].x1 = startPixel.x + bitmap->width;
				font.cData[glyphIndex].y1 = startPixel.y + bitmap->rows;
				font.cData[glyphIndex].y0 = startPixel.y;

				font.cData[glyphIndex].xBearing = face->glyph->metrics.horiBearingX / pointFraction;
				font.cData[glyphIndex].yBearing = face->glyph->metrics.horiBearingY / pointFraction;
				font.cData[glyphIndex].width =    face->glyph->metrics.width        / pointFraction;
				font.cData[glyphIndex].height =   face->glyph->metrics.height       / pointFraction;

				font.cData[glyphIndex].xadvance = face->glyph->metrics.horiAdvance / pointFraction;

				for(int y = 0; y < bitmap->rows; y++) {
					for(int x = 0; x < bitmap->width; x++) {
						Vec2i coord = startPixel + vec2i(x,y);
						fontBitmapBuffer[coord.y*texSize.w + coord.x] = bitmap->buffer[y*bitmap->width + x];
					}
				}

				glyphIndex++;
			}
		}
	}


	Texture tex;
	uchar* fontBitmap = mallocArray(unsigned char, texSize.x*texSize.y*4);
	memSet(fontBitmap, 255, texSize.w*texSize.h*4);
	for(int i = 0; i < texSize.w*texSize.h; i++) fontBitmap[i*4+3] = fontBitmapBuffer[i];

	loadTexture(&tex, fontBitmap, texSize.w, texSize.h, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
	font.tex = tex;


	free(fontBitmapBuffer);
	free(fontBitmap);

	*fontSlot = font;
	return fontSlot;
}

void freeFont(Font* font) {
	freeZero(font->cData);
	// FT_Done_Face(font->face);
	// FT_Done_Library(font->library);
	glDeleteTextures(1, &font->tex.id);
	font->heightIndex = 0;
}

Font* getFont(char* fontFile, float heightIndex, char* boldFontFile = 0, char* italicFontFile = 0) {

	int fontCount = arrayCount(globalGraphicsState->fonts);
	int fontSlotCount = arrayCount(globalGraphicsState->fonts[0]);
	Font* fontSlot = 0;
	for(int i = 0; i < fontCount; i++) {
		if(globalGraphicsState->fonts[i][0].heightIndex == 0) {
			fontSlot = &globalGraphicsState->fonts[i][0];
			break;
		} else {
			if(strCompare(fontFile, globalGraphicsState->fonts[i][0].file)) {
				for(int j = 0; j < fontSlotCount; j++) {
					float h = globalGraphicsState->fonts[i][j].heightIndex;
					if(h == 0 || h == heightIndex) {
						fontSlot = &globalGraphicsState->fonts[i][j];
						goto forEnd;
					}
				}
			}
		}
	}
	forEnd:

	// We are going to assume for now that a font size of 0 means it is uninitialized.
	if(fontSlot->heightIndex == 0) {
		Font* font = fontInit(fontSlot, fontFile, heightIndex);
		if(!font) {
			return getFont(globalGraphicsState->fallbackFont, heightIndex, globalGraphicsState->fallbackFontBold, globalGraphicsState->fallbackFontItalic);
		}

		if(boldFontFile) {
			fontSlot->boldFont = getPStruct(Font);
			fontInit(fontSlot->boldFont, boldFontFile, heightIndex);
		} else font->boldFont = 0;

		if(italicFontFile) {
			fontSlot->italicFont = getPStruct(Font);
			fontInit(fontSlot->italicFont, italicFontFile, heightIndex);
		} else font->italicFont = 0;
	}

	return fontSlot;
}


void setViewPort(Rect r) {
	int left   = roundInt(r.left);
	int bottom = roundInt(r.bottom);
	int right  = roundInt(r.right);
	int top    = roundInt(r.top);

	glViewport(left, bottom, right-left, top-bottom);
}

Rect scissorRectScreenSpace(Rect r, float screenHeight) {
	Rect scissorRect = {r.min.x, r.min.y+screenHeight, r.max.x, r.max.y+screenHeight};
	return scissorRect;
}

void scissorTest(Rect r) {
	int left   = roundInt(r.left);
	int bottom = roundInt(r.bottom);
	int right  = roundInt(r.right);
	int top    = roundInt(r.top);

	glScissor(left, bottom, right-left, top-bottom);
}

void scissorTest(Rect r, float screenHeight) {
	Rect sr = scissorRectScreenSpace(r, screenHeight);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	sr = rectTrans(sr, globalGraphicsState->clientRectOffset);

	scissorTest(sr);
}

void scissorTestScreen(Rect r) {
	Rect sr = scissorRectScreenSpace(r, globalGraphicsState->screenRes.h);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	sr = rectTrans(sr, globalGraphicsState->clientRectOffset);

	scissorTest(sr);
}

void setClientViewport(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->clientRectBL.min);
	setViewPort(vp);
}

void setClientScissor(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->clientRectBL.min);
	scissorTest(vp);
}

void setWindowViewport(WindowSettings* ws, Rect vp) {
	vp = rectTrans(vp, ws->windowRectBL.min);
	setViewPort(vp);
}

void scissorState(bool state = true) {
	if(state) glEnable(GL_SCISSOR_TEST);
	else glDisable(GL_SCISSOR_TEST);
}

void depthState(bool state = true) {
	if(state) glDepthFunc(GL_LESS);
	else glDepthFunc(GL_ALWAYS);
}

void drawLinesHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
}

void drawLineStripHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINE_STRIP);
}

void drawPointsHeader(Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_POINTS);
}

inline void pushVecs(Vec2 p0, Vec2 p1) {
	glVertex2f(p0.x, p0.y);
	glVertex2f(p1.x, p1.y);
}
inline void pushVecs(Vec2 p0, Vec2 p1, float z) {
	glVertex3f(p0.x, p0.y, z);
	glVertex3f(p1.x, p1.y, z);
}
inline void pushVec(Vec2 p0) {
	glVertex2f(p0.x, p0.y);
}
inline void pushVec(Vec2 p0, float z) {
	glVertex3f(p0.x, p0.y, z);
}
inline void pushVec(Vec3 v) {
	glVertex3f(v.x, v.y, v.z);
}
inline void pushColor(Vec4 c) {
	glColor4f(c.r, c.g, c.b, c.a);
}

inline void pushRect(Rect r, Rect uv, float z) {
	glTexCoord2f(uv.left,  uv.top);    glVertex3f(r.left, r.bottom, z);
	glTexCoord2f(uv.left,  uv.bottom); glVertex3f(r.left, r.top, z);
	glTexCoord2f(uv.right, uv.bottom); glVertex3f(r.right, r.top, z);
	glTexCoord2f(uv.right, uv.top);    glVertex3f(r.right, r.bottom, z);
}

void drawPoint(Vec2 p, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_POINTS);
		glVertex3f(p.x, p.y, globalGraphicsState->zOrder);
	glEnd();
}

void drawLine(Vec2 p0, Vec2 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, globalGraphicsState->zOrder);
		glVertex3f(p1.x, p1.y, globalGraphicsState->zOrder);
	glEnd();
}

void drawLineNewOff(Vec2 p0, Vec2 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, 0.0f);
		glVertex3f(p0.x + p1.x, p0.y + p1.y, globalGraphicsState->zOrder);
	glEnd();
}

void drawRect(Rect r, Vec4 color, Rect uv, int texture) {	
	// if(texture == -1) texture = getTexture(TEXTURE_WHITE)->id;
	float z = globalGraphicsState->zOrder;

	glBindTexture(GL_TEXTURE_2D, texture);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);
		glTexCoord2f(uv.left,  uv.top); glVertex3f(r.left, r.bottom, z);
		glTexCoord2f(uv.left,  uv.bottom); glVertex3f(r.left, r.top, z);
		glTexCoord2f(uv.right, uv.bottom); glVertex3f(r.right, r.top, z);
		glTexCoord2f(uv.right, uv.top); glVertex3f(r.right, r.bottom, z);
	glEnd();
}

void drawRect(Rect r, Rect uv, int texture) {
	drawRect(r, vec4(1,1), uv, texture);
}

void drawRect(Rect r, Vec4 color) {	
	glBindTexture(GL_TEXTURE_2D, 0);

	float z = globalGraphicsState->zOrder;
	Vec4 c = COLOR_SRGB(color);

	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_QUADS);
		glVertex3f(r.left, r.bottom, z);
		glVertex3f(r.left, r.top, z);
		glVertex3f(r.right, r.top, z);
		glVertex3f(r.right, r.bottom, z);
	glEnd();
}

void drawRectOutline(Rect r, Vec4 color, int offset = -1) {	
	// We assume glLineWidth(1.0f) for now.
	// Offset -1 means draw the lines inside, offset 0 is directly on edge.

	float z = globalGraphicsState->zOrder;

	// More complicated to get the corners right 
	// or they won't be filled correctly.

	drawLinesHeader(color);
	rectExpand(&r, offset);
	pushVec(rectBL(r)+vec2(0, 0.5f), z);
	pushVec(rectTL(r)+vec2(0, 0.5f), z);
	pushVec(rectTL(r)+vec2(0.5f, 0), z);
	pushVec(rectTR(r)+vec2(0.5f, 0), z);
	pushVec(rectTR(r)+vec2(0,-0.5f), z);
	pushVec(rectBR(r)+vec2(0,-0.5f), z);
	pushVec(rectBR(r)+vec2(-0.5f,0), z);
	pushVec(rectBL(r)+vec2(-0.5f,0), z);
	glEnd();
}

void drawRectOutlined(Rect r, Vec4 color, Vec4 colorOutline, int offset = -1) {	

	drawRect(r, color);
	drawRectOutline(r, colorOutline, offset);
}

void drawRectNewColored(Rect r, Vec4 c0, Vec4 c1, Vec4 c2, Vec4 c3) {	
	glBindTexture(GL_TEXTURE_2D, 0);

	float z = globalGraphicsState->zOrder;

	Vec4 cs0 = COLOR_SRGB(c0);
	Vec4 cs1 = COLOR_SRGB(c1);
	Vec4 cs2 = COLOR_SRGB(c2);
	Vec4 cs3 = COLOR_SRGB(c3);

	glColor4f(1,1,1,1);
	glBegin(GL_QUADS);
		glColor4f(cs0.r, cs0.g, cs0.b, cs0.a); glVertex3f(r.left, r.bottom, z);
		glColor4f(cs1.r, cs1.g, cs1.b, cs1.a); glVertex3f(r.left, r.top, z);
		glColor4f(cs2.r, cs2.g, cs2.b, cs2.a); glVertex3f(r.right, r.top, z);
		glColor4f(cs3.r, cs3.g, cs3.b, cs3.a); glVertex3f(r.right, r.bottom, z);
	glEnd();
}

void drawRectNewColoredH(Rect r, Vec4 c0, Vec4 c1) {	
	drawRectNewColored(r, c0, c1, c1, c0);
}
void drawRectNewColoredW(Rect r, Vec4 c0, Vec4 c1) {	
	drawRectNewColored(r, c0, c0, c1, c1);
}

#define Rounding_Mod 1/2

void drawRectRounded(Rect r, Vec4 color, float size) {
	if(size == 0) {
		drawRect(r, color);
		return;
	}

	int steps = roundInt(M_PI_2 * size * Rounding_Mod);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	float s = size;
	s = min(s, min(rectW(r)/2, rectH(r)/2));

	float z = globalGraphicsState->zOrder;

	drawRect(rect(r.min.x+s, r.min.y, r.max.x-s, r.max.y), color);
	drawRect(rect(r.min.x, r.min.y+s, r.max.x, r.max.y-s), color);

	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	Rect rc = rectExpand(r, -vec2(s,s)*2);
	Vec2 corners[] = {rc.max, vec2(rc.max.x, rc.min.y), rc.min, vec2(rc.min.x, rc.max.y)};
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
		glBegin(GL_TRIANGLE_FAN);

		Vec2 corner = corners[cornerIndex];
		float round = s;
		float start = M_PI_2*cornerIndex;

		glVertex3f(corner.x, corner.y, z);

		for(int i = 0; i < steps; i++) {
			float angle = start + i*(M_PI_2/(steps-1));
			Vec2 v = vec2(sin(angle), cos(angle));
			Vec2 vv = corner + v*round;

			glVertex3f(vv.x, vv.y, z);
		}
		glEnd();
	}

	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
};

void drawRectShadow(Rect r, Vec4 color, float alpha, float size, bool drawBackground = false) {
	float z = globalGraphicsState->zOrder;

	if(drawBackground) drawRect(r, color);

	size *= 2;

	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	Vec4 c2 = vec4(0,alpha);
	glBegin(GL_QUAD_STRIP);

	pushColor(c); pushVec(rectBL(r), z); pushColor(c2); pushVec(rectBL(r) + normVec2(vec2(-1,-1))*size, z);
	pushColor(c); pushVec(rectTL(r), z); pushColor(c2); pushVec(rectTL(r) + normVec2(vec2(-1, 1))*size, z);
	pushColor(c); pushVec(rectTR(r), z); pushColor(c2); pushVec(rectTR(r) + normVec2(vec2( 1, 1))*size, z);
	pushColor(c); pushVec(rectBR(r), z); pushColor(c2); pushVec(rectBR(r) + normVec2(vec2( 1,-1))*size, z);
	pushColor(c); pushVec(rectBL(r), z); pushColor(c2); pushVec(rectBL(r) + normVec2(vec2(-1,-1))*size, z);

	glEnd();
};

void drawRectShadow(Rect r, Vec4 color, float size, bool drawBackground = false) {
	return drawRectShadow(r, color, 0, size, drawBackground);
};

void drawRectRoundedOutline(Rect r, Vec4 color, float size, float offset = -1) {
	if(size == 0) {
		drawRectOutline(r, color);
		return;
	}

	int steps = roundInt(M_PI_2 * size * Rounding_Mod);

	float s = size;
	s = min(s, min(rectW(r)/2, rectH(r)/2));
	float z = globalGraphicsState->zOrder;

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBegin(GL_LINE_STRIP);

	float off = offset*0.5f;
	Rect rc = rectExpand(r, -vec2(s-off,s-off)*2);
	Vec2 corners[] = {rc.max, rectBR(rc), rc.min, rectTL(rc)};
	for(int cornerIndex = 0; cornerIndex < 4; cornerIndex++) {

		Vec2 corner = corners[cornerIndex];
		float round = s;
		float start = M_PI_2*cornerIndex;

		for(int i = 0; i < steps; i++) {
			float angle = start + i*(M_PI_2/(steps-1));
			Vec2 v = vec2(sin(angle), cos(angle));
			Vec2 vv = corner + v*round;

			glVertex3f(vv.x, vv.y, z);
		}
	}
	glVertex3f(rc.max.x, rc.max.y + s, z);

	glEnd();
};

void drawRectRoundedOutlined(Rect r, Vec4 color, Vec4 color2, float size, float offset = -1) {
	drawRectRounded(r, color, size);
	drawRectRoundedOutline(r, color2, size, offset);
};

void drawRectHollow(Rect r, float size, Vec4 c) {
	drawRect(rectSetB(r, r.top-size), c);
	drawRect(rectSetL(r, r.right-size), c);
	drawRect(rectSetT(r, r.bottom+size), c);
	drawRect(rectSetR(r, r.left+size), c);
}

void drawRectProgress(Rect r, float p, Vec4 c0, Vec4 c1, bool outlined, Vec4 oc) {	
	if(outlined) {
		drawRectOutlined(rectSetR(r, r.left + rectW(r)*p), c0, oc, 1);
		drawRectOutlined(rectSetL(r, r.right - rectW(r)*(1-p)), c1, oc, 1);
	} else {
		drawRect(rectSetR(r, r.left + rectW(r)*p), c0);
		drawRect(rectSetL(r, r.right - rectW(r)*(1-p)), c1);
	}
}

void drawRectProgressHollow(Rect r, float p, Vec4 c0, Vec4 oc) {	
	Rect leftR = rectSetR(r, r.left + rectW(r)*p);
	drawRect(leftR, c0);

	drawLine(rectBR(leftR), rectTR(leftR), oc);

	glLineWidth(1);
	drawRectOutline(r, oc);
}

void drawTriangle(Vec2 p, float size, Vec2 dir, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	float z = globalGraphicsState->zOrder;

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	dir = normVec2(dir) * size;
	Vec2 tp;
	glBegin(GL_TRIANGLES);
		tp = p + dir;
		glVertex3f(tp.x, tp.y, z);
		tp = p + rotateVec2(dir, degreeToRadian(360/3));
		glVertex3f(tp.x, tp.y, z);
		tp = p + rotateVec2(dir, degreeToRadian(360/3*2));
		glVertex3f(tp.x, tp.y, z);
	glEnd();
}

void drawArrow(Vec3 start, Vec3 end, Vec3 up, float thickness, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	float hw = thickness/2;
	Vec3 down = normVec3(cross(normVec3(end-start), normVec3(up)));

	float headWidth = thickness*2;
	float headHeight = thickness*2;

	float len = lenVec3(start - end);
	bool tooSmall = false;
	if(len < headHeight) tooSmall = true;

	Vec3 dir = normVec3(end - start);
	if(!tooSmall) end -= dir * headHeight;

	glBegin(GL_QUADS);
		pushVec(start + down*hw);
		pushVec(start - down*hw);
		pushVec(end - down*hw);
		pushVec(end + down*hw);
	glEnd();

	if(!tooSmall) {
		glBegin(GL_TRIANGLES);
			pushVec(end - down*headWidth/2);
			pushVec(end + dir*headHeight);
			pushVec(end + down*headWidth/2);
		glEnd();
	}
}

void drawTriangleFan(Vec3 pos, Vec3 start, float angle, Vec3 up, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLE_FAN);

	float r = lenVec3(start-pos);

	// float angle = angleBetweenVectors(left-pos, right-pos);

	pushVec(pos);

	int segments = (abs(angle) * r)*10;
	for(int i = 0; i < segments+1; i++) {
		Vec3 v = rotateVec3Around(start, (i * angle/(float)segments), up, pos);
		pushVec(v);
	}

	glEnd();
	glEnable(GL_CULL_FACE);
}

void drawCross(Vec2 p, float size, float size2, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	float z = globalGraphicsState->zOrder;

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	size2 = size2 / 2;

	Vec2 tr = p + vec2(1,1) * size*0.5f;
	Vec2 br = p + vec2(1,-1) * size*0.5f;
	Vec2 bl = p + vec2(-1,-1) * size*0.5f;
	Vec2 tl = p + vec2(-1,1) * size*0.5f;

	glBegin(GL_TRIANGLE_FAN);
		pushVec(p, z);

		pushVec(p + vec2(0,1) * size2, z);
			pushVec(tr + vec2(-1,0) * size2, z);
			pushVec(tr, z);
			pushVec(tr + vec2(0,-1) * size2, z);
		pushVec(p + vec2(1,0) * size2, z);
			pushVec(br + vec2(0,1) * size2, z);
			pushVec(br, z);
			pushVec(br + vec2(-1,0) * size2, z);
		pushVec(p + vec2(0,-1) * size2, z);
			pushVec(bl + vec2(1,0) * size2, z);
			pushVec(bl, z);
			pushVec(bl + vec2(0,1) * size2, z);
		pushVec(p + vec2(-1,0) * size2, z);
			pushVec(tl + vec2(0,-1) * size2, z);
			pushVec(tl, z);
			pushVec(tl + vec2(1,0) * size2, z);
		pushVec(p + vec2(0,1) * size2, z);
	glEnd();
}

void drawCircle(Vec2 pos, float r, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINE_STRIP);

	int segments = (M_2PI * r)*10;
	for(int i = 0; i < segments+1; i++) {
		Vec2 dv = pos + rotateVec2(vec2(0,1), (i * M_2PI/(float)segments)) * r;
		glVertex3f(dv.x, dv.y, 0);
	}

	glEnd();
}

// 3D

Vec3 boxVertices[] = {
	vec3(-0.5f, 0.5f,-0.5f), 
	vec3(-0.5f, 0.5f, 0.5f), 
	vec3(-0.5f,-0.5f, 0.5f), 
	vec3(-0.5f,-0.5f,-0.5f), 
	vec3(-1,0,0),
	
	vec3( 0.5f,-0.5f,-0.5f), 
	vec3( 0.5f,-0.5f, 0.5f), 
	vec3( 0.5f, 0.5f, 0.5f), 
	vec3( 0.5f, 0.5f,-0.5f), 
	vec3(1,0,0),
	
	vec3(-0.5f,-0.5f,-0.5f), 
	vec3(-0.5f,-0.5f, 0.5f), 
	vec3( 0.5f,-0.5f, 0.5f), 
	vec3( 0.5f,-0.5f,-0.5f), 
	vec3(0,-1,0),
	
	vec3( 0.5f, 0.5f,-0.5f), 
	vec3( 0.5f, 0.5f, 0.5f), 
	vec3(-0.5f, 0.5f, 0.5f), 
	vec3(-0.5f, 0.5f,-0.5f), 
	vec3(0,1,0),
	
	vec3(-0.5f, 0.5f,-0.5f), 
	vec3(-0.5f,-0.5f,-0.5f), 
	vec3( 0.5f,-0.5f,-0.5f), 
	vec3( 0.5f, 0.5f,-0.5f), 
	vec3(0,0,-1),
	
	vec3(-0.5f,-0.5f, 0.5f), 
	vec3(-0.5f, 0.5f, 0.5f), 
	vec3( 0.5f, 0.5f, 0.5f), 
	vec3( 0.5f,-0.5f, 0.5f), 
	vec3(0,0,1),
};

void triangleSubDivVertex(MeshVertex* vertices, int* count, Vec3 a, Vec3 b, Vec3 c, int divIndex) {
	if(divIndex == 0) {
		Vec3 n = normVec3(a+b+c);
		vertices[(*count)] = { a, n }; (*count) += 1;
		vertices[(*count)] = { b, n }; (*count) += 1;
		vertices[(*count)] = { c, n }; (*count) += 1;
	} else {
		Vec3 ab = normVec3((a+b)/2.0f);
		Vec3 bc = normVec3((b+c)/2.0f);
		Vec3 ca = normVec3((c+a)/2.0f);

		triangleSubDivVertex(vertices, count, a,ab,ca,  divIndex-1);
		triangleSubDivVertex(vertices, count, b,bc,ab,  divIndex-1);
		triangleSubDivVertex(vertices, count, c,ca,bc,  divIndex-1);
		triangleSubDivVertex(vertices, count, ab,bc,ca, divIndex-1);
	}
}

Vec3 sphereVertices[] = {
	vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0),  
	vec3(0,0,1),  vec3(-1,0,0), vec3(0,1,0),  
	vec3(0,0,1),  vec3(0,-1,0), vec3(-1,0,0), 
	vec3(0,0,1),  vec3(1,0,0),  vec3(0,-1,0), 
	vec3(0,0,-1), vec3(1,0,0),  vec3(0,1,0),  
	vec3(0,0,-1), vec3(0,1,0),  vec3(-1,0,0), 
	vec3(0,0,-1), vec3(-1,0,0), vec3(0,-1,0), 
	vec3(0,0,-1), vec3(0,-1,0), vec3(1,0,0),  
};

inline void pushTriangle(Vec3 v0, Vec3 v1, Vec3 v2) {
	glVertex3f(v0.x, v0.y, v0.z);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
}

inline void pushQuad(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3) {
	glVertex3f(v0.x, v0.y, v0.z);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glVertex3f(v3.x, v3.y, v3.z);
}

inline void pushQuadAsTriangles(Vec3 v0, Vec3 v1, Vec3 v2, Vec3 v3) {
	glVertex3f(v0.x, v0.y, v0.z);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glVertex3f(v3.x, v3.y, v3.z);
	glVertex3f(v0.x, v0.y, v0.z);
}

void drawLine(Vec3 p0, Vec3 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, p0.z);
		glVertex3f(p1.x, p1.y, p1.z);
	glEnd();
}

void drawPlane(Vec3 pos, Vec3 normal, Vec3 up, Vec2 dim, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	dim = dim/2.0f;
	Vec3 right = cross(up, normal);

	glBegin(GL_QUADS);
		glNormal3f(normal.x, normal.y, normal.z);
		pushVec(pos + right*dim.x + up*dim.y);
		pushVec(pos + right*dim.x - up*dim.y);
		pushVec(pos - right*dim.x - up*dim.y);
		pushVec(pos - right*dim.x + up*dim.y);
	glEnd();
}

void drawPlane(Vec3 pos, Vec3 normal, Vec3 up, Vec2 dim, Vec4 color, Rect uv, int texture) {
	glBindTexture(GL_TEXTURE_2D, texture);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	dim = dim/2.0f;
	Vec3 right = cross(up, normal);

	glBegin(GL_QUADS);
		glNormal3f(normal.x, normal.y, normal.z);
		glTexCoord2f(uv.left,  uv.top); pushVec(pos + right*dim.x + up*dim.y);
		glTexCoord2f(uv.left,  uv.bottom); pushVec(pos + right*dim.x - up*dim.y);
		glTexCoord2f(uv.right, uv.bottom); pushVec(pos - right*dim.x - up*dim.y);
		glTexCoord2f(uv.right, uv.top); pushVec(pos - right*dim.x + up*dim.y);
	glEnd();
}

void drawBox(Vec3 pos, Vec3 dim, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(dim.x, dim.y, dim.z);


	Mesh* mesh = &globalGraphicsState->meshes[MESH_CUBE];

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vec3)*2, mesh->vertices);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(Vec3)*2, ((char*)mesh->vertices) + sizeof(Vec3));

	glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);


	glPopMatrix();
}

void drawCircle(Vec3 pos, float r, Vec3 dir, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINE_STRIP);

	// Get any right angle vector.
	Vec3 v = dir == vec3(1,0,0) ? vec3(0,1,0) : vec3(1,0,0);
	v = normVec3(cross(dir, normVec3(v)));

	int segments = (M_2PI * r)*10;
	for(int i = 0; i < segments+1; i++) {
		Vec3 dv = pos + rotateVec3(v, (i * M_2PI/(float)segments), dir) * r;
		glVertex3f(dv.x, dv.y, dv.z);
	}

	glEnd();
}

void drawRing(Vec3 pos, Vec3 normal, float r, float thickness, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	int div = 4;
	glBegin(GL_QUAD_STRIP);

	Vec3 otherVector = normal == vec3(1,0,0)? vec3(0,1,0) : vec3(1,0,0);
	Vec3 up = normVec3(cross(normal, otherVector));
	float ri = r - thickness;

	int segments = (M_2PI * r)*10;
	for(int i = 0; i < segments+1; i++) {
		Vec3 v = rotateVec3(up, (i * M_2PI/(float)segments), normal);
		pushVec(pos + v*ri);
		pushVec(pos + v*r);
	}

	glEnd();
}

void drawSphere(Vec3 pos, float r, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(r, r, r);

	Mesh* mesh = &globalGraphicsState->meshes[MESH_SPHERE];

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vec3)*2, mesh->vertices);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(Vec3)*2, ((char*)mesh->vertices) + sizeof(Vec3));

	glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	glPopMatrix();
}

void drawSphereRaw() {
	glBindTexture(GL_TEXTURE_2D, 0);

	Mesh* mesh = &globalGraphicsState->meshes[MESH_SPHERE];

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vec3)*2, mesh->vertices);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(Vec3)*2, ((char*)mesh->vertices) + sizeof(Vec3));

	glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void drawBoxRaw() {
	glBindTexture(GL_TEXTURE_2D, 0);

	Mesh* mesh = &globalGraphicsState->meshes[MESH_CUBE];

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vec3)*2, mesh->vertices);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(Vec3)*2, ((char*)mesh->vertices) + sizeof(Vec3));

	glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);
	// glDrawArrays(GL_QUADS, 0, 24);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

// Bad beans.
void drawBox(Vec3 pos, Vec3 dim, Mat4* vm, Quat rot, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	Mat4 tm = translationMatrix(pos);
	Mat4 rm = quatRotationMatrix(rot);
	Mat4 sm = scaleMatrix(dim);
	Mat4 fm = (*vm) * tm * rm * sm;
	rowToColumn(&fm);
	glPushMatrix();
	glLoadMatrixf(fm.e);

	drawBoxRaw();

	glPopMatrix();
}

//

enum TextStatus {
	TEXTSTATUS_END = 0, 
	TEXTSTATUS_NEWLINE, 
	TEXTSTATUS_WRAPPED, 
	TEXTSTATUS_DEFAULT, 
	TEXTSTATUS_SIZE, 
};

inline char getRightBits(char n, int count) {
	int bitMask = 0;
	for(int i = 0; i < count; i++) bitMask += (1 << i);
	return n&bitMask;
}

int unicodeDecode(uchar* s, int* byteCount) {
	if(s[0] <= 127) {
		*byteCount = 1;
		return s[0];
	}

	int bitCount = 1;
	for(;;) {
		char bit = (1 << 8-bitCount-1);
		if(s[0]&bit) bitCount++;
		else break;
	}

	(*byteCount) = bitCount;

	int unicodeChar = 0;
	for(int i = 0; i < bitCount; i++) {
		char byte = i==0 ? getRightBits(s[i], 8-(bitCount+1)) : getRightBits(s[i], 6);

		unicodeChar += ((int)byte) << (6*((bitCount-1)-i));
	}

	return unicodeChar;
}

int unicodeGetSize(uchar* s) {
	if(s[0] <= 127) return 1;

	int bitCount = 1;
	for(;;) {
		char bit = (1 << 8-bitCount-1);
		if(s[0]&bit) bitCount++;
		else break;
	}

	return bitCount;
}

int getUnicodeRangeOffset(int c, Font* font) {
	int unicodeOffset = -1;

	bool found = false;
	for(int i = 0; i < font->glyphRangeCount; i++) {
		if(valueBetweenInt(c, font->glyphRanges[i].x, font->glyphRanges[i].x+font->glyphRanges[i].y)) {
			unicodeOffset += c - font->glyphRanges[i].x + 1;
			found = true;
			break;
		}
		unicodeOffset += font->glyphRanges[i].y;
	}

	if(!found) {
		if(c == Font_Error_Glyph) return 0;
		unicodeOffset = getUnicodeRangeOffset(Font_Error_Glyph, font);
	}

	return unicodeOffset;
}

// Taken from stbtt_truetype.
void getPackedQuad(PackedChar *chardata, Vec2i texDim, int char_index, Vec2 pos, Rect* r, Rect* uv, int alignToInteger)
{
   PackedChar *b = chardata + char_index;

   if (alignToInteger) {
   	  *r = rectBLDim(roundFloat(pos.x + b->xBearing), roundFloat(pos.y - (b->height - b->yBearing)), b->width, b->height);
   } else {
   	  *r = rectBLDim(pos.x + b->xBearing, pos.y - (b->height - b->yBearing), b->width, b->height);
   }

   Vec2 ip = vec2(1.0f / texDim.w, 1.0f / texDim.h);
   *uv = rect(b->x0*ip.x, b->y0*ip.y, b->x1*ip.x, b->y1*ip.y);
}

void getTextQuad(int c, Font* font, Vec2 pos, Rect* r, Rect* uv) {

	int unicodeOffset = getUnicodeRangeOffset(c, font);
	getPackedQuad(font->cData, font->tex.dim, unicodeOffset, pos, r, uv, font->pixelAlign);

	float off = font->baseOffset;
	if(font->pixelAlign) off = roundInt(off);
	r->bottom -= off;
	r->top -= off;
}

float getCharAdvance(int c, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;
	return result;
}

float getCharAdvance(int c, int c2, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;

	if(FT_HAS_KERNING(font->face)) {
		FT_Vector kerning;

		uint charIndex1 = FT_Get_Char_Index(font->face, c);
		uint charIndex2 = FT_Get_Char_Index(font->face, c2);

		FT_Get_Kerning(font->face, charIndex1, charIndex2, FT_KERNING_DEFAULT, &kerning);
		float kernAdvance = kerning.x * font->pixelScale;

		result += kernAdvance;
	}

	return result;
}



struct TextInfo {
	Vec2 pos;
	int index;
	Vec2 posAdvance;
	Rect r;
	Rect uv;

	bool lineBreak;
	Vec2 breakPos;
};

struct TextSimInfo {
	Vec2 pos;
	int index;
	int wrapIndex;

	bool lineBreak;
	Vec2 breakPos;

	bool bold;
	bool italic;

	bool colorMode;
	Vec3 colorOverwrite;
};

TextSimInfo initTextSimInfo(Vec2 startPos) {
	TextSimInfo tsi = {};
	tsi.pos = startPos;
	tsi.index = 0;
	tsi.wrapIndex = 0;
	tsi.lineBreak = false;
	tsi.breakPos = vec2(0,0);
	return tsi;
}

enum {
	TEXT_MARKER_BOLD = 0,
	TEXT_MARKER_ITALIC,
	TEXT_MARKER_COLOR,
};

#define Marker_Size 3
#define Bold_Marker "<b>"
#define Italic_Marker "<i>"
#define Color_Marker "<c>"

// The marker system is badly designed and has all kinds of edge cases where it breaks if you
// don't pay attention, so... put markers in sparingly.

int parseTextMarkers(char* text, TextSimInfo* tsi, int* type = 0) {
	// Return how many characters to skip.

	if(text[0] == '<') {
		if(text[1] != '\0' && text[2] != '\0' && text[2] == '>') {
			switch(text[1]) {
				case 'b': if(type) { *type = TEXT_MARKER_BOLD; } return Marker_Size;
				case 'i': if(type) { *type = TEXT_MARKER_ITALIC; } return Marker_Size;
				case 'c': if(type) { *type = TEXT_MARKER_COLOR; } 
					if(tsi->colorMode) return Marker_Size;
					else return Marker_Size + 6; // FFFFFF
			}
		}
	}

	return 0;
}

void updateMarkers(char* text, TextSimInfo* tsi, Font* font, bool skip = false) {

	int type;
	int length = 0;
	while(length = parseTextMarkers(text + tsi->index, tsi, &type)) {
		switch(type) {
			case TEXT_MARKER_BOLD: {
				tsi->index += length;
				tsi->wrapIndex += length;

				if(!font->boldFont) return;
				if(!skip) {
					if(!tsi->bold) {
						glEnd();
						glBindTexture(GL_TEXTURE_2D, font->boldFont->tex.id);
						glBegin(GL_QUADS);
					} else {
						glEnd();
						glBindTexture(GL_TEXTURE_2D, font->tex.id);
						glBegin(GL_QUADS);
					}
				}

				tsi->bold = !tsi->bold;
			} break;

			case TEXT_MARKER_ITALIC: {
				tsi->index += length;
				tsi->wrapIndex += length;

				if(!font->italicFont) return;
				if(!skip) {
					if(!tsi->italic && font->italicFont) {
						glEnd();
						glBindTexture(GL_TEXTURE_2D, font->italicFont->tex.id);
						glBegin(GL_QUADS);
					} else {
						glEnd();
						glBindTexture(GL_TEXTURE_2D, font->tex.id);
						glBegin(GL_QUADS);
					}
				}

				tsi->italic = !tsi->italic;
			} break;

			case TEXT_MARKER_COLOR: {
				if(skip) {
					tsi->index += length;
					tsi->wrapIndex += length;
					if(type == TEXT_MARKER_COLOR) tsi->colorMode = !tsi->colorMode;
					continue;
				} 
				tsi->index += Marker_Size;
				tsi->wrapIndex += Marker_Size;
				Vec3 c;
				if(!tsi->colorMode) {
					c.r = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
					c.g = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
					c.b = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
					tsi->colorOverwrite = COLOR_SRGB(c);
					
					tsi->wrapIndex += 6;
				}

				tsi->colorMode = !tsi->colorMode;
			} break;
		}
	}
}

int textSim(char* text, Font* font, TextSimInfo* tsi, TextInfo* ti, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {
	ti->lineBreak = false;

	if(tsi->lineBreak) {
		ti->lineBreak = true;
		ti->breakPos = tsi->breakPos;
		tsi->lineBreak = false;
	}

	if(text[tsi->index] == '\0') {
		ti->pos = tsi->pos;
		ti->index = tsi->index;
		return 0;
	}

	Vec2 oldPos = tsi->pos;

	int i = tsi->index;
	int tSize;
	int t = unicodeDecode((uchar*)(&text[i]), &tSize);

	bool wrapped = false;

	if(wrapWidth != 0 && i == tsi->wrapIndex) {
		int size;
		int c = unicodeDecode((uchar*)(&text[i]), &size);
		float wordWidth = 0;
		if(c == '\n') wordWidth = getCharAdvance(c, font);

		char* tempText = text;
		int it = i;
		while(c != '\n' && c != '\0' && c != ' ') {

			// Awkward.
			bool hadMarker = false;
			int markerLength = 0;
			while(markerLength = parseTextMarkers(tempText + it, tsi)) {
				// Pretend markers aren't there by moving text pointer.
				tempText += markerLength;
				hadMarker = true;
			}
			if(hadMarker) {
				c = unicodeDecode((uchar*)(&tempText[it]), &size);
				continue;
			}

			wordWidth += getCharAdvance(c, font);
			it += size;
			c = unicodeDecode((uchar*)(&tempText[it]), &size);
		}

		if(tsi->pos.x + wordWidth > startPos.x + wrapWidth) {
			wrapped = true;
		}

		if(it != i) tsi->wrapIndex = it;
		else tsi->wrapIndex++;
	}

	if(t == '\n' || wrapped) {
		tsi->lineBreak = true;
		if(t == '\n') tsi->breakPos = tsi->pos + vec2(getCharAdvance(t, font),0);
		if(wrapped) tsi->breakPos = tsi->pos;

		tsi->pos.x = startPos.x;
		// tsi->pos.y -= font->height;
		tsi->pos.y -= font->lineSpacing;

		if(wrapped) {
			return textSim(text, font, tsi, ti, startPos, wrapWidth);
		}
	} else {
		getTextQuad(t, font, tsi->pos, &ti->r, &ti->uv);

		if(text[i+1] != '\0') {
			int tSize2;
			int t2 = unicodeDecode((uchar*)(&text[i+tSize]), &tSize2);
			tsi->pos.x += getCharAdvance(t, t2, font);
		} else tsi->pos.x += getCharAdvance(t, font);
	}

	if(ti) {
		ti->pos = oldPos;
		ti->index = tsi->index;
		ti->posAdvance = tsi->pos - oldPos;
	}

	tsi->index += tSize;

	return 1;
}

struct TextSettings {
	Font* font;
	Vec4 color;

	int shadowMode;
	Vec2 shadowDir;
	float shadowSize;
	Vec4 shadowColor;
};

TextSettings textSettings(Font* font, Vec4 color, int shadowMode, Vec2 shadowDir, float shadowSize, Vec4 shadowColor) {
	return {font, color, shadowMode, shadowDir, shadowSize, shadowColor};
}
TextSettings textSettings(Font* font, Vec4 color, int shadowMode, float shadowSize, Vec4 shadowColor) {
	return {font, color, shadowMode, vec2(-1,-1), shadowSize, shadowColor};
}
TextSettings textSettings(Font* font, Vec4 color) {
	return {font, color};
}


enum {
	TEXTSHADOW_MODE_NOSHADOW = 0,
	TEXTSHADOW_MODE_SHADOW,
	TEXTSHADOW_MODE_OUTLINE,
};

Vec2 getTextDim(char* text, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {
	float maxX = startPos.x;

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		Font* f = font;
		updateMarkers(text, &tsi, font, true);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		if(!textSim(text, f, &tsi, &ti, startPos, wrapWidth)) break;

		maxX = max(maxX, ti.pos.x + ti.posAdvance.x);
	}

	Vec2 dim = vec2(maxX - startPos.x, startPos.y - (tsi.pos.y - font->height));

	return dim;
}

Vec2 testgetTextStartPos(char* text, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	Vec2 dim = getTextDim(text, font, startPos, wrapWidth);
	startPos.x -= (align.x+1)*0.5f*dim.w;
	startPos.y -= (align.y-1)*0.5f*dim.h;

	return startPos;
}

Rect getTextLineRect(char* text, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1)) {
	startPos = testgetTextStartPos(text, font, startPos, align, 0);

	Vec2 textDim = getTextDim(text, font);
	Rect r = rectTLDim(startPos, textDim);

	return r;
}

void drawText(char* text, Vec2 startPos, Vec2i align, int wrapWidth, TextSettings settings) {
	float z = globalGraphicsState->zOrder;
	Font* font = settings.font;

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	setSRGB(false);

	Vec4 c = COLOR_SRGB(settings.color);
	Vec4 sc = COLOR_SRGB(settings.shadowColor);

	pushColor(c);

	int texId = font->tex.id;
	glBindTexture(GL_TEXTURE_2D, texId);
	glBegin(GL_QUADS);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		
		Font* f = font;
		updateMarkers(text, &tsi, font);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		if(!textSim(text, f, &tsi, &ti, startPos, wrapWidth)) break;
		if(text[ti.index] == '\n') continue;

		if(settings.shadowMode != TEXTSHADOW_MODE_NOSHADOW) {
			pushColor(sc);

			if(settings.shadowMode == TEXTSHADOW_MODE_SHADOW) {
				Vec2 p = ti.r.min + normVec2(settings.shadowDir) * settings.shadowSize;
				Rect sr = rectBLDim(vec2(roundFloat(p.x), roundFloat(p.y)), rectDim(ti.r));
				pushRect(sr, ti.uv, z);
			} else if(settings.shadowMode == TEXTSHADOW_MODE_OUTLINE) {
				for(int i = 0; i < 8; i++) {
					
					// Not sure if we should align to pixels on an outline.

					Vec2 dir = rotateVec2(vec2(1,0), (M_2PI/8)*i);
					Rect r = rectTrans(ti.r, dir*settings.shadowSize);
					pushRect(r, ti.uv, z);

					// Vec2 dir = rotateVec2(vec2(1,0), (M_2PI/8)*i);
					// Vec2 p = ti.r.min + dir * settings.shadowSize;
					// Rect sr = rectBLDim(vec2(roundFloat(p.x), roundFloat(p.y)), rectDim(ti.r));
					// pushRect(sr, ti.uv, z);
				}
			}
		}

		if(tsi.colorMode) pushColor(vec4(tsi.colorOverwrite, 1));
		else pushColor(c);

		pushRect(ti.r, ti.uv, z);
	}
	
	glEnd();

	setSRGB();
}
void drawText(char* text, Vec2 startPos, TextSettings settings) {
	return drawText(text, startPos, vec2i(-1,1), 0, settings);
}
void drawText(char* text, Vec2 startPos, Vec2i align, TextSettings settings) {
	return drawText(text, startPos, align, 0, settings);
}

void drawTextLineCulled(char* text, Font* font, Vec2 startPos, float width, Vec4 color, Vec2i align = vec2i(-1,1)) {
	startPos = testgetTextStartPos(text, font, startPos, align, 0);
	startPos = vec2(roundInt((int)startPos.x), roundInt((int)startPos.y));

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		Font* f = font;
		updateMarkers(text, &tsi, font, true);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		if(!textSim(text, f, &tsi, &ti, startPos, 0)) break;
		if(text[ti.index] == '\n') continue;

		if(ti.pos.x > startPos.x + width) break;

		drawRect(ti.r, color, ti.uv, f->tex.id);
	}
}

Vec2 textIndexToPos(char* text, Font* font, Vec2 startPos, int index, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		Font* f = font;
		updateMarkers(text, &tsi, font, true);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		int result = textSim(text, f, &tsi, &ti, startPos, wrapWidth);

		if(ti.index == index) {
			Vec2 pos = ti.pos - vec2(0, f->height/2);
			return pos;
		}

		if(!result) break;
	}

	return vec2(0,0);
}

void drawTextSelection(char* text, Font* font, Vec2 startPos, int index1, int index2, Vec4 color, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	if(index1 == index2) return;
	if(index1 > index2) swap(&index1, &index2);

	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	Vec2 lineStart;
	bool drawSelection = false;

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		Font* f = font;
		updateMarkers(text, &tsi, font, true);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		int result = textSim(text, f, &tsi, &ti, startPos, wrapWidth);

		bool endReached = ti.index == index2;

		if(drawSelection) {
			if(ti.lineBreak || endReached) {

				Vec2 lineEnd;
				if(ti.lineBreak) lineEnd = ti.breakPos;
				else if(!result) lineEnd = tsi.pos;
				else lineEnd = ti.pos;

				Rect r = rect(lineStart - vec2(0,f->height), lineEnd);
				drawRect(r, color);

				lineStart = ti.pos;

				if(endReached) break;
			}
		}

		if(!drawSelection && (ti.index >= index1)) {
			drawSelection = true;
			lineStart = ti.pos;
		}

		if(!result) break;
	}
}

int textMouseToIndex(char* text, Font* font, Vec2 startPos, Vec2 mousePos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	if(mousePos.y > startPos.y) return 0;
	
	bool foundLine = false;
	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		Font* f = font;
		updateMarkers(text, &tsi, font, true);
		if(tsi.bold) f = font->boldFont;
		else if(tsi.italic) f = font->italicFont;

		TextInfo ti;
		int result = textSim(text, f, &tsi, &ti, startPos, wrapWidth);
		
		bool fLine = valueBetween(mousePos.y, ti.pos.y - f->height, ti.pos.y);
		if(fLine) foundLine = true;
		else if(foundLine) return ti.index-1;

	    if(foundLine) {
	    	float charMid = ti.pos.x + ti.posAdvance.x*0.5f;
			if(mousePos.x < charMid) return ti.index;
		}

		if(!result) break;
	}

	return tsi.index;
}

// char* textSelectionToString(char* text, int index1, int index2) {
// 	myAssert(index1 >= 0 && index2 >= 0);

// 	int range = abs(index1 - index2);
// 	char* str = getTStringDebug(range + 1); // We assume text selection will only be used for debug things.
// 	strCpy(str, text + minInt(index1, index2), range);
// 	return str;
// }

uint createSampler(float ani, int wrapS, int wrapT, int magF, int minF, int wrapR = GL_CLAMP_TO_EDGE) {
	uint result;
	glCreateSamplers(1, &result);

	glSamplerParameteri(result, GL_TEXTURE_MAX_ANISOTROPY_EXT, ani);
	glSamplerParameteri(result, GL_TEXTURE_WRAP_S, wrapS);
	glSamplerParameteri(result, GL_TEXTURE_WRAP_T, wrapT);
	glSamplerParameteri(result, GL_TEXTURE_MAG_FILTER, magF);
	glSamplerParameteri(result, GL_TEXTURE_MIN_FILTER, minF);

	glSamplerParameteri(result, GL_TEXTURE_WRAP_R, wrapR);

	return result;
}


void openglDebug() {
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	const int count = 1;
	GLenum sources;
	GLenum types;
	GLuint ids;
	GLenum severities;
	GLsizei lengths;

	int bufSize = 1000;
	char* messageLog = getTString(bufSize);
	memSet(messageLog, 0, bufSize);

	uint fetchedLogs = 1;
	while(fetchedLogs = glGetDebugMessageLog(count, bufSize, &sources, &types, &ids, &severities, &lengths, messageLog)) {
		if(severities == GL_DEBUG_SEVERITY_NOTIFICATION) continue;

		if(severities == GL_DEBUG_SEVERITY_HIGH) printf("HIGH: \n");
		else if(severities == GL_DEBUG_SEVERITY_MEDIUM) printf("MEDIUM: \n");
		else if(severities == GL_DEBUG_SEVERITY_LOW) printf("LOW: \n");
		else if(severities == GL_DEBUG_SEVERITY_NOTIFICATION) printf("NOTE: \n");

		printf("\t%s \n", messageLog);
	}
}

void openglClearFrameBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	bindFrameBuffer(FRAMEBUFFER_2dMsaa);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void openglDefaultSetup() {
	glEnable(GL_DITHER);

	glDepthRange(-10.0,10.0);
	glFrontFace(GL_CW);
	// glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	scissorState(false);
	// glEnable(GL_LINE_SMOOTH);
	// glEnable(GL_POLYGON_SMOOTH);
	// glDisable(GL_POLYGON_SMOOTH);
	// glDisable(GL_SMOOTH);
	
	glEnable(GL_TEXTURE_2D);
	// glEnable(GL_ALPHA_TEST);
	// glAlphaFunc(GL_GREATER, 0.9);
	// glDisable(GL_LIGHTING);
	// glDepthFunc(GL_LESS);
	// glClearDepth(1);
	// glDepthMask(GL_TRUE);
	// glEnable(GL_MULTISAMPLE);
	// glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	// glViewport(0,0, ad->cur3dBufferRes.x, ad->cur3dBufferRes.y);


	// glBindTexture(GL_TEXTURE_2D, getFrameBuffer(FRAMEBUFFER_2dMsaa)->colorSlot[0]->id);
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	// glDepthFunc(GL_NEVER);
	// glDisable(GL_DEPTH_TEST);

	glUseProgram(0);
	glBindProgramPipeline(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void setWindowViewport(WindowSettings* ws, Rect vp);
void openglDrawFrameBufferAndSwap(WindowSettings* ws, SystemData* sd, Timer* swapTimer, bool init, float panelAlpha) {
	
	//
	// Render.
	//

	{		
		Vec2i frameBufferRes = getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->dim;
		Vec2i res = ws->windowRes;
		Rect frameBufferUV = rect(0,(float)res.h/frameBufferRes.h,(float)res.w/frameBufferRes.w,0);

		{
			blitFrameBuffers(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_2dNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glLoadIdentity();

			scissorState(false);
			glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);

			// glClearColor(1,0,1,1);
			// glClear(GL_COLOR_BUFFER_BIT);

			Vec2i res = ws->windowRes;
			setWindowViewport(ws, rect(0,0,res.w,res.h));

			glOrtho(0,1,1,0, -1, 1);
			drawRect(rect(0, 1, 1, 0), frameBufferUV, getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);


			// Panel.
			glEnable(GL_BLEND);
			blitFrameBuffers(FRAMEBUFFER_2dPanels, FRAMEBUFFER_2dNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			drawRect(rect(0, 1, 1, 0), vec4(1,1,1,panelAlpha), frameBufferUV, getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);
			glDisable(GL_BLEND);
		}
	}

	//
	// Swap window background buffer.
	//

	{
		if(!ws->vsync) sd->vsyncTempTurnOff = false;

		// Sleep until monitor refresh.
		if(!init && ws->vsync && !sd->vsyncTempTurnOff) {
			double frameTime = timerStop(swapTimer);
			double fullFrameTime = ((double)1/ws->frameRate);

			// If we missed a frame we have to sleep longer.
			// For example: going from 60hz to 30hz.
			while(frameTime > fullFrameTime) fullFrameTime *= 2;

			double sleepTime = fullFrameTime - frameTime;

			int sleepTimeMS = sleepTime*1000.0 - 0.5;

			// timerStart(swapTimer);
			if(sleepTimeMS > 0) Sleep(sleepTimeMS);
			// timerStop(swapTimer);

			// printf("%f %f %f %i\n", fullFrameTime, frameTime, sleepTime, sleepTimeMS);
			// sleepTimeMS += 1;
			// printf("sleep %i: %i %f %f diff:%f\n", swapTimer->dt > sleepTimeMS/1000.0, sleepTimeMS, sleepTime, swapTimer->dt, sleepTimeMS/1000.0 - swapTimer->dt);
		}

		// Cap max framerate if vsync disabled.
		if(!init && !ws->vsync) {
			double frameTime = timerStop(swapTimer);
			double maxFrameTime = ((double)1/ws->frameRate);

			if(frameTime < maxFrameTime) {
				double fullSleepTime = maxFrameTime - frameTime;

				timerStart(swapTimer);

				int sleepTimeMS = fullSleepTime*1000 - 0.5;
				Sleep(sleepTimeMS);

				f64 sleptTime = timerStop(swapTimer);

				double restTime = fullSleepTime - sleptTime;

				// Not Good.
				while(restTime > 0) {
					timerStart(swapTimer);

					int temp = 0;
					for(int i = 0; i < 100; i++) temp *= temp;

					f64 dt = timerStop(swapTimer);
					restTime -= dt;
				}
			}
		}

		if(init) {
			showWindow(sd->windowHandle);
			GLenum glError = glGetError(); printf("GLError: %i\n", glError);
		}

		if(!init) {
        	if(sd->vsyncTempTurnOff) {
        		wglSwapIntervalEXT(0);
        	}

			swapBuffers(sd);

        	if(sd->vsyncTempTurnOff) {
        		wglSwapIntervalEXT(1);
        		sd->vsyncTempTurnOff = false;
        	}
		}

		timerStart(swapTimer);
	}
}
