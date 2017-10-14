// ----------------------------------------------------------------
// The contents of this file are distributed under the CC0 license.
// See http://creativecommons.org/publicdomain/zero/1.0/
// ----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "json.h"
#include <vector>
#include <deque>
#include <sstream>
#include <fstream>
#include <errno.h>
#include <string>
#include "string.h"


using std::vector;
using std::deque;

// virtual
GHeap::~GHeap()
{
	clear();
}

void GHeap::clear()
{
	while(m_pCurrentBlock)
	{
		char* pNext = *(char**)m_pCurrentBlock;
		delete[] m_pCurrentBlock;
		m_pCurrentBlock = pNext;
	}
	m_nCurrentPos = m_nMinBlockSize;
}


#define BLOCK_BITS (sizeof(size_t) * 8)


GBitTable::GBitTable(size_t bitCount)
  :m_size((bitCount + BLOCK_BITS - 1) / BLOCK_BITS),
   m_pBits(new size_t[m_size])
{
	memset(m_pBits, '\0', sizeof(size_t) * m_size);
}

GBitTable::GBitTable(const GBitTable&o)
  :m_size(o.m_size), m_pBits(new size_t[m_size])
{
	memcpy(m_pBits, o.m_pBits, sizeof(size_t) * m_size);
}

GBitTable& GBitTable::operator=(const GBitTable& o)
{
	delete[] m_pBits;
	m_size = o.m_size;
	m_pBits = new size_t[m_size];
	memcpy(m_pBits, o.m_pBits, sizeof(size_t) * m_size);
	return *this;
}


GBitTable::~GBitTable()
{
	delete[] m_pBits;
}

void GBitTable::clearAll()
{
	memset(m_pBits, '\0', sizeof(size_t) * m_size);
}

void GBitTable::setAll()
{
	memset(m_pBits, 255, sizeof(size_t) * m_size);
}

bool GBitTable::bit(size_t index) const
{
	size_t n = m_pBits[index / BLOCK_BITS];
	size_t m = index & (BLOCK_BITS - 1);
	return ((n & ((size_t)1 << m)) ? true : false);
}

void GBitTable::set(size_t index)
{
	size_t m = index & (BLOCK_BITS - 1);
	m_pBits[index / BLOCK_BITS] |= ((size_t)1 << m);
}

void GBitTable::unset(size_t index)
{
	size_t m = index & (BLOCK_BITS - 1);
	m_pBits[index / BLOCK_BITS] &= (~((size_t)1 << m));
}

void GBitTable::toggle(size_t index)
{
	size_t m = index & (BLOCK_BITS - 1);
	m_pBits[index / BLOCK_BITS] ^= ((size_t)1 << m);
}

bool GBitTable::equals(const GBitTable& that) const
{
	if(this->m_size != that.m_size)
		return false;
	for(size_t i = 0; i < m_size; i++)
	{
		if(this->m_pBits[i] != that.m_pBits[i])
			return false;
	}
	return true;
}

bool GBitTable::areAllSet(size_t count)
{
	size_t head = count / BLOCK_BITS;
	size_t tail = count % BLOCK_BITS;
	size_t* pBits = m_pBits;
	for(size_t i = 0; i < head; i++)
	{
		if(*(pBits++) != ~((size_t)0))
			return false;
	}
	if(tail > 0)
	{
		if((*pBits | ~((((size_t)1) << tail) - 1)) != ~((size_t)0))
			return false;
	}
	return true;
}

bool GBitTable::areAllClear(size_t count)
{
	size_t head = count / BLOCK_BITS;
	size_t tail = count % BLOCK_BITS;
	size_t* pBits = m_pBits;
	for(size_t i = 0; i < head; i++)
	{
		if(*(pBits++) != (size_t)0)
			return false;
	}
	if(tail > 0)
	{
		if((*pBits & ((((size_t)1) << tail) - 1)) != (size_t)0)
			return false;
	}
	return true;
}




GCharSet::GCharSet(const char* szChars)
: m_bt(256)
{
	char c = '\0';
	while(*szChars != '\0')
	{
		if(*szChars == '-')
		{
			if(c == '\0')
				m_bt.set((unsigned char)*szChars);
			else
			{
				char d = szChars[1];
				if(d <= c)
					throw Ex("invalid character range");
				for(c++; c <= d && c != 0; c++)
					m_bt.set((unsigned char)c);
				szChars++;
			}
		}
		else
			m_bt.set((unsigned char)*szChars);
		c = *szChars;
		szChars++;
	}
}

bool GCharSet::find(char c) const
{
	return m_bt.bit((unsigned char)c);
}

bool GCharSet::equals(const GCharSet& other) const
{
	return m_bt.equals(other.m_bt);
}



