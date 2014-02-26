#pragma once
#include "GlobalSettings.h"

namespace settings {

class SettingsInterface {


public:
	virtual ~SettingsInterface() {}
	virtual void registerSettings() = 0;
	virtual void updateSettings() = 0;
	virtual void getSettings() = 0;

protected:

	void registerVariable( const boost::any & variable, std::string varName, std::string classDesciptor ) {
		settings::GlobalSettings::get().registerVariable(variable, varName, classDesciptor);
	}

	bool updateVal( const boost::any & variable, std::string varName, std::string classDesciptor) {
		return settings::GlobalSettings::get().updateVal(variable, varName, classDesciptor);
	}

	void getVal(int &destinationVal, std::string varName, std::string classDesciptor) {
		settings::GlobalSettings::get().getVal(destinationVal, varName, classDesciptor);
	}

	void getVal(float &destinationVal, std::string varName, std::string classDesciptor) {
		settings::GlobalSettings::get().getVal(destinationVal, varName, classDesciptor);
	}

	void getVal(double &destinationVal, std::string varName, std::string classDesciptor){
		settings::GlobalSettings::get().getVal(destinationVal, varName, classDesciptor);
	}

	void getVal(std::string &destinationVal, std::string varName, std::string classDesciptor){
		settings::GlobalSettings::get().getVal(destinationVal, varName, classDesciptor);
	}
	
	void getVal(bool &destinationVal, std::string varName, std::string classDesciptor){
		settings::GlobalSettings::get().getVal(destinationVal, varName, classDesciptor);
	}
};

}