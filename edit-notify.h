#include <filesystem>

namespace edit_notify {
	///calls onModify when any file in the directory is modified
	void start_watch(std::filesystem::path path, void(*onModify)());
	void delete_watch(std::filesystem::path path);
	///calls the events if an event happened
	void poll();
}
// vim: sw=2 ts=2
