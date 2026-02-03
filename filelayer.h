#include <filesystem>
namespace filelayer {
///find an actually existing path from the search paths we look in
std::filesystem::path canonise_path(const std::filesystem::path&);

}
// vim: ts=2 sw=2
