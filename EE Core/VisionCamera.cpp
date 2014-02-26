#include "VisionCamera.h"

VisionCamera::VisionCamera()
{
	isDrawDebug_ = false;
	updateWorkerFlag_ = 0;
	lowCutOff_ = 0.0f;
	highCutOff_ = 255.0f;
	maxDepth_ = DEFAULT_CLAMP_MAX;
	minDepth_ = DEFAULT_CLAMP_MIN;
	setPerspectiveQuad_ = false;
	mTrackedPoint_ = -1;
	depthSurfaceWidth_ = 320;
	depthSurfaceHeight_ = 240;
	windowWidthRatio_ = 1.0f;
	windowHeightRatio_ = 1.0f;
	numErodeIter_ = 3;
	numDilateIter_ = 1;
	minArea_ = 500;
	sampleRate_ = 10;
	restartCameraFrame_ = 0;
	isClampUnreliableDepths_ = false;
	cvContours_.clear();
	cvConvexHull_.clear();
	convexHulls_.clear();
	cvMoments_.clear();
	contours_.clear();

	workerDoneCount_ = 0;
	isRunning_ = false;
	workerThread_ = NULL;
	isNewDepthImage_ = false; 
	numCentroids_ = 0;
	currCvMatFrame_ = &mCvMatFrameA_;
	workCvMatFrame_ = &mCvMatFrameB_;
	currCvMatThreshold_ = &mCvMatThresholdA_;
	workCvMatThreshold_ = &mCvMatThresholdB_;
	currDepthSurface_ = &mDepthSurfaceA_;
	workDepthSurface_ = &mDepthSurfaceB_;
	currCvMatErode_ = &mCvMatErodeA_;
	currCvMatDilate_ = &mCvMatDilateA_;
	workCvMatErode_ = &mCvMatErodeB_;
	workCvMatDilate_ = &mCvMatDilateB_;
}

VisionCamera::~VisionCamera()
{	
	if(workerThread_) {
		isRunning_ = false;
		raiseUpdateFlag();
		workerThread_->join();
		delete workerThread_;
	}
	cvContours_.clear();
	cvConvexHull_.clear();
	convexHulls_.clear();
	cvMoments_.clear();
	contours_.clear();

	currCvMatThreshold_->release();
	workCvMatThreshold_->release();
	currCvMatErode_->release();
	workCvMatErode_->release();
	currCvMatDilate_->release();
	workCvMatDilate_->release();
	currCvMatFrame_->release();
	workCvMatFrame_->release();
	mCvMatContour_.release();

	// Stop Kinect input
	mKinect->stop();
}

void VisionCamera::setup(int windowWidth, int windowHeight)
{
	setWindowSize(windowWidth, windowHeight);
	// Start image capture
	mKinect = KinectSdk::Kinect::create();
	setMinReliableDepth(DEFAULT_CLAMP_MIN);
	setMaxReliableDepth(DEFAULT_CLAMP_MAX);
	startKinect();

	// Add callbacks

	mPoints_[0] = ci::Vec2f( 0.0f, 0.0f );
	mPoints_[1] = ci::Vec2f( (float)windowWidth, 0.0f );
	mPoints_[2] = ci::Vec2f((float)windowWidth, (float)windowHeight);
	mPoints_[3] = ci::Vec2f( 0.0f, (float)windowHeight );
	updateQuad();

	isRunning_ = true;
	//Make threads
	workerThread_ = new std::thread(&VisionCamera::runVision, this);
}

void VisionCamera::onColorData(ci::Surface8u surface, const KinectSdk::DeviceOptions& deviceOptions)
{
	mColorSurface = surface;
}

void VisionCamera::onSkeletonData(std::vector<KinectSdk::Skeleton> skeletons, const KinectSdk::DeviceOptions& deviceOptions)
{
	
}

void VisionCamera::onDepthData(ci::Surface16u surface, const KinectSdk::DeviceOptions& deviceOptions)
{
	mDepthSurface = surface;
}

