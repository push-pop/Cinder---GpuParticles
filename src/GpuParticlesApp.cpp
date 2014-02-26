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
	mMaxAttractors = 20;
	//Load Shaders
	try{
		mParticlesShader = gl::GlslProg(loadResource(PASSTHROUGH_VERT), loadResource(PARTICLES_FRAG));
		mDisplacementShader = gl::GlslProg(loadResource(VERTEXDISPLACEMENT_VERT), loadResource(VERTEXDISPLACEMENT_FRAG));
	}
	catch (gl::GlslProgCompileExc &exc) {
		std::cout << "Shader compile error: " << std::endl;
		std::cout << exc.what();
	}
	catch (Exception e) {
		std::cout << "Unable to load shader" << std::endl;
	}
	gl::clear();
	setupPingPongFbo();
	setupVbo();

	mAttractors = new ci::Vec3f[mMaxAttractors];
	mAttractorSizes = new float[mMaxAttractors];

	CameraPersp cam;
	cam.setEyePoint(Vec3f(5.0f, 10.0f, 10.0f));
	cam.setCenterOfInterestPoint(Vec3f(0.0f, 0.0f, 0.0f));
	cam.setPerspective(60.0f, getWindowAspectRatio(), 1.0f, 1000.0f);
	mMayaCam.setCurrentCam(cam);

	//mLeap = LeapMotion::Device::create();
	//mLeap->connectEventHandler(&GpuParticlesApp::onFrame, this);

	mVisionCamera = new VisionCamera();
	mVisionCamera->setup(getWindowWidth(), getWindowHeight());
	mVisionCamera->registerColorCallback();
	mVisionCamera->registerDepthCallback();


	if (settings::settings().loadConfig("settings.xml"))
	{
		mVisionCamera->getSettings();
	}
	else
	{
		mVisionCamera->registerSettings();
		settings::settings().saveConfig("settings.xml");
	}


	mListener.setup(3000);


	mImgTexture = ci::gl::Texture(loadImage(loadAsset("ski.jpg")));
}

void GpuParticlesApp::setupPingPongFbo()
{
	float scale = 8.0f;
	// TODO: Test with more than 2 texture attachments
	std::vector<Surface32f> surfaces;
	// Position 2D texture array
	surfaces.push_back(Surface32f(PARTICLES, PARTICLES, true));
	Surface32f::Iter pixelIter = surfaces[0].getIter();
	while (pixelIter.line()) {
		while (pixelIter.pixel()) {
			/* Initial particle positions are passed in as R,G,B
			float values. Alpha is used as particle invMass. */
			surfaces[0].setPixel(pixelIter.getPos(),
				ColorAf(scale*(Rand::randFloat() - 0.5f),
				scale*(Rand::randFloat() - 0.5f),
				scale*(Rand::randFloat() - 0.5f),
				Rand::randFloat(0.2f, 1.0f)));
		}
	}
	int i = 0;
	//Velocity 2D texture array
	surfaces.push_back(Surface32f(PARTICLES, PARTICLES, true));
	pixelIter = surfaces[1].getIter();
	while (pixelIter.line())
	while (pixelIter.pixel()) {
		/* Initial particle velocities are
		passed in as R,G,B float values. A is
		used for index inside texture*/
		surfaces[1].setPixel(pixelIter.getPos(),
			ColorAf(0, 0, 0, i));
		i++;
	}

	mPingPongFbo = PingPongFbo(surfaces);
}

