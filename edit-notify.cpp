#include "edit-notify.h"
#include <print>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>

#ifdef __linux__
#include <sys/inotify.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

namespace edit_notify {

using std::println;

#ifdef __linux__

///linux file handle
using f_hld = int;

struct watcher {
	f_hld inotify_file;
	void(*event_handler)();
};

std::unordered_map<fs::path, watcher> watchers;
void recurse_register(f_hld inotify_hld, fs::path path); 

void delete_watch(fs::path path) {
	if (watchers.contains(path)) {
		close(watchers[path].inotify_file);
		watchers.erase(path);
	}
}

void start_watch(fs::path path, void(*onModify)()) {
	//delete the watch if it already exists
	delete_watch(path);
	//now that the watch is definitely non-existant continue
	f_hld inotify_file = inotify_init1(IN_NONBLOCK);	
	watcher w = {inotify_file,onModify};
	watchers[path] = w;
	recurse_register(inotify_file, path);	
}

void recurse_register(f_hld inotify_hld, fs::path path) {
	auto events_to_listen = IN_CLOSE_WRITE | IN_MOVE_SELF; 
	if ( fs::is_directory(path) ) {
		for (auto const& dir_entry : fs::directory_iterator{path}) {
			//in practice, merely watching the directory is enough
			//so we don't have to watch new files as they get added.
			recurse_register(inotify_hld, dir_entry);	
			inotify_add_watch(inotify_hld, path.c_str(), events_to_listen );
		}
	} else {
			//strictly speaking it seems to be enough to watch the directory
			//but this is useful if we are passed a file 
			inotify_add_watch(inotify_hld, path.c_str(), events_to_listen );
	}
}

static char read_buffer[sizeof(struct inotify_event) + NAME_MAX + 1];


void poll() {
	//make a temp copy of watchers so watchers being mutated doesn't hurt us
	//watchers is small so it is fine to copy
	auto my_watchers = watchers;
	for (auto [path, this_watcher] : my_watchers) {
		auto [file_handle, event_handler] = this_watcher;

		//set errno to 0, so we can be sure if a problem happened or not
		//strictly speaking might be unneeded since bytes_read == -1 on new errors
		errno = 0; 
		ssize_t bytes_read = read(file_handle, &read_buffer, sizeof(read_buffer));
		if (bytes_read > 0) {
			//TODO: there is a race condition and a sleep fixes it in practice
			// the bug was using vim to edit a file would sometimes trigger an
			// event while the file wasn't there for loading
			using std::this_thread::sleep_for;
			using namespace std::chrono_literals;
			sleep_for(100ms);
			event_handler();
			continue;
		}
		
		//I don't understand why this happens sometimes, but I think we can ignore it	
		if (bytes_read == 0) continue;
		//no event happened so we can continue
		if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
		//the file_handle went bad just siliently ignore
		if (errno == EBADF) continue;
		println(stderr,
			 "unexpected error while reading inotify file; errno: {}", errno);
	}
}

#else
#warn "no support for automatic updates on this build or OS"
void start_watch(std::string dirpath, void(*onModify)()) {
	std::println(stderr, "unfortunately, automatic file changes are only supported on linux (;-;)");
}
#endif

} //end of namespace

#ifdef SELF_TEST
#include <chrono>
#include <thread>
#include <cstdlib>


using std::this_thread::sleep_for;
using namespace std::chrono_literals;

void run_poll() {
	while (true) {
		edit_notify::poll();	
		sleep_for(100ms);	
	}
}
#define TEST_PATH "/tmp/aaaaa-test-edit-notify"

void echo() {
	std::println("file modified!");
	//edit_notify::start_watch(TEST_PATH, echo); //in our actual code, we endup with this pattern.
}

int main() {
	fs::create_directory(TEST_PATH);
	std::println("create file");
	system("echo \"hehe\" >" TEST_PATH "/test.txt");
	edit_notify::start_watch(TEST_PATH, echo); //in our actual code, we endup with this pattern.

	std::jthread poll_worker(run_poll);
	poll_worker.detach();

	sleep_for(300ms);
	std::println("writing to existing file");
	system("echo \"haha\" >>" TEST_PATH "/test.txt");
	sleep_for(300ms);
	std::println("writing to new file");
	system("echo \"haha\" >>" TEST_PATH "/test2.txt");
	sleep_for(300ms);
	std::println("writing more to the new file");
	system("echo \"more\" >>" TEST_PATH "/test2.txt");
	sleep_for(300ms);
	std::println("replacing a file with a file of the same size");
	system("echo \"hehe\" >" TEST_PATH "/test.txt");
	sleep_for(700ms);	
	std::println("delete everything");
	fs::remove_all(TEST_PATH);

	return 0;
}

#endif



// vim: sw=2 ts=2
