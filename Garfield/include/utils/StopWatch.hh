// -----------------------------------
// Author: Allan Jales
//
// Description: This class functions as a stopwatch to measure the elapsed time between two points in the code. 
// It provides functionality to start, stop, and retrieve the elapsed time in a human-readable format with appropriate units.
// -----------------------------------

#pragma once

#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;

#include <sstream>
#include <iomanip>

// Enum para controlar o nível de precisão desejado na string
enum class TimePrecision {
	Seconds,
	Milliseconds,
	Microseconds
};

class StopWatch
{
private:
	using clock = std::chrono::steady_clock;
	using time_point = clock::time_point;
	using duration = clock::duration;

	time_point begin;
	time_point end;

	/// @brief Indicates whether the timer has been stopped.
	bool hasStoppedTimer = false;
public:
	StopWatch();
	~StopWatch();

	/// @brief Starts or restarts the timer.
	void Start();

	/// @brief Stops the timer.
	void Stop();

	/// @brief Retrieves the current elapsed time as a string in old way
	/// @return A string representing the elapsed time.
	string TimeElapsedStringClassic();

	/// @brief Retrieves the current elapsed time as a string, automatically selecting the most appropriate units.
	/// @param precision The desired precision for the output string (Seconds, Milliseconds, Microseconds).
	/// @return A string representing the elapsed time.
	string TimeElapsedString(TimePrecision precision);
};

inline StopWatch::StopWatch()
{
	Start();
	end = clock::now();
}

inline StopWatch::~StopWatch()
{}

inline void StopWatch::Start()
{
	begin = clock::now();
	hasStoppedTimer = false;
}

inline void StopWatch::Stop()
{
	hasStoppedTimer = true;
	end = clock::now();
}

/// @brief Retrieves the current elapsed time as a string, automatically selecting the most appropriate units.
/// @return A string representing the elapsed time.
inline std::string StopWatch::TimeElapsedString(TimePrecision precision = TimePrecision::Microseconds)
{
	// If it was not stopped, the time elapsed is from the start to now
	if (!hasStoppedTimer)
		end = clock::now();

	auto elapsed = end - begin;

	// Extract each unit from the total elapsed time
	auto h = std::chrono::duration_cast<std::chrono::hours>(elapsed);
	auto m = std::chrono::duration_cast<std::chrono::minutes>(elapsed % std::chrono::hours(1));
	auto s = std::chrono::duration_cast<std::chrono::seconds>(elapsed % std::chrono::minutes(1));

	std::ostringstream oss;

	// Base format: HH:MM:SS (always with 2 digits and zero-padded)
	oss << std::setfill('0') 
		<< std::setw(2) << h.count() << ":"
		<< std::setw(2) << m.count() << ":"
		<< std::setw(2) << s.count();

	// Adds fractions depending on the requested precision
	if (precision == TimePrecision::Milliseconds) 
	{
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed % std::chrono::seconds(1));
		oss << "." << std::setfill('0') << std::setw(3) << ms.count();
	} 
	else if (precision == TimePrecision::Microseconds) 
	{
		auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed % std::chrono::seconds(1));
		oss << "." << std::setfill('0') << std::setw(6) << us.count();
	}

	return oss.str();
}


inline string StopWatch::TimeElapsedStringClassic()
{
	// If wasn't stopped, then elapsed time is from begin to now
	if (!hasStoppedTimer)
		end = clock::now();
	
	duration elapsed = end - begin;
	
	// Extract each unit and subtract from the remaining time
	auto h  = chrono::duration_cast<chrono::hours>(elapsed);
	elapsed -= h;
	auto m  = chrono::duration_cast<chrono::minutes>(elapsed);
	elapsed -= m;
	auto s  = chrono::duration_cast<chrono::seconds>(elapsed);
	elapsed -= s;
	auto ms = chrono::duration_cast<chrono::milliseconds>(elapsed);
	elapsed -= ms;
	auto us = chrono::duration_cast<chrono::microseconds>(elapsed);
	elapsed -= us;
	auto ns = chrono::duration_cast<chrono::nanoseconds>(elapsed);

	ostringstream oss;
	bool hasPrintedLargerUnit = false;

	// [&] capture all variables in this scope by reference
	auto printUnit = [&](auto value, const char* unit) 
	{
		if (hasPrintedLargerUnit)
			oss << " ";

		if (value > 0 || hasPrintedLargerUnit) 
		{
			oss << value << unit;
			hasPrintedLargerUnit = true;
		}
	};

	// Try to print units from largest to smallest
	printUnit(h.count(),  " h");
	printUnit(m.count(),  " min");
	printUnit(s.count(),  " s");
	printUnit(ms.count(), " ms");
	if (ms.count() <= 0)
		printUnit(us.count(), " us");
	if (us.count() <= 0)
		printUnit(ns.count(), " ns");
	
	return oss.str();
}