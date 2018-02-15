
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

	// char* name;
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

struct Font {
	char* file;
	int id;

	stbtt_fontinfo info;
	float pixelScale;
	float emScale;

	char* fileBuffer;
	Texture tex;
	Texture brightTex, darkTex;
	Vec2i glyphRanges[5];
	int glyphRangeCount;
	stbtt_packedchar* cData;
	float height;
	float baseOffset;

	float pointHeight;
	bool usePointHeight;

	Font* boldFont;
	Font* italicFont;

	bool pixelAlign;
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
	// FrameBuffer* fb = getFrameBuffer(id);

	bool isRenderBuffer = msaa > 0;

	Texture t;
	createTexture(&t, isRenderBuffer);
	t.internalFormat = internalFormat;
	t.dim.w = w;
	t.dim.h = h;
	t.isRenderBuffer = isRenderBuffer;
	t.msaa = msaa;

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

struct GraphicsState {
	Texture textures[32]; //TEXTURE_SIZE
	int textureCount;
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
};

Texture* getTexture(int textureId) {
	Texture* t = globalGraphicsState->textures + textureId;
	return t;
}

Texture* getTextureX(int textureId) {
	GraphicsState* gs = globalGraphicsState;
	for(int i = 0; i < arrayCount(gs->textures); i++) {
		if(gs->textures[i].id == textureId) {
			return gs->textures + i;
		}
	}

	return 0;
}

Texture* addTexture(Texture tex) {
	GraphicsState* gs = globalGraphicsState;
	gs->textures[gs->textureCount++] = tex;
	return gs->textures + (gs->textureCount - 1);
}

FrameBuffer* getFrameBuffer(int id) {
	FrameBuffer* fb = globalGraphicsState->frameBuffers + id;
	return fb;
}

#define Font_Error_Glyph (int)0x20-1

Font* fontInit(Font* fontSlot, char* file, float height, bool enableHinting = false) {
	TIMER_BLOCK();

	char* fontFolder = 0;
	for(int i = 0; i < globalGraphicsState->fontFolderCount; i++) {
		if(fileExists(fillString("%s%s", globalGraphicsState->fontFolders[i], file))) {
			fontFolder = globalGraphicsState->fontFolders[i];
			break;
		}
	}

	if(!fontFolder) return 0;

	char* path = fillString("%s%s", fontFolder, file);
	char* fileBuffer = mallocString(fileSize(path) + 1);
	readFileToBuffer(fileBuffer, path);

	// Vec2i size = vec2i(256,256);
	Vec2i size = vec2i(512,512);
	// Vec2i size = vec2i(750,750);

	pushTMemoryStack();

	uchar* fontBitmapBuffer = (unsigned char*)getTMemory(size.x*size.y);
	uchar* fontBitmap = (unsigned char*)getTMemory(size.x*size.y*4);


	Font font;
	stbtt_InitFont(&font.info, (const unsigned char*)fileBuffer, 0);
	font.file = getPString(strLen(file)+1);
	strCpy(font.file, file);

	// Use point size instead of pixel size.
	if(height < 0) {
		height *= -1;
		float pointSize = height;
		float pixelScale = stbtt_ScaleForPixelHeight(&font.info, height);
		float emScale = stbtt_ScaleForMappingEmToPixels(&font.info, height);
		height = (pointSize/pixelScale)*emScale;
	} 

	font.height = height;
	font.pixelScale = stbtt_ScaleForPixelHeight(&font.info, font.height);
	font.emScale = stbtt_ScaleForMappingEmToPixels(&font.info, font.height);
	font.pointHeight = (font.height/font.emScale)*font.pixelScale;

	#define setupRange(a,b) vec2i(a, b - a + 1)
	#define setupRangeCount(a,b) vec2i(a, b)
	int rc = 0;
	font.glyphRanges[rc++] = setupRange(0x20, 0x7F);
	font.glyphRanges[rc++] = setupRange(0xA1, 0xFF);

	// font.glyphRanges[rc++] = setupRangeCount(0x6B, 1);
	
	// font.glyphRanges[rc++] = setupRangeCount(0x48, 1);
	// font.glyphRanges[rc++] = setupRangeCount(0xDC, 1);
	// font.glyphRanges[rc++] = setupRangeCount(0xE4, 1);

	// font.glyphRanges[rc++] = setupRangeCount(0xF2, 1);
	font.glyphRangeCount = rc;

	int totalGlyphCount = 0;
	for(int i = 0; i < font.glyphRangeCount; i++) totalGlyphCount += font.glyphRanges[i].y;
	font.cData = mallocArray(stbtt_packedchar, totalGlyphCount);


	int ascent = 0;
	stbtt_GetFontVMetrics(&font.info, &ascent,0,0);
	font.baseOffset = (ascent*font.pixelScale);
	font.pixelAlign = true;



	// float pixelSize = font.height;
	// float scale1 = stbtt_ScaleForPixelHeight(&font.info, pixelSize);
	// float scale2 = stbtt_ScaleForMappingEmToPixels(&font.info, pixelSize);

	// int unitsPerEm = ttUSHORT(font.info.data + font.info.head + 18);

	// float points = pixelSize * (float)72 / (float)96;
	// printf("%f, %f\n", pixelSize, points);
	// printf("%f, %f, %f\n", pixelSize, (pixelSize/scale1)*scale2, (pixelSize/scale2)*scale1);
	// printf("%f, %f\n", scale1, scale2);
	// printf("%i\n", unitsPerEm);
	// exit(1);

	{
		stbtt_pack_context context;
		int result = stbtt_PackBegin(&context, fontBitmapBuffer, size.w, size.h, 0, 1, 0);
		stbtt_PackSetOversampling(&context, 1, 1);

		stbtt_pack_range ranges[5];
		int cDataOffset = 0;
		for(int i = 0; i < font.glyphRangeCount; i++) {
			ranges[i].first_unicode_codepoint_in_range = font.glyphRanges[i].x;
			ranges[i].array_of_unicode_codepoints = NULL;
			ranges[i].num_chars                   = font.glyphRanges[i].y;
			ranges[i].chardata_for_range          = font.cData + cDataOffset;
			ranges[i].font_size                   = font.height;

			cDataOffset += font.glyphRanges[i].y;
		}

		if(strCompare(file, "kaiu.ttf")) {
			int stop = 234;
		}

		// TrueTypeInterpreter interpreter;
		// interpreter.init();
		// interpreter.setupFunctionsAndCvt(&font.info, font.height);

		enableHinting = false;

		if(!enableHinting) {
			stbtt_PackFontRanges(&context, (uchar*)fileBuffer, 0, ranges, 	font.glyphRangeCount);
		} else {
			// stbtt_PackFontRangesHinted(&interpreter, &font.info, &context, ranges, font.glyphRangeCount);
		}

		stbtt_PackEnd(&context);
		// interpreter.freeInterpreter();
	}


	Texture tex;

	// Clear to white, we only care about alpha.
	memSet(fontBitmap, 255, size.w*size.h*4);

	// Normal texture.
	for(int i = 0; i < size.w*size.h; i++) {
		fontBitmap[i*4+3] = fontBitmapBuffer[i];
	}
	// GL_RGBA8
	loadTexture(&tex, fontBitmap, size.w, size.h, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
	font.tex = tex;

	// Bright texture.
	for(int i = 0; i < size.w*size.h; i++) {
		float v = colorIntToFloat(fontBitmapBuffer[i]);
		if(v > 0) {
			// v += 0.2f;
			// v = pow(v, 2.2f);
			// v = pow(v, 1/2.2f);
		}
		v = clampMax(v, 1.0f);
		fontBitmap[i*4+3] = colorFloatToInt(v);
	}
	// loadTexture(&tex, fontBitmap, size.w, size.h, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
	loadTexture(&tex, fontBitmap, size.w, size.h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	font.brightTex = tex;

	// Dark texture.
	for(int i = 0; i < size.w*size.h; i++) {
		float v = colorIntToFloat(fontBitmapBuffer[i]);
		// if(v < 0.8
		if(v > 0) {
			// v = 1;
			// v += 0.2f;
			// v = pow(v, 2.2f);

			// v = pow(v, 1/2.2f);
			// v = pow(v, 1/2.2f);
			// v = pow(v, 1/2.2f);
		}
		v = clampMax(v, 1.0f);
		fontBitmap[i*4+3] = colorFloatToInt(v);
	}
	loadTexture(&tex, fontBitmap, size.w, size.h, 1, INTERNAL_TEXTURE_FORMAT, GL_RGBA, GL_UNSIGNED_BYTE);
	// loadTexture(&tex, fontBitmap, size.w, size.h, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
	font.darkTex = tex;

	// GL_RGBA8
	// INTERNAL_TEXTURE_FORMAT

	popTMemoryStack();

	*fontSlot = font;
	return fontSlot;
}

void freeFont(Font* font) {
	freeAndSetNullSave(font->cData);
	freeAndSetNullSave(font->info.data);
	glDeleteTextures(1, &font->tex.id);
	font->height = 0;
}

Font* getFont(char* fontFile, float height, char* boldFontFile = 0, char* italicFontFile = 0) {

	int fontCount = arrayCount(globalGraphicsState->fonts);
	int fontSlotCount = arrayCount(globalGraphicsState->fonts[0]);
	Font* fontSlot = 0;
	for(int i = 0; i < fontCount; i++) {
		if(globalGraphicsState->fonts[i][0].height == 0) {
			fontSlot = &globalGraphicsState->fonts[i][0];
			break;
		} else {
			if(strCompare(fontFile, globalGraphicsState->fonts[i][0].file)) {
				for(int j = 0; j < fontSlotCount; j++) {
					int h = globalGraphicsState->fonts[i][j].height;
					if(h == 0 || h == height) {
						fontSlot = &globalGraphicsState->fonts[i][j];
						break;
					}
				}
			}
		}
	}

	// We are going to assume for now that a font size of 0 means it is uninitialized.
	if(fontSlot->height == 0) {
		Font* font = fontInit(fontSlot, fontFile, height);
		if(!font) {
			return getFont(globalGraphicsState->fallbackFont, height, globalGraphicsState->fallbackFontBold, globalGraphicsState->fallbackFontItalic);
		}

		if(boldFontFile) {
			fontSlot->boldFont = getPStruct(Font);
			fontInit(fontSlot->boldFont, boldFontFile, height);
		}
		if(italicFontFile) {
			fontSlot->italicFont = getPStruct(Font);
			fontInit(fontSlot->italicFont, italicFontFile, height);
		}
	}

	return fontSlot;
}

Rect scissorRectScreenSpace(Rect r, float screenHeight) {
	Rect scissorRect = {r.min.x, r.min.y+screenHeight, r.max.x, r.max.y+screenHeight};
	return scissorRect;
}

void scissorTest(Rect r) {
	Rect sr = r;
	Vec2 dim = rectDim(sr);
	glScissor(sr.min.x, sr.min.y, dim.x, dim.y);
}

void scissorTest(Rect r, float screenHeight) {
	Rect sr = scissorRectScreenSpace(r, screenHeight);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	glScissor(sr.min.x, sr.min.y, rectW(sr), rectH(sr));
}

void scissorTestScreen(Rect r) {
	Rect sr = scissorRectScreenSpace(r, globalGraphicsState->screenRes.h);
	if(rectW(sr) < 0 || rectH(sr) < 0) sr = rect(0,0,0,0);

	glScissor(sr.min.x, sr.min.y, rectW(sr), rectH(sr));
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
	// Vec4 c = color;
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

	drawLineStripHeader(color);
	rectExpand(&r, offset);
	pushVec(rectBL(r), z);
	pushVec(rectTL(r), z);
	pushVec(rectTR(r), z);
	pushVec(rectBR(r), z);
	pushVec(rectBL(r), z);
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

	// Vec4 cs0 = c0;
	// Vec4 cs1 = c1;
	// Vec4 cs2 = c2;
	// Vec4 cs3 = c3;

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

void drawRectShadow(Rect r, Vec4 color, float size) {
	float z = globalGraphicsState->zOrder;

	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	Vec4 c2 = vec4(0,0);
	glBegin(GL_QUAD_STRIP);

	pushColor(c); pushVec(rectBL(r), z); pushColor(c2); pushVec(rectBL(r) + normVec2(vec2(-1,-1))*size, z);
	pushColor(c); pushVec(rectTL(r), z); pushColor(c2); pushVec(rectTL(r) + normVec2(vec2(-1, 1))*size, z);
	pushColor(c); pushVec(rectTR(r), z); pushColor(c2); pushVec(rectTR(r) + normVec2(vec2( 1, 1))*size, z);
	pushColor(c); pushVec(rectBR(r), z); pushColor(c2); pushVec(rectBR(r) + normVec2(vec2( 1,-1))*size, z);
	pushColor(c); pushVec(rectBL(r), z); pushColor(c2); pushVec(rectBL(r) + normVec2(vec2(-1,-1))*size, z);

	glEnd();
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

	glLineWidth(0.5f);
	drawRectOutline(r, oc);
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
		pushVec(start - down*hw);
		pushVec(start + down*hw);
		pushVec(end + down*hw);
		pushVec(end - down*hw);
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

void drawCross(Vec2 p, float size, float size2, Vec2 dir, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	float z = globalGraphicsState->zOrder;

	size2 = size2 / 2 * 1/0.707f;

	dir = normVec2(dir);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	Vec2 tr = p + vec2(1,1)*size;
	Vec2 br = p + vec2(1,-1)*size;
	Vec2 bl = p + vec2(-1,-1)*size;
	Vec2 tl = p + vec2(-1,1)*size;

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

void drawLine(Vec3 p0, Vec3 p1, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);

	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);
	glBegin(GL_LINES);
		glVertex3f(p0.x, p0.y, p0.z);
		glVertex3f(p1.x, p1.y, p1.z);
	glEnd();
}

Vec3 boxVertices[] = {
	vec3(-0.5f, 0.5f,-0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f,-0.5f,-0.5f), 
	vec3( 0.5f,-0.5f,-0.5f), vec3( 0.5f,-0.5f, 0.5f), vec3( 0.5f, 0.5f, 0.5f), vec3( 0.5f, 0.5f,-0.5f), 
	vec3(-0.5f,-0.5f,-0.5f), vec3(-0.5f,-0.5f, 0.5f), vec3( 0.5f,-0.5f, 0.5f), vec3( 0.5f,-0.5f,-0.5f), 
	vec3( 0.5f, 0.5f,-0.5f), vec3( 0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3(-0.5f, 0.5f,-0.5f), 
	vec3(-0.5f, 0.5f,-0.5f), vec3(-0.5f,-0.5f,-0.5f), vec3( 0.5f,-0.5f,-0.5f), vec3( 0.5f, 0.5f,-0.5f), 
	vec3(-0.5f,-0.5f, 0.5f), vec3(-0.5f, 0.5f, 0.5f), vec3( 0.5f, 0.5f, 0.5f), vec3( 0.5f,-0.5f, 0.5f), 
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
	glBegin(GL_QUADS);
		int i = 0;
		glNormal3f(-1,0,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(1,0,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,-1,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,1,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,0,-1); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,0,1); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
	glEnd();
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

void drawTriangleSubDiv(Vec3 a, Vec3 b, Vec3 c, int divIndex) {
	if(divIndex == 0) {
		Vec3 n = normVec3((a+b+c)/3.0f);
		glNormal3f(n.x, n.y, n.z);
		pushTriangle(a,b,c);
	} else {
		Vec3 ab = normVec3((a+b)/2.0f);
		Vec3 bc = normVec3((b+c)/2.0f);
		Vec3 ca = normVec3((c+a)/2.0f);

		drawTriangleSubDiv(a,ab,ca,  divIndex-1);
		drawTriangleSubDiv(b,bc,ab,  divIndex-1);
		drawTriangleSubDiv(c,ca,bc,  divIndex-1);
		drawTriangleSubDiv(ab,bc,ca, divIndex-1);
	}
}

void drawSphere(Vec3 pos, float r, Vec4 color) {
	glBindTexture(GL_TEXTURE_2D, 0);
	Vec4 c = COLOR_SRGB(color);
	glColor4f(c.r, c.g, c.b, c.a);

	glPushMatrix();
	glTranslatef(pos.x, pos.y, pos.z);
	glScalef(r, r, r);

	int div = 4;
	glBegin(GL_TRIANGLES);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0),  div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(-1,0,0), vec3(0,1,0),  div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(0,-1,0), vec3(-1,0,0), div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(1,0,0),  vec3(0,-1,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(1,0,0),  vec3(0,1,0),  div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(0,1,0),  vec3(-1,0,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(-1,0,0), vec3(0,-1,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(0,-1,0), vec3(1,0,0),  div);
	glEnd();
	glPopMatrix();
}



void drawSphereRaw() {
	glBindTexture(GL_TEXTURE_2D, 0);

	int div = 4;
	glBegin(GL_TRIANGLES);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0),  div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(-1,0,0), vec3(0,1,0),  div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(0,-1,0), vec3(-1,0,0), div);
	drawTriangleSubDiv(vec3(0,0,1),  vec3(1,0,0),  vec3(0,-1,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(1,0,0),  vec3(0,1,0),  div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(0,1,0),  vec3(-1,0,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(-1,0,0), vec3(0,-1,0), div);
	drawTriangleSubDiv(vec3(0,0,-1), vec3(0,-1,0), vec3(1,0,0),  div);
	glEnd();
}

void drawBoxRaw() {
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_QUADS);
		int i = 0;
		glNormal3f(-1,0,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(1,0,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,-1,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,1,0); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,0,-1); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
		glNormal3f(0,0,1); pushQuad(boxVertices[i+0], boxVertices[i+1], boxVertices[i+2], boxVertices[i+3]); i+= 4;
	glEnd();
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

// @Getglyphbox.
void getTextQuad(int c, Font* font, Vec2 pos, Rect* r, Rect* uv) {
	stbtt_aligned_quad q;
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	stbtt_GetPackedQuad(font->cData, font->tex.dim.w, font->tex.dim.h, unicodeOffset, pos.x, pos.y, &q, font->pixelAlign);

	float off = font->baseOffset;
	if(font->pixelAlign) off = roundInt(off);

	*r = rect(q.x0, q.y0 - off, q.x1, q.y1 - off);
	*uv = rect(q.s0, q.t0, q.s1, q.t1);
}

float getCharAdvance(int c, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;
	return result;
}

float getCharAdvance(int c, int c2, Font* font) {
	int unicodeOffset = getUnicodeRangeOffset(c, font);
	float result = font->cData[unicodeOffset].xadvance;
	float kernAdvance = stbtt_GetCodepointKernAdvance(&font->info, c, c2) * font->pixelScale;

	// result += kernAdvance;
	// result = roundFloat(result);

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

		int it = i;
		while(c != '\n' && c != '\0' && c != ' ') {
			wordWidth += getCharAdvance(c, font);
			it += size;
			c = unicodeDecode((uchar*)(&text[it]), &size);
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
		tsi->pos.y -= font->height;

		if(wrapped) {
			return textSim(text, font, tsi, ti, startPos, wrapWidth);
		}
	} else {
		getTextQuad(t, font, tsi->pos, &ti->r, &ti->uv);
		// tsi->pos.x += getCharAdvance(t, font);

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
	return {font, color, shadowMode, vec2(1,-1), shadowSize, shadowColor};
}
TextSettings textSettings(Font* font, Vec4 color) {
	return {font, color};
}


enum {
	TEXT_NOSHADOW = 0,
	TEXT_SHADOW,
	TEXT_OUTLINE,
};

#define Bold_Mark "<b>"
#define Bold_Mark_Size 3
#define Italic_Mark "<i>"
#define Italic_Mark_Size 3
#define Color_Mark "<c>"
#define Color_Mark_Size 3

void textParseMarkers(char* text, TextSimInfo* tsi, Font* font, bool skip = false) {
	if(text[tsi->index] == Bold_Mark[0]) {
		if(strStartsWith(&text[tsi->index], Bold_Mark, Bold_Mark_Size)) {
			tsi->index += Bold_Mark_Size;
			if(skip) return;

			if(!tsi->bold) {
				glEnd();
				glBindTexture(GL_TEXTURE_2D, font->boldFont->tex.id);
				glBegin(GL_QUADS);
			} else {
				glEnd();
				glBindTexture(GL_TEXTURE_2D, font->tex.id);
				glBegin(GL_QUADS);
			}
			tsi->bold = !tsi->bold;
		} else if(strStartsWith(&text[tsi->index], Italic_Mark, Italic_Mark_Size)) {
			tsi->index += Italic_Mark_Size;
			if(skip) return;
			
			if(!tsi->italic) {
				glEnd();
				glBindTexture(GL_TEXTURE_2D, font->italicFont->tex.id);
				glBegin(GL_QUADS);
			} else {
				glEnd();
				glBindTexture(GL_TEXTURE_2D, font->tex.id);
				glBegin(GL_QUADS);
			}
			tsi->italic = !tsi->italic;
		} else if(strStartsWith(&text[tsi->index], Color_Mark, Color_Mark_Size)) {
			if(skip) {
				tsi->index += 9; // <c>FFFFFF
				return;
			}

			tsi->index += 3;
			Vec3 c;
			if(!tsi->colorMode) {
				c.r = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
				c.g = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
				c.b = colorIntToFloat(strHexToInt(getTStringCpy(&text[tsi->index], 2))); tsi->index += 2;
				tsi->colorOverwrite = COLOR_SRGB(c);
			}

			tsi->colorMode = !tsi->colorMode;
		}
	}
}

Vec2 getTextDim(char* text, Font* font, Vec2 startPos = vec2(0,0), int wrapWidth = 0) {
	float maxX = startPos.x;

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		textParseMarkers(text, &tsi, font, true);

		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, wrapWidth)) break;

		maxX = max(maxX, ti.pos.x + ti.posAdvance.x);
	}

	Vec2 dim = vec2(maxX - startPos.x, startPos.y - (tsi.pos.y - font->height));

	return dim;
}

Vec2 testgetTextStartPos(char* text, Font* font, Vec2 startPos, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	Vec2 dim = getTextDim(text, font, startPos, wrapWidth);
	startPos.x -= (align.x+1)*0.5f*dim.w;
	startPos.y -= (align.y-1)*0.5f*dim.h;

	// startPos.x = roundFloat(startPos.x);
	// startPos.y = roundFloat(startPos.y);

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

	Vec4 c = COLOR_SRGB(settings.color);
	Vec4 sc = COLOR_SRGB(settings.shadowColor);

	pushColor(c);
	// Choose texture based on color brightness.
	int texId = font->tex.id;
	float colorSum = c.x + c.y + c.z;
	if(colorSum <= 1.0f) texId = font->darkTex.id;
	else if(colorSum >= 2.0f) texId = font->brightTex.id;
	glBindTexture(GL_TEXTURE_2D, texId);
	glBegin(GL_QUADS);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		
		Font* f = font;
		textParseMarkers(text, &tsi, font);
		if(tsi.bold) f = font->boldFont;
		if(tsi.italic) f = font->italicFont;
		if(tsi.colorMode) pushColor(vec4(tsi.colorOverwrite, 1));
		else pushColor(c);

		TextInfo ti;
		if(!textSim(text, f, &tsi, &ti, startPos, wrapWidth)) break;
		if(text[ti.index] == '\n') continue;

		if(settings.shadowMode != TEXT_NOSHADOW) {
			pushColor(sc);

			if(settings.shadowMode == TEXT_SHADOW) {
				Rect r = rectTrans(ti.r, normVec2(settings.shadowDir) * settings.shadowSize);
				pushRect(r, ti.uv, z);
			} else if(settings.shadowMode == TEXT_OUTLINE) {
				for(int i = 0; i < 8; i++) {
					Vec2 dir = rotateVec2(vec2(1,0), (M_2PI/8)*i);
					Rect r = rectTrans(ti.r, dir*settings.shadowSize);
					pushRect(r, ti.uv, z);
				}
			}

			pushColor(c);
		}

		pushRect(ti.r, ti.uv, z);
	}
	
	glEnd();
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
		TextInfo ti;
		if(!textSim(text, font, &tsi, &ti, startPos, 0)) break;
		if(text[ti.index] == '\n') continue;

		if(ti.pos.x > startPos.x + width) break;

		drawRect(ti.r, color, ti.uv, font->tex.id);
	}
}

Vec2 textIndexToPos(char* text, Font* font, Vec2 startPos, int index, Vec2i align = vec2i(-1,1), int wrapWidth = 0) {
	startPos = testgetTextStartPos(text, font, startPos, align, wrapWidth);

	TextSimInfo tsi = initTextSimInfo(startPos);
	while(true) {
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);

		if(ti.index == index) {
			Vec2 pos = ti.pos - vec2(0, font->height/2);
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
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);

		bool endReached = ti.index == index2;

		if(drawSelection) {
			if(ti.lineBreak || endReached) {

				Vec2 lineEnd;
				if(ti.lineBreak) lineEnd = ti.breakPos;
				else if(!result) lineEnd = tsi.pos;
				else lineEnd = ti.pos;

				Rect r = rect(lineStart - vec2(0,font->height), lineEnd);
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
		TextInfo ti;
		int result = textSim(text, font, &tsi, &ti, startPos, wrapWidth);
		
		bool fLine = valueBetween(mousePos.y, ti.pos.y - font->height, ti.pos.y);
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

	bindFrameBuffer(FRAMEBUFFER_DebugMsaa);
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
	// glBlendEquation(GL_FUNC_ADD);
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
}


void openglDrawFrameBufferAndSwap(WindowSettings* ws, SystemData* sd, i64* swapTime, bool init) {
	
	//
	// Render.
	//

	{
		TIMER_BLOCK_NAMED("Render");

		Vec2i frameBufferRes = getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->dim;
		Vec2i res = ws->currentRes;
		Rect frameBufferUV = rect(0,(float)res.h/frameBufferRes.h,(float)res.w/frameBufferRes.w,0);

		{
			TIMER_BLOCK_NAMED("BlitBuffers");
			blitFrameBuffers(FRAMEBUFFER_2dMsaa, FRAMEBUFFER_2dNoMsaa, res, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		#if USE_SRGB 
		glEnable(GL_FRAMEBUFFER_SRGB);
		#endif 

		{
			TIMER_BLOCK_NAMED("RenderMainBuffer");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glLoadIdentity();
			glViewport(0,0, res.w, res.h);
			glOrtho(0,1,1,0, -1, 1);
			drawRect(rect(0, 1, 1, 0), frameBufferUV, getFrameBuffer(FRAMEBUFFER_2dNoMsaa)->colorSlot[0]->id);
		}

		#if USE_SRGB
		glDisable(GL_FRAMEBUFFER_SRGB);
		#endif
	}

	//
	// Swap window background buffer.
	//

	{
		TIMER_BLOCK_NAMED("Swap");

		// Sleep until monitor refresh.
		if(!init) {
			double frameTime = timerUpdate(*swapTime);
			double sleepTime = ((double)1/60) - frameTime;
			if(sleepTime < 0) sleepTime = ((double)1/30) - frameTime;

			int sleepTimeMS = sleepTime*1000;
			if(sleepTimeMS > 1) Sleep(sleepTimeMS);
		}

		if(init) {
			showWindow(sd->windowHandle);
			GLenum glError = glGetError(); printf("GLError: %i\n", glError);
		}

		swapBuffers(sd);

		*swapTime = timerInit();
	}
}