void VisionCamera::registerColorCallback()
{
	mColorCallbackId = mKinect->addColorCallback(&VisionCamera::onColorData, this);
}

void VisionCamera::registerSkeletonCallback()
{
	mSkeletonCallbackId = mKinect->addSkeletonTrackingCallback(&VisionCamera::onSkeletonData, this);
}

void VisionCamera::registerDepthCallback()
{
	mDepthCallbackId = mKinect->addDepthCallback(&VisionCamera::onDepthData, this);
}

void VisionCamera::setWindowSize(int windowWidth, int windowHeight) {
	for(int i = 0; i < 4; i ++) {
		mPoints_[i].x /= windowWidth_;
		mPoints_[i].y /= windowHeight_;
	}

	windowHeight_ = windowHeight;
	windowWidth_ = windowWidth;
	windowWidthRatio_ = (float)windowWidth_/(float)depthSurfaceWidth_;
	windowHeightRatio_ = (float)windowHeight_/(float)depthSurfaceHeight_;

	for(int i = 0; i < 4; i ++) {
		mPoints_[i].x *= windowWidth_;
		mPoints_[i].y *= windowHeight_;
	}
}

void VisionCamera::registerSettings() {
	settings::SettingsInterface::registerVariable(lowCutOff_, "Theshold_LowCutoff", "VisionCamera");
	settings::SettingsInterface::registerVariable(highCutOff_, "Theshold_HighCutoff", "VisionCamera");
	settings::SettingsInterface::registerVariable(sampleRate_, "Contour_SampleRate", "VisionCamera");
	settings::SettingsInterface::registerVariable(minArea_, "Contour_MinArea", "VisionCamera");
	settings::SettingsInterface::registerVariable(numErodeIter_, "Erode_NumIterations", "VisionCamera");
	settings::SettingsInterface::registerVariable(numDilateIter_, "Dilate_NumIterations", "VisionCamera");
	settings::SettingsInterface::registerVariable((float)(mPoints_[0].x/windowWidth_), "PerspectiveQuad_0_x", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[0].y/windowHeight_), "PerspectiveQuad_0_y", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[1].x/windowWidth_), "PerspectiveQuad_1_x", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[1].y/windowHeight_), "PerspectiveQuad_1_y", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[2].x/windowWidth_), "PerspectiveQuad_2_x", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[2].y/windowHeight_), "PerspectiveQuad_2_y", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[3].x/windowWidth_), "PerspectiveQuad_3_x", "VisionCamera");
	settings::SettingsInterface::registerVariable(((float)mPoints_[3].y/windowHeight_), "PerspectiveQuad_3_y", "VisionCamera");
	settings::SettingsInterface::registerVariable(minDepth_, "Initial_Min_Depth", "VisionCamera");
	settings::SettingsInterface::registerVariable(maxDepth_, "Initial_Max_Depth", "VisionCamera");
	settings::SettingsInterface::registerVariable(isClampUnreliableDepths_, "clampUnreliableDepths", "VisionCamera");
}

