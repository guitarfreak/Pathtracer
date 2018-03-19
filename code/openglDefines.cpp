
#define GL_TEXTURE_CUBE_MAP_SEAMLESS      0x884F
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_TEXTURE_BUFFER                 0x8C2A
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_PERSISTENT_BIT             0x0040
#define GL_MAP_COHERENT_BIT               0x0080
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER_BIT              0x00000001
#define GL_FRAGMENT_SHADER_BIT            0x00000002
#define GL_DEBUG_OUTPUT                   0x92E0
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x0001
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_RGB32F                         0x8815
#define GL_RGBA8I                         0x8D8E
#define GL_RGBA8UI                        0x8D7C
#define GL_R8                             0x8229
#define GL_ARRAY_BUFFER                   0x8892

#define GL_NUM_EXTENSIONS                 0x821D

#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9

#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_STATIC_DRAW                    0x88E4
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_TEXTURE_CUBE_MAP_ARRAY         0x9009
#define GL_STREAM_DRAW                    0x88E0
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_FRAMEBUFFER                    0x8D40
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH32F_STENCIL8              0x8CAD
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#define GL_MULTISAMPLE                    0x809D
#define GL_FUNC_ADD                       0x8006
#define GL_CLIP_DISTANCE0                 0x3000
#define GL_CLIP_DISTANCE1                 0x3001
#define GL_FUNC_ADD                       0x8006
#define GL_MAX                            0x8008
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B
#define GL_SRGB8                          0x8C41
#define GL_DEPTH_COMPONENT32F             0x8CAC


#define makeGLFunction(returnType, name, ...) \
	typedef returnType WINAPI name##Function(__VA_ARGS__); \
	name##Function* gl##name; 
#define loadGLFunction(name) \
	gl##name = (name##Function*)wglGetProcAddress("gl" #name);


#define GL_FUNCTION_LIST \
	GLOP(void, TexStorage2D, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) \
	GLOP(void, GenSamplers, GLsizei n, GLuint *samplers) \
	GLOP(void, GenFramebuffers, GLsizei n, GLuint *framebuffers) \
	GLOP(void, GenRenderbuffers, GLsizei n, GLuint *renderbuffers) \
	GLOP(void, BindRenderbuffer, GLenum target, GLuint renderbuffer) \
	GLOP(void, BindVertexArray, GLuint array) \
	GLOP(void, GenerateTextureMipmap, GLuint texture) \
	GLOP(void, TextureSubImage2D, GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) \
	GLOP(void, SamplerParameteri, GLuint sampler, GLenum pname, GLint param) \
	GLOP(void, TextureParameteri, GLuint texture, GLenum pname, GLint param) \
	GLOP(void, UseProgram, GLuint program) \
	GLOP(void, BindBuffer, GLenum target, GLuint buffer) \
	GLOP(void, BindSampler, GLuint unit, GLuint sampler) \
	GLOP(void, NamedRenderbufferStorageMultisample, GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) \
	GLOP(void, NamedFramebufferRenderbuffer, GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \
	GLOP(void, BindFramebuffer, GLenum target, GLuint framebuffer) \
	GLOP(void, BlitNamedFramebuffer, GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) \
	GLOP(void, NamedFramebufferTexture, GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) \
	GLOP(GLenum, CheckNamedFramebufferStatus, GLuint framebuffer, GLenum target) \
	GLOP(void, BlendFuncSeparate, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) \
	GLOP(void, BlendEquation, GLenum mode) \
	GLOP(void, BlendEquationSeparate, GLenum modeRGB, GLenum modeAlpha) \
	GLOP(GLubyte*, GetStringi, GLenum name, GLuint index) 


#define GLOP(returnType, name, ...) makeGLFunction(returnType, name, __VA_ARGS__) 
	GL_FUNCTION_LIST
#undef GLOP


// typedef HGLRC wglCreateContextAttribsARBFunction(HDC hDC, HGLRC hshareContext, const int *attribList);
// wglCreateContextAttribsARBFunction* wglCreateContextAttribsARB;
typedef int WINAPI wglGetSwapIntervalEXTFunction(void);
wglGetSwapIntervalEXTFunction* wglGetSwapIntervalEXT;
typedef int WINAPI wglSwapIntervalEXTFunction(int);
wglSwapIntervalEXTFunction* wglSwapIntervalEXT;

typedef const char* WINAPI wglGetExtensionsStringEXTFunction(void);
wglGetExtensionsStringEXTFunction* wglGetExtensionsStringEXT;

typedef HGLRC WINAPI wglCreateContextAttribsARBFunction(HDC hDC, HGLRC hshareContext, const int *attribList);
wglCreateContextAttribsARBFunction* wglCreateContextAttribsARB;


void loadFunctions() {
#define GLOP(returnType, name, ...) loadGLFunction(name)
	GL_FUNCTION_LIST

	wglGetSwapIntervalEXT = (wglGetSwapIntervalEXTFunction*)wglGetProcAddress("wglGetSwapIntervalEXT");
	wglSwapIntervalEXT = (wglSwapIntervalEXTFunction*)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetExtensionsStringEXT = (wglGetExtensionsStringEXTFunction*)wglGetProcAddress("wglGetExtensionsStringEXT");
	wglCreateContextAttribsARB = (wglCreateContextAttribsARBFunction*)wglGetProcAddress("wglCreateContextAttribsARB");

#undef GLOP
}

void printGlExtensions() {
	for(int i = 0; i < GL_NUM_EXTENSIONS; i++) {
		char* s = (char*)glGetStringi(GL_EXTENSIONS, i);
		printf("%s\n", s);
	}
}


