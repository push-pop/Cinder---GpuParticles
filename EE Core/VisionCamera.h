// Kinect Includes
#include <vector>
#include "Kinect.h"
#include "CinderOpenCV.h"

#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/app/MouseEvent.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include "SettingsInterface.h"



class VisionCamera :public settings::SettingsInterface {
#define DEFAULT_CLAMP_MIN			0x00000000
#define DEFAULT_CLAMP_MAX			0x000019B7
public:

	VisionCamera();
	~VisionCamera();
	void	draw();
	void	update();	//updateDepthBuffer
	void	setup(int windowWidth, int windowHeight);	//initKinectSDK
	void	setWindowSize(int windowWidth, int windowHeight);
	void	mouseDown(ci::app::MouseEvent event);
	void	mouseDrag(ci::app::MouseEvent event);
	void	mouseUp(ci::app::MouseEvent event);

	void registerSettings();
	void updateSettings();
	void getSettings();

	void registerColorCallback();
	void registerSkeletonCallback();
	void registerDepthCallback();

	// Access the IsDrawDebug
	bool getIsDrawDebug(void) const			{ return(isDrawDebug_); };
	void setIsDrawDebug(bool isDrawDebug)	{ isDrawDebug_ = isDrawDebug; };

	// Access the LowCutOff
	float getLowCutOff(void) const		{ return(lowCutOff_); };
	void setLowCutOff(float lowCutOff)	{ if (lowCutOff <= 255.0f && lowCutOff >= 0.0f) lowCutOff_ = lowCutOff; };

	// Access the HightCutOff
	float getHighCutOff(void) const		{ return(highCutOff_); };
	void setHighCutOff(float highCutOff)	{ if (highCutOff <= 255.0f && highCutOff >= 0.0f) highCutOff_ = highCutOff; };

	// Access the SetPerspectiveQuad
	bool getModePerspectiveQuad(void) const				{ return(setPerspectiveQuad_); };
	void setModePerspectiveQuad(bool setPerspectiveQuad)	{ setPerspectiveQuad_ = setPerspectiveQuad; };

	// Access the ConvexHulls
	const std::vector<std::vector<ci::Vec2f> >& getConvexHulls(void) const			{ return(convexHulls_); };

	// Access the MinArea
	int getMinArea(void) const		{ return(minArea_); };
	void setMinArea(int minArea)	{ minArea_ = minArea; };

	// Access the SampleRate
	int getContourSampleRate(void) const		{ return(sampleRate_); };
	void setContourSampleRate(int sampleRate)	{ sampleRate_ = sampleRate; };

	// Access the Contours
	const std::vector<std::vector<ci::Vec2f> >& getContours(void) const		{ return(contours_); };

	static const int maxCentroids_ = 100;

	// Access the NumCentroids
	int getNumCentroids(void) const			{ return(numCentroids_); };

	// Access the Centroids_[maxCentroids_]
	const ci::Vec2f* getCentroids() { return centroids_; }

	// Access the NumErodeIter
	int getNumErodeIter(void) const			{ return(numErodeIter_); };
	void setNumErodeIter(int numErodeIter)	{ numErodeIter_ = numErodeIter; };

	// Access the NumDilateIter
	int getNumDilateIter(void) const			{ return(numDilateIter_); };
	void setNumDilateIter(int numDilateIter)	{ numDilateIter_ = numDilateIter; };

	// Access depths
	void setMinReliableDepth(USHORT depth) { mKinect->setMinReliableDepth(depth); minDepth_ = depth; }
	USHORT getMinReliableDepth() { return mKinect->getMinReliableDepth(); }

	void setMaxReliableDepth(USHORT depth) { mKinect->setMaxReliableDepth(depth); maxDepth_ = depth; }
	USHORT getMaxReliableDepth() { return mKinect->getMaxReliableDepth(); }

	// Access the IsClampUnreliableDepths
	bool getIsClampUnreliableDepths(void) const						{ return(isClampUnreliableDepths_); };
	void setIsClampUnreliableDepths(bool isClampUnreliableDepths)	{
		if (isClampUnreliableDepths != isClampUnreliableDepths_) {
			isClampUnreliableDepths_ = isClampUnreliableDepths;
			if (isClampUnreliableDepths_) {
				mKinect->setDepthTreatment(KinectSdk::Kinect::DepthTreatment::CLAMP_UNRELIABLE_DEPTHS);
			}
			else {
				mKinect->setDepthTreatment(KinectSdk::Kinect::DepthTreatment::DISPLAY_ALL_DEPTHS);
			}
		}
	}

