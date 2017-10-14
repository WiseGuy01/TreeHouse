// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#include "matrix.h"
#include "rand.h"
#include "error.h"
#include "string.h"
#include "vec.h"
#include "json.h"
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include <cmath>

using std::string;
using std::ifstream;
using std::map;
using std::vector;



Matrix::Matrix(const Matrix& other)
{
	throw Ex("Big objects should generally be passed by reference, not by value"); // (See the comment in the header file for more details.)
}

Matrix::Matrix(size_t r, size_t c)
{
	setSize(r, c);
}

Matrix::Matrix(const JsonNode& node)
{
	JsonListIterator it(&node);
	size_t r = it.remaining();
	if(r == 0)
		return;
	size_t c = 0;
	{
		JsonListIterator it2(it.current());
		c = it2.remaining();
	}
	setSize(0, c);
	while(it.remaining() > 0)
	{
		m_data.push_back(new Vec(*it.current()));
		it.advance();
	}
}

Matrix::~Matrix()
{
	clear();
}

JsonNode* Matrix::marshal(Json& doc)
{
	for(size_t i = 0; i < cols(); i++)
	{
		if(valueCount(i) > 0)
			throw Ex("Sorry, marshaling categorical values is not yet implemented");
	}
	JsonNode* pList = doc.newList();
	for(size_t i = 0; i < m_data.size(); i++)
		pList->addItem(&doc, row(i).marshal(doc));
	return pList;
}

void Matrix::clear()
{
	for(size_t i = 0; i < m_data.size(); i++)
		delete(m_data[i]);
	m_data.clear();
}

void Matrix::setSize(size_t rowCount, size_t columnCount)
{
	clear();

	// Set the meta-data
	m_filename = "";
	m_attr_name.resize(columnCount);
	m_str_to_enum.resize(columnCount);
	m_enum_to_str.resize(columnCount);
	for(size_t i = 0; i < columnCount; i++)
	{
		m_str_to_enum[i].clear();
		m_enum_to_str[i].clear();
	}

	newRows(rowCount);
}

void Matrix::copyMetaData(const Matrix& that)
{
	clear();
	m_attr_name = that.m_attr_name;
	m_str_to_enum = that.m_str_to_enum;
	m_enum_to_str = that.m_enum_to_str;
}

void Matrix::newColumn(size_t vals)
{
	clear();
	size_t c = cols();
	string name = "col_";
	name += to_str(c);
	m_attr_name.push_back(name);
	map <string, size_t> temp_str_to_enum;
	map <size_t, string> temp_enum_to_str;
	for(size_t i = 0; i < vals; i++)
	{
		string sVal = "val_";
		sVal += to_str(i);
		temp_str_to_enum[sVal] = i;
		temp_enum_to_str[i] = sVal;
	}
	m_str_to_enum.push_back(temp_str_to_enum);
	m_enum_to_str.push_back(temp_enum_to_str);
}

Vec& Matrix::newRow()
{
	size_t c = cols();
	if(c == 0)
		throw Ex("You must add some columns before you add any rows.");
	Vec* pNewVec = new Vec(c);
	m_data.push_back(pNewVec);
	return *pNewVec;
}

void Matrix::newRows(size_t n)
{
	m_data.reserve(m_data.size() + n);
	for(size_t i = 0; i < n; i++)
		newRow();
}

void Matrix::copy(const Matrix& that)
{
	setSize(that.rows(), that.cols());
	copyBlock(0, 0, that, 0, 0, that.rows(), that.cols());
}

double Matrix::columnMean(size_t col) const
{
	double sum = 0.0;
	size_t count = 0;
	for(size_t i = 0; i < m_data.size(); i++)
	{
		double val = (*this)[i][col];
		if(val != UNKNOWN_VALUE)
		{
			sum += val;
			count++;
		}
	}
	return sum / count;
}