void VisionCamera::updateSettings(){
	settings::SettingsInterface::updateVal(lowCutOff_, "Theshold_LowCutoff", "VisionCamera");
	settings::SettingsInterface::updateVal(highCutOff_, "Theshold_HighCutoff", "VisionCamera");
	settings::SettingsInterface::updateVal(sampleRate_, "Contour_SampleRate", "VisionCamera");
	settings::SettingsInterface::updateVal(minArea_, "Contour_MinArea", "VisionCamera");
	settings::SettingsInterface::updateVal(numErodeIter_, "Erode_NumIterations", "VisionCamera");
	settings::SettingsInterface::updateVal(numDilateIter_, "Dilate_NumIterations", "VisionCamera");
	settings::SettingsInterface::updateVal((float)(mPoints_[0].x/windowWidth_), "PerspectiveQuad_0_x", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[0].y/windowHeight_), "PerspectiveQuad_0_y", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[1].x/windowWidth_), "PerspectiveQuad_1_x", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[1].y/windowHeight_), "PerspectiveQuad_1_y", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[2].x/windowWidth_), "PerspectiveQuad_2_x", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[2].y/windowHeight_), "PerspectiveQuad_2_y", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[3].x/windowWidth_), "PerspectiveQuad_3_x", "VisionCamera");
	settings::SettingsInterface::updateVal(((float)mPoints_[3].y/windowHeight_), "PerspectiveQuad_3_y", "VisionCamera");
	settings::SettingsInterface::updateVal(minDepth_, "Initial_Min_Depth", "VisionCamera");
	settings::SettingsInterface::updateVal(maxDepth_, "Initial_Max_Depth", "VisionCamera");
	settings::SettingsInterface::updateVal(isClampUnreliableDepths_, "clampUnreliableDepths", "VisionCamera");
}

void VisionCamera::getSettings(){
	settings::SettingsInterface::getVal(lowCutOff_, "Theshold_LowCutoff", "VisionCamera");
	settings::SettingsInterface::getVal(highCutOff_, "Theshold_HighCutoff", "VisionCamera");
	settings::SettingsInterface::getVal(sampleRate_, "Contour_SampleRate", "VisionCamera");
	settings::SettingsInterface::getVal(minArea_, "Contour_MinArea", "VisionCamera");
	settings::SettingsInterface::getVal(numErodeIter_, "Erode_NumIterations", "VisionCamera");
	settings::SettingsInterface::getVal(numDilateIter_, "Dilate_NumIterations", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[0].x, "PerspectiveQuad_0_x", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[0].y, "PerspectiveQuad_0_y", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[1].x, "PerspectiveQuad_1_x", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[1].y, "PerspectiveQuad_1_y", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[2].x, "PerspectiveQuad_2_x", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[2].y, "PerspectiveQuad_2_y", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[3].x, "PerspectiveQuad_3_x", "VisionCamera");
	settings::SettingsInterface::getVal(mPoints_[3].y, "PerspectiveQuad_3_y", "VisionCamera");
	settings::SettingsInterface::getVal(minDepth_, "Initial_Min_Depth", "VisionCamera");
	settings::SettingsInterface::getVal(maxDepth_, "Initial_Max_Depth", "VisionCamera");
	bool isClampUnreliableDepths = isClampUnreliableDepths_;
	settings::SettingsInterface::registerVariable(isClampUnreliableDepths, "clampUnreliableDepths", "VisionCamera");
	for(int i = 0; i < 4; i++) {
		mPoints_[i].x *= windowWidth_;
		mPoints_[i].y *= windowHeight_;
	}
	updateQuad();
	setMinReliableDepth(minDepth_);
	setMaxReliableDepth(maxDepth_);
	setIsClampUnreliableDepths(isClampUnreliableDepths);
}

void VisionCamera::startKinect()
{
	// Update device count
	//mDeviceCount = Kinect::getDeviceCount();

	// Configure device
	mDeviceOptions.enableDepth( true );
	mDeviceOptions.enableNearMode( false );
	mDeviceOptions.enableSkeletonTracking( false, false );
	mDeviceOptions.enableColor(true);
	mDeviceOptions.enableUserTracking(false);
	if(isClampUnreliableDepths_) {
		mKinect->setDepthTreatment(KinectSdk::Kinect::DepthTreatment::CLAMP_UNRELIABLE_DEPTHS);
	} else {
		mKinect->setDepthTreatment(KinectSdk::Kinect::DepthTreatment::DISPLAY_ALL_DEPTHS);
	}
	mKinect->setMinReliableDepth(minDepth_);
	mKinect->setMaxReliableDepth(maxDepth_);
	mKinect->enableUserColor( false );
	mKinect->enableBinaryMode( false );
	mKinect->removeBackground( false );
	mKinect->setFlipped( false );


	// Stop, if capturing
	if ( mKinect->isCapturing() ) {
		mKinect->stop();
	}

	// Start Kinect
	mKinect->start(  );

	// Get device angle angle
	//mTilt = mKinect->getTilt();
	//mTiltPrev = mTilt;


}

