// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#ifndef JSON_H
#define JSON_H

#include "error.h"
#include <iostream>
#include <stddef.h>
#include <string.h>

class JsonNode;
class Json;
class JsonObjField;
class JsonListItem;
class GJsonTokenizer;




#define BITS_PER_POINTER (sizeof(void*) * 8)
#define ALIGN_DOWN(p) (((p) / BITS_PER_POINTER) * BITS_PER_POINTER)
#define ALIGN_UP(p) ALIGN_DOWN((p) + BITS_PER_POINTER - 1)

/// Provides a heap in which to put strings or whatever
/// you need to store. If you need to allocate space for
/// a lot of small objects, it's much more efficient to
/// use this class than the C++ heap. Plus, you can
/// delete them all by simply deleting the heap. You can't,
/// however, reuse the space for individual objects in
/// this heap.
class GHeap
{
protected:
	char* m_pCurrentBlock;
	size_t m_nMinBlockSize;
	size_t m_nCurrentPos;

public:
	GHeap(size_t nMinBlockSize)
	{
		m_pCurrentBlock = NULL;
		m_nMinBlockSize = nMinBlockSize;
		m_nCurrentPos = nMinBlockSize;
	}

	GHeap(const GHeap& that)
	{
		throw Ex("This object is not intended to be copied by value");
	}

	virtual ~GHeap();

	/// Deletes all the blocks and frees up memory
	void clear();

	/// Allocate space in the heap and copy a string to it.  Returns
	/// a pointer to the string
	char* add(const char* szString)
	{
		return add(szString, (int)strlen(szString));
	}

	/// Allocate space in the heap and copy a string to it.  Returns
	/// a pointer to the string
	char* add(const char* pString, size_t nLength)
	{
		char* pNewString = allocate(nLength + 1);
		memcpy(pNewString, pString, nLength);
		pNewString[nLength] = '\0';
		return pNewString;
	}

	/// Allocate space in the heap and return a pointer to it
	char* allocate(size_t nLength)
	{
		if(m_nCurrentPos + nLength > m_nMinBlockSize)
		{
			char* pNewBlock = new char[sizeof(char*) + std::max(nLength, m_nMinBlockSize)];
			*(char**)pNewBlock = m_pCurrentBlock;
			m_pCurrentBlock = pNewBlock;
			m_nCurrentPos = 0;
		}
		char* pNewBytes = m_pCurrentBlock + sizeof(char*) + m_nCurrentPos;
		m_nCurrentPos += nLength;
		return pNewBytes;
	}

	/// Allocate space in the heap and return a pointer to it. The returned pointer
	/// will be aligned to start at a location divisible by the size of a pointer,
	/// so it will be suitable for use with placement new even on architectures that
	/// require aligned pointers.
	char* allocAligned(size_t nLength)
	{
		size_t nAlignedCurPos = ALIGN_UP(m_nCurrentPos);
		if(nAlignedCurPos + nLength > m_nMinBlockSize)
		{
			char* pNewBlock = new char[sizeof(char*) + std::max(nLength, m_nMinBlockSize)];
			*(char**)pNewBlock = m_pCurrentBlock;
			m_pCurrentBlock = pNewBlock;
			m_nCurrentPos = 0;
			nAlignedCurPos = 0;
		}
		char* pNewBytes = m_pCurrentBlock + sizeof(char*) + nAlignedCurPos;
		m_nCurrentPos = nAlignedCurPos + nLength;
		return pNewBytes;
	}
};


/// Represents a table of bits.
class GBitTable
{
protected:
	size_t m_size;
	size_t* m_pBits;

public:
	/// All bits are initialized to false
	GBitTable(size_t bitCount);
	
	///Copy Constructor
	GBitTable(const GBitTable& o);

	///Operator=
	GBitTable& operator=(const GBitTable& o);

	virtual ~GBitTable();

	/// Sets all bits to false
	void clearAll();

	/// Sets all bits to true
	void setAll();

	/// Returns the bit at index
	bool bit(size_t index) const;

	/// Sets the bit at index
	void set(size_t index);

