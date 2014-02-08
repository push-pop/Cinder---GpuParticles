#pragma once

#include "cinder/app/AppBasic.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "Resources.h"
#include "PingPongFbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder\MayaCamUI.h"
#include "Kinect.h"

class GpuParticlesApp : public ci::app::AppBasic {
public:
	void setup();
	void mouseDown( ci::app::MouseEvent event );
	void mouseMove( ci::app::MouseEvent event );
	void keyDown( ci::app::KeyEvent event );
	void update();
	void draw();
	void prepareSettings( ci::app::AppBasic::Settings *settings );

	void drawFullScreenRect();

private:
	void				computeAttractorPosition();
	ci::Vec3f			computeAttractorPositionWithRay( ci::Vec2i pos );
	ci::MayaCamUI mMayaCam;
	// Frame buffer for ping-pong
	void					setupPingPongFbo();
	void					setupVbo();
	void					emitParticle( ci::Vec2f pos );
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

	//Kinect Members
	KinectSdk::KinectRef				mKinect;
	std::vector<KinectSdk::Skeleton>	mSkeletons;
	KinectSdk::DeviceOptions			mDeviceOptions;
	void								startKinect();

	//Kinect Surfaces
	ci::Surface16u						mDepthSurface;
	ci::Surface8u						mColorSurface;


	//Kinect Callbacks
	int32_t								mCallbackDepthId;
	int32_t								mCallbackSkeletonId;
	int32_t								mCallbackColorId;
	void								onDepthData( ci::Surface16u surface, const KinectSdk::DeviceOptions& deviceOptions );
	void								onSkeletonData( std::vector<KinectSdk::Skeleton> skeletons, const KinectSdk::DeviceOptions& deviceOptions );
	void								onColorData( ci::Surface8u surface, const KinectSdk::DeviceOptions& deviceOptions );



};