void VisionCamera::update()
{
	if(!isRunning_) return;

	// Update Kinect
	if ( mKinect->isCapturing() ) {
		mKinect->update();
		if(isNewDepthImage_) {
			std::unique_lock<std::mutex> workerLock( workerDoneMutex_);
			isWorkerDone_.wait(workerLock, [&]{return workerDoneCount_ >0;});
			workerDoneCount_ = 0;
			isNewDepthImage_ = false;		 
			swapImagesAdvanceThread();		

			cvContours_.clear();
			cvConvexHull_.clear();
			convexHulls_.clear();
			cvMoments_.clear();
			contours_.clear();
			numCentroids_ = 0;
			if ( cv::countNonZero( *currCvMatDilate_) > 0 ) {
				currCvMatDilate_->copyTo(mCvMatContour_);
				cv::findContours( mCvMatContour_, cvContours_, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE );
				cvConvexHull_.resize(cvContours_.size());
				convexHulls_.resize(cvContours_.size());
				cvMoments_.resize(cvContours_.size());
				contours_.resize(cvContours_.size());
				ci::Vec2f point;
				for(unsigned int i = 0; i < cvContours_.size(); i++) {
					cvMoments_[i] = cv::moments(cvContours_[i]);
					if(cvMoments_[i].m00 > minArea_) {
						cv::convexHull(cvContours_[i], cvConvexHull_[i]);
						for(unsigned int j = 0; j < cvContours_[i].size(); j+= sampleRate_) {
							point.x =((float)cvContours_[i][j].x)*windowWidthRatio_;
							point.y = ((float)cvContours_[i][j].y)*windowHeightRatio_;
							contours_[i].push_back(point);
						}
					}
				}
				for(unsigned int i = 0; i < cvConvexHull_.size(); i++) {
					point.x =  ((float)cvMoments_[i].m10/(float)cvMoments_[i].m00)*windowWidthRatio_;
					point.y = ((float)cvMoments_[i].m01/(float)cvMoments_[i].m00)*windowHeightRatio_;
					if(numCentroids_ < maxCentroids_) centroids_[numCentroids_++] = point;
					for(unsigned int j = 0; j < cvConvexHull_[i].size(); j++) {
						point.x = ((float)cvConvexHull_[i][j].x)*windowWidthRatio_;
						point.y = ((float)cvConvexHull_[i][j].y)*windowHeightRatio_;
						convexHulls_[i].push_back(point);
					}
				}
			} 
		} 
	}else {
		// If Kinect initialization failed, try again every 90 frames
		restartCameraFrame_++;
		if ( restartCameraFrame_  >= 90) {
			mKinect->start();
			restartCameraFrame_ = 0;
		}

	}
}

void VisionCamera::swapImagesAdvanceThread() {
	cv::Mat* temp = currCvMatFrame_;
	currCvMatFrame_ = workCvMatFrame_;
	workCvMatFrame_ = temp;
	temp = currCvMatThreshold_;
	currCvMatThreshold_ = workCvMatThreshold_;
	workCvMatThreshold_ = temp;
	temp = currCvMatDilate_;
	currCvMatDilate_ = workCvMatDilate_;
	workCvMatDilate_ = temp;
	temp = currCvMatErode_;
	currCvMatErode_ = workCvMatErode_;
	workCvMatErode_ = temp;

	ci::Surface16u*	tempSurface = currDepthSurface_;
	currDepthSurface_ = workDepthSurface_;
	workDepthSurface_ = tempSurface;

	raiseUpdateFlag();
}

