/*
** gl_quadstereo.cpp
** Quad-buffered OpenGL stereoscopic 3D mode for GZDoom
**
**---------------------------------------------------------------------------
** Copyright 2015 Christopher Bruns
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "gl_quadstereo.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_renderbuffers.h"

namespace s3d {

QuadStereo::QuadStereo(double ipdMeters)
	: leftEye(ipdMeters), rightEye(ipdMeters)
{
	// Check whether quad-buffered stereo is supported in the current context
	// We are assuming the OpenGL context is already current at this point,
	// i.e. this constructor is called "just in time".
	GLboolean supportsStereo, supportsBuffered;
	glGetBooleanv(GL_STEREO, &supportsStereo);
	glGetBooleanv(GL_DOUBLEBUFFER, &supportsBuffered);
	bQuadStereoSupported = supportsStereo && supportsBuffered;
	leftEye.bQuadStereoSupported = bQuadStereoSupported;
	rightEye.bQuadStereoSupported = bQuadStereoSupported;

	eye_ptrs.Push(&leftEye);
	// If stereo is not supported, just draw scene once (left eye view only)
	if (bQuadStereoSupported) {
		eye_ptrs.Push(&rightEye);
	}
}

void QuadStereo::Present() const
{
	if (bQuadStereoSupported)
	{
		GLRenderer->mBuffers->BindOutputFB();

		glDrawBuffer(GL_BACK_LEFT);
		GLRenderer->ClearBorders();
		GLRenderer->mBuffers->BindEyeTexture(0, 0);
		GLRenderer->DrawPresentTexture(GLRenderer->mOutputLetterbox, true);

		glDrawBuffer(GL_BACK_RIGHT);
		GLRenderer->ClearBorders();
		GLRenderer->mBuffers->BindEyeTexture(1, 0);
		GLRenderer->DrawPresentTexture(GLRenderer->mOutputLetterbox, true);

		glDrawBuffer(GL_BACK);
	}
	else
	{
		GLRenderer->mBuffers->BindOutputFB();
		GLRenderer->ClearBorders();
		GLRenderer->mBuffers->BindEyeTexture(1, 0);
		GLRenderer->DrawPresentTexture(GLRenderer->mOutputLetterbox, true);
	}
}

/* static */
const QuadStereo& QuadStereo::getInstance(float ipd)
{
	static QuadStereo instance(ipd);
	return instance;
}

} /* namespace s3d */
