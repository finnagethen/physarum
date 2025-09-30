#ifndef SHADER_H
#define SHADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>

#include "shader.h"

void checkCompileError(unsigned int shader, char* shaderName);

void checkLinkingError(unsigned int program);

char* file2Str(const char* path);

unsigned int createComputeShader(const char* computeShaderFilePath);

unsigned int createShader(const char* vertexShaderFilePath, const char* fragmentShaderFilePath);

#endif
