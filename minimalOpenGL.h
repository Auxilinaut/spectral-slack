/**
  Credit of original goes to Morgan McGuire, http://graphics.cs.williams.edu

  Minimal headers emulating a basic set of 3D graphics classes.
*/
#pragma once
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"

#ifdef __APPLE__
#   define _OSX
#elif defined(_WIN64)
#   ifndef _WINDOWS
#       define _WINDOWS
#   endif
#elif defined(__linux__)
#   define _LINUX
#endif

#include <GL/glew.h>
#ifdef _WINDOWS
#   include <GL/wglew.h>
#elif defined(_LINUX)
#   include <GL/xglew.h>
#endif
#include <glfw3.h> 


#ifdef _WINDOWS
    // Link against OpenGL
#   pragma comment(lib, "opengl32")
#   pragma comment(lib, "glew32")
#   pragma comment(lib, "glfw3")
#endif

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <vector>


void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    if ((type == GL_DEBUG_TYPE_ERROR) || (type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)) {
        fprintf(stderr, "GL Debug: %s\n", message);
    }
}


GLFWwindow* initOpenGL(int width, int height, const std::string& title) {
    if (! glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW\n");
        ::exit(1);
    } 

    // Without these, shaders actually won't initialize properly
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#   ifdef _DEBUG
       glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#   endif

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (! window) {
        fprintf(stderr, "ERROR: could not open window with GLFW\n");
        glfwTerminate();
        ::exit(2);
    }
    glfwMakeContextCurrent(window);

    // Start GLEW extension handler, with improved support for new features
    glewExperimental = GL_TRUE;
    glewInit();

    // Clear startup errors
    while (glGetError() != GL_NONE) {}

#   ifdef _DEBUG
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glEnable(GL_DEBUG_OUTPUT);
#       ifndef _OSX
            // Causes a segmentation fault on OS X
            glDebugMessageCallback(debugCallback, nullptr);
#       endif
#   endif

    // Negative numbers allow buffer swaps even if they are after the vertical retrace,
    // but that causes stuttering in VR mode
    glfwSwapInterval(0);

    fprintf(stderr, "GPU: %s (OpenGL version %s)\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

    // Check for errors
    { const GLenum error = glGetError(); assert(error == GL_NONE); }

    return window;
}


std::string loadTextFile(const std::string& filename) {
    std::stringstream buffer;
    buffer << std::ifstream(filename.c_str()).rdbuf();
    return buffer.str();
}


GLuint compileShaderStage(GLenum stage, const std::string& source) {
    GLuint shader = glCreateShader(stage);
    const char* srcArray[] = { source.c_str() };

    glShaderSource(shader, 1, srcArray, NULL);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint logSize = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);

        std::vector<GLchar> errorLog(logSize);
        glGetShaderInfoLog(shader, logSize, &logSize, &errorLog[0]);

        fprintf(stderr, "Error while compiling\n %s\n\nError: %s\n", source.c_str(), &errorLog[0]);
        assert(false);

        glDeleteShader(shader);
        shader = GL_NONE;
    }

    return shader;
}


GLuint createShaderProgram(const std::string& vertexShaderSource, const std::string& pixelShaderSource) {
    GLuint shader = glCreateProgram();

    glAttachShader(shader, compileShaderStage(GL_VERTEX_SHADER, vertexShaderSource));
    glAttachShader(shader, compileShaderStage(GL_FRAGMENT_SHADER, pixelShaderSource));
    glLinkProgram(shader);

    return shader;
}