GTokenizer::GTokenizer(const char* szFilename)
{
	std::ifstream* pStream = new std::ifstream();
	m_pStream = pStream;
	pStream->exceptions(std::ios::badbit); // don't include std::ios::failbit here because the has_more method reads first, then checks for EOF.
	try
	{
		pStream->open(szFilename, std::ios::binary);
	}
	catch(const std::exception&)
	{
		throw Ex("Error while trying to open the file, ", szFilename, ". ", strerror(errno));
	}
	if(pStream->fail())
		throw Ex("Error while trying to open the file, ", szFilename, ". ", strerror(errno));
	m_qPos = 0;
	m_qCount = 0;
	m_pBufStart = new char[256];
	m_pBufPos = m_pBufStart;
	m_pBufEnd = m_pBufStart + 256;
	m_lineCol = 0;
	m_line = 1;
}

GTokenizer::GTokenizer(const char* pFile, size_t len)
{
	if(len > 0)
	{
		m_pStream = new std::istringstream(std::string(pFile, len));
	}
	else
	{
		m_pStream = new std::istringstream(pFile);
	}
	m_qPos = 0;
	m_qCount = 0;
	m_pBufStart = new char[256];
	m_pBufPos = m_pBufStart;
	m_pBufEnd = m_pBufStart + 256;
	m_lineCol = 0;
	m_line = 1;
}

GTokenizer::~GTokenizer()
{
	delete[] m_pBufStart;
	delete(m_pStream);
}

bool GTokenizer::has_more()
{
	if(m_qCount == 0)
	{
		int c = m_pStream->get();
		if(c == EOF)
			return false;
		m_q[m_qPos] = (char)c;
		m_qCount = 1;
	}
	return true;
}

char GTokenizer::peek()
{
	if(m_qCount == 0)
	{
		int c = m_pStream->get();
		if(c == EOF)
			return '\0';
		m_q[m_qPos] = (char)c;
		m_qCount = 1;
	}
	return m_q[m_qPos];
}

char GTokenizer::peek(size_t n)
{
	if(n >= GTOKENIZER_MAX_LOOKAHEAD)
		throw Ex("out of range");
	while(m_qCount <= n)
	{
		int c = m_pStream->get();
		if(c == EOF)
			return '\0';
		m_q[(m_qPos + m_qCount) % GTOKENIZER_MAX_LOOKAHEAD] = (char)c;
		m_qCount++;
	}
	return m_q[(m_qPos + n) % GTOKENIZER_MAX_LOOKAHEAD];
}

char GTokenizer::get()
{
	if(m_qCount == 0)
	{
		int cc = m_pStream->get();
		if(cc == EOF)
			return '\0';
		m_q[m_qPos] = (char)cc;
		m_qCount = 1;
	}
	char c = m_q[m_qPos];
	if(++m_qPos >= GTOKENIZER_MAX_LOOKAHEAD)
		m_qPos = 0;
	m_qCount--;
	if(c == '\n')
	{
		m_line++;
		m_lineCol = 0;
	} else {
		m_lineCol++;
	}
	return c;
}

void GTokenizer::growBuf()
{
	size_t len = m_pBufEnd - m_pBufStart;
	char* pNewBuf = new char[len * 2];
	m_pBufEnd = pNewBuf + (len * 2);
	memcpy(pNewBuf, m_pBufStart, len);
	m_pBufPos = pNewBuf + len;
	delete[] m_pBufStart;
	m_pBufStart = pNewBuf;
}

void GTokenizer::bufferChar(char c)
{
	if(m_pBufPos == m_pBufEnd)
		growBuf();
	*m_pBufPos = c;
	m_pBufPos++;
}

char* GTokenizer::nullTerminate()
{
	if(m_pBufPos == m_pBufEnd)
		growBuf();
	*m_pBufPos = '\0';
	return m_pBufStart;
}

char* GTokenizer::appendToToken(const char* string)
{
	while(*string != '\0')
	{
		bufferChar(*string);
		string++;
	}
	return nullTerminate();
}

char* GTokenizer::nextUntil(const GCharSet& delimeters, size_t minLen)
{
	m_pBufPos = m_pBufStart;
	while(has_more())
	{
		char c = peek();
		if(delimeters.find(c))
			break;
		c = get();
		bufferChar(c);
	}
	if((size_t)(m_pBufPos - m_pBufStart) < minLen)
		throw Ex("On line ", to_str(m_line), ", col ", to_str(col()), ", expected a token of at least size ", to_str(minLen), ", but got only ", to_str(m_pBufPos - m_pBufStart));
	return nullTerminate();
}

char* GTokenizer::nextUntilNotEscaped(char escapeChar, const GCharSet& delimeters)
{
	m_pBufPos = m_pBufStart;
	char cCur = '\0';
	while(has_more())
	{
		char c = peek();
		if(delimeters.find(c) && cCur != escapeChar)
			break;
		c = get();
		bufferChar(c);
		cCur = c;
	}
	return nullTerminate();
}