	ci::Surface16u getDepthSurface() { return mDepthSurface; }
	ci::Surface8u getColorSurface() { return mColorSurface; }
	

	void	onColorData(ci::Surface8u surface, const KinectSdk::DeviceOptions& deviceOptions);
	void	onSkeletonData(std::vector<KinectSdk::Skeleton> skeletons, const KinectSdk::DeviceOptions& deviceOptions);
	void	onDepthData(ci::Surface16u surface, const KinectSdk::DeviceOptions& deviceOptions);
	
	ci::Surface16u	mDepthSurface;
	ci::Surface8u	mColorSurface;
	std::vector<KinectSdk::Skeleton> mSkeletons;

private:

	uint32_t mColorCallbackId;
	uint32_t mSkeletonCallbackId;
	uint32_t mDepthCallbackId;

	bool isClampUnreliableDepths_;
	bool isDrawDebug_;
	bool setPerspectiveQuad_;
	ci::Vec2f				mPoints_[4];
	ci::Vec2f				mImgPoints_[4];
	cv::Point2f perspSrc_[4];
	cv::Point2f perspDst_[4];
	int	mTrackedPoint_;
	int numErodeIter_;
	int numDilateIter_;
	float lowCutOff_;
	float highCutOff_;
	int minArea_;
	int sampleRate_;

	unsigned int restartCameraFrame_;
	std::vector<std::vector<cv::Point> > cvContours_;
	std::vector<cv::Moments> cvMoments_;
	std::vector<std::vector<cv::Point> > cvConvexHull_;

	//Ideally these two would be replaced with static implementations
	std::vector<std::vector<ci::Vec2f> > contours_;
	std::vector<std::vector<ci::Vec2f> > convexHulls_;

	int numCentroids_;
	ci::Vec2f centroids_[maxCentroids_];


	//init Kinect
	void								startKinect();

	// Kinect
	ci::Surface16u						mDepthSurfaceA_;
	ci::Surface16u						mDepthSurfaceB_;
	ci::Surface16u*						currDepthSurface_;
	ci::Surface16u*						workDepthSurface_;


	KinectSdk::DeviceOptions			mDeviceOptions;

	KinectSdk::KinectRef				mKinect;

	ci::Vec2i depthImageSize_;
	float windowWidthRatio_;
	float windowHeightRatio_;
	int windowWidth_;
	int windowHeight_;
	int depthSurfaceWidth_;
	int depthSurfaceHeight_;
	int minDepth_;
	int maxDepth_;

	cv::Mat					mCvMatContour_;
	cv::Mat					mCvMatErodeA_;
	cv::Mat					mCvMatDilateA_;
	cv::Mat					mCvMatErodeB_;
	cv::Mat					mCvMatDilateB_;
	cv::Mat*				currCvMatErode_;
	cv::Mat*				currCvMatDilate_;
	cv::Mat*				workCvMatErode_;
	cv::Mat*				workCvMatDilate_;
	cv::Mat					mCvMatThresholdA_;
	cv::Mat					mCvMatThresholdB_;
	cv::Mat*				currCvMatThreshold_;
	cv::Mat*				workCvMatThreshold_;
	cv::Mat					mCvMatFrameA_;
	cv::Mat					mCvMatFrameB_;
	cv::Mat*				currCvMatFrame_;
	cv::Mat*				workCvMatFrame_;

	bool isNewDepthImage_;
	std::mutex perspMutex_;
	std::mutex workerDoneMutex_;
	std::mutex workerUpdateMutex_;
	std::mutex updateFlagMutex_;
	std::thread* workerThread_;
	std::condition_variable isWorkerDone_;
	std::condition_variable isUpdateWorker_;
	int workerDoneCount_;
	int updateWorkerFlag_;
	bool isRunning_;
	void increaseWorkerDoneCount();

	int findNearestPt(const ci::Vec2f &aPt, float minDistance);
	void updateQuad();
	void runVision();
	void swapImagesAdvanceThread();
	cv::Mat calcPerspMatrix();
	void raiseUpdateFlag();
	void lowerUpdateFlag();
};