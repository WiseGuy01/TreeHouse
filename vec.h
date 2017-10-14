// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#ifndef VEC_H
#define VEC_H

#include "error.h"
#include "rand.h"
#include <stdio.h>
#include <iostream>
#include <vector>


class Json;
class JsonNode;


/// Contains some useful functions for operating on vectors
class Vec
{
friend class VecWrapper;
protected:
	double* m_data;
	size_t m_size;

public:
	/// General-purpose constructor. n specifies the initial size of the vector.
	Vec(size_t n = 0);

	Vec(int n);

	/// Copy constructor. Copies all the values in orig.
	Vec(const Vec& orig);

	/// Unmarshaling constructor
	Vec(const JsonNode& node);

	/// Copies all the values in orig.
	~Vec();

	/// Marshals this object into a JSON DOM
	JsonNode* marshal(Json& doc);

	/// Returns the size of this vector.
	size_t size() const { return m_size; }

	/// Copies all the values in orig.
	void copy(const Vec& orig);

	/// Resizes this vector
	void resize(size_t n);

	/// Sets all the elements in this vector to val.
	void fill(const double val, size_t startPos = 0, size_t endPos = (size_t)-1);

	/// \brief Returns a reference to the specified element.
	inline double& operator [](size_t index) { return m_data[index]; }

	/// \brief Returns a const reference to the specified element
	inline const double& operator [](size_t index) const { return m_data[index]; }

	/// Returns a pointer to the raw element values.
	double* data() { return m_data; }

	/// Returns a const pointer to the raw element values.
	const double* data() const { return m_data; }

	/// Adds two vectors to make a new one.
	Vec operator+(const Vec& that) const;

	/// Adds another vector to this one.
	Vec& operator+=(const Vec& that);

	/// Subtracts a vector from this one to make a new one.
	Vec operator-(const Vec& that) const;

	/// Subtracts another vector from this one.
	Vec& operator-=(const Vec& that);

	/// Makes a scaled version of this vector.
	Vec operator*(double scalar) const;

	/// Scales this vector.
	Vec& operator*=(double scalar);

	/// Sets the data in this vector.
	void set(const double* pSource, size_t size);

	/// Returns the squared Euclidean magnitude of this vector.
	double squaredMagnitude() const;

	/// Scales this vector to have a magnitude of 1.0.
	void normalize();

	/// Returns the squared Euclidean distance between this and that vector.
	double squaredDistance(const Vec& that) const;

	/// Fills with random values from a uniform distribution.
	void fillUniform(Rand& rand, double min = 0.0, double max = 1.0);

	/// Fills with random values from a Normal distribution.
	void fillNormal(Rand& rand, double deviation = 1.0);

	/// Fills with random values on the surface of a sphere.
	void fillSphericalShell(Rand& rand, double radius = 1.0);

	/// Fills with random values uniformly distributed within a sphere of radius 1.
	void fillSphericalVolume(Rand& rand);

	/// Fills with random values uniformly distributed within a probability simplex.
	/// In other words, the values will sum to 1, will all be non-negative,
	/// and will not be biased toward or away from any of the extreme corners.
	void fillSimplex(Rand& rand);

	/// Prints a representation of this vector to the specified stream.
	void print(std::ostream& stream = std::cout) const;

	/// Returns the sum of the elements in this vector
	double sum() const;

	/// Returns the index of the max element.
	/// The returned value will be >= startPos.
	/// The returned value will be < endPos.
	size_t indexOfMax(size_t startPos = 0, size_t endPos = (size_t)-1) const;

	/// Returns the dot product of this and that.
	double dotProduct(const Vec& that) const;

	/// Returns the dot product of this and that, ignoring elements in which either vector has UNKNOWN_REAL_VALUE.
	double dotProductIgnoringUnknowns(const Vec& that) const;

	/// Estimates the squared distance between two points that may have some missing values. It assumes
	/// the distance in missing dimensions is approximately the same as the average distance in other
	/// dimensions. If there are no known dimensions that overlap between the two points, it returns
	/// 1e50.
	double estimateSquaredDistanceWithUnknowns(const Vec& that) const;

	/// Adds scalar * that to this vector.
	void addScaled(double scalar, const Vec& that);

	/// Applies L1 regularization to this vector.
	void regularize_L1(double amount);

	/// Puts a copy of that at the specified location in this.
	/// Throws an exception if it does not fit there.
	void put(size_t pos, const Vec& that, size_t start = 0, size_t length = (size_t)-1);

	/// Erases the specified elements. The remaining elements are shifted over.
	/// The size of the vector is decreased, but the buffer is not reallocated
	/// (so this operation wastes some memory to save a little time).
	void erase(size_t start, size_t count = 1);

	/// Returns the cosine of the angle between this and that (with the origin as the common vertex).
	double correlation(const Vec& that) const;

private:
	/// This method is deliberately private, so calling it will trigger a compiler error. Call "copy" instead.
	Vec& operator=(const Vec& orig);

	/// This method is deliberately private, so calling it will trigger a compiler error.
	Vec(double d);
};




#endif // VEC_H