char* GTokenizer::nextWhile(const GCharSet& set, size_t minLen)
{
	m_pBufPos = m_pBufStart;
	while(has_more())
	{
		char c = peek();
		if(!set.find(c))
			break;
		c = get();
		bufferChar(c);
	}
	if((size_t)(m_pBufPos - m_pBufStart) < minLen)
		throw Ex("Unexpected token on line ", to_str(m_line), ", col ", to_str(col()));
	return nullTerminate();
}

void GTokenizer::skip(const GCharSet& delimeters)
{
	while(has_more())
	{
		char c = peek();
		if(!delimeters.find(c))
			break;
		c = get();
	}
}

void GTokenizer::skipTo(const GCharSet& delimeters)
{
	while(has_more())
	{
		char c = peek();
		if(delimeters.find(c))
			break;
		c = get();
	}
}

char* GTokenizer::nextArg(const GCharSet& delimiters, char escapeChar)
{
	m_pBufPos = m_pBufStart;
	char c = peek();
	if(c == '"')
	{
		bufferChar('"');
		advance(1);
		while(has_more())
		{
			char c2 = peek();
			if(c2 == '\"' || c2 == '\n')
				break;
			c2 = get();
			bufferChar(c2);
		}
		if(peek() != '"')
			throw Ex("Expected matching double-quotes on line ",
								 to_str(m_line), ", col ", to_str(col()));
		bufferChar('"');
		advance(1);
		while(!delimiters.find(peek()))
			advance(1);
		return nullTerminate();
	}
	else if(c == '\'')
	{
		bufferChar('\'');
		advance(1);
		while(has_more())
		{
			char c2 = peek();
			if(c2 == '\'' || c2 == '\n')
				break;
			c2 = get();
			bufferChar(c2);
		}
		if(peek() != '\'')
			throw Ex("Expected a matching single-quote on line ", to_str(m_line),
								 ", col ", to_str(col()));
		bufferChar('\'');
		advance(1);
		while(!delimiters.find(peek()))
			advance(1);
		return nullTerminate();
	}

	bool inEscapeMode = false;
	while(has_more())
	{
		char c2 = peek();
		if(inEscapeMode)
		{
			if(c2 == '\n')
			{
				throw Ex("Error: '", to_str(escapeChar), "' character used as "
									 "last character on a line to attempt to extend string over "
									 "two lines on line" , to_str(m_line), ", col ",
									 to_str(col()) );
			}
			c2 = get();
			bufferChar(c2);
			inEscapeMode = false;
		}
		else
		{
			if(c2 == '\n' || delimiters.find(c2)){ break; }
			c2 = get();
			if(c2 == escapeChar)	{	inEscapeMode = true;	}
			else { bufferChar(c2);	}
		}
	}

	return nullTerminate();
}

void GTokenizer::advance(size_t n)
{
	while(n > 0 && has_more())
	{
		get();
		n--;
	}
}

size_t GTokenizer::line()
{
	return m_line;
}

void GTokenizer::expect(const char* szString)
{
	while(*szString != '\0' && has_more())
	{
		char c = get();
		if(c != *szString)
			throw Ex("Expected \"", szString, "\" on line ", to_str(m_line), ", col ", to_str(col()));
		szString++;
	}
	if(*szString != '\0')
		throw Ex("Expected \", szString, \". Reached end-of-file instead.");
}

size_t GTokenizer::tokenLength()
{
	return m_pBufPos - m_pBufStart;
}

char* GTokenizer::trim(const GCharSet& set)
{
	char* pStart = m_pBufStart;
	while(pStart < m_pBufPos && set.find(*pStart))
		pStart++;
	for(char* pEnd = m_pBufPos - 1; pEnd >= pStart && set.find(*pEnd); pEnd--)
		*pEnd = '\0';
	return pStart;
}

char* GTokenizer::filter(const GCharSet& set)
{
	size_t end = m_pBufPos - m_pBufStart;
	for(size_t i = 0; i < end; i++)
	{
		if(!set.find(m_pBufStart[i]))
		{
			for(size_t j = i; j < end - 1; j++)
				m_pBufStart[j] = m_pBufStart[j + 1];
			end--;
			m_pBufStart[end] = '\0';
			i--;
		}
	}
	return m_pBufStart;
}

size_t GTokenizer::col()
{
	return m_lineCol;
}





class JsonObjField
{
public:
	const char* m_pName;
	JsonNode* m_pValue;
	JsonObjField* m_pPrev;
};

/// An element in a Json list
class JsonListItem
{
public:
	/// Pointer to the value contained in this list item
	JsonNode* m_pValue;

