#ifndef __STRINGS_H
#define __STRINGS_H

#include <string>
#include <vector>

class StringTools {

public:

	/** Splits the string at the specified separator character into parts.
	 * If the separator character appears twice in a row it is replaced by one character and appended to the previous part.
	 * Ideally splits strings that have been assembled using the join() method.
	 * This method guarantees that there is at least one part even if the string is empty or null.
	 * @param str
	 * @param c
	 * @return
	 */
	static void split(std::string str, char c, std::vector<std::string>& results);
	
	/** Joins the strings separated by the character c.
	 * If the character c appears in one of the strings it is replaced by twice c in order to
	 * allow splitting of the string using the split() method.
	 * The strings may be of any type. This method uses the toString() method. If an object is null
	 * it will be represented in the output by an empty part.
	 * @param c
	 * @param strings
	 * @return
	 */
	static std::string join(char c, std::vector<std::string>);

	static std::string join(char c, std::string s1, std::string s2);
	static std::string join(char c, std::string s1, std::string s2, std::string s3);
	static std::string join(char c, std::string s1, std::string s2, std::string s3, std::string s4);
	static std::string join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5);
	static std::string join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5, std::string s6);
	static std::string join(char c, std::string s1, std::string s2, std::string s3, std::string s4, std::string s5, std::string s6, std::string s7);

	/** Joins the strings separated by the character c, skipping the first skip objects.
	 * If the character c appears in one of the strings it is replaced by twice c in order to
	 * allow splitting of the string using the split() method.
	 * The strings may be of any type. This method uses the toString() method. If an object is null
	 * it will be represented in the output by an empty part.
	 * @param c
	 * @param strings
	 * @return
	 */
	static std::string join(int skipBegin, int skipEnd, char c, std::vector<std::string> strings);
	
};

#endif
