//
//  SwapTexture.cpp
//  gpuPS
//
//  Created by Éric Renaud-Houde on 2013-01-06.
//
//

#include "PingPongFbo.h"
#include <assert.h>
#include <utility>


PingPongFbo::PingPongFbo( const std::vector<ci::Surface32f>& surfaces )
: mCurrentFbo(0)
{
	if( surfaces.empty() ) return;
	
	int i = 0;
	mTextureSize = surfaces[0].getSize();
	for( const ci::Surface32f& s : surfaces) {
		mAttachments.push_back(GL_COLOR_ATTACHMENT0_EXT + i);
		addTexture(s);
		i++;
	}
	
	int max =ci::gl::Fbo::getMaxAttachments();
	std::cout << "Maximum supported number of texture attachments: " << max << std::endl;
	assert(surfaces.size() < max);
	
	ci::gl::Fbo::Format format;
	format.enableDepthBuffer(false);
	format.enableColorBuffer(true, mAttachments.size());
	format.setMinFilter( GL_NEAREST );
	format.setMagFilter( GL_NEAREST );
	format.setColorInternalFormat( GL_RGBA32F_ARB );
	mFbos[0] = ci::gl::Fbo( mTextureSize.x, mTextureSize.y, format );
	mFbos[1] = ci::gl::Fbo( mTextureSize.x, mTextureSize.y, format );
	
	reset();
}

void PingPongFbo::addTexture(const ci::Surface32f &surface)
{
    assert(mTextures.size() < mAttachments.size());
    assert(surface.getSize() == mTextureSize);
    
    ci::gl::Texture::Format format;
    format.setInternalFormat( GL_RGBA32F_ARB );
	ci::gl::Texture tex = ci::gl::Texture( surface, format );
    tex.setWrap( GL_REPEAT, GL_REPEAT );
    tex.setMinFilter( GL_NEAREST );
    tex.setMagFilter( GL_NEAREST );
    mTextures.push_back( tex );
}

void PingPongFbo::reset()
{
	mFbos[mCurrentFbo].bindFramebuffer();
    ci::gl::setMatricesWindow( getSize(), false );
    ci::gl::setViewport( getBounds() );
    ci::gl::clear();
    for(int i=0; i<mAttachments.size(); ++i) {
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + i);
        
        mTextures[i].enableAndBind();
        ci::gl::draw( mTextures[i], getBounds() );
        mTextures[i].unbind();
        mTextures[i].disable();
    }
    mFbos[mCurrentFbo].unbindFramebuffer();
	
	mFbos[!mCurrentFbo] = mFbos[mCurrentFbo];
}

void PingPongFbo::swap()
{
    mCurrentFbo = !mCurrentFbo;
}

void PingPongFbo::bindUpdate()
{
    mFbos[ mCurrentFbo ].bindFramebuffer();
    
    glDrawBuffers(mAttachments.size(), &mAttachments[0]);
    
    for(int i=0; i<mAttachments.size(); ++i) {
        mFbos[!mCurrentFbo].bindTexture(i, i);
    }
}

void PingPongFbo::unbindUpdate()
{
    mFbos[ !mCurrentFbo ].unbindTexture();
    mFbos[ mCurrentFbo ].unbindFramebuffer();
	
	swap();
}

void PingPongFbo::bindTexture(int textureUnit)
{
    mFbos[mCurrentFbo].bindTexture(textureUnit, textureUnit);
}

void PingPongFbo::unbindTexture()
{
    mFbos[mCurrentFbo].unbindTexture();
}

ci::Vec2i PingPongFbo::getSize() const
{
    return mTextureSize;
}


ci::Area PingPongFbo::getBounds() const
{
    return mFbos[0].getBounds();
}