double Matrix::columnMin(size_t col) const
{
	double m = 1e300;
	for(size_t i = 0; i < m_data.size(); i++)
	{
		double val = (*this)[i][col];
		if(val != UNKNOWN_VALUE)
			m = std::min(m, val);
	}
	return m;
}

double Matrix::columnMax(size_t col) const
{
	double m = -1e300;
	for(size_t i = 0; i < m_data.size(); i++)
	{
		double val = (*this)[i][col];
		if(val != UNKNOWN_VALUE)
			m = std::max(m, val);
	}
	return m;
}

double Matrix::mostCommonValue(size_t col) const
{
	map<double, size_t> counts;
	for(size_t i = 0; i < m_data.size(); i++)
	{
		double val = (*this)[i][col];
		if(val != UNKNOWN_VALUE)
		{
			map<double, size_t>::iterator pair = counts.find(val);
			if(pair == counts.end())
				counts[val] = 1;
			else
				pair->second++;
		}
	}
	size_t value_Count = 0;
	double value = 0;
	for(map<double, size_t>::iterator i = counts.begin(); i != counts.end(); i++)
	{
		if(i->second > value_Count)
		{
			value = i->first;
			value_Count = i->second;
		}
	}
	return value;
}

void Matrix::copyBlock(size_t destRow, size_t destCol, const Matrix& that, size_t rowBegin, size_t colBegin, size_t rowCount, size_t colCount)
{
	if (destRow + rowCount > rows() || destCol + colCount > cols())
		throw Ex("Out of range for destination matrix.");
	if(rowBegin + rowCount > that.rows() || colBegin + colCount > that.cols())
		throw Ex("Out of range for source matrix.");

	// Copy the specified region of meta-data
	for(size_t i = 0; i < colCount; i++)
	{
		m_attr_name[destCol + i] = that.m_attr_name[colBegin + i];
		m_str_to_enum[destCol + i] = that.m_str_to_enum[colBegin + i];
		m_enum_to_str[destCol + i] = that.m_enum_to_str[colBegin + i];
	}

	// Copy the specified region of data
	for(size_t i = 0; i < rowCount; i++)
		(*this)[destRow + i].put(destCol, that[rowBegin + i], colBegin, colCount);
}

string toLower(string strToConvert)
{
	//change each element of the string to lower case
	for(size_t i = 0; i < strToConvert.length(); i++)
		strToConvert[i] = tolower(strToConvert[i]);
	return strToConvert;//return the converted string
}

void Matrix::saveARFF(string filename) const
{
	std::ofstream s;
	s.exceptions(std::ios::failbit|std::ios::badbit);
	try
	{
		s.open(filename.c_str(), std::ios::binary);
	}
	catch(const std::exception&)
	{
		throw Ex("Error creating file: ", filename);
	}
	s.precision(10);
	s << "@RELATION " << m_filename << "\n";
	for(size_t i = 0; i < m_attr_name.size(); i++)
	{
		s << "@ATTRIBUTE " << m_attr_name[i];
		if(m_attr_name[i].size() == 0)
			s << "x";
		size_t vals = valueCount(i);
		if(vals == 0)
			s << " REAL\n";
		else
		{
			s << " {";
			for(size_t j = 0; j < vals; j++)
			{
				s << attrValue(i, j);
				if(j + 1 < vals)
					s << ",";
			}
			s << "}\n";
		}
	}
	s << "@DATA\n";
	for(size_t i = 0; i < rows(); i++)
	{
		const Vec& r = (*this)[i];
		for(size_t j = 0; j < cols(); j++)
		{
			if(r[j] == UNKNOWN_VALUE)
				s << "?";
			else
			{
				size_t vals = valueCount(j);
				if(vals == 0)
					s << to_str(r[j]);
				else
				{
					size_t val = (size_t)r[j];
					if(val >= vals)
						throw Ex("value out of range");
					s << attrValue(j, val);
				}
			}
			if(j + 1 < cols())
				s << ",";
		}
		s << "\n";
	}
}