/* Submits a full-screen quad at the far plane and runs a procedural sky shader on it.*/
void drawSky(int windowWidth, int windowHeight, const float* cameraToWorldMatrix, const float* projectionMatrixInverse) {
#   define VERTEX_SHADER(s) "#version 410\n" #s
#   define PIXEL_SHADER(s) VERTEX_SHADER(s)

    static const GLuint skyShader = createShaderProgram(VERTEX_SHADER
    (void main() {
        gl_Position = vec4(gl_VertexID & 1, gl_VertexID >> 1, 0.0, 0.5) * 4.0 - 1.0;
    }),

    PIXEL_SHADER
    (out vec3 pixelColor;

    //uniform vec3  light; //sunlight
    uniform vec2  resolution;
    uniform mat4  cameraToWorldMatrix;
    uniform mat4  invProjectionMatrix;

    float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

    float noise(vec2 x) {
        vec2 i = floor(x);
        float a = hash(i);
        float b = hash(i + vec2(1.0, 0.0));
        float c = hash(i + vec2(0.0, 1.0));
        float d = hash(i + vec2(1.0, 1.0));

        vec2 f = fract(x);
        vec2 u = f * f * (3.0 - 2.0 * f);
        return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
    }

	// FBM Noise
    float fbm(vec2 p) {
        const mat2 m2 = mat2(0.8, -0.6, 0.6, 0.8);
        float f = 0.5000 * noise(p); p = m2 * p * 2.02;
        f += 0.2500 * noise(p); p = m2 * p * 2.03;
        f += 0.1250 * noise(p); p = m2 * p * 2.01;
        f += 0.0625 * noise(p);
        return f / 0.9375;
    }

	// Sky
    vec3 render(in vec3 ro, in vec3 rd, in float resolution) {
        vec3 col;
        
        col = vec3(0.2, 0.2, 0.2) * (1.0 - 0.8 * rd.y) * 0.9;

		// Sun (uses in vec3 sun for sunlight)
        /*float sundot = clamp(dot(rd, sun), 0.0, 1.0);
        col += 0.25 * vec3(1.0, 0.7, 0.4) * pow(sundot, 8.0);
        col += 0.75 * vec3(1.0, 0.8, 0.5) * pow(sundot, 64.0);*/
        col = mix(col, vec3(0.663, 0.663, 0.663), 0.1 * smoothstep(0.5, 0.8, fbm((ro.xz + rd.xz * (25000.0 - ro.y) / rd.y) * 0.000008)));
        return mix(col, vec3(0.0, 0.0, 0.0), pow(1.0 - max(abs(rd.y), 0.0), 8.0));
    }

    void main() {
        vec3 rd = normalize(mat3(cameraToWorldMatrix) * vec3((invProjectionMatrix * vec4(gl_FragCoord.xy / resolution.xy * 2.0 - 1.0, -1.0, 1.0)).xy, -1.0));
        pixelColor = render(cameraToWorldMatrix[3].xyz, rd, resolution.x);
    }));

    static const GLint resolutionUniform                 = glGetUniformLocation(skyShader, "resolution");
    static const GLint cameraToWorldMatrixUniform        = glGetUniformLocation(skyShader, "cameraToWorldMatrix");
    static const GLint invProjectionMatrixUniform        = glGetUniformLocation(skyShader, "invProjectionMatrix");

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(skyShader);
    glUniform2f(resolutionUniform, float(windowWidth), float(windowHeight));

#ifdef _VR
    glUniformMatrix4fv(cameraToWorldMatrixUniform, 1, GL_TRUE, cameraToWorldMatrix);
    glUniformMatrix4fv(invProjectionMatrixUniform, 1, GL_TRUE, projectionMatrixInverse);
#else
	glUniformMatrix4fv(cameraToWorldMatrixUniform, 1, GL_FALSE, cameraToWorldMatrix);
	glUniformMatrix4fv(invProjectionMatrixUniform, 1, GL_FALSE, projectionMatrixInverse);
#endif

    glDrawArrays(GL_TRIANGLES, 0, 3);

#   undef PIXEL_SHADER
#   undef VERTEX_SHADER
}


/** Loads a 24- or 32-bit BMP file into memory */
void loadBMP(const std::string& filename, int& width, int& height, int& channels, std::vector<std::uint8_t>& data) {
    std::fstream hFile(filename.c_str(), std::ios::in | std::ios::binary);
    if (! hFile.is_open()) { throw std::invalid_argument("Error: File Not Found."); }

    hFile.seekg(0, std::ios::end);
    size_t len = hFile.tellg();
    hFile.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> header(len);
    hFile.read(reinterpret_cast<char*>(header.data()), 54);

    if ((header[0] != 'B') && (header[1] != 'M')) {
        hFile.close();
        throw std::invalid_argument("Error: File is not a BMP.");
    }

    if ((header[28] != 24) && (header[28] != 32)) {
        hFile.close();
        throw std::invalid_argument("Error: File is not uncompressed 24 or 32 bits per pixel.");
    }

    const short bitsPerPixel = header[28];
    channels = bitsPerPixel / 8;
    width  = header[18] + (header[19] << 8);
    height = header[22] + (header[23] << 8);
    std::uint32_t offset = header[10] + (header[11] << 8);
    std::uint32_t size = ((width * bitsPerPixel + 31) / 32) * 4 * height;
    data.resize(size);

    hFile.seekg(offset, std::ios::beg);
    hFile.read(reinterpret_cast<char*>(data.data()), size);
    hFile.close();

    // Flip the y axis
    std::vector<std::uint8_t> tmp;
    const size_t rowBytes = width * channels;
    tmp.resize(rowBytes);
    for (int i = height / 2 - 1; i >= 0; --i) {
        const int j = height - 1 - i;
        // Swap the rows
        memcpy(tmp.data(), &data[i * rowBytes], rowBytes);
        memcpy(&data[i * rowBytes], &data[j * rowBytes], rowBytes);
        memcpy(&data[j * rowBytes], tmp.data(), rowBytes);
    }

    // Convert BGR[A] format to RGB[A] format
    if (channels == 4) {
        // BGRA
        std::uint32_t* p = reinterpret_cast<std::uint32_t*>(data.data());
        for (int i = width * height - 1; i >= 0; --i) {
            const unsigned int x = p[i];
            p[i] = ((x >> 24) & 0xFF) | (((x >> 16) & 0xFF) << 8) | (((x >> 8) & 0xFF) << 16) | ((x & 0xFF) << 24);
        }
    } else {
        // BGR
        for (int i = (width * height - 1) * 3; i >= 0; i -= 3) {
            std::swap(data[i], data[i + 2]);
        }
    }
}

#pragma clang diagnostic pop
