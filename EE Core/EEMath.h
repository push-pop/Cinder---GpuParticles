#pragma once
#include "cinder/Vector.h"

namespace EEMath 
{
	/**********************************************************************************************//**
	 * @fn	static float InvSqrt(float x)
	 *
	 * @brief	Inverse sqrt. Frome Quake3. 
	 * 			http://en.wikipedia.org/wiki/Fast_inverse_square_root
	 *
	 * @author	
	 * @date	10/1/2013
	 *
	 * @param	x	The x coordinate.
	 *
	 * @return	.
	 **************************************************************************************************/

	static float InvSqrt(float x){
		float xhalf = 0.5f * x;
		int i = *(int*)&x; // store floating-point bits in integer
		i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
		x = *(float*)&i; // convert new bits into float
		x = x*(1.5f - xhalf*x*x); // One round of Newton's method
		return x;
	}

	/**********************************************************************************************//**
	 * @fn	static void absFloorClamp(cinder::Vec3f* vec, float absFloorVal, float clampVal)
	 *
	 * @brief	Tests if a value is between positive and negative of the same value and then calmps.
	 * 			Example. If x > -1 and x < 1 and 1 is the absFloorVal then x can be set to the clampVal
	 * 			So we can set x to be 0 or x to be 0.5 etc. 
	 * 			Useful to clamp velocity or other values that are +/- a range.
	 *
	 * @author	Alan
	 * @date	9/10/2013
	 *
	 * @param [in,out]	vec	If non-null, the vector.
	 * @param	absFloorVal	The abs floor value.
	 * @param	clampVal   	The clamp value.
	 **************************************************************************************************/

	static void absFloorClamp(cinder::Vec3f* vec, float absFloorVal, float clampVal) {
		vec->x = vec->x < absFloorVal ? vec->x > -absFloorVal ? clampVal: vec->x : vec->x;
		vec->y = vec->y < absFloorVal ? vec->y > -absFloorVal ? clampVal: vec->y : vec->y;	
		vec->z = vec->z < absFloorVal ? vec->z > -absFloorVal ? clampVal: vec->z : vec->z;
	}

	/**********************************************************************************************//**
	 * @fn	static bool fastVec3fLimit(cinder::Vec3f* vec, float maxLength )
	 *
	 * @brief	Fast vector 3f limit. Uses the fast invsqrt in calcuation.
	 *
	 * @author	Alan
	 * @date	10/1/2013
	 *
	 * @param [in,out]	vec	If non-null, the vector.
	 * @param	maxLength  	The maximum length.
	 *
	 * @return	true if the vec exceeds maxLength, false if not.
	 **************************************************************************************************/

	static bool fastVec3fLimit(cinder::Vec3f* vec, float maxLength )
	{
		float lengthSq = vec->lengthSquared() + 0.0001f;
		if(lengthSq > maxLength*maxLength) {
			maxLength = maxLength*InvSqrt(lengthSq);
			vec->x = vec->x*maxLength;
			vec->y = vec->y*maxLength;
			vec->z = vec->z*maxLength;
			return true;
 		}
		return false;
	}

	/**********************************************************************************************//**
	 * @fn	static float fastSin(float x)
	 *
	 * @brief	Fast sine. This is a fast and accurate sine method with a max error of
	 * 			0.001. It beats the Taylor series in accuracy and speed and comes from:
	 * 			http://devmaster.net/posts/9648/fast-and-accurate-sine-cosine
	 * 			
	 * 			This was an interesting link from the forum too, faster math funct:
	 * 			http://www.research.scea.com/gdc2003/fast-math-functions.html
	 *
	 * @author	
	 * @date	10/1/2013
	 *
	 * @param	x	The x coordinate.
	 *
	 * @return	.
	 **************************************************************************************************/
	static float fastSin(float x)
	{		
		// Convert the input value to a range of -1 to 1
		x = x * (0.31830988f); //1/PI 

		// Wrap around
		volatile float z = (x + 25165824.0f);
		x = x - (z - 25165824.0f);

#if LOW_SINE_PRECISION
		return 4.0f * (x - x * abs(x));
#else
		float y = x - x * abs(x);

		const float Q = 3.1f;
		const float P = 3.6f;

		return y * (Q + P * abs(y));
#endif
	}

	static float fastCos(float x)
	{
		return fastSin((x + 1.5707963f));
	}