	/// Clears the bit at index
	void unset(size_t index);

	/// Toggles the bit at index
	void toggle(size_t index);

	/// Returns true iff the bit tables are exactly equal.
	/// Returns false if the tables are not the same size.
	bool equals(const GBitTable& that) const;

	/// Returns true iff the first "count" bits are set. (Note that
	/// for most applications, it is more efficient to simply maintain
	/// a count of the number of bits that are set than to call this method.)
	bool areAllSet(size_t count);

	/// Returns true iff the first "count" bits are clear
	bool areAllClear(size_t count);
};


/// This class represents a set of characters.
class GCharSet
{
friend class GTokenizer;
protected:
	GBitTable m_bt;

public:
	/// szChars is an un-ordered set of characters (with no separator between
	/// them). The only special character is '-', which is used to indicate a
	/// range of characters if it is not the first character in the string.
	/// (So, if you want '-' in your set of characters, it should come first.)
	/// For example, the following string includes all letters: "a-zA-Z", and the
	/// following string includes all characters that might appear in a
	/// floating-point number: "-.,0-9e". (There is no way to include '\0' as
	/// a character in the set, since that character indicates the end of the
	/// string, but that is okay since '\0' should not occur in text files
	/// anyway, and this class is designed for parsing text files.)
	GCharSet(const char* szChars);

	/// Returns true iff c is in the character set
	bool find(char c) const;

	/// Returns true iff other is the same as this character set
	bool equals(const GCharSet& other) const;
};



#define GTOKENIZER_MAX_LOOKAHEAD 8

/// This is a simple tokenizer that reads a file, one token at-a-time.
/// Example usage:
///
/// GCharSet whitespace("\t\n\r ");
/// GCharSet alphanum("a-zA-Z0-9");
/// GCharSet float("-.,0-9e");
/// GCharSet commanewline(",\n");
/// GTokenizer tok(filename);
/// while(true)
/// {
/// 	tok.skip(whitespace);
/// 	if(!tok.has_more())
/// 		break;
/// 	char* mystr = tok.nextWhile(alphanum);
/// 	tok.skip(commanewline);
/// }
class GTokenizer
{
protected:
	char m_q[GTOKENIZER_MAX_LOOKAHEAD]; // a look-ahead character queue for the stream
	size_t m_qPos; // the current head of the queue (it is a revolving queue)
	size_t m_qCount; // the number of characters in the queue
	char* m_pBufStart; // a buffer where the most-recently read token is stored
	char* m_pBufPos; // the current tail of the token buffer
	char* m_pBufEnd; // the end of the capacity of the token buffer
	std::istream* m_pStream; // the stream that is the source of the data
	size_t m_lineCol; // column number
	size_t m_line; // line number

public:
	/// Opens the specified filename.
	/// charSets is a class that inherits from GCharSetHolder
	GTokenizer(const char* szFilename);

	/// Uses the provided buffer of data. (If len is 0, then it
	/// will read until a null-terminator is found.)
	GTokenizer(const char* pFile, size_t len);

	virtual ~GTokenizer();

	/// Returns whether there is more data to be read
	bool has_more();

	/// Returns the next character in the stream. Returns '\0' if there are
	/// no more characters in the stream. (This could theoretically be ambiguous if the
	/// the next character in the stream is '\0', but presumably this class
	/// is mostly used for parsing text files, and that character should not
	/// occur in a text file.)
	char peek();

	/// Peek up to GTOKENIZER_MAX_LOOKAHEAD characters ahead. If n=0, returns the next character to be read.
	/// If n=1, retuns the second character ahead to be read, and so on.
	/// If n>=GTOKENIZER_MAX_LOOKAHEAD, throws an exception.
	char peek(size_t n);

	/// Appends a string to the current token (without modifying the file), and returns
	/// the full modified token.
	char* appendToToken(const char* string);

	/// Reads until the next character would be one of the specified delimeters.
	/// The delimeter character is not read. Throws an exception if fewer than
	/// minLen characters are read.
	/// The token returned by this method will have been copied into an
	/// internal buffer, null-terminated, and a pointer to that buffer is returned.
	char* nextUntil(const GCharSet& delimeters, size_t minLen = 1);

