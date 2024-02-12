#include <glaze/glaze.hpp>
#include <Smyth/VFS.hh>

using namespace smyth;

namespace smyth::vfs {
struct VFSImpl : VFS {
    DBRef database;
    std::mutex lock;

    VFSImpl(DBRef database);
};

} // namespace smyth::vfs

/*
/// ====================================================================
///  Implementation
/// ====================================================================
vfs::VFSImpl::VFSImpl(smyth::DBRef database) : database(std::move(database)) {
    database->exec(R"sql(
        CREATE TABLE IF NOT EXISTS vfs (
            inode INTEGER PRIMARY KEY AUTOINCREMENT,
            flags INTEGER NOT NULL,
            parent INTEGER NOT NULL,
            data BLOB DEFAULT ""
        ) STRICT;
    )sql")
        .unwrap();

    auto stmt = database->prepare("INSERT OR IGNORE INTO vfs (inode, flags, parent) VALUES (?, ?, ?);").unwrap();
    stmt.bind(1, Inode::Root);
    stmt.bind(2, static_cast<int>(Inode::F_DIR));
    stmt.bind(3, Inode::Root);
    stmt.exec().unwrap();
}

auto vfs::VFS::Impl() {
    return static_cast<VFSImpl*>(this);
}

auto vfs::VFSImpl::CreateDirectoryImpl(
    const std::string_view path,
    const bool parents
) -> Result<> {
    return CreateAux([&](Directory& in_dir, std::string_view name, Inum parent) -> Result<> {
        return CreateInodeEntry(in_dir, name, parent, Inode::F_DIR);
    }, path, parents);
}

auto vfs::VFSImpl::CreateFileImpl(std::string_view path, bool parents) -> Result<> {
    return CreateAux([&](Directory& in_dir, std::string_view name, Inum parent) -> Result<> {
        return CreateInodeEntry(in_dir, name, parent, Inode::F_FILE);
    }, path, parents);
}

auto vfs::VFSImpl::ListDirectoryImpl(std::string_view path) -> Result<std::vector<std::string>> {
    return Err("Not implemented");
}

auto vfs::VFSImpl::RemoveImpl(std::string_view path) -> Result<> {
    return Err("Not implemented");
}

auto vfs::VFSImpl::WriteFileImpl(std::string_view path, std::string_view contents) -> Result<> {
    return Err("Not implemented");
}

/// ====================================================================
///  API
/// ====================================================================
auto vfs::VFS::Create(DBRef db) -> std::shared_ptr<VFS> {
    return std::make_shared<VFSImpl>(std::move(db));
}

void vfs::VFS::operator delete(VFS* vfs, std::destroying_delete_t) noexcept {
    vfs->Impl()->~VFSImpl();
    ::operator delete(vfs);
}

auto vfs::VFS::create_directory(std::string_view path) -> Result<> {
    std::unique_lock _{Impl()->lock};
    return Impl()->CreateDirectoryImpl(path, true);
}

auto vfs::VFS::create_file(std::string_view path) -> Result<> {
    std::unique_lock _{Impl()->lock};
    return Impl()->CreateFileImpl(path, true);
}

auto vfs::VFS::list_directory(std::string_view path) -> Result<std::vector<std::string>> {
    std::unique_lock _{Impl()->lock};
    return Impl()->ListDirectoryImpl(path);
}

auto vfs::VFS::remove(std::string_view path) -> Result<> {
    std::unique_lock _{Impl()->lock};
    return Impl()->RemoveImpl(path);
}

auto vfs::VFS::write_file(std::string_view path, std::string_view contents) -> Result<> {
    std::unique_lock _{Impl()->lock};
    return Impl()->WriteFileImpl(path, contents);
}
*/