	/// Pointer to the previous node in the list
	JsonListItem* m_pPrev;
};


JsonListIterator::JsonListIterator(const JsonNode* pNode)
{
	if(pNode->m_type != JsonNode::type_list)
		throw Ex("\"", to_str(pNode), "\" is not a list type");
	m_pList = pNode;
	m_remaining = m_pList->reverseItemOrder();
	m_pCurrent = m_pList->m_value.m_pLastItem;
}

JsonListIterator::~JsonListIterator()
{
	m_pList->reverseItemOrder();
}

JsonNode* JsonListIterator::current()
{
	return m_pCurrent ? m_pCurrent->m_pValue : NULL;
}

void JsonListIterator::advance()
{
	m_pCurrent = m_pCurrent->m_pPrev;
	m_remaining--;
}

size_t JsonListIterator::remaining()
{
	return m_remaining;
}


JsonNode* JsonNode::fieldIfExists(const char* szName) const
{
	if(m_type != type_obj)
		throw Ex("\"", to_str(this), "\" is not an obj");
	JsonObjField* pField;
	for(pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
	{
		if(strcmp(szName, pField->m_pName) == 0)
			return pField->m_pValue;
	}
	return NULL;
}

size_t JsonNode::reverseFieldOrder() const
{
	size_t count = 0;
	JsonObjField* pNewHead = NULL;
	while(m_value.m_pLastField)
	{
		JsonObjField* pTemp = m_value.m_pLastField;
		((JsonNode*)this)->m_value.m_pLastField = pTemp->m_pPrev;
		pTemp->m_pPrev = pNewHead;
		pNewHead = pTemp;
		count++;
	}
	((JsonNode*)this)->m_value.m_pLastField = pNewHead;
	return count;
}

size_t JsonNode::reverseItemOrder() const
{
	size_t count = 0;
	JsonListItem* pNewHead = NULL;
	while(m_value.m_pLastItem)
	{
		JsonListItem* pTemp = m_value.m_pLastItem;
		((JsonNode*)this)->m_value.m_pLastItem = pTemp->m_pPrev;
		pTemp->m_pPrev = pNewHead;
		pNewHead = pTemp;
		count++;
	}
	((JsonNode*)this)->m_value.m_pLastItem = pNewHead;
	return count;
}

JsonNode* JsonNode::addField(Json* pDoc, const char* szName, JsonNode* pNode)
{
	if(m_type != type_obj)
		throw Ex("\"", to_str(this), "\" is not an obj");
	JsonObjField* pField = pDoc->newField();
	pField->m_pPrev = m_value.m_pLastField;
	m_value.m_pLastField = pField;
	GHeap* pHeap = pDoc->heap();
	pField->m_pName = pHeap->add(szName);
	pField->m_pValue = pNode;
	return pNode;
}

JsonNode* JsonNode::addItem(Json* pDoc, JsonNode* pNode)
{
	if(m_type != type_list)
		throw Ex("\"", to_str(this), "\" is not a list");
	JsonListItem* pItem = pDoc->newItem();
	pItem->m_pPrev = m_value.m_pLastItem;
	m_value.m_pLastItem = pItem;
	pItem->m_pValue = pNode;
	return pNode;
}

void writeJSONString(std::ostream& stream, const char* szString)
{
	stream << '"';
	while(*szString != '\0')
	{
		if(*szString < ' ')
		{
			switch(*szString)
			{
				case '\b': stream << "\\b"; break;
				case '\f': stream << "\\f"; break;
				case '\n': stream << "\\n"; break;
				case '\r': stream << "\\r"; break;
				case '\t': stream << "\\t"; break;
				default:
					stream << (*szString);
			}
		}
		else if(*szString == '\\')
			stream << "\\\\";
		else if(*szString == '"')
			stream << "\\\"";
		else
			stream << (*szString);
		szString++;
	}
	stream << '"';
}

size_t writeJSONStringCpp(std::ostream& stream, const char* szString)
{
	stream << "\\\"";
	size_t chars = 2;
	while(*szString != '\0')
	{
		if(*szString < ' ')
		{
			switch(*szString)
			{
				case '\b': stream << "\\\\b"; break;
				case '\f': stream << "\\\\f"; break;
				case '\n': stream << "\\\\n"; break;
				case '\r': stream << "\\\\r"; break;
				case '\t': stream << "\\\\t"; break;
				default:
					stream << (*szString);
			}
			chars += 3;
		}
		else if(*szString == '\\')
		{
			stream << "\\\\\\\\";
			chars += 4;
		}
		else if(*szString == '"')
		{
			stream << "\\\\\\\"";
			chars += 4;
		}
		else
		{
			stream << (*szString);
			chars++;
		}
		szString++;
	}
	stream << "\\\"";
	chars += 2;
	return chars;
}

void JsonNode::saveJson(const char* filename) const
{
	Json doc;
	doc.setRoot(this);
	doc.saveJson(filename);
}

void JsonNode::writeJson(std::ostream& stream) const
{
	std::ios_base::fmtflags oldflags = stream.flags();
	stream << std::fixed;
	switch(m_type)
	{
		case type_obj:
			stream << "{";
			reverseFieldOrder();
			for(JsonObjField* pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
			{
				if(pField != m_value.m_pLastField)
					stream << ",";
				writeJSONString(stream, pField->m_pName);
				stream << ":";
				pField->m_pValue->writeJson(stream);
			}
			reverseFieldOrder();
			stream << "}";
			break;
		case type_list:
			stream << "[";
			reverseItemOrder();
			for(JsonListItem* pItem = m_value.m_pLastItem; pItem; pItem = pItem->m_pPrev)
			{
				if(pItem != m_value.m_pLastItem)
					stream << ",";
				pItem->m_pValue->writeJson(stream);
			}
			reverseItemOrder();
			stream << "]";
			break;
		case type_bool:
			stream << (m_value.m_bool ? "true" : "false");
			break;
		case type_int:
			stream << m_value.m_int;
			break;
		case type_double:
			stream << m_value.m_double;
			break;
		case type_string:
			writeJSONString(stream, m_value.m_string);
			break;
		case type_null:
			stream << "null";
			break;
		default:
			throw Ex("Unrecognized node type");
	}
	stream.flags(oldflags);
}

void newLineAndIndent(std::ostream& stream, size_t indents)
{
	stream << "\n";
	for(size_t i = 0; i < indents; i++)
		stream << "	";
}

void JsonNode::writeJsonPretty(std::ostream& stream, size_t indents) const
{
	std::ios_base::fmtflags oldflags = stream.flags();
	stream << std::fixed;
	switch(m_type)
	{
		case type_obj:
			stream << "{";
			reverseFieldOrder();
			for(JsonObjField* pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
			{
				newLineAndIndent(stream, indents + 1); writeJSONString(stream, pField->m_pName);
				stream << ":";
				pField->m_pValue->writeJsonPretty(stream, indents + 1);
				if(pField->m_pPrev)
					stream << ",";
			}
			reverseFieldOrder();
			newLineAndIndent(stream, indents); stream << "}";
			break;
		case type_list:
			{
				reverseItemOrder();

				// Check whether all items in the list are atomic
				bool allAtomic = true;
				size_t itemCount = 0;
				for(JsonListItem* pItem = m_value.m_pLastItem; pItem && allAtomic; pItem = pItem->m_pPrev)
				{
					if(pItem->m_pValue->type() == JsonNode::type_obj || pItem->m_pValue->type() == JsonNode::type_list)
						allAtomic = false;
					if(++itemCount >= 1024)
						allAtomic = false;
				}

				// Print the items
				if(allAtomic)
				{
					// All items are atomic, so let's put them all on one line
					stream << "[";
					for(JsonListItem* pItem = m_value.m_pLastItem; pItem; pItem = pItem->m_pPrev)
					{
						pItem->m_pValue->writeJson(stream);
						if(pItem->m_pPrev)
							stream << ",";
					}
					stream << "]";
				}
				else
				{
					// Some items are non-atomic, so let's spread across multiple lines
					newLineAndIndent(stream, indents);
					stream << "[";
					for(JsonListItem* pItem = m_value.m_pLastItem; pItem; pItem = pItem->m_pPrev)
					{
						newLineAndIndent(stream, indents + 1);
						pItem->m_pValue->writeJsonPretty(stream, indents + 1);
						if(pItem->m_pPrev)
							stream << ",";
					}
					newLineAndIndent(stream, indents);
					stream << "]";
				}
				reverseItemOrder();
			}
			break;
		case type_bool:
			stream << (m_value.m_bool ? "true" : "false");
			break;
		case type_int:
			stream << m_value.m_int;
			break;
		case type_double:
			stream << m_value.m_double;
			break;
		case type_string:
			writeJSONString(stream, m_value.m_string);
			break;
		case type_null:
			stream << "null";
			break;
		default:
			throw Ex("Unrecognized node type");
	}
	stream.flags(oldflags);
}

size_t JsonNode::writeJsonCpp(std::ostream& stream, size_t col) const
{
	std::ios_base::fmtflags oldflags = stream.flags();
	stream << std::fixed;
	switch(m_type)
	{
		case type_obj:
			stream << "{";
			col++;
			reverseFieldOrder();
			for(JsonObjField* pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
			{
				if(pField != m_value.m_pLastField)
				{
					stream << ",";
					col++;
				}
				if(col >= 200)
				{
					stream << "\"\n\"";
					col = 0;
				}
				col += writeJSONStringCpp(stream, pField->m_pName);
				stream << ":";
				col++;
				col = pField->m_pValue->writeJsonCpp(stream, col);
			}
			reverseFieldOrder();
			stream << "}";
			col++;
			break;
		case type_list:
			stream << "[";
			col++;
			reverseItemOrder();
			for(JsonListItem* pItem = m_value.m_pLastItem; pItem; pItem = pItem->m_pPrev)
			{
				if(pItem != m_value.m_pLastItem)
				{
					stream << ",";
					col++;
				}
				if(col >= 200)
				{
					stream << "\"\n\"";
					col = 0;
				}
				col = pItem->m_pValue->writeJsonCpp(stream, col);
			}
			reverseItemOrder();
			stream << "]";
			col++;
			break;
		case type_bool:
			stream << (m_value.m_bool ? "true" : "false");
			col += 4;
			break;
		case type_int:
			stream << m_value.m_int;
			col += 4; // just a guess
			break;
		case type_double:
			stream << m_value.m_double;
			col += 8; // just a guess
			break;
		case type_string:
			col += writeJSONStringCpp(stream, m_value.m_string);
			break;
		case type_null:
			stream << "null";
			col += 4;
			break;
		default:
			throw Ex("Unrecognized node type");
	}
	if(col >= 200)
	{
		stream << "\"\n\"";
		col = 0;
	}
	stream.flags(oldflags);
	return col;
}

bool isXmlInlineType(int type)
{
	if(type == JsonNode::type_string ||
		type == JsonNode::type_int ||
		type == JsonNode::type_double ||
		type == JsonNode::type_bool ||
		type == JsonNode::type_null)
		return true;
	else
		return false;
}

void JsonNode::writeXmlInlineValue(std::ostream& stream)
{
	switch(m_type)
	{
		case type_string:
			stream << m_value.m_string; // todo: escape the string as necessary
			break;
		case type_int:
			stream << m_value.m_int;
			break;
		case type_double:
			stream << m_value.m_double;
			break;
		case type_bool:
			stream << (m_value.m_bool ? "true" : "false");
			break;
		case type_null:
			stream << "null";
			break;
		default:
			throw Ex("Type cannot be inlined");
	}
}

void JsonNode::writeXml(std::ostream& stream, const char* szLabel) const
{
	switch(m_type)
	{
		case type_obj:
			{
			stream << "<" << szLabel;
			reverseFieldOrder();
			size_t nonInlinedChildren = 0;
			for(JsonObjField* pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
			{
				if(isXmlInlineType(pField->m_pValue->m_type))
				{
					stream << " " << pField->m_pName << "=\"";
					pField->m_pValue->writeXmlInlineValue(stream);
					stream << "\"";
				}
				else
					nonInlinedChildren++;
			}
			if(nonInlinedChildren == 0)
				stream << " />";
			else
			{
				stream << ">";
				for(JsonObjField* pField = m_value.m_pLastField; pField; pField = pField->m_pPrev)
				{
					if(!isXmlInlineType(pField->m_pValue->m_type))
						pField->m_pValue->writeXml(stream, pField->m_pName);
				}
				stream << "</" << szLabel << ">";
			}
			reverseFieldOrder();
			}
			return;
		case type_list:
			stream << "<" << szLabel << ">";
			reverseItemOrder();
			for(JsonListItem* pItem = m_value.m_pLastItem; pItem; pItem = pItem->m_pPrev)
				pItem->m_pValue->writeXml(stream, "i");
			reverseItemOrder();
			stream << "</" << szLabel << ">";
			return;
		case type_bool:
			stream << "<" << szLabel << ">";
			stream << (m_value.m_bool ? "true" : "false");
			stream << "</" << szLabel << ">";
			break;
		case type_int:
			stream << "<" << szLabel << ">";
			stream << m_value.m_int;
			stream << "</" << szLabel << ">";
			break;
		case type_double:
			stream << "<" << szLabel << ">";
			stream << m_value.m_double;
			stream << "</" << szLabel << ">";
			break;
		case type_string:
			stream << "<" << szLabel << ">";
			stream << m_value.m_string; // todo: escape the string as necessary
			stream << "</" << szLabel << ">";
			break;
		case type_null:
			stream << "<" << szLabel << ">";
			stream << "null";
			stream << "</" << szLabel << ">";
			break;
		default:
			throw Ex("Unrecognized node type");
	}
}

// -------------------------------------------------------------------------------

class GJsonTokenizer : public GTokenizer
{
public:
	GCharSet m_whitespace, m_real, m_quot;

	GJsonTokenizer(const char* szFilename) : GTokenizer(szFilename),
	m_whitespace("\t\n\r "), m_real("-.+0-9eE"), m_quot("\"") {}
	GJsonTokenizer(const char* pFile, size_t len) : GTokenizer(pFile, len),
	m_whitespace("\t\n\r "), m_real("-.+0-9eE"), m_quot("\"") {}
	virtual ~GJsonTokenizer() {}
};

class Bogus1
{
public:
	int m_type;
	double m_double;
};

Json::Json()
: m_heap(2000), m_pRoot(NULL), m_line(0), m_len(0), m_pDoc(NULL)
{
}

Json::~Json()
{
}

void Json::clear()
{
	m_pRoot = NULL;
	m_heap.clear();
}

JsonNode* Json::newObj()
{
	JsonNode* pNewObj = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + sizeof(JsonObjField*));
	pNewObj->m_type = JsonNode::type_obj;
	pNewObj->m_value.m_pLastField = NULL;
	return pNewObj;
}

JsonNode* Json::newList()
{
	JsonNode* pNewList = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + sizeof(JsonListItem*));
	pNewList->m_type = JsonNode::type_list;
	pNewList->m_value.m_pLastItem = NULL;
	return pNewList;
}

JsonNode* Json::newNull()
{
	JsonNode* pNewNull = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double));
	pNewNull->m_type = JsonNode::type_null;
	return pNewNull;
}

JsonNode* Json::newBool(bool b)
{
	JsonNode* pNewBool = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + sizeof(bool));
	pNewBool->m_type = JsonNode::type_bool;
	pNewBool->m_value.m_bool = b;
	return pNewBool;
}

