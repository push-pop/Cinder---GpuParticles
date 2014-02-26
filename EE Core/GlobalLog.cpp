#include "GlobalLog.h"

namespace EECore
{
	GlobalLog::GlobalLog()
	{




		try{
			boost::filesystem::create_directory( "logs" );
		} catch (...) {

		}

		std::lock_guard<std::mutex> lock(logMutex_);

		std::ostringstream dateString;

		auto* facet(new boost::posix_time::time_facet("%Y-%m"));
		dateString.imbue(std::locale(dateString.getloc(), facet));
		dateString << boost::posix_time::second_clock::local_time();

		mOutFileName = "logs/" + dateString.str() + ".log";

		deleteOldLogs( );
		logNewline();

		std::stringstream msg;
		msg << "Log begin at "
			<< boost::posix_time::second_clock::local_time()
			<< std::endl
			<< "--------------------";
		info( "Global Log", msg.str() );

	}

	GlobalLog::~GlobalLog()
	{

	}

	void GlobalLog::info( std::string classDescriptor, std::string message )
	{
		GlobalLog::get().logMessage( classDescriptor, "Info", message );
	}

	void GlobalLog::warn( std::string classDescriptor, std::string message )
	{
		GlobalLog::get().logMessage( classDescriptor, "Warning", message );
	}

	void GlobalLog::error( std::string classDescriptor, std::string message )
	{
		GlobalLog::get().logMessage( classDescriptor, "Error", message );
	}

	void GlobalLog::logMessage( std::string classDescriptor, std::string level, std::string message )
	{
		std::ostringstream dateString;

		auto* facet(new boost::posix_time::time_facet("%m-%d %H:%M:%S"));
		dateString.imbue(std::locale(dateString.getloc(), facet));

		dateString << boost::posix_time::second_clock::local_time();

		std::stringstream outString;

		outString << level
			<< "["
			<< dateString.str()
			<< "]: "
			<< classDescriptor
			<< " - "
			<< message
			<< std::endl;

		writeLogFile( outString.str() );

	}

	void GlobalLog::logNewline( )
	{
		writeLogFile( std::string( "\n" ) );
	}

	void GlobalLog::writeLogFile( std::string out )
	{
		mOutFile.open(mOutFileName, std::ofstream::app | std::ofstream::out);

		if( mOutFile.good() )
			mOutFile << out;

		mOutFile.close();


	}

	void GlobalLog::deleteOldLogs( )	
	{
		boost::filesystem::path targetPosDir("logs"); 
		boost::filesystem::directory_iterator itPos(targetPosDir), eodPos;

		std::time_t currentTime;
		std::time( &currentTime);


		BOOST_FOREACH(boost::filesystem::path const &p, std::make_pair(itPos, eodPos))   
		{ 
			std::time_t fileTime = boost::filesystem::last_write_time(p);

			//525900 is # seconds in two months.
			if( (currentTime - fileTime) > 5259000 )
				boost::filesystem::remove(p);
		}
	}
}