	/// Reads until the next character would be one of the specified delimeters,
	/// and the current character is not escapeChar.
	/// The token returned by this method will have been copied into an
	/// internal buffer, null-terminated, and a pointer to that buffer is returned.
	char* nextUntilNotEscaped(char escapeChar, const GCharSet& delimeters);

	/// Reads while the character is one of the specified characters. Throws an
	/// exception if fewer than minLen characters are read.
	/// The token returned by this method will have been copied into an
	/// internal buffer, null-terminated, and a pointer to that buffer is returned.
	char* nextWhile(const GCharSet& set, size_t minLen = 1);

	/// \brief Returns the next token defined by the given delimiters.
	/// \brief Allows quoting " or ' and escapes with an escape
	/// \brief character.
	///
	/// Returns the next token delimited by the given delimiters.
	///
	/// The token may include delimiter characters if it is enclosed in quotes or
	/// the delimiters are escaped.
	///
	/// If the next token begins with single or double quotes, then the
	/// token will be delimited by the quotes. If a newline character or
	/// the end-of-file is encountered before the matching quote, then
	/// an exception is thrown. The quotation marks are included in
	/// the token.  The escape
	/// character is ignored inside quotes (unlike what would happen in
	/// C++).
	///
	/// If the first character of the token is not an apostrophe or quotation mark
	/// then it attempts to use the escape character to escape any special characters.
	/// That is, if the escape character appears, then the next character is
	/// interpreted to be part of the token. The
	/// escape character is consumed but not included in the token.
	/// Thus, if the input is (The \\\\rain\\\\ in \\\"spain\\\") (not
	/// including the parentheses) and the esapeChar is '\\', then the
	/// token read will be (The \\rain\\ in "spain").
	///
	/// No token may extend over multiple lines, thus the new-line
	/// character acts as an unescapable delimiter, no matter what set
	/// of delimiters is passed to the function.
	///
	///\param delimiters the set of delimiters used to separate tokens
	///
	///\param escapeChar the character that can be used to escape
	///                  delimiters when quoting is not active
	///
	///\return a pointer to an internal character buffer containing the
	///        null-terminated token
	char* nextArg(const GCharSet& delimiters, char escapeChar = '\\');

	/// Reads past any characters specified in the list of delimeters.
	/// If szDelimeters is NULL, then any characters <= ' ' are considered
	/// to be delimeters. (This method is similar to nextWhile, except that
	/// it does not buffer the characters it reads.)
	void skip(const GCharSet& delimeters);

	/// Skip until the next character is one of the delimeters.
	/// (This method is the same as nextUntil, except that it does not buffer what it reads.)
	void skipTo(const GCharSet& delimeters);

	/// Advances past the next 'n' characters. (Stops if the end-of-file is reached.)
	void advance(size_t n);

	/// Reads past the specified string of characters. If the characters
	/// that are read from the file do not exactly match those in the string,
	/// an exception is thrown.
	void expect(const char* szString);

	/// Returns the previously-returned token, except with any of the specified characters
	/// trimmed off of both the beginning and end of the token. For example, this method could
	/// be used to convert "  tok  " to "tok".
	/// (Calling this method will not change the value returned by tokenLength.)
	char* trim(const GCharSet& set);

	/// Returns the previously-returned token, except with any characters not in the specified set removed.
	/// (Calling this method will not change the value returned by tokenLength.)
	char* filter(const GCharSet& set);

	/// Returns the current line number. (Begins at 1. Each time a '\n' is encountered,
	/// the line number is incremented. Mac line-endings do not increment the
	/// line number.)
	size_t line();

	/// Returns the current column index, which is the number of characters that have
	/// been read since the last newline character, plus 1.
	size_t col();

	/// Returns the length of the last token that was returned.
	size_t tokenLength();

protected:
	/// Double the size of the token buffer.
	void growBuf();