JsonNode* Json::newInt(long long n)
{
	JsonNode* pNewInt = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + sizeof(long long));
	pNewInt->m_type = JsonNode::type_int;
	pNewInt->m_value.m_int = n;
	return pNewInt;
}

JsonNode* Json::newDouble(double d)
{
	if(d >= -1.5e308 && d <= 1.5e308)
	{
		JsonNode* pNewDouble = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + sizeof(double));
		pNewDouble->m_type = JsonNode::type_double;
		pNewDouble->m_value.m_double = d;
		return pNewDouble;
	}
	else
	{
		throw Ex("Invalid value: ", to_str(d));
		return NULL;
	}
}

JsonNode* Json::newString(const char* pString, size_t len)
{
	JsonNode* pNewString = (JsonNode*)m_heap.allocAligned(offsetof(Bogus1, m_double) + len + 1);
	pNewString->m_type = JsonNode::type_string;
	memcpy(pNewString->m_value.m_string, pString, len);
	pNewString->m_value.m_string[len] = '\0';
	return pNewString;
}

JsonNode* Json::newString(const char* szString)
{
	return newString(szString, strlen(szString));
}

JsonObjField* Json::newField()
{
	return (JsonObjField*)m_heap.allocAligned(sizeof(JsonObjField));
}

