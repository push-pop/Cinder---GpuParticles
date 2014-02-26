#pragma once
#include "cinder/System.h"
#include "cinder/Utilities.h"

#include <boost/any.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>

#include <thread>
#include <mutex>

namespace EECore
{

	class GlobalLog
	{
	public:
		
		static GlobalLog& get(){
			static GlobalLog log;
			return log;
		}

		~GlobalLog();

		static void info( std::string classDescriptor, std::string message );

		static void warn( std::string classDescriptor, std::string message );

		static void error( std::string classDescriptor, std::string message );

	private:

		GlobalLog();

		void logNewline( );

		void logMessage( std::string classDescriptor, std::string level, std::string message );

		void writeLogFile( std::string out );

		void deleteOldLogs( );

		std::mutex logMutex_;

		std::string mOutFileName;
		std::ofstream mOutFile;
	};

}