	/**********************************************************************************************//**
	 * @fn	static float CalculateHeading(float lat1, float long1, float lat2, float long2)
	 *
	 * @brief	Calculates the heading from lat1/lon1 to lat2/lon2. Modified to use fast approximations of sin/cos
	 *
	 * @author	Ed Kahler
	 * @date	9/17/2013
	 *
	 * @param	lat1 	The first lat.
	 * @param	long1	The first long.
	 * @param	lat2 	The second lat.
	 * @param	long2	The second long.
	 *
	 * @return	The calculated heading.
	 **************************************************************************************************/

	static float CalculateHeading(float lat1, float long1, float lat2, float long2)
	{
		//convert polar coord to radians
		float a = lat1 * 0.017453292519943295769f; // pi/180
		float c = lat2 *  0.017453292519943295769f;
		float dsubB = (long2 *  0.017453292519943295769f) - (long1 *  0.017453292519943295769f); //d - b

		if (fastCos(c) * fastSin(dsubB) == 0.0f)
			if (c > a)
				return 0.0f;
			else
				return 180.0f;
		else
		{
			return fmod( (atan2(fastCos(c) * fastSin(dsubB), fastSin(c) * fastCos(a) - fastSin(a) * fastCos(c) * fastCos(dsubB)) 
							*57.2957795130823f + 360.0f), 360.0f );  // 57.2957795130823 = 180/pi

		}
	}

	/**********************************************************************************************//**
	 * @fn	float distance(float lat1, float long1, float lat2, float long2)
	 *
	 * @brief	Returns distance between two geo coordinates
	 * 			uses 6371.01Km as earths radius
	 * 			Modified to use fast approximations of sin/cos
	 *
	 * @author	Ed Kahler
	 * @date	9/17/2013
	 *
	 * @param	lat1 	The first lat.
	 * @param	long1	The first long.
	 * @param	lat2 	The second lat.
	 * @param	long2	The second long.
	 *
	 * @return	distance (in Km)
	 **************************************************************************************************/

	static float CalculatePolarDistance(float lat1, float long1, float lat2, float long2) {
	   
		//convert polar coord to radians
		float a = lat1 * 0.017453292519943295769f;// pi/180
		float b = long1 * 0.017453292519943295769f;
		float c = lat2 *  0.017453292519943295769f;
		float d = long2 *  0.017453292519943295769f;

		if (a == c && b == d)
			return 0.0f;
		else
		{
			return (6371.01f) * acos((fastSin(a) * fastSin(c) + fastCos(a) * fastCos(c) * fastCos(b - d)));   //convert earth radius to Kilometer, then multiply bt acos
		}
	}

	/**********************************************************************************************//**
	 * @fn	Vec2f CalculateTerminalPolarPoint(float lat1, float long1, float calcDegree,
	 * 		float calcDistance)
	 *
	 * @brief	Calculates the terminal polar point given start position, Azimuths, and distance
	 * 			Modified to use fast approximations of sin/cos
	 *
	 * @author	Ed Kahler
	 * @date	9/17/2013
	 *
	 * @param	lat1			The first lat.
	 * @param	long1			The first long.
	 * @param	calcDegree  	The calculate degree.
	 * @param	calcDistance	The calculate distance.
	 *
	 * @return	The calculated terminal polar point.
	 **************************************************************************************************/

	static void CalculateTerminalPolarPoint(float & resultX, float & resultY, float lat1, float long1, float calcDegree, float calcDistance) {	
		float distRatio = calcDistance / 6371.01f;//radiusEarthKilometres;
		float distRatioSine = fastSin(distRatio);
		float distRatioCosine = fastCos(distRatio);

		float startLatRad = ci::toRadians(lat1);

		float startLatCos = fastCos(startLatRad);
		float startLatSin = fastSin(startLatRad);

		float endLatRads = asin((startLatSin * distRatioCosine) + (startLatCos * distRatioSine * fastCos( ci::toRadians(calcDegree ) )));

		resultY = ci::toDegrees(ci::toRadians(long1)
			+ atan2(
			fastSin(ci::toRadians(calcDegree)) * distRatioSine * startLatCos,
			distRatioCosine - startLatSin * fastSin(endLatRads)));

		resultX = ci::toDegrees(endLatRads);
		
		if(resultY > 180.0f){
			resultY -= 360.0f;
		}else if(resultY < -180.0f){
			resultY += 360.0f;
		}
	}
}