void Matrix::loadARFF(string fileName)
{
	size_t lineNum = 0;
	string line;                 //line of input from the arff file
	ifstream inputFile;          //input stream
	map <string, size_t> tempMap;   //temp map for int->string map (attrInts)
	map <size_t, string> tempMapS;  //temp map for string->int map (attrString)
	size_t attrCount = 0;           //Count number of attributes
	clear();
	inputFile.open ( fileName.c_str() );
	if ( !inputFile )
		throw Ex ( "failed to open the file: ", fileName );
	while ( !inputFile.eof() && inputFile )
	{
		//Iterate through each line of the file
		getline ( inputFile, line );
		lineNum++;
		if ( toLower ( line ).find ( "@relation" ) == 0 )
			m_filename = line.substr ( line.find_first_of ( " " ) );
		else if ( toLower ( line ).find ( "@attribute" ) == 0 )
		{
			line = line.substr ( line.find_first_of ( " \t" ) + 1 );
			
			string attr_name;
			
			// If the attribute name is delimited by ''
			if ( line.find_first_of( "'" ) == 0 )
			{
				attr_name = line.substr ( 1 );
				attr_name = attr_name.substr ( 0, attr_name.find_first_of( "'" ) );
				line = line.substr ( attr_name.size() + 2 );
			}
			else
			{
				attr_name = line.substr( 0, line.find_first_of( " \t" ) );
				line = line.substr ( attr_name.size() );
			}
			
			m_attr_name.push_back ( attr_name );
			
			string value = line.substr ( line.find_first_not_of ( " \t" ) );
			
			tempMap.clear();
			tempMapS.clear();

			//If the attribute is nominal
			if ( value.find_first_of ( "{" ) == 0 )
			{
				size_t firstComma;
				size_t firstSpace;
				size_t firstLetter;
				value = value.substr ( 1, value.find_last_of ( "}" ) - 1 );
				size_t valCount = 0;
				string tempValue;

				//Parse the attributes--push onto the maps
				while ( ( firstComma = value.find_first_of ( "," ) ) != string::npos )
				{
					firstLetter = value.find_first_not_of ( " \t," );

					value = value.substr ( firstLetter );
					firstComma = value.find_first_of ( "," );
					firstSpace = value.find_first_of ( " \t" );
					tempMapS[valCount] = value.substr ( 0, firstComma );
					string valName = value.substr ( 0, firstComma );
					valName = valName.substr ( 0, valName.find_last_not_of(" \t") + 1);
					tempMap[valName] = valCount++;
					firstComma = ( firstComma < firstSpace &&
						firstSpace < ( firstComma + 2 ) ) ? firstSpace :
						firstComma;
					value = value.substr ( firstComma + 1 );
				}

				//Push final attribute onto the maps
				firstLetter = value.find_first_not_of ( " \t," );
				value = value.substr ( firstLetter );
				string valName = value.substr ( 0, value.find_last_not_of(" \t") + 1);
				tempMapS[valCount] = valName;
				tempMap[valName] = valCount++;
				m_str_to_enum.push_back ( tempMap );
				m_enum_to_str.push_back ( tempMapS );

				//Brian trying to keep the attribute types
				attrTypes.push_back(valCount);
			}
			else
			{
				//The attribute is continuous
				m_str_to_enum.push_back ( tempMap );
				m_enum_to_str.push_back ( tempMapS );

				//Brian trying to keep the attribute types
				attrTypes.push_back(0);
			}
			attrCount++;
		}
		else if ( toLower ( line ).find ( "@data" ) == 0 )
		{
			while ( !inputFile.eof() )
			{
				getline ( inputFile, line );
				lineNum++;
				if(line.length() == 0 || line[0] == '%' || line[0] == '\n' || line[0] == '\r')
					continue;
				size_t pos = 0;
				Vec& newrow = newRow();
				for ( size_t i = 0; i < attrCount; i++ )
				{
					size_t vals = valueCount ( i );
					size_t valStart = line.find_first_not_of ( " \t", pos );
					if(valStart == string::npos)
						throw Ex("Expected more elements on line ", to_str(lineNum));
					size_t valEnd = line.find_first_of ( ",\n\r", valStart );
					string val;
					if(valEnd == string::npos)
					{
						if(i + 1 == attrCount)
							val = line.substr( valStart );
						else
							throw Ex("Expected more elements on line ", to_str(lineNum));
					}
					else
						val = line.substr ( valStart, valEnd - valStart );
					pos = valEnd + 1;
					if ( vals > 0 ) //if the attribute is nominal...
					{
						if ( val == "?" )
							newrow[i] = UNKNOWN_VALUE;
						else
						{
							map<string, size_t>::iterator it = m_str_to_enum[i].find ( val );
							if(it == m_str_to_enum[i].end())
								throw Ex("Unrecognized enumeration value, \"", val, "\" on line ", to_str(lineNum), ", attr ", to_str(i)); 
							newrow[i] = (double)m_str_to_enum[i][val];
						}
					}
					else
					{
						// The attribute is continuous
						if ( val == "?" )
							newrow[i] = UNKNOWN_VALUE;
						else
							newrow[i] = atof( val.c_str() );
					}
				}
			}
		}
	}
}