void VisionCamera::draw() {
	if(!isRunning_ ) return;
	if(setPerspectiveQuad_  && *currDepthSurface_) {
		ci::gl::color( ci::Colorf::white() );
		ci::Area srcArea( 0, 0, currDepthSurface_->getWidth(), currDepthSurface_->getHeight() );
		ci::Rectf destRect(0.0f, 0.0f, (float)(windowWidth_ - 1), (float)(windowHeight_ - 1));
		ci::gl::draw( ci::gl::Texture( *currDepthSurface_ ), srcArea, destRect );

		glColor3f( 1.0f, 0.5f, 0.25f );
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glVertex2f(mPoints_[0].x, mPoints_[0].y);
		glVertex2f(mPoints_[1].x, mPoints_[1].y);
		glVertex2f(mPoints_[1].x, mPoints_[1].y);
		glVertex2f(mPoints_[2].x, mPoints_[2].y);
		glVertex2f(mPoints_[2].x, mPoints_[2].y);
		glVertex2f(mPoints_[3].x, mPoints_[3].y);
		glVertex2f(mPoints_[3].x, mPoints_[3].y);
		glVertex2f(mPoints_[0].x, mPoints_[0].y);
		glVertex2f(mPoints_[0].x, mPoints_[0].y);
		glVertex2f(mPoints_[2].x, mPoints_[2].y);
		glVertex2f(mPoints_[3].x, mPoints_[3].y);
		glVertex2f(mPoints_[1].x, mPoints_[1].y);
		glEnd();

		for( int i = 0; i < 4; ++i ) {
			switch(i) {
			case 0:
				glColor4f( 1.0f, 0.0f, 0.0f, 0.5f );
				break;
			case 1:
				glColor4f( 0.0f, 1.0f, 0.0f, 0.5f );
				break;
			case 2:
				glColor4f( 0.0f, 1.0f, 0.0f, 0.5f );
				break;
			case 3:
				glColor4f( 0.0f, 0.0f, 1.0f, 0.5f );
				break;
			}
			ci::gl::drawSolidCircle( mPoints_[i], 4.0f );
		}
	}

	if(isDrawDebug_) {
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		//	for( size_t idx = 0; idx < cvContours_.size(); idx++){
		//		cv::drawContours( mCvMatFrame_, cvContours_, idx, cv::Scalar(255, 255, 255));
		//	}

		ci::Area srcArea( 0, 0, depthSurfaceWidth_, depthSurfaceHeight_ );
		ci::Rectf destRect( 265.0f, 15.0f, 505.0f, 195.0f );
		ci::gl::draw( ci::gl::Texture( ci::fromOcv(*currCvMatFrame_)), srcArea, destRect );
		destRect.set(265.0f, 196.0f, 505.0f, 376.0f);
		ci::gl::draw( ci::gl::Texture( ci::fromOcv(*currCvMatThreshold_)), srcArea, destRect );
		destRect.set(265.0f, 377.0f, 505.0f, 557.0f);
		ci::gl::draw( ci::gl::Texture( ci::fromOcv(*currCvMatErode_)), srcArea, destRect );
		destRect.set(265.0f,558.0f, 505.0f, 738.0f);
		ci::gl::draw( ci::gl::Texture( ci::fromOcv(*currCvMatDilate_)), srcArea, destRect );
	}
}

void VisionCamera::updateQuad() {
	std::lock_guard<std::mutex> lock(perspMutex_);
	//Convert mPoints
	for( int i = 0; i < 4; ++i ) {
		mImgPoints_[i].x = (mPoints_[i].x/windowWidth_)*depthSurfaceWidth_;
		mImgPoints_[i].y = (mPoints_[i].y/windowHeight_)*depthSurfaceHeight_;
		perspSrc_[i] = toOcv( mImgPoints_[i] );
	}
}

void VisionCamera::mouseDown( ci::app::MouseEvent event )
{
	mTrackedPoint_ = findNearestPt( event.getPos(), 50 );
}