	/// Returns the next character in the stream. If the next character is EOF, then it returns '\0'.
	char get();

	/// Read the next character into the token buffer.
	void bufferChar(char c);

	/// Add a '\0' to the end of the token buffer and return the token buffer.
	char* nullTerminate();
};



#ifdef WINDOWS
//	ensure compact packing
#	pragma pack(1)
#endif

///\brief Converts a JsonNode to a string
std::string to_str(const JsonNode& node);

///\brief Converts a Json to a string
std::string to_str(const Json& doc);

/// This class iterates over the items in a list node
class JsonListIterator
{
protected:
	const JsonNode* m_pList;
	JsonListItem* m_pCurrent;
	size_t m_remaining;

public:
	JsonListIterator(const JsonNode* pNode);
	~JsonListIterator();

	/// Returns the current item in the list
	JsonNode* current();

	/// Advances to the next item in the list
	void advance();

	/// Returns the number of items remaining to be visited.  When
	/// the current item in the list is the first item, the number
	/// remaining is the number of items in the list.
	size_t remaining();
};


/// Represents a single node in a DOM
class JsonNode
{
friend class Json;
friend class JsonListIterator;
public:
	enum nodetype
	{
		type_obj = 0,
		type_list,
		type_bool,
		type_int,
		type_double,
		type_string,
		type_null,
	};

private:
	int m_type;
	union
	{
		JsonObjField* m_pLastField;
		JsonListItem* m_pLastItem;
		bool m_bool;
		long long m_int;
		double m_double;
		char m_string[8]; // 8 is a bogus value
	} m_value;

	JsonNode() {}
	~JsonNode() {}

public:
	/// Returns the type of this node
	nodetype type() const
	{
		return (nodetype)m_type;
	}

	/// Returns the boolean value stored by this node. Throws if this is not a bool type
	bool asBool() const
	{
		if(m_type != type_bool)
			throw Ex("not a bool");
		return m_value.m_bool;
	}

	/// Returns the 64-bit integer value stored by this node. Throws if this is not an integer type
	long long asInt() const
	{
		if(m_type != type_int)
			throw Ex("not an int");
		return m_value.m_int;
	}

	/// Returns the double value stored by this node. Throws if this is not a double type
	double asDouble() const
	{
		if(m_type == type_double)
			return m_value.m_double;
		else if(m_type == type_int)
			return (double)m_value.m_int;
		else
			throw Ex("not a double");
		return 0.0;
	}

	/// Returns the string value stored by this node. Throws if this is not a string type
	const char* asString() const
	{
		if(m_type != type_string)
			throw Ex("not a string");
		return m_value.m_string;
	}

	/// Returns the node with the specified field name. Throws if this is not an object type. Returns
	/// NULL if this is an object type, but there is no field with the specified name
	JsonNode* fieldIfExists(const char* szName) const;

	/// Returns the node with the specified field name. Throws if this is not an object type. Throws
	/// if there is no field with the name specified by szName
	JsonNode* field(const char* szName) const
	{
		JsonNode* pNode = fieldIfExists(szName);
		if(!pNode)
			throw Ex("There is no field named ", szName);
		return pNode;
	}

	/// Adds a field with the specified name to this object. Throws if this is not an object type
	/// Returns pNode. (Yes, it returns the same node that you pass in. This is useful for
	/// writing compact marshalling code.)
	JsonNode* addField(Json* pDoc, const char* szName, JsonNode* pNode);

	/// Adds an item to a list node. Returns a pointer to the item passed in (pNode).
	JsonNode* addItem(Json* pDoc, JsonNode* pNode);

	/// Writes this node to a JSON file
	void saveJson(const char* filename) const;

	/// Writes this node in JSON format.
	void writeJson(std::ostream& stream) const;

	/// Writes this node in JSON format indented in a manner suitable for human readability.
	void writeJsonPretty(std::ostream& stream, size_t indents) const;

	/// Writes this node in JSON format, escaped in a manner suitable for hard-coding in a C/C++ string.
	size_t writeJsonCpp(std::ostream& stream, size_t col) const;

