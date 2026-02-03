#include <filesystem>
#include <vector>
#include <stdexcept>
#include "libs/wai/whereami.h"
#include <cstdlib>

using namespace std::string_literals;
using std::filesystem::path;
namespace fs = std::filesystem;

namespace filelayer {
path get_executable_directory() {
  int length = wai_getExecutablePath(NULL, 0, NULL);
	char* filepath = (char*)malloc(length + 1);
	int dirname_length;
	wai_getExecutablePath(filepath, length, &dirname_length);
	filepath[length] = 0;
	path p(filepath);
	free(filepath);
	//get the directory the executable is in
	return p.parent_path();
}

#ifdef _WIN32

path get_home_director() {
    const char *home = getenv("USERPROFILE");
		return path;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
path get_home_director() {
	const char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
			homedir = getpwuid(getuid())->pw_dir;
	}

	return homedir;
}

#endif

static std::vector<std::filesystem::path> search_paths = {
	get_executable_directory(),
	get_home_director() / "waifuteer"
};

///find an actually existing path from the search paths we look in or throw
path canonise_path(const path& rel) {
	if (rel.is_absolute()) return fs::canonical(rel);

	for (auto&& s : search_paths) {
		auto p = s / rel;
		if (fs::exists(p)) {
			return fs::canonical(p);	
		}
	}	
	throw std::runtime_error(std::format("didn't find path: {}", rel.string()));
}
}
// vim: ts=2 sw=2
