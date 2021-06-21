
// Minimal DSA/Bindless/Multidraw OpenGL by Jesse Fong
// 
// OpenGL is a Chonker of a spec at this point and while most people are moving to Vulkan and DX12,
// I find that modern OpenGL is still usefull for personal projects because of its turseness and fire and forget approach to gpu memory
// Finding reliable modern opengl resources can be difficult, so I wanted to make a sample app that brings a bit of it together.
//
// Here are some useful resources I have used in the past
//
// Fendevel's Guide To Modern OpenGL Functions (DSA)
// https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions
// 
// Litasa's Blog Post on MultiDrawIndirect
// https://litasa.github.io/blog/2017/09/04/OpenGL-MultiDrawIndirect-with-Individual-Textures
//
// NVIDIA's Bindless Graphgics Tutorial
// https://www.nvidia.com/en-us/drivers/bindless-graphics/
//

#pragma comment(lib, "user32")
#pragma comment(lib, "Gdi32")
#pragma comment(lib, "Opengl32")

#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "opengl_defines.h"
#include "win32_opengl_defines.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0])) 
static bool GlobalRunning;

LRESULT CALLBACK
WindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_CLOSE:
        case WM_QUIT:
        {
            GlobalRunning = false;
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

static HGLRC GlobalWin32OpenGLContext;

static void*
Win32LoadOpenGLFunction(char *FunctionName) {
    void *Result = 0;
    Result = (void *)wglGetProcAddress(FunctionName);
    if (Result == 0 || (Result == (void *)0x1) || (Result == (void *)0x2)
        || (Result == (void *)0x3) || (Result == (void *)-1)) {
        HMODULE OpenglLibrary = LoadLibraryA("opengl32.dll");
        Result = (void *)GetProcAddress(OpenglLibrary, FunctionName);
        FreeLibrary(OpenglLibrary);
    }
    return Result;
}

static PFNWGLCHOOSEPIXELFORMATARBPROC     wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC  wglCreateContextAttribsARB;
static PFNWGLMAKECONTEXTCURRENTARBPROC    wglMakeContextCurrentARB;
static PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT;

static bool
Win32InitializeOpenGL(HDC WindowDC) {
    
    int MajorVersion = 4;
    int MinorVersion = 5;
    
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cStencilBits = 8;
    
    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(PIXELFORMATDESCRIPTOR), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC Win32RenderContext = wglCreateContext(WindowDC);
    wglMakeCurrent(WindowDC, Win32RenderContext);
    
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)Win32LoadOpenGLFunction("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)Win32LoadOpenGLFunction("wglCreateContextAttribsARB");
    wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC)Win32LoadOpenGLFunction("wglMakeContextCurrentARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)Win32LoadOpenGLFunction("wglSwapIntervalEXT");
    
    
    wglMakeCurrent(WindowDC, 0);
    wglDeleteContext(Win32RenderContext);
    
    int AttributeList[] = {
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        WGL_CONTEXT_MAJOR_VERSION_ARB, MajorVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, MinorVersion,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0,
    };
    
    GlobalWin32OpenGLContext = wglCreateContextAttribsARB(WindowDC, 0, AttributeList);
    if (GlobalWin32OpenGLContext) {
        if (wglMakeCurrent(WindowDC, GlobalWin32OpenGLContext) == TRUE) {
            LoadGLFunctions(Win32LoadOpenGLFunction);
            return true;
        }
    }
    
    return false;
}

#define GL_DEBUG_CALLBACK(Name) void WINAPI Name(GLenum Source,GLenum Type,GLuint ID,GLenum Severity,GLsizei Length,const GLchar* Message,const void *userParam)

GL_DEBUG_CALLBACK(OpenGLDebugMessageCallback) {
    
    char* _Source;
    char* _Type;
    char* _Severity;
    
    switch (Source) {
        case GL_DEBUG_SOURCE_API: _Source = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: _Source = "WINDOW SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: _Source = "SHADER COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: _Source = "THIRD PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: _Source = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: _Source = "UNKNOWN"; break;
        
        default:_Source = "UNKNOWN";break;
    }
    
    switch (Type) {
        case GL_DEBUG_TYPE_ERROR: _Type = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: _Type = "DEPRECATED BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: _Type = "UDEFINED BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: _Type = "PORTABILITY";break;
        case GL_DEBUG_TYPE_PERFORMANCE:_Type = "PERFORMANCE";break;
        case GL_DEBUG_TYPE_OTHER:_Type = "OTHER";break;
        
        case GL_DEBUG_TYPE_MARKER:_Type = "MARKER";break;
        default:_Type = "UNKNOWN";break;
    }
    
    switch (Severity) {
        case GL_DEBUG_SEVERITY_HIGH:_Severity = "HIGH";break;
        case GL_DEBUG_SEVERITY_MEDIUM:_Severity = "MEDIUM";break;
        case GL_DEBUG_SEVERITY_LOW:_Severity = "LOW";break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:_Severity = "NOTIFICATION";break;
        default:_Severity = "UNKNOWN";break;
    }
    
    char Buffer[512] = {};
    snprintf(Buffer, 512, "%s, %s, %s, : %s\n", _Source, _Type, _Severity, Message);
    OutputDebugStringA(Buffer);
}

static GLuint
CompileShaders() {
    
    const GLchar* VSSource = 
        
    R"(
#version 450 core
#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require //Required for uint64_t type

layout(location = 0)in vec3 Position;
layout(location = 1)in vec2 TexCoord;

struct draw_uniform {
float X;
float Y;
};

layout(binding = 0)uniform UniformArray
{
 draw_uniform Uniforms[512];
};

out vec2 UV;
out uint TextureIndex;
void main() {
float FinalX = Uniforms[gl_InstanceID].X;
float FinalY = Uniforms[gl_InstanceID].Y;

//Just Fudging with numbers as a uniform example
vec3 FinalPosition = (Position * 0.2);
 FinalPosition.x += FinalX + 0.125;
FinalPosition.y += FinalY + 0.125;

 gl_Position = vec4((FinalPosition), 1.0f);
UV = TexCoord;
TextureIndex = gl_InstanceID;
}
)";
    
    const GLchar* FSSource = 
    R"(
#version 450 core
#extension GL_NV_bindless_texture : require
#extension GL_NV_gpu_shader5 : require //Required for uint64_t type

layout(binding = 1)uniform TextureArray
{
uint64_t Textures[512];
};

layout(location = 0) out vec4 OutColor;

in vec2 UV;
flat in uint TextureIndex;
void main() {

sampler2D Sampler = sampler2D(Textures[TextureIndex]);
vec4 TextureColor = texture(Sampler, UV);
OutColor = TextureColor;
}
)";
    
    GLuint ProgramID = glCreateProgram();
    GLuint VSID = glCreateShader(GL_VERTEX_SHADER);
    
    glShaderSource(VSID, 1, &VSSource, 0);
    glCompileShader(VSID);
    
    GLuint FSID = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(FSID, 1, &FSSource, 0);
    glCompileShader(FSID);
    
    glAttachShader(ProgramID, VSID);
    glAttachShader(ProgramID, FSID);
    
    glLinkProgram(ProgramID);
    glValidateProgram(ProgramID);
    
    GLint Linked = false;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked);
    if(!Linked)
    {
        GLsizei Ignored;
        char VertexErrors[4096];
        char FragmentErrors[4096];
        char ProgramErrors[4096];
        glGetShaderInfoLog(VSID, sizeof(VertexErrors), &Ignored, VertexErrors);
        glGetShaderInfoLog(FSID, sizeof(FragmentErrors), &Ignored, FragmentErrors);
        glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);
        
    }
    
    //glGetProgramiv(), glGetShaderInfoLog() and glGetPrgoramInfoLog()  would go here to catch errors
    
    glDeleteShader(VSID);
    glDeleteShader(FSID);
    
    return ProgramID;
    
}

