#pragma once
#include "cinder/System.h"
#include "cinder/Utilities.h"
#include "cinder/Xml.h"

#include <boost/any.hpp>
#include <map>

namespace settings{




	typedef struct {		
		boost::any				variable;
		int						TYPE;	
		std::string				varName;
		std::string				classDesciptor;
	}s;
	typedef std::map<std::string, settings::s*> AnyMap;

class GlobalSettings
{





public:
	static GlobalSettings& get(){ 
		static GlobalSettings settings;
		return settings;
	}
	~GlobalSettings();
	//int getTest(){return settingsMap_.size(); }//used to test methods in constructor

	/**********************************************************************************************//**
	 * @fn	void GlobalSettings::loadConfig();
	 *
	 * @brief	Loads the configuration file from disk
	 *
	 * @author	Ed
	 * @date	8/8/2013
	 **************************************************************************************************/

	bool loadConfig(std::string settingsName);

	/**********************************************************************************************//**
	 * @fn	void GlobalSettings::saveConfig();
	 *
	 * @brief	Saves the configuration file to disk
	 *
	 * @author	Ed
	 * @date	8/8/2013
	 **************************************************************************************************/

	bool saveConfig(std::string settingsName);	
	

protected:
	friend class SettingsInterface;

	/**********************************************************************************************//**
	 * @fn	void GlobalSettings::registerVariable( const boost::any & variable, std::string varName,
	 * 		std::string classDesciptor );
	 *
	 * @brief	Registers the variable.
	 *
	 * @author	Ed Kahler
	 * @date	8/19/2013
	 *
	 * @param	variable	  	The variable. currently accepts int, string, float, double
	 * @param	varName		  	Name of the variable.
	 * @param	classDesciptor	The class desciptor.
	 **************************************************************************************************/
	void registerVariable( const boost::any & variable, std::string varName, std::string classDesciptor );

	/**********************************************************************************************//**
	 * @fn	bool GlobalSettings::updateVal( const boost::any & variable, std::string varName,
	 * 		std::string classDesciptor);
	 *
	 * @brief	Updates the variable.
	 *
	 * @author	Ed
	 * @date	8/8/2013
	 *
	 * @param	variable	  	The variable.
	 * @param	varName		  	Name of the variable.
	 * @param	classDesciptor	The class desciptor.
	 *
	 * @return	true if it succeeds, false if it fails.
	 *
	 * ### tparam	typename	T	Type of the typename t.
	 * ### param	var 	The variable.
	 * ### param	name	The name.
	 * ### param	who 	The who.
	 **************************************************************************************************/
	bool updateVal( const boost::any & variable, std::string varName, std::string classDesciptor);


	void getVal(int &destinationVal, std::string varName, std::string classDesciptor);
	void getVal(float &destinationVal, std::string varName, std::string classDesciptor);
	void getVal(double &destinationVal, std::string varName, std::string classDesciptor);
	void getVal(std::string &destinationVal, std::string varName, std::string classDesciptor);
	void getVal(bool &destinationVal, std::string varName, std::string classDesciptor);


private:
	GlobalSettings();

	enum typeEnum_ { STRING = 0 , INT = 1, FLOAT = 2, DOUBLE = 3, BOOL = 4 };

	/**********************************************************************************************//**
	 * @brief	The settings map.
	 **************************************************************************************************/

	AnyMap settingsMap_;
	



};
// allow easy access to settings
inline GlobalSettings& settings() { return settings::GlobalSettings::get(); };


}