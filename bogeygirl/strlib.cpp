#include <cctype>
#include <string_view>
#include <string>
#include <cctype>

namespace strlib {
std::string_view strip(std::string_view str) {		
	//find the start
	size_t s = 0;
	for (; s < str.size() && str[s] == ' '; ++s);
	
	//find the end
	size_t e = str.size()-1;	
	for (; e > s && str[e] == ' '; --e);
	//we want a range that exclused e, so we increase e by 1
	e += 1;
	
	return {str.begin() + s, str.begin() + e};	
}

void replace(std::string& str,  char old_char,  char new_char) {
	for (char& i : str) {
		if (i == old_char) i = new_char;
	}
}

void lower(std::string& str) {
	for (char& i : str) {
		i = std::tolower(static_cast<unsigned char>(i));
	}
}

void upper(std::string& str) {
	for (char& i : str) {
		i = std::toupper(static_cast<unsigned char>(i));
	}
}

}
// vim: sw=2 ts=2