int CALLBACK
WinMain(HINSTANCE CurrentInstance, 
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    //
    //
    // Win32 Init
    
    WNDCLASSA WindowClass = {};
    WindowClass.lpfnWndProc = WindowCallback;
    WindowClass.hInstance = CurrentInstance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "MinimalBindlessGLWindowClass";
    RegisterClassA(&WindowClass);
    
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, 
                                  "Minimal Bindless GL",
                                  WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  0, 0, CurrentInstance, 0);
    
    HDC WindowDC = GetDC(Window);
    Win32InitializeOpenGL(WindowDC);
    
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallbackARB(OpenGLDebugMessageCallback, NULL);
    
    float VertexData[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.5f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    };
    
    GLuint IndexData[] = {
        0, 1, 2,
    };
    
    GLuint VertexBuffer;
    glCreateBuffers(1, &VertexBuffer);
    glNamedBufferStorage(VertexBuffer, sizeof(VertexData), VertexData, GL_DYNAMIC_STORAGE_BIT);
    
    GLuint IndexBuffer;
    glCreateBuffers(1, &IndexBuffer);
    glNamedBufferStorage(IndexBuffer, sizeof(IndexData), IndexData, GL_DYNAMIC_STORAGE_BIT);
    
    
    GLuint VAO;
    glCreateVertexArrays(1, &VAO);
    
    glVertexArrayVertexBuffer(VAO, 0, VertexBuffer, 0, sizeof(float)*5);
    glVertexArrayElementBuffer(VAO, IndexBuffer);
    
    glEnableVertexArrayAttrib(VAO, 0);
    glEnableVertexArrayAttrib(VAO, 1);
    
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2);
    
    glVertexArrayAttribBinding(VAO, 0, 0);
    glVertexArrayAttribBinding(VAO, 1, 0);
    
    
    ////////////////////////////
    //NOTE(Jesse): The Demo Uses gl_InstanceID to index into buffers but you could upload a vertex buffer containing the same information with the following
    //It get a little tricky when you are interleaving draw calls with different instance counts
