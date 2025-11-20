#pragma once
#include <filesystem>
namespace logger {

using fpath = std::filesystem::path;

namespace fs {

const size_t GetFileSize(fpath file_path);

}

}