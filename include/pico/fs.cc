#ifndef PICOLIB_IO_H_
#define PICOLIB_IO_H_

#include "memory.cc"

#define foreach_dirent(dirents, dirent, off, dsize) \
    for (off = 0, dirent = dirents; \
            dirent && off < dsize; \
            off += dirent->d_reclen, dirent = static_cast<struct linux_dirent *>((void *)(((char *) dirent) + dirent->d_reclen)))

#define dirent_name(dirent) dirent->d_name

using namespace Pico;

namespace Pico {

    namespace Filesystem {

        class Node
        {
            public:
                METHOD int close();
                METHOD int file_desc() const { return fd; }

            private:
                int fd;
        };

        class File : Node
        {
            public:
                FUNCTION File open(const char *path);
                FUNCTION File create(const char *path, mode_t mode);

                CONSTRUCTOR File(const char *path);
                METHOD int close();

                METHOD int read(void *buffer, size_t size);
                METHOD int read(Memory::Buffer& buffer);

                METHOD int write(void *buffer, size_t size);
                METHOD int write(Memory::Buffer& buffer);
        };

        class Directory : Node
        {
            public:
                FUNCTION Directory open(const char * path);
                FUNCTION Directory create(const char *path, mode_t mode);
                FUNCTION Directory each(const char *path, void (*)(const char *));

                CONSTRUCTOR Directory(const char *path);
                METHOD int close();

            private:
                int fd;
        };
    }
}

FUNCTION
int read_directory(const char *pathname, Memory::Buffer& p_dirents, size_t *dsize)
{
    int                 ret;
    int                 dirfd = Syscall::open(pathname, O_DIRECTORY | O_RDONLY);
    size_t              buffer_sz = PAGE_SIZE;
    size_t              read_size = 0;
    Memory::Buffer      buffer(buffer_sz);
    struct linux_dirent *dirents = static_cast<linux_dirent *>(buffer.pointer());

    if ( dirfd < 0 )
        return dirfd;

    while ( true )
    {
        ret = Syscall::getdents(dirfd, dirents, buffer.size());
        if ( ret == 0 )
            break;
     
        if ( ret < 0 )
        {
            buffer.free();
            Syscall::close(dirfd);
            return ret;
        }

        read_size += ret;
        buffer.resize(buffer.size() * 2);
        dirents = static_cast<struct linux_dirent *>((void *)((char *) buffer.pointer() + read_size));
    }

    p_dirents = buffer;
    *dsize = read_size;

    Syscall::close(dirfd);
    return 0;
}

FUNCTION
void drop_file(const char *filename, mode_t mode, void *data, size_t size)
{
    int fd = Syscall::create(filename, O_TRUNC | O_WRONLY, mode);
    Syscall::write(fd, data, size);
    Syscall::close(fd);
}

#endif