// Author: Allan Jales
// A simple Vector 3D class

#pragma once

#include <cmath>
#include <string>

using namespace std;

class Vector3D
{
public:
	double x, y, z;

	Vector3D(double x, double y, double z)
	{
		this->x = x; this->y = y; this->z = z;
	}
	
	double Magnitude() const
	{
		return std::sqrt(x*x + y*y + z*z);
	}

	// Operators

	Vector3D operator+(const Vector3D& other) const
	{
		return Vector3D(x + other.x, y + other.y, z + other.z);
	}

	Vector3D operator-(const Vector3D& other) const
	{
		return Vector3D(x - other.x, y - other.y, z - other.z);
	}

	Vector3D operator*(double scalar) const
	{
		return Vector3D(x * scalar, y * scalar, z * scalar);
	}

	Vector3D operator/(double scalar) const
	{
		return Vector3D(x / scalar, y / scalar, z / scalar);
	}

	Vector3D& operator*=(double scalar)
	{
		x *= scalar; y *= scalar; z *= scalar;
		return *this;
	}

	Vector3D& operator/=(double scalar)
	{
		x /= scalar; y /= scalar; z /= scalar;
		return *this;
	}

	std::string toString() const
	{
		std::string str = "(";
		str += to_string(x) + ", ";
		str += to_string(y) + ", ";
		str += to_string(z) + ")";
		return str;
	}
};