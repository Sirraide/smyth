#include <Smyth/Utils.hh>

auto smyth::utils::WriteFile(const fs::path& file_path, std::string_view contents) -> Result<> {
    errno = 0;

    FileHandle file{std::fopen(file_path.c_str(), "wb"), std::fclose};
    if (not file) return Error(
        "Failed to open file '{}': {}",
        file_path,
        std::strerror(errno)
    );

    while (not contents.empty()) {
        auto written = std::fwrite(contents.data(), 1, contents.size(), &*file);
        if (written == 0 or std::ferror(&*file)) return Error( //
            "Failed to write to file '{}': {}",
            file_path,
            std::strerror(errno)
        );

        // Remove the written portion from the contents
        contents.remove_prefix(written);
    }

    return {};
}
