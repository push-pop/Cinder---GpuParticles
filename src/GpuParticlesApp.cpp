#include "GpuParticlesApp.h"
#include "cinder\Rand.h"
#include "cinder\Perlin.h"
#include "cinder\Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace KinectSdk;
#define PARTICLES 512


void GpuParticlesApp::setup()
{
	insertIdx = 0;
	particlesToAdd = false;
	//Load Shaders
	try{
		mParticlesShader = gl::GlslProg(loadResource(PASSTHROUGH_VERT), loadResource(PARTICLES_FRAG));
		mDisplacementShader = gl::GlslProg(loadResource(VERTEXDISPLACEMENT_VERT), loadResource(VERTEXDISPLACEMENT_FRAG));
	}
	catch( gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
	catch( Exception e ) {
		std::cout << "Unable to load shader" << std::endl;
	}
	gl::clear();
	setupPingPongFbo();
	setupVbo();

	CameraPersp cam;
	cam.setEyePoint( Vec3f(5.0f, 10.0f, 10.0f) );
	cam.setCenterOfInterestPoint( Vec3f(0.0f, 0.0f, 0.0f) );
	cam.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mMayaCam.setCurrentCam( cam );

	mKinect = KinectSdk::Kinect::create();
	startKinect();

	//Add Kinect Callbacks
	mCallbackDepthId	= mKinect->addDepthCallback( &GpuParticlesApp::onDepthData, this );
	mCallbackColorId	= mKinect->addColorCallback( &GpuParticlesApp::onColorData, this );
	mCallbackSkeletonId = mKinect->addSkeletonTrackingCallback( &GpuParticlesApp::onSkeletonData, this );
}

void GpuParticlesApp::setupPingPongFbo()
{
	float scale = 8.0f;
	// TODO: Test with more than 2 texture attachments
	std::vector<Surface32f> surfaces;
	// Position 2D texture array
	surfaces.push_back( Surface32f( PARTICLES, PARTICLES, true) );
	Surface32f::Iter pixelIter = surfaces[0].getIter();
	while( pixelIter.line() ) {
		while( pixelIter.pixel() ) {
			/* Initial particle positions are passed in as R,G,B
			float values. Alpha is used as particle invMass. */
			surfaces[0].setPixel(pixelIter.getPos(),
				ColorAf(scale*(Rand::randFloat()-0.5f),
				scale*(Rand::randFloat()-0.5f),
				scale*(Rand::randFloat()-0.5f),
				Rand::randFloat(0.2f, 1.0f) ) );
		}
	}
	int i = 0;
	//Velocity 2D texture array
	surfaces.push_back( Surface32f( PARTICLES, PARTICLES, true) );
	pixelIter = surfaces[1].getIter();
	while( pixelIter.line() )
		while( pixelIter.pixel() ) {
			/* Initial particle velocities are
			passed in as R,G,B float values. A is 
			used for index inside texture*/
			surfaces[1].setPixel( pixelIter.getPos(), 
				ColorAf(0,0,0,i) );
			i++;
		}

		mPingPongFbo = PingPongFbo( surfaces );
}

void GpuParticlesApp::keyDown( ci::app::KeyEvent event )
{
	switch(event.getChar())
	{
	case 'f':
		setFullScreen( !isFullScreen() );
		break;
	}
}
void GpuParticlesApp::setupVbo()
{
	/*  A dummy VboMesh the same size as the
	texture to keep the vertices on the GPU */
	int totalVertices = PARTICLES * PARTICLES;
	vector<Vec2f> texCoords;
	vector<uint32_t> indices;
	gl::VboMesh::Layout layout;
	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();
	//layout.setDynamicColorsRGBA();
	glPointSize(1.0f);
	mVboMesh = gl::VboMesh( totalVertices, totalVertices, layout, GL_POINTS);
	for( int x = 0; x < PARTICLES; ++x ) {
		for( int y = 0; y < PARTICLES; ++y ) {
			indices.push_back( x * PARTICLES + y );
			texCoords.push_back( Vec2f( x/(float)PARTICLES, y/(float)PARTICLES ) );
		}
	}
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
}

void GpuParticlesApp::mouseDown( MouseEvent event )
{


}

void GpuParticlesApp::emitParticle( ci::Vec2f pos )
{
	// Set pos to insert
	newParticle = computeAttractorPositionWithRay( pos );
	particlesToAdd = true;
	insertIdx = (insertIdx + 1) % (PARTICLES*PARTICLES);

}

void GpuParticlesApp::computeAttractorPosition( )
{
	// The attractor is positioned at the intersection of a ray
	// from the mouse to a plane perpendicular to the camera.
	float t = 0;
	Vec3f right, up;
	mMayaCam.getCamera().getBillboardVectors(&right, &up);
	CameraPersp cam = mMayaCam.getCamera();

	//float u = mMousePos.x / (float) getWindowWidth();
	//float v = mMousePos.y / (float) getWindowHeight();
	float u = 0.2f;
	float v = 0.2f;
	Ray ray = cam.generateRay(u , 1.0f - v, cam.getAspectRatio() );
	if (ray.calcPlaneIntersection(Vec3f(0.0f,0.0f,0.0f), right.cross(up), &t)) {
		mAttractor.set(ray.calcPosition(t));
	}
}

ci::Vec3f GpuParticlesApp::computeAttractorPositionWithRay( ci::Vec2i pos )
{
	// The attractor is positioned at the intersection of a ray
	// from the mouse to a plane perpendicular to the camera.
	float t = 0;
	Vec3f right, up;
	mMayaCam.getCamera().getBillboardVectors(&right, &up);
	CameraPersp cam = mMayaCam.getCamera();

	float u = pos.x / (float) getWindowWidth();
	float v = pos.y / (float) getWindowHeight();
	ci::Vec3f ret;
	Ray ray = cam.generateRay(u , 1.0f - v, cam.getAspectRatio() );
	if (ray.calcPlaneIntersection(Vec3f(0.0f,0.0f,0.0f), right.cross(up), &t)) {
		ret.set(ray.calcPosition(t));
		return ret;	
	}
	return Vec3f(pos, 0.0f);

}

void GpuParticlesApp::mouseMove( MouseEvent event )
{
	mMousePos.set(event.getPos());
}

void GpuParticlesApp::onDepthData( Surface16u surface, const KinectSdk::DeviceOptions& deviceOptions )
{
	mDepthSurface = surface;
}

void GpuParticlesApp::onSkeletonData( vector<Skeleton> skeletons, const DeviceOptions& deviceOptions )
{
	mSkeletons = skeletons;
}

void GpuParticlesApp::onColorData( Surface8u surface, const DeviceOptions& deviceOptions )
{
	mColorSurface = surface;
}


void GpuParticlesApp::update()
{
	emitParticle( ci::Vec2f(mMousePos));
	if(mStep)
	{
		computeAttractorPosition();

		gl::setMatricesWindow( mPingPongFbo.getSize(), false ); // false to prevent vertical flipping
		gl::setViewport( mPingPongFbo.getBounds() );

		mPingPongFbo.bindUpdate();

		mParticlesShader.bind();
		mParticlesShader.uniform( "positions", 0 );
		mParticlesShader.uniform( "velocities", 1 );
		mParticlesShader.uniform( "attractorPos", mAttractor);
		mParticlesShader.uniform("newVert", (ci::Vec3f(newParticle)));
		if(particlesToAdd)
		{
			mParticlesShader.uniform("newVertIdx", (int) insertIdx);
			particlesToAdd = false;
		}
		else 
		{
			mDisplacementShader.uniform("newVertIdx", (int) -1);
		}
		gl::drawSolidRect(mPingPongFbo.getBounds());
		mParticlesShader.unbind();

		mPingPongFbo.unbindUpdate();
	}
}

void GpuParticlesApp::draw()
{
	//gl::enableAlphaBlending();
	gl::setMatrices( mMayaCam.getCamera() );
	gl::setViewport( getWindowBounds() );
	gl::clear( Color::black() );
	glPointSize(1.0f);
	mPingPongFbo.bindTexture(0);
	mPingPongFbo.bindTexture(1);

	mDisplacementShader.bind();
	mDisplacementShader.uniform("displacementMap", 0 );
	mDisplacementShader.uniform("velocityMap", 1);

	gl::draw( mVboMesh );
	mDisplacementShader.unbind();
	mPingPongFbo.unbindTexture();

	gl::setMatricesWindow(getWindowSize());
	gl::drawString( toString( PARTICLES*PARTICLES ) + " vertices", Vec2f(32.0f, 32.0f));
	gl::drawString( toString((int) getAverageFps()) + " fps", Vec2f(32.0f, 52.0f));
	//gl::disableAlphaBlending();

}

void GpuParticlesApp::startKinect()
{
	// Configure device
	mDeviceOptions.enableColor();
	mDeviceOptions.enableDepth();
	mDeviceOptions.enableSkeletonTracking();
	mDeviceOptions.enableUserTracking();
	mKinect->start( mDeviceOptions );
	mKinect->enableUserColor(false);
	mKinect->enableBinaryMode();
	mKinect->removeBackground();

}

void GpuParticlesApp::prepareSettings( AppBasic::Settings *settings )
{
	settings->setWindowSize(800, 800);
	settings->setFrameRate(60.0f);
	settings->setResizable(false);
}

CINDER_APP_BASIC( GpuParticlesApp, RendererGl )
