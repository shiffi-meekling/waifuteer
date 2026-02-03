#include <string_view>
namespace strlib {
///strip whitespace and return a string view without that whitespace
std::string_view strip(std::string_view str);	
///inplace, replace one char with another, throughout
void replace(std::string& str,  char old_char,  char new_char);
///convert to lowercase in place
void lower(std::string& str) ;
///convert to uppercase in place
void upper(std::string& str) ;
}
// vim: sw=2 ts=2
