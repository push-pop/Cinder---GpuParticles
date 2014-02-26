#pragma once

#include "cinder/app/AppBasic.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "Resources.h"
#include "PingPongFbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder\MayaCamUI.h"
#include "VisionCamera.h"
#include "OscListener.h"
#include "GlobalSettings.h"
#include "GlobalLog.h"

class GpuParticlesApp : public ci::app::AppBasic {
public:
	void setup();
	void mouseDown(ci::app::MouseEvent event);
	void mouseMove(ci::app::MouseEvent event);
	void keyDown(ci::app::KeyEvent event);
	void update();
	void draw();
	void prepareSettings(ci::app::AppBasic::Settings *settings);

	void drawFullScreenRect();

private:
	void				computeAttractorPosition();
	ci::Vec3f			computeAttractorPositionWithRay(ci::Vec2i pos);
	ci::MayaCamUI mMayaCam;
	// Frame buffer for ping-pong
	void					setupPingPongFbo();
	void					setupVbo();
	void					emitParticle(ci::Vec2f pos);
	ci::Vec3f				newParticle;
	bool					particlesToAdd;
	//Shaders
	ci::gl::GlslProg		mParticlesShader;
	ci::gl::GlslProg		mDisplacementShader;

	//Dummy Vbo
	ci::gl::VboMesh			mVboMesh;

	//Ping Pong
	PingPongFbo				mPingPongFbo;

	ci::Vec2f				mMousePos;
	ci::Vec3f				mAttractor;
	uint32_t				insertIdx;

	bool					mStep;

	ci::Vec2f				mOscPos;

	VisionCamera*			mVisionCamera;

	//Kinect Surfaces
	ci::Surface16u						mDepthSurface;
	ci::Surface8u						mColorSurface;
	ci::gl::Texture							mImgTexture;
	ci::Vec3f*							mAttractors;
	float*								mAttractorSizes;
	int									mNumAttractors;
	uint16_t							mMaxAttractors;
	void								calculateAttractors();

	double								
		mMinContourSize,
		mMaxContourSize,
		mMinForce,
		mMaxFource;

	ci::osc::Listener					mListener;
	void								handleOscMessage(ci::osc::Message message);

};