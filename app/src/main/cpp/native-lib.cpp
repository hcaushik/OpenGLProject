#include <jni.h>
#include <string>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define PI 3.14159265
#define MAX_WORDS 10

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
                                                    = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader =
        "uniform mat4 uMVPMatrix;\n"
        "attribute vec4 vPosition;\n"
        "void main() {\n"
        "  gl_Position = vPosition * uMVPMatrix;\n"
        "}\n";

auto gFragmentShader =
        "precision mediump float;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
        "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                         shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint MVPHandle;

const GLfloat gTriangleVertices[] = { 0.0f, 0.25f, -0.25f, -0.25f,
                                      0.25f, -0.25f };
GLfloat rotation[] = {0.0f, -1.0f, 0.0f, 0.0f,
                            1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f};
double angle = 0.0;
bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    MVPHandle = glGetUniformLocation(gProgram, "uMVPMatrix");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
         gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");

    return true;
}


void renderFrame() {
    static float grey;
    /*grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }*/
    grey = 1.0f;
    double inc = 5.0;
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");
    angle = angle > 360 ? angle - 360: angle;

    rotation[0] = cos(angle * PI / 180);
    rotation[1] = -1 * sin(angle * PI / 180);
    rotation[4] = sin(angle * PI / 180);
    rotation[5] = cos(angle * PI / 180);
    angle += inc;
    glUniformMatrix4fv(MVPHandle, 1, GL_FALSE, rotation);
    checkGlError("glUniformMatrix2fv");
    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");


    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_helloworldnative_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject o/* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_helloworldnative_MainActivity_randomString(
        JNIEnv *env,
        jobject o/* this */) {
    std::vector<std::string> strCollection = {"Java", "C", "C++", "Matlab", "Python"};
    int nWords = rand() % MAX_WORDS;
    int i;
    std::string entry = "";
    for (i = 0; i < nWords; i++)
    {
        entry += " " + strCollection[rand() % strCollection.size()];
    }
    return env->NewStringUTF(entry.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_helloworldnative_RendererWrapper_onSurfaceCreated(
        JNIEnv *env,
        jobject o/* this */) {
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_helloworldnative_RendererWrapper_onSurfaceChanged(
        JNIEnv *env,
        jobject o,
        jint width,
        jint height/* this */) {
    setupGraphics(width, height);

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_helloworldnative_RendererWrapper_onDrawFrame(
        JNIEnv *env,
        jobject o/* this */) {
    glClear(GL_COLOR_BUFFER_BIT);
    renderFrame();
}
