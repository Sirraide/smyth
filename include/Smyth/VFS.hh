#ifndef SMYTH_VFS_HH
#define SMYTH_VFS_HH

#include <Smyth/Database.hh>
#include <mutex>

namespace smyth::vfs {
/// ID that refers to a file or directory.
struct FileID {
    u64 value{};
    static constexpr FileID Root() { return FileID{0}; }
};

/// Virtual file system used to manage files within a Smyth project.
///
/// Since all VFS accesses are synchronous, we don’t have to worry
/// about race conditions with file accesses or anything like that.
class VFS : public std::enable_shared_from_this<VFS> {
public:
    SMYTH_IMMOVABLE(VFS);

protected:
    VFS() = default;

public:
    void operator delete(VFS*, std::destroying_delete_t);

    /// Create a new VFS.
    static auto Create(DBRef db) -> std::shared_ptr<VFS>;

private:
    auto Impl();
};
}

#endif // SMYTH_VFS_HH