JsonListItem* Json::newItem()
{
	return (JsonListItem*)m_heap.allocAligned(sizeof(JsonListItem));
}

char* Json::loadJsonString(GJsonTokenizer& tok)
{
	tok.expect("\"");
	char* szTok = tok.nextUntilNotEscaped('\\', tok.m_quot);
	tok.advance(1);
	size_t eat = 0;
	char* szString = szTok;
	while(szString[eat] != '\0')
	{
		char c = szString[eat];
		if(c == '\\')
		{
			switch(szString[eat + 1])
			{
				case '"': c = '"'; break;
				case '\\': c = '\\'; break;
				case '/': c = '/'; break;
				case 'b': c = '\b'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'u':
					c = '_';
					eat += 3;
					break;
				default:
					throw Ex("Unrecognized escape sequence");
			}
			eat++;
		}
		*szString = c;
		szString++;
	}
	*szString = '\0';
	return szTok;
}

JsonNode* Json::loadJsonObject(GJsonTokenizer& tok)
{
	tok.expect("{");
	JsonNode* pNewObj = newObj();
	bool readyForField = true;
	GCharSet& whitespace = tok.m_whitespace;
	while(tok.has_more())
	{
		tok.skip(whitespace);
		char c = tok.peek();
		if(c == '}')
		{
			tok.advance(1);
			break;
		}
		else if(c == ',')
		{
			if(readyForField)
				throw Ex("Unexpected ',' in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
			tok.advance(1);
			readyForField = true;
		}
		else if(c == '\"')
		{
			if(!readyForField)
				throw Ex("Expected a ',' before the next field in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
			JsonObjField* pNewField = newField();
			pNewField->m_pPrev = pNewObj->m_value.m_pLastField;
			pNewObj->m_value.m_pLastField = pNewField;
			pNewField->m_pName = m_heap.add(loadJsonString(tok));
			tok.skip(whitespace);
			tok.expect(":");
			tok.skip(whitespace);
			pNewField->m_pValue = loadJsonValue(tok);
			readyForField = false;
		}
		else if(c == '\0')
			throw Ex("Expected a matching '}' in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
		else
			throw Ex("Expected a '}' or a '\"' at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
	}
	return pNewObj;
}

JsonNode* Json::loadJsonArray(GJsonTokenizer& tok)
{
	tok.expect("[");
	JsonNode* pNewList = newList();
	bool readyForValue = true;
	while(tok.has_more())
	{
		tok.skip(tok.m_whitespace);
		char c = tok.peek();
		if(c == ']')
		{
			tok.advance(1);
			break;
		}
		else if(c == ',')
		{
			if(readyForValue)
				throw Ex("Unexpected ',' in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
			tok.advance(1);
			readyForValue = true;
		}
		else if(c == '\0')
			throw Ex("Expected a matching ']' in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
		else
		{
			if(!readyForValue)
				throw Ex("Expected a ',' or ']' in JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
			JsonListItem* pNewItem = newItem();
			pNewItem->m_pPrev = pNewList->m_value.m_pLastItem;
			pNewList->m_value.m_pLastItem = pNewItem;
			pNewItem->m_pValue = loadJsonValue(tok);
			readyForValue = false;
		}
	}
	return pNewList;
}

JsonNode* Json::loadJsonNumber(GJsonTokenizer& tok)
{
	char* szString = tok.nextWhile(tok.m_real);
	bool hasPeriod = false;
	for(char* szChar = szString; *szChar != '\0'; szChar++)
	{
		if(*szChar == '.')
			hasPeriod = true;
	}
	if(hasPeriod)
		return newDouble(atof(szString));
	else
	{
#ifdef WINDOWS
		return newInt(_atoi64(szString));
#else
		return newInt(strtoll(szString, (char**)NULL, 10));
#endif
	}
}

JsonNode* Json::loadJsonValue(GJsonTokenizer& tok)
{
	char c = tok.peek();
	if(c == '"')
		return newString(loadJsonString(tok));
	else if(c == '{')
		return loadJsonObject(tok);
	else if(c == '[')
		return loadJsonArray(tok);
	else if(c == 't')
	{
		tok.expect("true");
		return newBool(true);
	}
	else if(c == 'f')
	{
		tok.expect("false");
		return newBool(false);
	}
	else if(c == 'n')
	{
		tok.expect("null");
		return newNull();
	}
	else if((c >= '0' && c <= '9') || c == '-')
		return loadJsonNumber(tok);
	else if(c == '\0')
	{
		throw Ex("Unexpected end of file while parsing JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
		return NULL;
	}
	else
	{
		throw Ex("Unexpected token, \"", to_str(c), "\", while parsing JSON file at line ", to_str(tok.line()), ", col ", to_str(tok.col()));
		return NULL;
	}
}

void Json::parseJson(const char* pJsonString, size_t len)
{
	GJsonTokenizer tok(pJsonString, len);
	tok.skip(tok.m_whitespace);
	setRoot(loadJsonValue(tok));
}

void Json::loadJson(const char* szFilename)
{
	GJsonTokenizer tok(szFilename);
	tok.skip(tok.m_whitespace);
	setRoot(loadJsonValue(tok));
}

void Json::writeJson(std::ostream& stream) const
{
	if(!m_pRoot)
		throw Ex("No root node has been set");
	stream.precision(14);
	m_pRoot->writeJson(stream);
}

void Json::writeJsonPretty(std::ostream& stream) const
{
	if(!m_pRoot)
		throw Ex("No root node has been set");
	stream.precision(14);
	m_pRoot->writeJsonPretty(stream, 0);
}

void Json::writeJsonCpp(std::ostream& stream) const
{
	if(!m_pRoot)
		throw Ex("No root node has been set");
	stream.precision(14);
	stream << "const char* g_rename_me = \"";
	m_pRoot->writeJsonCpp(stream, 0);
	stream << "\";\n\n";
}

void Json::saveJson(const char* szFilename) const
{
	std::ofstream os;
	os.exceptions(std::ios::badbit | std::ios::failbit);
	try
	{
		os.open(szFilename, std::ios::binary);
	}
	catch(const std::exception&)
	{
		throw Ex("Error while trying to create the file, ", szFilename, ". ", strerror(errno));
	}
	writeJson(os);
}

void Json::writeXml(std::ostream& stream) const
{
	if(!m_pRoot)
		throw Ex("No root node has been set");
	stream.precision(14);
	stream << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>";
	m_pRoot->writeXml(stream, "root");
}

std::string to_str(const JsonNode& node)
{
	std::ostringstream os;
	node.writeJsonPretty(os, 0);
	return os.str();
}

std::string to_str(const Json& doc)
{
	std::ostringstream os;
	doc.writeJsonPretty(os);
	return os.str();
}


