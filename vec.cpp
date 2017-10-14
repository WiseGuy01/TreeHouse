// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#include "vec.h"
#include "rand.h"
#include "error.h"
#include "matrix.h"
#include "string.h"
#include "json.h"
#include <cmath>
#include <cstdio>
#include <cstring>


using std::vector;



Vec::Vec(size_t n)
: m_size(n)
{
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

Vec::Vec(int n)
: m_size(n)
{
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

Vec::Vec(double d)
{
	throw Ex("Calling this method is an error");
}

Vec::Vec(const Vec& orig)
{
	m_size = orig.m_size;
	if(m_size == 0)
		m_data = NULL;
	else
	{
		m_data = new double[m_size];
		for(size_t i = 0; i < m_size; i++)
			m_data[i] = orig.m_data[i];
	}
}

Vec::Vec(const JsonNode& node)
{
	JsonListIterator it(&node);
	m_size = it.remaining();
	if(m_size == 0)
		m_data = NULL;
	else
		m_data = new double[m_size];
	size_t i = 0;
	while(it.remaining() > 0)
	{
		m_data[i++] = it.current()->asDouble();
		it.advance();
	}
}

Vec::~Vec()
{
	delete[] m_data;
}

Vec& Vec::operator=(const Vec& orig)
{
	throw Ex("Call copy instead to avoid confusion");
}

JsonNode* Vec::marshal(Json& doc)
{
	JsonNode* pList = doc.newList();
	for(size_t i = 0; i < m_size; i++)
		pList->addItem(&doc, doc.newDouble(m_data[i]));
	return pList;
}

void Vec::copy(const Vec& orig)
{
	resize(orig.m_size);
	for(size_t i = 0; i < m_size; i++)
		m_data[i] = orig.m_data[i];
}

void Vec::resize(size_t n)
{
	if(m_size == n)
		return;
	delete[] m_data;
	m_size = n;
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

void Vec::fill(const double val, size_t startPos, size_t endPos)
{
	endPos = std::min(endPos, m_size);
	for(size_t i = startPos; i < endPos; i++)
		m_data[i] = val;
}

Vec Vec::operator+(const Vec& that) const
{
	Vec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] + that[i];
	return v;
}

Vec& Vec::operator+=(const Vec& that)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] += that[i];
	return *this;
}

Vec Vec::operator-(const Vec& that) const
{
	Vec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] - that[i];
	return v;
}

Vec& Vec::operator-=(const Vec& that)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] -= that[i];
	return *this;
}

Vec Vec::operator*(double scalar) const
{
	Vec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] * scalar;
	return v;
}

Vec& Vec::operator*=(double scalar)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] *= scalar;
	return *this;
}

void Vec::set(const double* pSource, size_t n)
{
	resize(n);
	for(size_t i = 0; i < n; i++)
		(*this)[i] = *(pSource++);
}

double Vec::squaredMagnitude() const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		double d = (*this)[i];
		s += (d * d);
	}
	return s;
}

void Vec::normalize()
{
	double mag = std::sqrt(squaredMagnitude());
	if(mag < 1e-16)
		fill(std::sqrt(1.0 / m_size));
	else
		(*this) *= (1.0 / mag);
}

double Vec::squaredDistance(const Vec& that) const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		double d = (*this)[i] - that[i];
		s += (d * d);
	}
	return s;
}

void Vec::fillUniform(Rand& rand, double min, double max)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.uniform() * (max - min) + min;
}

void Vec::fillNormal(Rand& rand, double deviation)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.normal() * deviation;
}

void Vec::fillSphericalShell(Rand& rand, double radius)
{
	fillNormal(rand);
	normalize();
	if(radius != 1.0)
		(*this) *= radius;
}

void Vec::fillSphericalVolume(Rand& rand)
{
	fillSphericalShell(rand);
	(*this) *= std::pow(rand.uniform(), 1.0 / m_size);
}

void Vec::fillSimplex(Rand& rand)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.exponential();
	(*this) *= (1.0 / sum());
}

void Vec::print(std::ostream& stream) const
{
	stream << "[";
	if(m_size > 0)
		stream << to_str((*this)[0]);
	for(size_t i = 1; i < m_size; i++)
		stream << "," << to_str((*this)[i]);
	stream << "]";
}

double Vec::sum() const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
		s += (*this)[i];
	return s;
}

size_t Vec::indexOfMax(size_t startPos, size_t endPos) const
{
	endPos = std::min(m_size, endPos);
	size_t maxIndex = startPos;
	double maxValue = -1e300;
	for(size_t i = startPos; i < endPos; i++)
	{
		if((*this)[i] > maxValue)
		{
			maxIndex = i;
			maxValue = (*this)[i];
		}
	}
	return maxIndex;
}

double Vec::dotProduct(const Vec& that) const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
		s += ((*this)[i] * that[i]);
	return s;
}

double Vec::dotProductIgnoringUnknowns(const Vec& that) const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		if((*this)[i] != UNKNOWN_VALUE && that[i] != UNKNOWN_VALUE)
			s += ((*this)[i] * that[i]);
	}
	return s;
}

double Vec::estimateSquaredDistanceWithUnknowns(const Vec& that) const
{
	double dist = 0;
	double d;
	size_t nMissing = 0;
	for(size_t n = 0; n < m_size; n++)
	{
		if((*this)[n] == UNKNOWN_VALUE || that[n] == UNKNOWN_VALUE)
			nMissing++;
		else
		{
			d = (*this)[n] - that[n];
			dist += (d * d);
		}
	}
	if(nMissing >= m_size)
		return 1e50; // we have no info, so let's make a wild guess
	else
		return dist * m_size / (m_size - nMissing);
}

void Vec::addScaled(double scalar, const Vec& that)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] += (scalar * that[i]);
}

void Vec::regularize_L1(double amount)
{
	for(size_t i = 0; i < m_size; i++)
	{
		if((*this)[i] < 0.0)
			(*this)[i] = std::min(0.0, (*this)[i] + amount);
		else
			(*this)[i] = std::max(0.0, (*this)[i] - amount);
	}
}

void Vec::put(size_t pos, const Vec& that, size_t start, size_t length)
{
	if(length == (size_t)-1)
		length = that.size() - start;
	else if(start + length > that.size())
		throw Ex("Input out of range. that size=", to_str(that.size()), ", start=", to_str(start), ", length=", to_str(length));
	if(pos + length > m_size)
		throw Ex("Out of range. this size=", to_str(m_size), ", pos=", to_str(pos), ", that size=", to_str(that.m_size));
	for(size_t i = 0; i < length; i++)
		(*this)[pos + i] = that[start + i];
}

void Vec::erase(size_t start, size_t count)
{
	if(start + count > m_size)
		throw Ex("out of range");
	size_t end = m_size - count;
	for(size_t i = start; i < end; i++)
		(*this)[i] = (*this)[i + count];
	m_size -= count;
}

double Vec::correlation(const Vec& that) const
{
	double d = this->dotProduct(that);
	if(d == 0.0)
		return 0.0;
	return d / (sqrt(this->squaredMagnitude() * that.squaredMagnitude()));
}



