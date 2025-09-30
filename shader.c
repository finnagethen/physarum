#ifndef SHADER_H
#define SHADER_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>

void checkCompileError(unsigned int shader, char* shaderName){
	int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::%s::COMPILATION_FAILED:\n%s", shaderName, infoLog);
    }
}

void checkLinkingError(unsigned int program){
	int success;
    char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED:\n%s", infoLog);    
    }
}

char* file2Str(const char* path){	// returns allocated memory pointer, must free after use!
	FILE* fptr = fopen(path, "r");
	
	int bufferSize = 512;
	char buffer[bufferSize];
	
	char* str = (char*)malloc(bufferSize * sizeof(char));
	
	int i = 0;
	while (fgets(buffer, bufferSize, fptr)){
		if (i == 0){
			strcpy(str, buffer);
		}else{
			str = (char*)realloc(str, bufferSize * i);
			strcat(str, buffer);
		}
		i++;			
	}
	fclose(fptr);
	
	return str;	// free str after use;
}

unsigned int createComputeShader(const char* computeShaderFilePath){
	char* computeShaderSource = file2Str(computeShaderFilePath);
	
	unsigned int computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, (const char* const*)&computeShaderSource, NULL);
	glCompileShader(computeShader);
	
	checkCompileError(computeShader, "COMPUTE");
	
	unsigned int computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);
	
	checkLinkingError(computeProgram);
	
	glDeleteShader(computeShader);
	free(computeShaderSource);
	
	return computeProgram;
}

unsigned int createShader(const char* vertexShaderFilePath, const char* fragmentShaderFilePath){
	char* vertexShaderSource = file2Str(vertexShaderFilePath);
	char* fragmentShaderSource = file2Str(fragmentShaderFilePath);
	
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const char* const*)&vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	
	// Check for shader compile errors
	checkCompileError(vertexShader, "VERTEX");
	
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const char* const*)&fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
	
	// Check for shader compile errors
    checkCompileError(fragmentShader, "FRAGMENT");
	
	// Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
	
	// Check for linking errors
    checkLinkingError(shaderProgram);
	
	glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	
	free(vertexShaderSource);
	free(fragmentShaderSource);
	
	return shaderProgram;
}
