//#include "stdafx.h"

#include <iterator>
#include <iostream>
#include <sstream>

#include "opdi_StringTools.h"


static std::string removeLeadingBlank(std::string sb)
{
	if (sb.size() > 0)
		if (sb[0] == ' ')
			sb.erase(0);
	return sb;
}
	
	/** Splits the string at the specified separator character into parts.
	 * If the separator character appears twice in a row it is replaced by one character and appended to the previous part.
	 * Ideally splits strings that have been assembled using the join() method.
	 * This method guarantees that there is at least one part even if the string is empty or null.
	 * @param str
	 * @param c
	 * @return
	 */
void StringTools::split(std::string str, char c, std::vector<std::string>& results)
{
	std::string sb;
	char ch; 
	char nextCh;
	results.clear();
	for (unsigned int i = 0; i < str.size(); i++) {
		ch = str[i];
		nextCh = ch + 1;
		// preview of next character
		if (i < str.size() - 1) 
			nextCh = str[i + 1];
		// two subsequent separator characters?
		if (ch == c && ch == nextCh) {
			sb.push_back(ch);
			// jump over the next one
			i++;
		} else if (ch == c) {
			// separator character
			results.push_back(removeLeadingBlank(sb));
			sb.clear();
		} else
			// other character
			sb.push_back(ch);
	}
	results.push_back(removeLeadingBlank(sb));
}

// convenience method
std::string StringTools::join(char c, std::vector<std::string> strings) {
	return StringTools::join(0, 0, c, strings);
}

/** Joins the strings separated by the character c, skipping the first skip objects.
	* If the character c appears in one of the strings it is replaced by twice c in order to
	* allow splitting of the string using the split() method.
	*/
std::string StringTools::join(int skipBegin, int skipEnd, char c, std::vector<std::string> strings) {
	std::string sb;
	for (std::vector<std::string>::iterator it = strings.begin() + skipBegin; it != strings.end() - skipEnd; ++it) {
		std::string str = (std::string)*it;
		// do not send empty strings because it messes up the separator character escape sequence
		if (str.size() == 0) str = " ";
		for (unsigned int i = 0; i < str.size(); i++) {
			char ch = str[i]; 
			// duplicate first character in case it's a blank
			// this preserves sending leading blanks
			if (i == 0 && ch == ' ') {
				sb.push_back(' ');
			}
			if (ch == c) {
				// append separator character twice
				sb.push_back(ch);
			}
			// all characters
			sb.push_back(ch);
		}
		// append separator
		sb.push_back(c);
	}
	return (sb.size() == 0 ? sb : sb.substr(0, sb.size() - 1));	// remove last separator 
}	

std::string StringTools::join(char c, std::string s1, std::string s2)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	return join(c, strings);
}
std::string StringTools::join(char c, std::string s1, std::string s2, std::string s3)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	strings.push_back(s3);
	return join(c, strings);
}
std::string StringTools::join(char c, std::string s1, std::string s2, std::string s3, std::string s4)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	strings.push_back(s3);
	strings.push_back(s4);
	return join(c, strings);
}
std::string StringTools::join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	strings.push_back(s3);
	strings.push_back(s4);
	strings.push_back(s5);
	return join(c, strings);
}
std::string StringTools::join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5, std::string s6)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	strings.push_back(s3);
	strings.push_back(s4);
	strings.push_back(s5);
	strings.push_back(s6);
	return join(c, strings);
}
std::string StringTools::join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5, std::string s6, std::string s7)
{
	std::vector<std::string> strings;
	strings.push_back(s1);
	strings.push_back(s2);
	strings.push_back(s3);
	strings.push_back(s4);
	strings.push_back(s5);
	strings.push_back(s6);
	strings.push_back(s7);
	return join(c, strings);
}