void GpuParticlesApp::keyDown(ci::app::KeyEvent event)
{
	switch (event.getChar())
	{
	case 'f':
		setFullScreen(!isFullScreen());
		break;
	case 'd':
		mVisionCamera->setIsDrawDebug(!mVisionCamera->getIsDrawDebug());
		break;
	case '1':
		mVisionCamera->setLowCutOff(mVisionCamera->getLowCutOff() - 1.0f);
		break;
	case '4':
		mVisionCamera->setLowCutOff(mVisionCamera->getLowCutOff() + 1.0f);
		break;
	case '2':
		mVisionCamera->setHighCutOff(mVisionCamera->getHighCutOff() - 1.0f);
		break;
	case '5':
		mVisionCamera->setHighCutOff(mVisionCamera->getHighCutOff() + 1.0f);
		break;
	case 's':
		mVisionCamera->updateSettings();
		settings::settings().saveConfig("settings.xml");
		break;
	case 'q':
		quit();
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
	mVboMesh = gl::VboMesh(totalVertices, totalVertices, layout, GL_POINTS);
	for (int x = 0; x < PARTICLES; ++x) {
		for (int y = 0; y < PARTICLES; ++y) {
			indices.push_back(x * PARTICLES + y);
			texCoords.push_back(Vec2f(x / (float)PARTICLES, y / (float)PARTICLES));
		}
	}
	mVboMesh.bufferIndices(indices);
	mVboMesh.bufferTexCoords2d(0, texCoords);
}

void GpuParticlesApp::mouseDown(MouseEvent event)
{


}

void GpuParticlesApp::emitParticle(ci::Vec2f pos)
{
	// Set pos to insert
	newParticle = computeAttractorPositionWithRay(pos);
	particlesToAdd = true;
	insertIdx = (insertIdx + 1) % (PARTICLES*PARTICLES);

}

void GpuParticlesApp::computeAttractorPosition()
{
	// The attractor is positioned at the intersection of a ray
	// from the mouse to a plane perpendicular to the camera.
	float t = 0;
	Vec3f right, up;
	mMayaCam.getCamera().getBillboardVectors(&right, &up);
	CameraPersp cam = mMayaCam.getCamera();

	//float u = mMousePos.x / (float) getWindowWidth();
	//float v = mMousePos.y / (float) getWindowHeight();
	float u = mOscPos.x / (float)getWindowWidth();
	float v = mOscPos.y / (float)getWindowHeight();

	Ray ray = cam.generateRay(u, 1.0f - v, cam.getAspectRatio());
	if (ray.calcPlaneIntersection(Vec3f(0.0f, 0.0f, 0.0f), right.cross(up), &t)) {
		mAttractor.set(ray.calcPosition(t));
	}
}

ci::Vec3f GpuParticlesApp::computeAttractorPositionWithRay(ci::Vec2i pos)
{
	// The attractor is positioned at the intersection of a ray
	// from the mouse to a plane perpendicular to the camera.
	float t = 0;
	Vec3f right, up;
	mMayaCam.getCamera().getBillboardVectors(&right, &up);
	CameraPersp cam = mMayaCam.getCamera();

	float u = pos.x / (float)getWindowWidth();
	float v = pos.y / (float)getWindowHeight();
	ci::Vec3f ret;
	Ray ray = cam.generateRay(u, 1.0f - v, cam.getAspectRatio());
	if (ray.calcPlaneIntersection(Vec3f(0.0f, 0.0f, 0.0f), right.cross(up), &t)) {
		ret.set(ray.calcPosition(t));
		return ret;
	}
	return Vec3f(pos, 0.0f);

}

void GpuParticlesApp::mouseMove(MouseEvent event)
{
	mMousePos.set(event.getPos());
}


void GpuParticlesApp::update()
{
	//if (mVisionCamera == NULL) return;
	mVisionCamera->update();


	if (mListener.hasWaitingMessages())
	{
		osc::Message msg;
		mListener.getNextMessage(&msg);

		handleOscMessage(msg);
	}

	//emitParticle(ci::Vec2f(mMousePos));
	if (mStep)
	{
		//computeAttractorPosition();
		calculateAttractors();
		gl::setMatricesWindow(mPingPongFbo.getSize(), false); // false to prevent vertical flipping
		gl::setViewport(mPingPongFbo.getBounds());

		mPingPongFbo.bindUpdate();

		mParticlesShader.bind();
		mParticlesShader.uniform("positions", 0);
		mParticlesShader.uniform("velocities", 1);
		mParticlesShader.uniform("attractors", mAttractors, 20);
		mParticlesShader.uniform("numAttractors", mNumAttractors);
		mParticlesShader.uniform("newVert", (ci::Vec3f(newParticle)));
		if (particlesToAdd)
		{
			mParticlesShader.uniform("newVertIdx", (int)insertIdx);
			particlesToAdd = false;
		}
		else
		{
			mDisplacementShader.uniform("newVertIdx", (int)-1);
		}
		gl::drawSolidRect(mPingPongFbo.getBounds());
		mParticlesShader.unbind();

		mPingPongFbo.unbindUpdate();
	}
}

void GpuParticlesApp::calculateAttractors()
{

	mNumAttractors = 0;

	const ci::Vec2f* centroids = mVisionCamera->getCentroids();
	const std::vector<std::vector<ci::Vec2f> > contours = mVisionCamera->getContours();


	mNumAttractors = mVisionCamera->getNumCentroids();

	if (mNumAttractors > mMaxAttractors) mNumAttractors = mMaxAttractors;
	for (int i = 0; i < mNumAttractors; i++)
	{
		mAttractors[i] = computeAttractorPositionWithRay(centroids[i]);

	}
}

void GpuParticlesApp::draw()
{

	gl::enableAlphaBlending();
	//glEnable(GL_TEXTURE_2D);
	glPointSize(5.0f);
	gl::setMatrices(mMayaCam.getCamera());
	gl::setViewport(getWindowBounds());
	gl::clear(Color::black());
	glPointSize(1.0f);
	mPingPongFbo.bindTexture(0);
	mPingPongFbo.bindTexture(1);
	mImgTexture.bind(2);
	mDisplacementShader.bind();
	mDisplacementShader.uniform("displacementMap", 0);
	mDisplacementShader.uniform("velocityMap", 1);
	mDisplacementShader.uniform("imgTex", 2);
	gl::draw(mVboMesh);
	mDisplacementShader.unbind();
	mPingPongFbo.unbindTexture();
	mImgTexture.unbind();
	gl::color(ci::ColorA::white());
	gl::setMatricesWindow(getWindowSize());
	gl::drawString(toString(PARTICLES*PARTICLES) + " vertices", Vec2f(32.0f, 32.0f));
	gl::drawString(toString((int)getAverageFps()) + " fps", Vec2f(32.0f, 52.0f));
	gl::drawString(toString((int)mNumAttractors) + "Attractors", Vec2f(32.0f, 72.0f));
	//glDisable(GL_TEXTURE_2D);
	gl::disableAlphaBlending();

	ci::Surface16u dSurface = mVisionCamera->getDepthSurface();


	mVisionCamera->draw();
}

void GpuParticlesApp::prepareSettings(AppBasic::Settings *settings)
{
	settings->setWindowSize(1800, 1000);
	settings->setFrameRate(60.0f);
	settings->setResizable(false);
}

void GpuParticlesApp::handleOscMessage(osc::Message message)
{

	vector<string> substr = split(message.getAddress(), '/');
	substr.erase(substr.begin());
	if (strcmp(substr.front().c_str(), "x") == 0)
		mOscPos.x = atof(substr.back().c_str());
	else
		mOscPos.y = atof(substr.back().c_str());

}


CINDER_APP_BASIC(GpuParticlesApp, RendererGl)
