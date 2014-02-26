#include "GlobalSettings.h"



namespace settings{

	GlobalSettings::GlobalSettings(){
	
	}

	GlobalSettings::~GlobalSettings(){
		typedef std::map<std::string, settings::s*>::iterator it_type;
		for(it_type iterator = settings::GlobalSettings::get().settingsMap_.begin(); iterator != settings::GlobalSettings::get().settingsMap_.end(); iterator++) {
			if(iterator->second != NULL) delete iterator->second;
			iterator->second = NULL;
		}
		GlobalSettings::get().settingsMap_.clear();
	}

	void GlobalSettings::registerVariable( const boost::any & variable, std::string varName, std::string classDesciptor )
	{
		//concat varable name and who for key
		std::string key = varName + ":_:" + classDesciptor;

		settings::s* newSettings_;
		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find( key  );
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			newSettings_ = settings::GlobalSettings::get().settingsMap_.at( key );
		} else {
			newSettings_ = new s;
			settings::GlobalSettings::get().settingsMap_.insert( std::make_pair(key, newSettings_) );
		}
		
		newSettings_->varName= varName;
		newSettings_->classDesciptor = classDesciptor;


		if(  variable.type() == typeid(std::string) ){
			newSettings_->variable = variable;
			newSettings_->TYPE = 0;
			//settingsMap_.insert(std::make_pair(name, newSettings_	));
		}else if(  variable.type() == typeid(int) ){
			newSettings_->variable = variable;
			newSettings_->TYPE = 1;
			//settingsMap_.insert(std::make_pair(name, newSettings_	));
		}else if(  variable.type() == typeid(float) ){
			newSettings_->variable = variable;
			newSettings_->TYPE = 2;
			//settingsMap_.insert(std::make_pair(name, newSettings_	));
		}else if(  variable.type() == typeid(double) ){
			newSettings_->variable = variable;
			newSettings_->TYPE = 3;
			//settingsMap_.insert(std::make_pair(name, newSettings_	));
		}else if(  variable.type() == typeid(bool) ){
			newSettings_->variable = variable;
			newSettings_->TYPE = 4;
			//settingsMap_.insert(std::make_pair(name, newSettings_	));
		}	
	}

	bool GlobalSettings::updateVal( const boost::any & variable, std::string varName, std::string classDesciptor )
	{
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			settings::GlobalSettings::get().settingsMap_.at(key)->variable = variable;
			return true;
		}else{
		
			return false;
		}


	
	}

	void GlobalSettings::getVal(std::string &destinationVal, std::string varName, std::string classDesciptor){
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			boost::any var = settings::GlobalSettings::get().settingsMap_.at(key)->variable;
			if(var.type() == typeid(std::string)){
				
				std::string fromAny = boost::any_cast<std::string>(var); 
				destinationVal = fromAny;
			}			
		}
	}
	void GlobalSettings::getVal(int &destinationVal, std::string varName, std::string classDesciptor){
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			boost::any var = settings::GlobalSettings::get().settingsMap_.at(key)->variable;
			if(var.type() == typeid(int)){
				
				int fromAny = boost::any_cast<int>(var); 
				destinationVal = fromAny;
			}			
		}
	}
	void GlobalSettings::getVal(float &destinationVal, std::string varName, std::string classDesciptor){
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			boost::any var = settings::GlobalSettings::get().settingsMap_.at(key)->variable;
			if(var.type() == typeid(float)){
				
				float fromAny = boost::any_cast<float>(var); 
				destinationVal = fromAny;

			}			
		}
	}
	void GlobalSettings::getVal(double &destinationVal, std::string varName, std::string classDesciptor){
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			boost::any var = settings::GlobalSettings::get().settingsMap_.at(key)->variable;
			if(var.type() == typeid(double)){
				
				double fromAny = boost::any_cast<double>(var); 
				destinationVal = fromAny;
			}			
		}
	}
	void GlobalSettings::getVal(bool &destinationVal, std::string varName, std::string classDesciptor){
	
		std::string key = varName + ":_:" + classDesciptor;

		std::map<std::string, settings::s*>::iterator it = settings::GlobalSettings::get().settingsMap_.find(key);
		if(it != settings::GlobalSettings::get().settingsMap_.end())
		{
			boost::any var = settings::GlobalSettings::get().settingsMap_.at(key)->variable;
			if(var.type() == typeid(bool)){
				
				bool fromAny = boost::any_cast<bool>(var); 

				destinationVal = fromAny;

			}			
		}
	}

	bool GlobalSettings::loadConfig(std::string file){
		try {
			boost::filesystem::create_directory("configs");
		} catch (...) {

		}
		file.insert(0, "./configs/");
		ci::fs::path testPath(file);
		if( !ci::fs::exists( testPath ) ) {
			return false;
		}
	
		try{
		
			ci::XmlTree doc( ci::loadFile( boost::filesystem::path(file) ) );

			for( ci::XmlTree::Iter track = doc.begin("general/variable"); track != doc.end(); ++track ){
				std::string type = track->getAttributeValue<std::string>("type");
				if(type == "string"){
					//console() <<" val:"  << track->getAttributeValue<std::string>("val");
					registerVariable( track->getAttributeValue<std::string>("variable"),
									 track->getAttributeValue<std::string>("varName"),
									 track->getAttributeValue<std::string>("classDesciptor") );
				}else if(type == "int"){
					//console() <<" val:"  << track->getAttributeValue<int>("val");
					registerVariable( track->getAttributeValue<int>("variable"),
									 track->getAttributeValue<std::string>("varName"),
									 track->getAttributeValue<std::string>("classDesciptor") );
				}else if(type == "float"){
					//console() <<" val:"  << track->getAttributeValue<float>("val");
					registerVariable( track->getAttributeValue<float>("variable"),
									 track->getAttributeValue<std::string>("varName"),
									 track->getAttributeValue<std::string>("classDesciptor") );
				}else if(type == "double"){
					//console() <<" val:"  << track->getAttributeValue<double>("val");
					registerVariable( track->getAttributeValue<double>("variable"),
									 track->getAttributeValue<std::string>("varName"),
									 track->getAttributeValue<std::string>("classDesciptor") );
				}else if(type == "bool"){
					//console() <<" val:"  << track->getAttributeValue<double>("val");
					registerVariable( track->getAttributeValue<bool>("variable"),
									 track->getAttributeValue<std::string>("varName"),
									 track->getAttributeValue<std::string>("classDesciptor") );
				}				
			}
			
		}catch(...){
		
		//	console() << "Error: loading/reading configuration file." << endl;
			return false;
		}
		return true;
	}

	bool GlobalSettings::saveConfig(std::string file){
		file.insert(0, "./configs/");
		try {
			boost::filesystem::create_directory("configs");
		} catch (...) {

		}

		std::string beginXmlStr( "<?xml version=\"1.0\"encoding=\"UTF-8\" ?>" );
		ci::XmlTree doc( beginXmlStr );

		ci::XmlTree generalNode;
		generalNode.setTag("general");


		for ( AnyMap::const_iterator it = settings::GlobalSettings::get().settingsMap_.begin(); it != settings::GlobalSettings::get().settingsMap_.end();	++it )
		{

			std::string name = it->first;
			settings::s* varSettings = it->second;

			ci::XmlTree varableNode( "variable", "" );
			
			if(varSettings->variable.type() == typeid(std::string)){
				varableNode.setAttribute( "variable",  boost::any_cast<std::string>(varSettings->variable) );
				varableNode.setAttribute( "type", "string" );
			}else if(varSettings->variable.type() == typeid(int)){
				varableNode.setAttribute( "variable",  boost::any_cast<int>(varSettings->variable) );
				varableNode.setAttribute( "type", "int" );
			}else if(varSettings->variable.type() == typeid(float)){
				varableNode.setAttribute( "variable",  boost::any_cast<float>(varSettings->variable) );
				varableNode.setAttribute( "type", "float" );
			}else if(varSettings->variable.type() == typeid(double)){
				varableNode.setAttribute( "variable",  boost::any_cast<double>(varSettings->variable) );
				varableNode.setAttribute( "type", "double" );
			}else if(varSettings->variable.type() == typeid(bool)){
				varableNode.setAttribute( "variable",  boost::any_cast<bool>(varSettings->variable) );
				varableNode.setAttribute( "type", "bool" );
			}

			//varableNode.setAttribute( "type", varSettings.TYPE );
			varableNode.setAttribute( "varName", varSettings->varName );
			varableNode.setAttribute( "classDesciptor", varSettings->classDesciptor );
			
			generalNode.push_back(varableNode);
		}

		doc.push_back(generalNode);

		try{
			doc.write(ci::writeFile( boost::filesystem::path(file) ) );
		}catch(...){
		//	console()<< "Error: unable to write config file..." <<endl;
			return false;
		}
		return true;
	}

}