void VisionCamera::mouseDrag( ci::app::MouseEvent event )
{
	if( mTrackedPoint_ >= 0 ) {
		mPoints_[mTrackedPoint_] = event.getPos();
		updateQuad();
	}
}

void VisionCamera::mouseUp( ci::app::MouseEvent event )
{
	mTrackedPoint_ = -1;
}

int VisionCamera::findNearestPt( const ci::Vec2f &aPt, float minDistance )
{
	int result = -1;
	float nearestDist;
	for( size_t i = 0; i < 4; ++i ) {
		float dist = mPoints_[i].distance( aPt );
		if( dist < minDistance ) {
			if( ( result == -1 ) || ( dist < nearestDist ) ) {
				result = i;
				nearestDist = dist;
			}
		}
	}

	return result;
}

cv::Mat VisionCamera::calcPerspMatrix() {
	std::lock_guard<std::mutex> lock(perspMutex_);
	return cv::getPerspectiveTransform( perspSrc_, perspDst_ );
}

void VisionCamera::runVision() {
	while(isRunning_) {
		if( mKinect->isCapturing()  && mKinect->updateDepth(workDepthSurface_)) {
			depthSurfaceWidth_ = workDepthSurface_->getWidth();
			depthSurfaceHeight_ = workDepthSurface_->getHeight();

			perspDst_[0] = cv::Point2f( 0.0f, 0.0f );
			perspDst_[1] = cv::Point2f((float)depthSurfaceWidth_, 0.0f );
			perspDst_[2] = cv::Point2f( (float)depthSurfaceWidth_,(float)workDepthSurface_->getHeight() );
			perspDst_[3] = cv::Point2f( 0.0f, (float)workDepthSurface_->getHeight() );
			depthImageSize_ =  ci::Vec2i( workDepthSurface_->getWidth(), workDepthSurface_->getHeight());

			*workCvMatFrame_ = ci::toOcv((ci::Channel8u)workDepthSurface_->getChannelRed());

			//Create corrected depth image
			cv::Mat warpMatrix = calcPerspMatrix() ;
			cv::warpPerspective( *workCvMatFrame_,*workCvMatFrame_, warpMatrix, toOcv( depthImageSize_ ), cv::INTER_CUBIC );

			// Threshold the image
			// thresh to zero first
			cv::threshold(*workCvMatFrame_, *workCvMatThreshold_, highCutOff_, 255.0, CV_THRESH_TOZERO_INV);
			cv::threshold( *workCvMatThreshold_, *workCvMatThreshold_, lowCutOff_, 255.0, CV_THRESH_BINARY );
			//erode
			cv::erode(*workCvMatThreshold_,  *workCvMatErode_, cv::Mat(), cv::Point(-1,-1), numErodeIter_);
			//dilate
			cv::dilate(*workCvMatErode_,  *workCvMatDilate_, cv::Mat(), cv::Point(-1,-1), numDilateIter_);
			isNewDepthImage_ = true; 
			std::unique_lock<std::mutex> threadLock( workerUpdateMutex_);
			increaseWorkerDoneCount();
			isUpdateWorker_.wait(threadLock, [&]{return updateWorkerFlag_ >0;});
			lowerUpdateFlag();
		}		
	} 
}

void VisionCamera::increaseWorkerDoneCount() {
	std::lock_guard<std::mutex> lock(workerDoneMutex_);
	workerDoneCount_++;
	isWorkerDone_.notify_one();
}

void VisionCamera::raiseUpdateFlag() {
	std::lock_guard<std::mutex> lock( workerUpdateMutex_);
	std::lock_guard<std::mutex> flagLock( updateFlagMutex_);
	updateWorkerFlag_ = 1;
	isUpdateWorker_.notify_one();
}

void VisionCamera::lowerUpdateFlag() {
	std::lock_guard<std::mutex> flagLock( updateFlagMutex_);
	updateWorkerFlag_ = 0;
}