#if 0
    //Setting up draw ID buffer to be able to index into bindless resources (texture/uniforms)
#define MAX_DRAW_COUNT 128
    GLuint DrawIDs[MAX_DRAW_COUNT];
    for(int DrawIndex = 0;
        DrawIndex < MAX_DRAW_COUNT;
        DrawIndex++) {
        DrawIDs[DrawIndex] = DrawIndex;
    }
    
    GLuint DrawIDBuffer;
    glCreateBuffers(1, &DrawIDBuffer);
    glNamedBufferStorage(DrawIDBuffer, sizeof(DrawIDs), DrawIDs, GL_DYNAMIC_STORAGE_BIT);
    
    glVertexArrayVertexBuffer(VAO, 1, DrawIDBuffer, 0, 0);
    
    glEnableVertexArrayAttrib(VAO, 2);
    glVertexArrayAttribFormat(VAO, 2, 1, GL_UNSIGNED_INT, GL_FALSE, 0);
    
    glVertexArrayAttribBinding(VAO, 2, 1);
    //Modify the rate of DrawIDs in shader to increase once per draw call
    glVertexArrayBindingDivisor(VAO, 2, 1);
#endif
    ///////////////////////////
    
    //NOTE(Jesse): Bulding Texture and Uniforms at the same time