	/// Writes this node as XML
	void writeXml(std::ostream& stream, const char* szLabel) const;

protected:
	/// Reverses the order of the fiels in the object and returns
	/// the number of fields.  Assumes this JsonNode is
	/// a object node.  Behavior is undefined if it is not an object
	/// node.
	///
	/// \note This method is hackishly marked const because it is always used twice, such that it has no net effect.
	///
	/// \return The number of fields
	size_t reverseFieldOrder() const;

	/// Reverses the order of the items in the list and returns
	/// the number of items in the list.  Assumes this JsonNode is
	/// a list node.  Behavior is undefined if it is not a list
	/// node.
	///
	/// \note This method is hackishly marked const because it is always used twice, such that it has no net effect.
	///
	/// \return The number of items in the list
	size_t reverseItemOrder() const;

	void writeXmlInlineValue(std::ostream& stream);
};
#ifdef WINDOWS
//	reset packing to the default
#	pragma pack()
#endif



/// A Document Object Model. This represents a document as a hierarchy of objects.
/// The DOM can be loaded-from or saved-to a file in JSON (JavaScript Object Notation)
/// format. (See http://json.org.) In the future, support for XML and/or other formats
/// may be added.
class Json
{
friend class JsonNode;
protected:
	GHeap m_heap;
	const JsonNode* m_pRoot;
	int m_line;
	size_t m_len;
	const char* m_pDoc;

public:
	Json();
	~Json();

	/// Clears the DOM.
	void clear();

	/// Load from the specified file in JSON format. (See http://json.org.)
	void loadJson(const char* szFilename);

	/// Saves to a file in JSON format. (See http://json.org.)
	void saveJson(const char* szFilename) const;

	/// Parses a JSON string. The resulting DOM can be retrieved by calling root().
	void parseJson(const char* pJsonString, size_t len);

	/// Writes this doc to the specified stream in JSON format. (See http://json.org.)
	/// (If you want to write to a memory buffer, you can use open_memstream.)
	void writeJson(std::ostream& stream) const;

	/// Writes this doc to the specified stream in JSON format with indentation to make it human-readable.
	/// (If you want to write to a memory buffer, you can use open_memstream.)
	void writeJsonPretty(std::ostream& stream) const;

	/// Writes this doc to the specified stream as an inlined C++ string in JSON format.
	/// (This method would be useful for hard-coding a serialized object in a C++ program.)
	void writeJsonCpp(std::ostream& stream) const;

	/// Write as XML to the specified stream.
	void writeXml(std::ostream& stream) const;

	/// Gets the root document node
	const JsonNode* root() const { return m_pRoot; }

	/// Sets the root document node. (Returns the same node that you pass in.)
	const JsonNode* setRoot(const JsonNode* pNode) { m_pRoot = pNode; return pNode; }

	/// Makes a new object node
	JsonNode* newObj();

	/// Makes a new list node
	JsonNode* newList();

	/// Makes a new node to represent a null value
	JsonNode* newNull();

	/// Makes a new boolean node
	JsonNode* newBool(bool b);

	/// Makes a new integer node
	JsonNode* newInt(long long n);

	/// Makes a new double node
	JsonNode* newDouble(double d);

	/// Makes a new string node from a null-terminated string. (If you want
	/// to parse a JSON string, call parseJson instead. This method just wraps
	/// the string in a node.)
	JsonNode* newString(const char* szString);

	/// Makes a new string node from the specified string segment
	JsonNode* newString(const char* pString, size_t len);

	/// Returns a pointer to the heap used by this doc
	GHeap* heap() { return &m_heap; }

protected:
	JsonObjField* newField();
	JsonListItem* newItem();
	JsonNode* loadJsonObject(GJsonTokenizer& tok);
	JsonNode* loadJsonArray(GJsonTokenizer& tok);
	JsonNode* loadJsonNumber(GJsonTokenizer& tok);
	JsonNode* loadJsonValue(GJsonTokenizer& tok);
	char* loadJsonString(GJsonTokenizer& tok);
};




#endif // JSON_H
