// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <map>
#include <string>


class Rand;
class Vec;
class Json;
class JsonNode;

#define UNKNOWN_VALUE -1e308


// This stores a matrix, A.K.A. data set, A.K.A. table. Each element is
// represented as a double value. Nominal values are represented using their
// corresponding zero-indexed enumeration value. For convenience,
// the matrix also stores some meta-data which describes the columns (or attributes)
// in the matrix. You can access the elements in the matrix using square
// brackets. (Row comes first. Column comes second. Both are zero-indexed.)
// For example:
//
// Matrix m;
// m.setSize(3, 2);
// m[0][0] = 1.0;
// m[0][1] = 1.5;
// m[1][0] = 2.3;
// m[1][1] = 3.5;
// m[2][0] = 0.0;
// m[2][1] = 1234.567;
//
class Matrix
{
private:
	// Data
	std::vector<Vec*> m_data; // matrix elements

	// Meta-data
	std::string m_filename; // the name of the file
	std::vector<std::string> m_attr_name; // the name of each attribute (or column)
	std::vector< std::map<std::string, size_t> > m_str_to_enum; // value to enumeration
	std::vector< std::map<size_t, std::string> > m_enum_to_str; // enumeration to value

public:
	/// Creates a 0x0 matrix. (Next, to give this matrix some dimensions, you should call:
	///    loadARFF,
	///    setSize,
	///    addColumn, or
	///    copyMetaData
	Matrix() {}

	Matrix(size_t rows, size_t cols);

	/// This is not a copy constructor. It just throws an exception.
	/// Why? Because many students who are not very familiar with C++ pass big objects by value
	/// when they should pass them by reference. In order to catch this bad behavior,
	/// I made this constructor throw an exception. If you want to make a copy, you will have to
	/// use the default constructor, then call "copy".
	Matrix(const Matrix& other);

	/// Unmarshaling constructor
	Matrix(const JsonNode& node);

	/// Destructor
	~Matrix();

	/// Marshals this object into a JSON DOM.
	JsonNode* marshal(Json& doc);

	/// Drops all rows.
	void clear();

	/// Loads the matrix from an ARFF file
	void loadARFF(std::string filename);

	/// Saves the matrix to an ARFF file
	void saveARFF(std::string filename) const;

	/// Makes a rows x columns matrix of *ALL CONTINUOUS VALUES*.
	/// This method wipes out any data currently in the matrix. It also
	/// wipes out any meta-data.
	void setSize(size_t rows, size_t cols);

	/// Clears this matrix and copies the meta-data from that matrix.
	/// In other words, it makes a zero-row matrix with the same number
	/// of columns as "that" matrix. You will need to call newRow or newRows
	/// to give the matrix some rows.
	void copyMetaData(const Matrix& that);

	/// Adds a column to this matrix with the specified number of values. (Use 0 for
	/// a continuous attribute.) This method also sets the number of rows to 0, so
	/// you will need to call newRow or newRows when you are done adding columns.
	void newColumn(size_t vals = 0);

	/// Adds one new row to this matrix. Returns a reference to the new row.
	Vec& newRow();

	/// Adds 'n' new rows to this matrix. (These rows are not initialized.)
	void newRows(size_t n);

	/// Copies that matrix
	void copy(const Matrix& that);

	/// Returns the number of rows in the matrix
	size_t rows() const { return m_data.size(); }

	/// Returns the number of columns (or attributes) in the matrix
	size_t cols() const { return m_attr_name.size(); }

	/// Returns the name of the specified attribute
	const std::string& attrName(size_t col) const { return m_attr_name[col]; }

	/// Returns the name of the specified value
	const std::string& attrValue(size_t attr, size_t val) const;

	/// Returns a reference to the specified row
	Vec& row(size_t index) { return *m_data[index]; }

	/// Returns a reference to the specified row
	Vec& operator [](size_t index) { return *m_data[index]; }

	/// Returns a reference to the specified row
	const Vec& operator [](size_t index) const { return *m_data[index]; }

	/// Returns the number of values associated with the specified attribute (or column)
	/// 0=continuous, 2=binary, 3=trinary, etc.
	size_t valueCount(size_t attr) const { return m_enum_to_str[attr].size(); }

	/// Returns the mean of the elements in the specified column. (Elements with the value UNKNOWN_VALUE are ignored.)
	double columnMean(size_t col) const;

	/// Returns the min elements in the specified column. (Elements with the value UNKNOWN_VALUE are ignored.)
	double columnMin(size_t col) const;

	/// Returns the min elements in the specified column. (Elements with the value UNKNOWN_VALUE are ignored.)
	double columnMax(size_t col) const;

	/// Returns the most common value in the specified column. (Elements with the value UNKNOWN_VALUE are ignored.)
	double mostCommonValue(size_t col) const;

	/// Copies the specified rectangular portion of that matrix (including relevant meta-data) into this matrix.
	void copyBlock(size_t destRow, size_t destCol, const Matrix& that, size_t rowBegin, size_t colBegin, size_t rowCount, size_t colCount);

	/// Sets every elements in the matrix to the specified value.
	void fill(double val);

	/// Throws an exception if that has a different number of columns than
	/// this, or if one of its columns has a different number of values.
	void checkCompatibility(const Matrix& that) const;

	/// Print this matrix to the specified stream. (For example, you could pass "std::cout" if you want to print to the console.
	void print(std::ostream& stream);

	/// Returns the transpose of this matrix.
	Matrix* transpose();

	/// Swaps the two specified rows.
	void swapRows(size_t a, size_t b);

	/// Swaps the two specified columns (including meta-data).
	void swapColumns(size_t nAttr1, size_t nAttr2);

	/// Multiplies two matrices.
	static Matrix* multiply(const Matrix& a, const Matrix& b, bool transposeA, bool transposeB);

	/// Multiplies all the values in this matrix by the scalar
	Matrix& operator*=(double scalar);

	/// Brian added this for fun
	/// Calculates standard deviation of the specified column.
	double columnStDev(size_t col) const;

	std::vector<size_t> attrTypes;
};


#endif // MATRIX_H
