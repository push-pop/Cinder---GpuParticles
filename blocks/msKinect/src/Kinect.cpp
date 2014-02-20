/*
* 
* Copyright (c) 2013, Ban the Rewind
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#include "Kinect.h"

#include "cinder/app/App.h"
#include "cinder/Utilities.h"	

#include <comutil.h>

namespace KinectSdk
{

using namespace ci;
using namespace ci::app;
using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////
#define BYTES_PER_PIXEL_DEPTH       sizeof(NUI_DEPTH_IMAGE_PIXEL)
#define BYTES_PER_PIXEL_RGB         4
#define BYTES_PER_PIXEL_INFRARED    2
#define BYTES_PER_PIXEL_BAYER       1

#define COLOR_INDEX_BLUE            0
#define COLOR_INDEX_GREEN           1
#define COLOR_INDEX_RED             2
#define COLOR_INDEX_ALPHA           3

#define MIN_DEPTH                   400
#define MAX_DEPTH                   16383
#define UNKNOWN_DEPTH               0
#define UNKNOWN_DEPTH_COLOR         0x003F3F07
#define TOO_NEAR_COLOR              0x001F7FFF
#define TOO_FAR_COLOR               0x007F0F3F
#define NEAREST_COLOR               0x00FFFFFF


	// intensity shift table to generate different render colors for different tracked players
	const BYTE Kinect::mIntensityShiftR[] = {0, 2, 0, 2, 0, 0, 2};
	const BYTE Kinect::mIntensityShiftG[] = {0, 2, 2, 0, 2, 0, 0};
	const BYTE Kinect::mIntensityShiftB[] = {0, 0, 2, 2, 0, 2, 0};

const double kTiltRequestInterval = 1.5;

Matrix44f toMatrix44f( const Matrix4& matrix ) 
{
	return Matrix44f( Vec4f( matrix.M11, matrix.M12, matrix.M13, matrix.M14 ), 
		Vec4f( matrix.M21, matrix.M22, matrix.M23, matrix.M24 ), 
		Vec4f( matrix.M31, matrix.M32, matrix.M33, matrix.M34 ), 
		Vec4f( matrix.M41, matrix.M42, matrix.M43, matrix.M44 ) );
}
Quatf toQuatf( const Vector4& vec ) 
{
	return Quatf( vec.w, vec.x, vec.y, vec.z );
}
Vec3f toVec3f( const Vector4& vec ) 
{
	return Vec3f( vec.x, vec.y, vec.z );
}

void CALLBACK deviceStatus( long hr, const WCHAR *instanceName, const WCHAR *deviceId, void *data )
{
	Kinect* kinect = reinterpret_cast<Kinect*>( data );
	if ( SUCCEEDED( hr ) ) {
		kinect->start( kinect->getDeviceOptions() );
	} else {
		kinect->error( hr );
		reinterpret_cast<Kinect*>( data )->stop();
	}
}

const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformNone			= { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformDefault		= { 0.5f, 0.5f, 0.5f, 0.05f, 0.04f };
const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformSmooth		= { 0.5f, 0.1f, 0.5f, 0.1f, 0.1f };
const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformVerySmooth	= { 0.7f, 0.3f, 1.0f, 1.0f, 1.0f };
const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformMax			= { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
const NUI_TRANSFORM_SMOOTH_PARAMETERS	kTransformParams[ 5 ]	= 
{ kTransformNone, kTransformDefault, kTransformSmooth, kTransformVerySmooth, kTransformMax };

//////////////////////////////////////////////////////////////////////////////////////////////

Bone::Bone( const Vector4& position, const _NUI_SKELETON_BONE_ORIENTATION& bone )
{
	mAbsRotQuat	= toQuatf( bone.absoluteRotation.rotationQuaternion );
	mAbsRotMat	= toMatrix44f( bone.absoluteRotation.rotationMatrix );
	mJointEnd	= bone.endJoint;
	mJointStart	= bone.startJoint;
	mPosition	= toVec3f( position );
	mRotQuat	= toQuatf( bone.hierarchicalRotation.rotationQuaternion );
	mRotMat		= toMatrix44f( bone.hierarchicalRotation.rotationMatrix );
}

const Quatf& Bone::getAbsoluteRotation() const 
{ 
	return mAbsRotQuat; 
}
const Matrix44f& Bone::getAbsoluteRotationMatrix() const 
{ 
	return mAbsRotMat; 
}
JointName Bone::getEndJoint() const
{
	return mJointEnd;
}
const Vec3f& Bone::getPosition() const 
{ 
	return mPosition; 
}
const Quatf& Bone::getRotation() const 
{ 
	return mRotQuat; 
}
const Matrix44f& Bone::getRotationMatrix() const 
{ 
	return mRotMat; 
}
JointName Bone::getStartJoint() const
{
	return mJointStart;
}

//////////////////////////////////////////////////////////////////////////////////////////////

DeviceOptions::DeviceOptions()
{
	mDeviceId					= "";
	mDeviceIndex				= 0;
	mEnabledDepth				= true;
	mEnabledNearMode			= false;
	mEnabledSeatedMode			= false;
	mEnabledSkeletonTracking	= true;
	mEnabledUserTracking		= true;
	mEnabledColor				= true;

	setDepthResolution( ImageResolution::NUI_IMAGE_RESOLUTION_320x240 );
	setColorResolution( ImageResolution::NUI_IMAGE_RESOLUTION_640x480 );
}

DeviceOptions& DeviceOptions::enableDepth( bool enable )
{
	mEnabledDepth = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableNearMode( bool enable )
{
	mEnabledNearMode = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableSkeletonTracking( bool enable, bool seatedMode )
{
	mEnabledSeatedMode			= seatedMode;
	mEnabledSkeletonTracking	= enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableUserTracking( bool enable )
{
	mEnabledUserTracking = enable;
	return *this;
}

DeviceOptions& DeviceOptions::enableColor( bool enable )
{
	mEnabledColor = enable;
	return *this;
}

ImageResolution DeviceOptions::getDepthResolution() const
{
	return mDepthResolution;
}
	
const Vec2i& DeviceOptions::getDepthSize() const
{
	return mDepthSize;
}

const string& DeviceOptions::getDeviceId() const
{
	return mDeviceId;
}
	
int32_t DeviceOptions::getDeviceIndex() const
{
	return mDeviceIndex;
}
	
ImageResolution DeviceOptions::getColorResolution() const
{
	return mColorResolution;
}

const Vec2i& DeviceOptions::getColorSize() const 
{
	return mColorSize;
}

bool DeviceOptions::isDepthEnabled() const
{
	return mEnabledDepth;
}
	
bool DeviceOptions::isNearModeEnabled() const
{
	return mEnabledNearMode;
}
	
bool DeviceOptions::isSeatedModeEnabled() const
{
	return mEnabledSeatedMode;
}

bool DeviceOptions::isSkeletonTrackingEnabled() const
{
	return mEnabledSkeletonTracking;
}
	
bool DeviceOptions::isUserTrackingEnabled() const
{
	return mEnabledUserTracking;
}

bool DeviceOptions::isColorEnabled() const
{
	return mEnabledColor;
}

DeviceOptions& DeviceOptions::setDepthResolution( const ImageResolution& resolution )
{
	mDepthResolution = resolution;
	switch ( mDepthResolution ) {
	case ImageResolution::NUI_IMAGE_RESOLUTION_640x480:
		mDepthSize					= Vec2i( 640, 480 );
		mEnabledUserTracking		= false;
		mEnabledSkeletonTracking	= false;
		mEnabledSeatedMode			= false;
		break;
	case ImageResolution::NUI_IMAGE_RESOLUTION_320x240:
		mDepthSize = Vec2i( 320, 240 );
		break;
	case ImageResolution::NUI_IMAGE_RESOLUTION_80x60:
		mDepthSize = Vec2i( 80, 60 );
		break;
	default:
		mDepthResolution = NUI_IMAGE_RESOLUTION_INVALID;
		mDepthSize = Vec2i::zero();
		mEnabledDepth = false;
		break;
	}
	return *this;
}

DeviceOptions& DeviceOptions::setDeviceId( const std::string& id )
{
	mDeviceId = id;
	return *this;
}
	
DeviceOptions& DeviceOptions::setDeviceIndex( int32_t index )
{
	mDeviceIndex = index;
	return *this;
}
	
DeviceOptions& DeviceOptions::setColorResolution( const ImageResolution& resolution )
{
	mColorResolution = resolution;
	switch ( mColorResolution ) {
	case ImageResolution::NUI_IMAGE_RESOLUTION_1280x960:
		mColorSize = Vec2i( 1280, 960 );
		break;
	case ImageResolution::NUI_IMAGE_RESOLUTION_640x480:
		mColorSize = Vec2i( 640, 480 );
		break;
	default:
		mColorResolution = NUI_IMAGE_RESOLUTION_INVALID;
		mColorSize = Vec2i::zero();
		mEnabledColor = false;
		break;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////

vector<Colorf> Kinect::sUserColors = getUserColors();

ci::Colorf Kinect::getUserColor( uint32_t id ) 
{ 
	return sUserColors.at( ci::math<uint32_t>::clamp( id, 0, 5 ) ); 
}

vector<Colorf> Kinect::getUserColors()
{
	if ( sUserColors.size() == NUI_SKELETON_COUNT ) {
		return sUserColors;
	}

	vector<Colorf> colors;
	colors.push_back( Colorf( 0.0f, 1.0f, 1.0f ) );
	colors.push_back( Colorf( 0.0f, 0.0f, 1.0f ) );
	colors.push_back( Colorf( 0.0f, 1.0f, 0.0f ) );
	colors.push_back( Colorf( 0.0f, 0.5f, 1.0f ) );
	colors.push_back( Colorf( 0.0f, 1.0f, 0.5f ) );
	colors.push_back( Colorf( 0.0f, 0.5f, 0.5f ) );
	return colors;
}

KinectRef Kinect::create()
{
	return KinectRef( new Kinect( ) );
}

//////////////////////////////////////////////////////////////////////////////////////////////

Kinect::Kinect()
{
	mColorSurface = &mColorSurfaceA_;
	currColorSurface = &mColorSurfaceB_;
	mSkeletons = &mSkeletonsA_;
	currSkeletons = &mSkeletonsB_;
	NuiSetDeviceStatusCallback( &KinectSdk::deviceStatus, this );
	init();
	for ( int32_t i = 0; i < NUI_SKELETON_COUNT; ++i ) {
		(*mSkeletons).push_back( Skeleton() );
		(*currSkeletons).push_back( Skeleton() );
	}
}

Kinect::~Kinect()
{
	stop();

	for ( CallbackList::iterator callbackIt = mCallbacks.begin(); callbackIt != mCallbacks.end(); ++callbackIt ) {
		if ( callbackIt->second->connected() ) {
			callbackIt->second->disconnect();
		}
	}
	mCallbacks.clear();

	if ( mSensor != 0 ) {
		mSensor->NuiShutdown();
		if ( mSensor ) {
			mSensor->Release();
			mSensor = 0;
			mDepthStreamHandle = 0;
			mColorStreamHandle = 0;
		}
	}
}

void Kinect::deactivateUsers()
{
	for ( uint32_t i = 0; i < NUI_SKELETON_COUNT; ++i ) {
		mActiveUsers[ i ] = false;
	}
}

void Kinect::enableBinaryMode( bool enable, bool invertImage )
{
	mBinary = enable;
	mInverted = invertImage;
}

void Kinect::enableUserColor( bool enable )
{
	mGreyScale = !enable;
}

void Kinect::enableVerbose( bool enable )
{
	mVerbose = enable;
}

void Kinect::error( long hr ) {
	if ( !mVerbose ) {
		return;
	}
	switch ( hr ) {
	case E_POINTER:
		trace( "Bad pointer." );
		break;
	case E_INVALIDARG:
		trace( "Invalid argument." );
		break;
	case E_NUI_DEVICE_NOT_READY:
		trace( "Device not ready." );
		break;
	case E_NUI_FEATURE_NOT_INITIALIZED:
		trace( "Feature not initialized." );
		break;
	case E_NUI_NOTCONNECTED:
		trace( "Unable to connect to device." );
		break;
	case E_FAIL:
		trace( "Attempt failed." );
		break;
	case E_NUI_IMAGE_STREAM_IN_USE:
		trace( "Image stream already in use." );
		break;
	case E_NUI_FRAME_NO_DATA:
		trace( "No frame data available" );
		break;
	case E_OUTOFMEMORY:
		trace( "Out of memory (maximum number of Kinect devices may have been reached)." );
		break;
	case ERROR_TOO_MANY_CMDS:
		trace( "Too many commands sent. Angle change requests must be made at least 1.5s apart." );
		break;
	case ERROR_RETRY:
		trace( "Device is busy.  Retry in a moment." );
		break;
	case S_FALSE:
		trace( "Data not available." );
	case S_OK:
		break;
	default:
		trace( "Unknown error (Code " + toString( hr ) + ")" );
	}
}

float Kinect::getDepthAt( const ci::Vec2i& pos ) const
{
	float depthNorm		= 0.0f;
	if ( mDepthSurface ) {
		uint16_t depth	= 0x10000 - mDepthSurface.getPixel( pos ).r;
		depth			= depth << 2;
		depthNorm		= 1.0f - (float)depth / 65535.0f;
	}
	return depthNorm;
}

float Kinect::getDepthFrameRate() const 
{
	return mFrameRateDepth;
}
	
int32_t Kinect::getDeviceCount()
{
	int32_t deviceCount = 0;
	NuiGetSensorCount( &deviceCount );
	return deviceCount;
}

const DeviceOptions& Kinect::getDeviceOptions() const
{
	return mDeviceOptions;
}

float Kinect::getSkeletonFrameRate() const 
{
	return mFrameRateSkeletons; 
}

int32_t Kinect::getTilt()
{
	long degrees = 0L;
	if ( mCapture && mSensor != 0 ) {
		long hr = mSensor->NuiCameraElevationGetAngle( &degrees );
		if ( FAILED( hr ) ) {
			trace( "Unable to retrieve device angle:" );
			error( hr );
		}
	}
	return (int32_t)degrees;
}

int_fast8_t Kinect::getTransform() const
{
	return mTransform;
}

int32_t Kinect::getUserCount()
{
	return mDeviceOptions.isDepthEnabled() ? mUserCount : 0;
}

float Kinect::getColorFrameRate() const
{
	return mFrameRateColor; 
}

Vec2i Kinect::getSkeletonDepthPos( const ci::Vec3f& position )
{
	float x;
	float y;
	Vector4 pos;
	pos.x = position.x;
	pos.y = position.y;
	pos.z = position.z;
	pos.w = 1.0f;
	NuiTransformSkeletonToDepthImage( pos, &x, &y, mDeviceOptions.getDepthResolution() );
	return Vec2i( (int32_t)x, (int32_t)y );
}

Vec2i Kinect::getSkeletonColorPos( const ci::Vec3f& position )
{
	float x;
	float y;
	Vector4 pos;
	pos.x = position.x;
	pos.y = position.y;
	pos.z = position.z;
	pos.w = 1.0f;
	NuiTransformSkeletonToDepthImage( pos, &x, &y, mDeviceOptions.getColorResolution() );
	return Vec2i( (int32_t)x, (int32_t)y );
}

Vec2i Kinect::getColorDepthPos( const ci::Vec2i& v )
{
	long x;
	long y;
	mSensor->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( mDeviceOptions.getColorResolution(), mDeviceOptions.getDepthResolution(), 0, v.x, v.y, mDepthSurface.getPixel( v ).r, &x, &y ); 
	return Vec2i( (int32_t)x, (int32_t)y );
}

void Kinect::init( bool reset )
{
	// Only set these when first initializing the device
	if ( !reset ) {
		mBinary				= false;
		mFlipped			= false;
		mGreyScale			= false;
		mInverted			= false;
		mRemoveBackground	= false;
		mTransform			= TRANSFORM_DEFAULT;
		mVerbose			= true;
			mDepthTreatment		= DepthTreatment::CLAMP_UNRELIABLE_DEPTHS;
	}

	mCapture			= false;
	mDepthStreamHandle	= 0;
	mFrameRateDepth		= 0.0f;
	mFrameRateSkeletons = 0.0f;
	mFrameRateColor		= 0.0f;
	mNewDepthSurface	= false;
	mNewSkeletons		= false;
	mNewColorSurface	= false;
	mIsSkeletonDevice	= false;
	mReadTimeDepth		= 0.0;
	mReadTimeSkeletons	= 0.0;
	mReadTimeColor		= 0.0;
	mRgbDepth			= 0;
	mRgbColor			= 0;
	mSensor				= 0;
	mTiltRequestTime	= 0.0;
	mUserCount			= 0;
	mColorStreamHandle	= 0;


	deactivateUsers();
}

bool Kinect::isCapturing() const 
{
	return mCapture; 
}

bool Kinect::isFlipped() const 
{ 
	return mFlipped; 
}

bool Kinect::openDepthStream()
{
	if ( mSensor != 0) {
		long hr = mSensor->NuiImageStreamOpen( mDeviceOptions.getDepthResolution() != ImageResolution::NUI_IMAGE_RESOLUTION_640x480 && 
			HasSkeletalEngine( mSensor ) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH, mDeviceOptions.getDepthResolution(), 0, 2, 0, &mDepthStreamHandle );;
		if ( FAILED( hr ) ) {
			trace( "Unable to open depth image stream: " );
			error( hr );
			stop();
			return false;
		}
	}
	return true;
}

bool Kinect::openColorStream()
{
	if ( mSensor != 0 ) {
		long hr = mSensor->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, mDeviceOptions.getColorResolution(), 0, 2, 0, &mColorStreamHandle );
		if ( FAILED( hr ) ) {
			trace( "Unable to open color image stream: " );
			error( hr );
			stop();
			return false;
		}
	}
	return true;
}

	void Kinect::pixelToDepthSurface( BYTE *buffer, UINT size, BOOL nearMode, DepthTreatment treatment )
{

	int32_t height		= mDepthSurface.getHeight();
	int32_t width		= mDepthSurface.getWidth();
		int32_t bSize		= width * height * 6; // 6 is 3 color channels * sizeof( uint16_t )

	Pixel16u* rgbRun	= mRgbDepth;
		NUI_DEPTH_IMAGE_PIXEL* bufferRun = (NUI_DEPTH_IMAGE_PIXEL*) buffer;


		// Check if range mode and depth treatment have been changed. Re-initlialize depth-color table with changed parameters
		if (mDeviceOptions.isNearModeEnabled() != (FALSE != nearMode) || mDepthTreatmentChanged || mReliableDepthChanged)
		{
			mDeviceOptions.enableNearMode( (FALSE != nearMode) );
			mReliableDepthChanged = false;
			mDepthTreatmentChanged = false;
			InitDepthColorTable();
			}

		// Allocate buffer for color image. If required buffer size hasn't changed, the previously allocated buffer is returned
		//UINT* rgbRun = (UINT*)ResetBuffer(width * height * BYTES_PER_PIXEL_RGB);

		// Run through pixels
		for ( int32_t i = 0; i < width * height; ++i ) {
			//// Get pixel depth and player index
			//USHORT depth = bufferRun->depth;
			//USHORT index = bufferRun->playerIndex;

			//// Get mapped color from depth-color table
			//Pixel16u pixel = mDepthColorTable[index][depth];
			*rgbRun = depthPixelToColorPixel(bufferRun);

			bufferRun++;


			rgbRun++;
		}



	std::lock_guard<std::mutex> lock(mDepthMutex_);
		memcpy( mDepthSurface.getData(), mRgbDepth, bSize );
}

void Kinect::pixelToColorSurface( uint8_t *buffer )
{
	int32_t height	= mColorSurface->getHeight();
	int32_t width	= mColorSurface->getWidth();
	int32_t size	= width * height * 4;

	if ( mFlipped ) {
		uint8_t *flipped = new uint8_t[ size ];
		for ( int32_t y = 0; y < height; y++ ) {
			for ( int32_t x = 0; x < width; x++ ) {
				int32_t dest	= ( y * width + x ) * 4;
				int32_t src		= ( y * width + ( ( width - x ) - 1 ) ) * 4;
				for ( int32_t i = 0; i < 4; ++i ) {
					flipped[ dest + i ] = buffer[ src + i ];
				}
				swap( flipped[ dest ], flipped[ dest + 2 ] );
				flipped[ dest + 3 ] = 255;
			}
		}


		std::lock_guard<std::mutex> lock(mColorMutex_);
		memcpy( mColorSurface->getData(), flipped, size );
		delete [] flipped;
	} else {
		for ( int32_t i = 0; i < size; i += 4 ) {
			swap( buffer[ i ], buffer[ i + 2 ] );
			buffer[ i + 3 ] = 255;
		}

		std::lock_guard<std::mutex> lock(mColorMutex_);
		memcpy( mColorSurface->getData(), buffer, size );
	}
}

void Kinect::removeBackground( bool remove )
{
	mRemoveBackground = remove;
}

void Kinect::removeCallback( uint32_t id )
{
	mCallbacks.find( id )->second->disconnect();
	mCallbacks.erase( id ); 
}

void Kinect::run()
{
	while ( mCapture ) {
		if ( mSensor != 0 ) {

			// Get elapsed time to calculate frame rate
			double time = getElapsedSeconds();

			//////////////////////////////////////////////////////////////////////////////////////////////

			if ( mDeviceOptions.isDepthEnabled() && mDepthStreamHandle != 0 && !mNewDepthSurface ) {
					
				_NUI_IMAGE_FRAME imageFrame;
				long hr = mSensor->NuiImageStreamGetNextFrame( mDepthStreamHandle, WAIT_TIME, &imageFrame );
				if ( FAILED( hr ) ) {
					error( hr );
				} else {

						BOOL nearMode;
						INuiFrameTexture * texture;
						hr = mSensor->NuiImageFrameGetDepthImagePixelFrameTexture( mDepthStreamHandle, &imageFrame, &nearMode , &texture );
						if ( FAILED( hr ) ) {
							error( hr );
						}
					_NUI_LOCKED_RECT lockedRect;
						hr = texture->LockRect( 0, &lockedRect, NULL, 0 );
					if ( FAILED( hr ) ) {
						error( hr );
					}
					if ( lockedRect.Pitch == 0 ) {
						trace( "Invalid buffer length received" );
					} else {
							pixelToDepthSurface( lockedRect.pBits, lockedRect.size, nearMode, mDepthTreatment );
					}
						texture->UnlockRect(0);
						texture->Release();

					hr = mSensor->NuiImageStreamReleaseFrame( mDepthStreamHandle, &imageFrame );
					if ( FAILED( hr ) ) {
						error( hr ); 
					}
						
					mFrameRateDepth = (float)( 1.0 / ( time - mReadTimeDepth ) );
					mReadTimeDepth = time;

					mUserCount = 0;
					for ( uint32_t i = 0; i < NUI_SKELETON_COUNT; ++i ) {
						if ( mActiveUsers[ i ] ) {
							mUserCount++;
						}
					}

					mNewDepthSurface = true;
				}

			}

			//////////////////////////////////////////////////////////////////////////////////////////////

			if ( mDeviceOptions.isSkeletonTrackingEnabled() && mIsSkeletonDevice && !mNewSkeletons ) {

				_NUI_SKELETON_FRAME skeletonFrame;
				long hr = mSensor->NuiSkeletonGetNextFrame( WAIT_TIME, &skeletonFrame );
				if ( FAILED( hr ) ) {
					error( hr );
				} else {
					processSkeletons(skeletonFrame, hr);
					mFrameRateSkeletons = (float)( 1.0 / ( time - mReadTimeSkeletons ) );
					mReadTimeSkeletons = time;

					mNewSkeletons = true;
				}

			}

			//////////////////////////////////////////////////////////////////////////////////////////////

			if ( mDeviceOptions.isColorEnabled() && mColorStreamHandle != 0 && !mNewColorSurface ) {

				_NUI_IMAGE_FRAME imageFrame;
				long hr = mSensor->NuiImageStreamGetNextFrame( mColorStreamHandle, WAIT_TIME, &imageFrame );
				if ( FAILED( hr ) ) {
					error( hr );
				} else {

					INuiFrameTexture * texture = imageFrame.pFrameTexture;
					_NUI_LOCKED_RECT lockedRect;
					hr = texture->LockRect( 0, &lockedRect, 0, 0 );
					if ( FAILED( hr ) ) {
						error( hr );
					}
					if ( lockedRect.Pitch != 0 ) {
						pixelToColorSurface( (uint8_t*)lockedRect.pBits );
					} else {
						trace( "Invalid buffer length received." );
					}

					hr = mSensor->NuiImageStreamReleaseFrame( mColorStreamHandle, &imageFrame );
					if ( FAILED( hr ) ) {
						error( hr );
					}

					mFrameRateColor = (float)( 1.0 / ( time - mReadTimeColor ) );
					mReadTimeColor = time;

					mNewColorSurface = true;
				}

			}

		}

		Sleep( 8 );

	}

	// Return to join thread
	return;
}

void Kinect::processSkeletons(_NUI_SKELETON_FRAME skeletonFrame, long hr ) {
	bool foundSkeleton = false;
	std::lock_guard<std::mutex> lock(mSkeletonMutex_);
	for ( int32_t i = 0; i < NUI_SKELETON_COUNT; ++i ) {

		(*mSkeletons).at( i ).clear();

		NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[ i ].eTrackingState;
		if ( trackingState == NUI_SKELETON_TRACKED || trackingState == NUI_SKELETON_POSITION_ONLY ) {

			if ( !foundSkeleton ) {
				_NUI_TRANSFORM_SMOOTH_PARAMETERS transform = kTransformParams[ mTransform ];
				hr = mSensor->NuiTransformSmooth( &skeletonFrame, &transform );
				if ( FAILED( hr ) ) {
					error( hr );
				}
				foundSkeleton = true;
			}

			// Flip X when flipping the image.
			if ( mFlipped ) {
				( skeletonFrame.SkeletonData + i )->Position.x *= -1.0f;
				for ( int32_t j = 0; j < (int32_t)NUI_SKELETON_POSITION_COUNT; ++j ) {
					( skeletonFrame.SkeletonData + i )->SkeletonPositions[ j ].x *= -1.0f;
				}
			}

			_NUI_SKELETON_BONE_ORIENTATION bones[ NUI_SKELETON_POSITION_COUNT ];
			hr = NuiSkeletonCalculateBoneOrientations( skeletonFrame.SkeletonData + i, bones );
			if ( FAILED( hr ) ) {
				error( hr );
			}

			for ( int32_t j = 0; j < (int32_t)NUI_SKELETON_POSITION_COUNT; ++j ) {
				Bone bone( *( ( skeletonFrame.SkeletonData + i )->SkeletonPositions + j ), *( bones + j ) );
				( (*mSkeletons).begin() + i )->insert( std::pair<JointName, Bone>( (JointName)j, bone ) );
			}
		}
	}
}

void Kinect::setFlipped( bool flipped ) 
{
	mFlipped = flipped;
}

void Kinect::setTilt( int32_t degrees )
{
	// Tilt requests should be spaced apart to prevent wear on the motor
	double elapsedSeconds = getElapsedSeconds();
	if ( mCapture && mSensor != 0 && elapsedSeconds - mTiltRequestTime > kTiltRequestInterval ) {
		long hr = mSensor->NuiCameraElevationSetAngle( (long)math<int32_t>::clamp( degrees, -MAXIMUM_TILT_ANGLE, MAXIMUM_TILT_ANGLE ) );
		if ( FAILED( hr ) ) {
			trace( "Unable to change device angle: " );
			error( hr );
		}
		mTiltRequestTime = elapsedSeconds;
	}
}

void Kinect::setTransform( int_fast8_t transform )
{
	mTransform = transform;
}

	void Kinect::setMinReliableDepth( USHORT minReliableDepth )
{
		mMinReliableDepth = minReliableDepth;
		mReliableDepthChanged = true;
	}
	USHORT Kinect::getMinReliableDepth()
	{
		return mMinReliableDepth;
	}

	void Kinect::setMaxReliableDepth( USHORT maxReliableDepth )
	{
		mMaxReliableDepth = maxReliableDepth;
		mReliableDepthChanged = true;
	}

	USHORT Kinect::getMaxReliableDepth()
	{
		return mMaxReliableDepth;
	}

	void Kinect::setDepthTreatment( DepthTreatment treatment )
	{
		mDepthTreatmentChanged = true;
		mDepthTreatment = treatment;
	}

	Kinect::DepthTreatment Kinect::getDepthTreatment( )
	{
		return mDepthTreatment;
	}


	Kinect::Pixel16u Kinect::depthPixelToColorPixel( NUI_DEPTH_IMAGE_PIXEL *dPixel )
	{
		//Get pixel depth and player index
		USHORT depth = dPixel->depth;
		USHORT user = dPixel->playerIndex;

		// Mark user active
		if ( user > 0 && user < 7 ) {
			mActiveUsers[ user - 1 ] = true;
		}

		return mDepthColorTable[user][depth];

		}

	BYTE Kinect::GetIntensity(int depth)
	{
		// Validate arguments
		if (depth < MIN_DEPTH || depth > MAX_DEPTH)
		{
			return UCHAR_MAX;
	}

		// Use a logarithmic scale that shows more detail for nearer depths.
		// The constants in this formula were chosen such that values between
		// MIN_DEPTH and MAX_DEPTH will map to the full range of possible
		// byte values.
		const float depthRangeScale = 500.0f;
		const int intensityRangeScale = 74;
		double val = log((double)(depth - MIN_DEPTH) / depthRangeScale + 1) * intensityRangeScale;
		return (BYTE) (((UCHAR_MAX) < (val)) ? (UCHAR_MAX) : (val));

}

void Kinect::start( const DeviceOptions& deviceOptions ) 
{


	if ( !mCapture ) {

		// Copy device options
		mDeviceOptions	= deviceOptions;
		string deviceId	= mDeviceOptions.getDeviceId();
		int32_t index	= mDeviceOptions.getDeviceIndex();

		// Clamp device index
		if ( index >= 0 ) {
			index = math<int32_t>::clamp( index, 0, math<int32_t>::max( getDeviceCount() - 1, 0 ) );
		}

		// Initialize device instance
		long hr = S_OK;
		if ( index >= 0 ) {
			hr = NuiCreateSensorByIndex( index, &mSensor );
			if ( FAILED( hr ) ) {
				trace( "Unable to create device instance " + toString( index ) + ": " );
				error( hr );
				return;
			}
		} else if ( deviceId.length() > 0 ) {
			_bstr_t id = deviceId.c_str();
			hr = NuiCreateSensorById( id, &mSensor );
			if ( FAILED( hr ) ) {
				trace( "Unable to create device instance " + deviceId + ":" );
				error( hr );
				return;
			}
		} else {
			trace( "Invalid device name or index." );
			return;
		}

		// Check device
		hr = mSensor != 0 ? mSensor->NuiStatus() : E_NUI_NOTCONNECTED;
		if ( hr == E_NUI_NOTCONNECTED ) {
			error( hr );
			return;
		}

		// Get device name and index
		if ( mSensor != 0 ) {
			mDeviceOptions.setDeviceIndex( mSensor->NuiInstanceIndex() );
			BSTR id = ::SysAllocString( mSensor->NuiDeviceConnectionId() ); 
			_bstr_t idStr( id );
			if ( idStr.length() > 0 ) {
				std::string str( idStr );
				mDeviceOptions.setDeviceId( str );
			}
			::SysFreeString( id );
		} else {
			index = -1;
			deviceId = "";
		}

		// Update device index and ID
		mDeviceOptions.setDeviceIndex( index );
		mDeviceOptions.setDeviceId( "" );

		// Initialize sensor image streams
		unsigned long flags;
		if ( !mDeviceOptions.isUserTrackingEnabled() ) {
			flags = NUI_INITIALIZE_FLAG_USES_DEPTH;
		} else {
			flags = NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX;
			if ( mDeviceOptions.isSkeletonTrackingEnabled() ) {
				flags |= NUI_INITIALIZE_FLAG_USES_SKELETON;
				if ( mDeviceOptions.isNearModeEnabled() ) {
					flags |= NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE;
				}
			}
		}
		if ( mDeviceOptions.isColorEnabled() ) {
			flags |= NUI_INITIALIZE_FLAG_USES_COLOR;
		}
		hr = mSensor->NuiInitialize( flags );
		if ( FAILED( hr ) ) {
			trace( "Unable to initialize device " + mDeviceOptions.getDeviceId() + ":" );
			error( hr );
			return;
		}

		// Skeletons are only supported on the first device
		if ( mDeviceOptions.isSkeletonTrackingEnabled() && HasSkeletalEngine( mSensor ) ) {
			flags = NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE;
			if ( mDeviceOptions.isSeatedModeEnabled() ) {
				flags |= NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT;
			}
			hr = mSensor->NuiSkeletonTrackingEnable( 0, flags );
			if ( FAILED( hr ) ) {
				trace( "Unable to initialize skeleton tracking for device " + mDeviceOptions.getDeviceId() + ": " );
				error( hr );
				return;
			}
			mIsSkeletonDevice = true;
		}

		// Initialize depth image
		if ( mDeviceOptions.getDepthResolution() != ImageResolution::NUI_IMAGE_RESOLUTION_INVALID ) {
				InitDepthColorTable();
			const Vec2i& depthSize = mDeviceOptions.getDepthSize();
			if ( mDeviceOptions.isDepthEnabled() && !openDepthStream() ) {
				return;
			}
			mDepthSurface	= Surface16u( depthSize.x, depthSize.y, false, SurfaceChannelOrder::RGB );
			mRgbDepth		= new Pixel16u[ depthSize.x * depthSize.y * 3 ];
		}

		// Initialize video image
		if ( mDeviceOptions.getColorResolution() != ImageResolution::NUI_IMAGE_RESOLUTION_INVALID ) {
			const Vec2i& videoSize = mDeviceOptions.getColorSize();
			if ( mDeviceOptions.isColorEnabled() && !openColorStream() ) {
				return;
			}
			*mColorSurface	= Surface8u( videoSize.x, videoSize.y, false, SurfaceChannelOrder::RGBA );
			*currColorSurface	= Surface8u( videoSize.x, videoSize.y, false, SurfaceChannelOrder::RGBA );
			mRgbColor		= new Pixel[ videoSize.x * videoSize.y * 4 ];
		}

		// Set image stream flags
		flags = NUI_IMAGE_STREAM_FRAME_LIMIT_MAXIMUM;
		if ( mDeviceOptions.isNearModeEnabled() ) {
			flags |= NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE | NUI_IMAGE_STREAM_FLAG_DISTINCT_OVERFLOW_DEPTH_VALUES;
		}
		hr = mSensor->NuiImageStreamSetImageFrameFlags( mDepthStreamHandle, flags );
		if ( FAILED( hr ) ) {
			trace( "Unable to set image frame flags: " );
			error( hr );
		}

		// Initialize skeletons
		(*mSkeletons).clear();
		(*currSkeletons).clear();
		for ( int32_t i = 0; i < NUI_SKELETON_COUNT; ++i ) {
			(*mSkeletons).push_back( Skeleton() );
			(*currSkeletons).push_back( Skeleton() );
		}

		// Start thread
		mCapture = true;
		mThread = std::shared_ptr<std::thread>( new std::thread( boost::bind( &Kinect::run, this ) ) );

	}

}

void Kinect::stop()
{
	mCapture = false;
	if ( mThread ) {
		mThread->join();
		mThread.reset();
	}
	if ( mRgbDepth != 0 ) {
		delete [] mRgbDepth;
	}
	if ( mRgbColor != 0 ) {
		delete [] mRgbColor;
	}
	init( true );
}

void Kinect::trace( const string& message ) 
{

	EECore::GlobalLog::error( "Kinect.cpp", message.c_str() );
	//OutputDebugStringA( ( message + "\n" ).c_str() );
}

void Kinect::swapColor() {
	std::lock_guard<std::mutex> lock(mColorMutex_);
	ci::Surface8u*	temp = mColorSurface;
	mColorSurface = currColorSurface;
	currColorSurface = temp;
}

void Kinect::swapSkeleton() {
	std::lock_guard<std::mutex> lock(mSkeletonMutex_);
	std::vector<Skeleton>* temp = mSkeletons;
	mSkeletons = currSkeletons;
	currSkeletons = temp;

}

void Kinect::update()
{
//	if ( mNewDepthSurface ) {
//		mSignalDepth( *currDepthSurface, mDeviceOptions );
//		mNewDepthSurface = false;
//	}
	if ( mNewSkeletons ) {
		mSignalSkeleton( *currSkeletons, mDeviceOptions );
		mNewSkeletons = false;
	}
	if ( mNewColorSurface ) {
		swapColor();
		mSignalColor( *currColorSurface, mDeviceOptions );
		mNewColorSurface = false;
	}
}

bool Kinect::updateDepth(ci::Surface16u* dst) {
	if ( mNewDepthSurface ) {
		mNewDepthSurface = false;
		std::lock_guard<std::mutex> lock(mDepthMutex_);
		*dst = mDepthSurface;
	//	memcpy(dst, &mDepthSurface, sizeof(ci::Surface16u));
		return true;
	}
	return false;
}

	void Kinect::InitDepthColorTable()
	{
		// Get the min and max reliable depth
	/*	USHORT minReliableDepth = (mDeviceOptions.isNearModeEnabled() ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
		USHORT maxReliableDepth = (mDeviceOptions.isNearModeEnabled() ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;*/

		USHORT minReliableDepth = mMinReliableDepth;
		USHORT maxReliableDepth = mMaxReliableDepth;

		

		RtlZeroMemory(mDepthColorTable, sizeof(mDepthColorTable));

		Pixel16u pixel;

		// Set color for unknown depth
		pixel.r = 0; pixel.g = 63; pixel.b = 63;
		mDepthColorTable[0][UNKNOWN_DEPTH] = pixel;



		switch (mDepthTreatment)
		{
		case DepthTreatment::CLAMP_UNRELIABLE_DEPTHS:
			// Fill in the "near" portion of the table with solid color
			for (int depth = UNKNOWN_DEPTH + 1; depth < minReliableDepth; depth++)
			{

				pixel.r = 31; pixel.g=127; pixel.b=255;
				mDepthColorTable[0][depth] = pixel;
			}

			// Fill in the "far" portion of the table with solid color
			for (int depth = maxReliableDepth + 1; depth <= USHRT_MAX; depth++)
			{

				pixel.r = 0; pixel.g = 127; pixel.b = 15;
				mDepthColorTable[0][depth] = pixel;
			}
			break;

		case DepthTreatment::TINT_UNRELIABLE_DEPTHS:
			{
				// Fill in the "near" portion of the table with a tinted gradient
				for (int depth = UNKNOWN_DEPTH + 1; depth < minReliableDepth; depth++)
				{
					BYTE intensity = GetIntensity(depth);
					pixel.r = intensity >> 3;
					pixel.g = intensity >> 1;
					pixel.b = intensity;
					mDepthColorTable[0][depth] = pixel;
					//SetColor(&mDepthColorTable[0][depth], r, g, b);
				}

				// Fill in the "far" portion of the table with a tinted gradient
				for (int depth = maxReliableDepth + 1; depth <= USHRT_MAX; depth++)
				{
					BYTE intensity = GetIntensity(depth);
					pixel.r = intensity;
					pixel.g = intensity >> 3;
					pixel.b = intensity >> 1;
					mDepthColorTable[0][depth] = pixel;
					//SetColor(&mDepthColorTable[0][depth], r, g, b);
				}
			}
			break;

		case DepthTreatment::DISPLAY_ALL_DEPTHS:
			minReliableDepth = MIN_DEPTH;
			maxReliableDepth = MAX_DEPTH;

			for (int depth = UNKNOWN_DEPTH + 1; depth < minReliableDepth; depth++)
			{

				pixel.r = 255; pixel.g = 255; pixel.b = 255;
				mDepthColorTable[0][depth] = pixel;
			}
			break;

		default:
			break;
		}

		for (USHORT depth = minReliableDepth; depth <= maxReliableDepth; depth++)
		{
			
			//pixel.r = pixel.g = pixel.b = ci::math<USHORT>::clamp( (depth-USHRT_MAX/2), 0, USHRT_MAX );
			pixel.r = pixel.g = pixel.b = MapColor(depth, minReliableDepth, maxReliableDepth );
			mDepthColorTable[0][depth] = pixel;
		}
	}

	void Kinect::SetColor(UINT* pColor, BYTE red, BYTE green, BYTE blue, BYTE alpha)
	{
		if (!pColor)
			return;

		BYTE* c = (BYTE*)pColor;
		c[COLOR_INDEX_RED]   = red;
		c[COLOR_INDEX_GREEN] = green;
		c[COLOR_INDEX_BLUE]  = blue;
		c[COLOR_INDEX_ALPHA] = alpha;
	}

	USHORT Kinect::MapColor( USHORT depth, USHORT minDepth, USHORT maxDepth )
	{
		float top = depth - minDepth;
		float bottom = maxDepth - minDepth;
		float ratio = USHRT_MAX*(top/bottom);
		
		return floor(ratio);

		
}
}