const std::string& Matrix::attrValue(size_t attr, size_t val) const
{
	std::map<size_t, std::string>::const_iterator it = m_enum_to_str[attr].find(val);
	if(it == m_enum_to_str[attr].end())
		throw Ex("no name");
	return it->second;
}

void Matrix::fill(double val)
{
	for(size_t i = 0; i < m_data.size(); i++)
		m_data[i]->fill(val);
}

void Matrix::checkCompatibility(const Matrix& that) const
{
	size_t c = cols();
	if(that.cols() != c)
		throw Ex("Matrices have different number of columns");
	for(size_t i = 0; i < c; i++)
	{
		if(valueCount(i) != that.valueCount(i))
			throw Ex("Column ", to_str(i), " has mis-matching number of values");
	}
}

void Matrix::print(std::ostream& stream)
{
	for(size_t i = 0; i < rows(); i++)
	{
		stream << "[" << row(i)[0];
		for(size_t j = 1; j < cols(); j++)
			stream << "," << row(i)[j];
		stream << "]\n";
	}
}

Matrix* Matrix::transpose()
{
	size_t r = rows();
	size_t c = cols();
	Matrix* pTarget = new Matrix(c, r);
	for(size_t i = 0; i < c; i++)
	{
		for(size_t j = 0; j < r; j++)
			pTarget->row(i)[j] = row(j)[i];
	}
	return pTarget;
}

void Matrix::swapRows(size_t a, size_t b)
{
	std::swap(m_data[a], m_data[b]);
}

void Matrix::swapColumns(size_t nAttr1, size_t nAttr2)
{
	if(nAttr1 == nAttr2)
		return;
	std::swap(m_attr_name[nAttr1], m_attr_name[nAttr2]);
	std::swap(m_str_to_enum[nAttr1], m_str_to_enum[nAttr2]);
	std::swap(m_enum_to_str[nAttr1], m_enum_to_str[nAttr2]);
	size_t nCount = rows();
	for(size_t i = 0; i < nCount; i++)
	{
		Vec& r = row(i);
		std::swap(r[nAttr1], r[nAttr2]);
	}
}

