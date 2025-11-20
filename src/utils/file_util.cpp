#include "utils/file_util.h"

namespace logger {

namespace fs {

const size_t GetFileSize(fpath file_path) {
    if (std::filesystem::is_regular_file(file_path)) {
        return std::filesystem::file_size(file_path);
    }
    return 0;
}

}  // namespace fs

}  // namespace logger