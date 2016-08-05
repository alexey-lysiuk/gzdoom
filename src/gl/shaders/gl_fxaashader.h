#ifndef __GL_FXAASHADER_H__
#define __GL_FXAASHADER_H__

#include "gl_shaderprogram.h"

class FFXAAShader
{
public:
	void Bind();

	FBufferedUniform1i InputTexture;
	FBufferedUniform2f Resolution;

private:
	FShaderProgram mShader;
};

#endif // __GL_FXAASHADER_H__