// static
Matrix* Matrix::multiply(const Matrix& a, const Matrix& b, bool transposeA, bool transposeB)
{
	if(transposeA)
	{
		if(transposeB)
		{
			size_t dims = a.rows();
			if((size_t)b.cols() != dims)
				throw Ex("dimension mismatch");
			size_t w = b.rows();
			size_t h = a.cols();
			Matrix* pOut = new Matrix(h, w);
			for(size_t y = 0; y < h; y++)
			{
				Vec& r = pOut->row(y);
				for(size_t x = 0; x < w; x++)
				{
					const Vec& pB = b[x];
					double sum = 0;
					for(size_t i = 0; i < dims; i++)
						sum += a[i][y] * pB[i];
					r[x] = sum;
				}
			}
			return pOut;
		}
		else
		{
			size_t dims = a.rows();
			if(b.rows() != dims)
				throw Ex("dimension mismatch");
			size_t w = b.cols();
			size_t h = a.cols();
			Matrix* pOut = new Matrix(h, w);
			for(size_t y = 0; y < h; y++)
			{
				Vec& r = pOut->row(y);
				for(size_t x = 0; x < w; x++)
				{
					double sum = 0;
					for(size_t i = 0; i < dims; i++)
						sum += a[i][y] * b[i][x];
					r[x] = sum;
				}
			}
			return pOut;
		}
	}
	else
	{
		if(transposeB)
		{
			size_t dims = (size_t)a.cols();
			if((size_t)b.cols() != dims)
				throw Ex("dimension mismatch");
			size_t w = b.rows();
			size_t h = a.rows();
			Matrix* pOut = new Matrix(h, w);
			for(size_t y = 0; y < h; y++)
			{
				Vec& r = pOut->row(y);
				const Vec& pA = a[y];
				for(size_t x = 0; x < w; x++)
				{
					const Vec& pB = b[x];
					double sum = 0;
					for(size_t i = 0; i < dims; i++)
						sum += pA[i] * pB[i];
					r[x] = sum;
				}
			}
			return pOut;
		}
		else
		{
			size_t dims = (size_t)a.cols();
			if(b.rows() != dims)
				throw Ex("dimension mismatch");
			size_t w = b.cols();
			size_t h = a.rows();
			Matrix* pOut = new Matrix(h, w);
			for(size_t y = 0; y < h; y++)
			{
				Vec& r = pOut->row(y);
				const Vec& pA = a[y];
				for(size_t x = 0; x < w; x++)
				{
					double sum = 0;
					for(size_t i = 0; i < dims; i++)
						sum += pA[i] * b[i][x];
					r[x] = sum;
				}
			}
			return pOut;
		}
	}
}

Matrix& Matrix::operator*=(double scalar)
{
	for(size_t i = 0; i < rows(); i++)
	{
		Vec& r = row(i);
		for(size_t j = 0; j < cols(); j++)
			r[j] *= scalar;
	}
	return *this;
}



double Matrix::columnStDev(size_t col) const
{	
	if ((*this).attrTypes[col] == 0)
	{
		/// Calculate the mean (this is Dr. Gashler's
		/// columnMean function)
		double sum = 0.0;
		size_t count = 0;
		for(size_t i = 0; i < m_data.size(); i++)
		{
		        double val = (*this)[i][col];
		        if(val != UNKNOWN_VALUE)
		        {
		                sum += val;
		                count++;
		        }
		}
		double mean = sum / count;

		/// Use mean to calculate std. dev.
		double sumDiffSq = 0.0;
		for(size_t i = 0; i< m_data.size(); i++)
		{
			double val = (*this)[i][col];
			if(val != UNKNOWN_VALUE)
			{
				double diff = val - mean;
				double diffSq = diff*diff;
				sumDiffSq += diffSq;
			}
		}

		double rdcnd = sumDiffSq/(m_data.size()-1);
		return sqrt(rdcnd);
	}
	else
	{
		vector<size_t> countVars(m_data[0]->size(),0);
		for (size_t i = 0; i < m_data.size(); i++)
		{
			countVars[(*this)[i][col]] = countVars[(*this)[i][col]] + 1;
		}
      		double sumErr = 0.0;
      		for (size_t i = 0; i < m_data[0]->size(); i++)
		{
		        double prob = (double(countVars[i])/double(m_data.size()));
        		double probNot = (double(m_data.size() - countVars[i])/double(m_data.size()));
        		sumErr += prob*probNot;
		}
		
		return sumErr;
	}


}