#define TRIANGLES_PER_ROW 8
#define TRIANGLES_PER_COLUMN 8
#define DRAW_COUNT (TRIANGLES_PER_ROW*TRIANGLES_PER_COLUMN)
    
    struct bitmap {
        uint8_t R;
        uint8_t G;
        uint8_t B;
        uint8_t A;
    }; 
    
    bitmap* BitmapArray = (bitmap*)VirtualAlloc(NULL, sizeof(bitmap)*DRAW_COUNT, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    uint64_t BindlessTextureHandles[DRAW_COUNT] = {};
    
    float InvColorX = 255.0f/(float)TRIANGLES_PER_ROW;
    float InvColorY = 255.0f/(float)TRIANGLES_PER_COLUMN;
    
    /////////////////////////////////////////////////////
    
    //NOTE(Jesse): Matches shader struct
    struct uniform {
        float X;
        float Y;
    };
    
    uniform* UniformArray = (uniform*)VirtualAlloc(NULL, sizeof(uniform)*DRAW_COUNT, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    
    float InvLocationX = 2.0f/(float)TRIANGLES_PER_ROW;
    float InvLocationY = 2.0f/(float)TRIANGLES_PER_COLUMN;
    
    int DrawIndex = 0;
    for(int Y = 0;
        Y < TRIANGLES_PER_COLUMN;
        Y++) {
        for(int X = 0;
            X < TRIANGLES_PER_ROW;
            X++) {
            
            //TEXTURE CREATION
            bitmap* Bitmap = &BitmapArray[DrawIndex];
            Bitmap->R = X*InvColorX;
            Bitmap->G = Y*InvColorY;
            Bitmap->B = 128;
            Bitmap->A = 255;
            
            GLuint TextureID;
            glCreateTextures(GL_TEXTURE_2D, 1, &TextureID);
            
            glTextureParameteri(TextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(TextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTextureParameteri(TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glTextureStorage2D(TextureID, 1, GL_RGBA8, 1, 1);
            glTextureSubImage2D(TextureID, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t*)Bitmap);
            
            
            //RETREVING BINDLESS HANDLE FOR LATER USAGE
            uint64_t BindlessHandle = glGetTextureHandleARB(TextureID);
            glMakeTextureHandleResidentARB(BindlessHandle);
            BindlessTextureHandles[DrawIndex] = BindlessHandle;
            
            
            //////////////////////////////////////////////////////////////////////////////
            
            //UNIFORM CREATION
            uniform* Uniform = &UniformArray[DrawIndex];
            Uniform->X = X*InvLocationX - 1.0f;
            Uniform->Y = Y*InvLocationY - 1.0f;
            
            DrawIndex++;
        }
    }
    
    GLuint TextureBuffer;
    glCreateBuffers(1, &TextureBuffer);
    glNamedBufferStorage(TextureBuffer, sizeof(BindlessTextureHandles), BindlessTextureHandles, GL_DYNAMIC_STORAGE_BIT);
    
    GLuint UniformBuffer;
    glCreateBuffers(1, &UniformBuffer);
    glNamedBufferData(UniformBuffer, sizeof(uniform)*DRAW_COUNT, UniformArray, GL_DYNAMIC_DRAW);
    
    //Building Multidraw data structures to put in a command buffer (must match opengl's data structure)
    //NOTE(Jesse):https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glMultiDrawElementsIndirect.xhtml
    
    struct gl_draw_element_indirect_command {
        GLuint Count;
        GLuint InstanceCount;
        GLuint FirstIndex;
        GLuint BaseVertex;
        GLuint BaseInstance;
    };
    
    // If you've never used indirect drawing before, essentially there is a trend towards grouping drawcalls together 
    // glMultiDrawElementsIndirect/glMultiDrawArraysIndirect is the latest and may seem a bit confusing as to what it's trying to do 
    // but if you work your way down the stack it becomes clear
    // glMultiDrawElementsIndirect -(multiple calls of)-> glDrawElementsInstancedBaseVertexBaseInstance -(a derivitive of)-> glDrawElementsInstanced
    // -(multiple calls of)-> glDrawElements
    // Eventually when khronos gets to glMultiDrawMultiDrawElementsIndirectInstancedBaseMultidrawInstanceWithASliceOfLime i'll update this again
    gl_draw_element_indirect_command IndirectCommand;
    IndirectCommand.Count = 3;
    IndirectCommand.InstanceCount = DRAW_COUNT;
    IndirectCommand.FirstIndex = 0;
    IndirectCommand.BaseVertex = 0;
    IndirectCommand.BaseInstance = 0;
    
    
    GLuint IndirectCommandBuffer;
    glCreateBuffers(1, &IndirectCommandBuffer);
    glNamedBufferStorage(IndirectCommandBuffer, sizeof(gl_draw_element_indirect_command), &IndirectCommand, GL_DYNAMIC_STORAGE_BIT);
    
    
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    
    GLuint ShaderID = CompileShaders();
    glUseProgram(ShaderID);
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    
    GLuint UniformArrayID = glGetUniformBlockIndex(ShaderID, "UniformArray");
    glUniformBlockBinding(ShaderID, UniformArrayID, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, UniformBuffer);
    
    GLuint TextureArrayID = glGetUniformBlockIndex(ShaderID, "TextureArray");
    glUniformBlockBinding(ShaderID, TextureArrayID, 1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, TextureBuffer);
    
    
    //While we are able to use DSA to modify state without binding, before drawing you must finally bind the objects you use
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, IndirectCommandBuffer);
    //
    //
    // Render Loop
    GlobalRunning = true;
    while(GlobalRunning)
    {
        MSG Message;
        if (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, NULL, 1, 0);
        
        SwapBuffers(WindowDC);
        Sleep(100);
    }
    
    return 0;
}