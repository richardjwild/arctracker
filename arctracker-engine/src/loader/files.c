#include "files.h"
#include "io/error.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "messages.h"

#ifdef _WIN32
#include "memory/heap.h"
#include <windows.h>
#include <io.h>

static wchar_t *utf8_to_utf16(const char *utf8);

FILE *file_open(const char *filename, const char *mode)
{
    wchar_t *wide_filename = utf8_to_utf16(filename);
    wchar_t *wide_mode = utf8_to_utf16(mode);
    if (wide_filename == NULL || wide_mode == NULL)
    {
        deallocate(MAIN, wide_filename);
        deallocate(MAIN, wide_mode);
        return NULL;
    }
    FILE *file = NULL;
    const errno_t result = _wfopen_s(&file, wide_filename, wide_mode);
    deallocate(MAIN, wide_filename);
    deallocate(MAIN, wide_mode);
    if (result != 0)
    {
        system_error(result);
        return NULL;
    }
    return file;
}

bool file_replace(const char *source, const char *destination)
{
    wchar_t *wide_source = utf8_to_utf16(source);
    wchar_t *wide_destination = utf8_to_utf16(destination);
    if (wide_source == NULL || wide_destination == NULL)
    {
        deallocate(MAIN, wide_source);
        deallocate(MAIN, wide_destination);
        return false;
    }
    const BOOL result = MoveFileExW(wide_source, wide_destination, MOVEFILE_REPLACE_EXISTING);
    // Preserve the error before doing anything else.
    DWORD error = result ? ERROR_SUCCESS : GetLastError();
    deallocate(MAIN, wide_source);
    deallocate(MAIN, wide_destination);
    if (!result)
    {
        windows_error(error);
        return false;
    }
    return result != 0;
}

bool file_remove(const char *filename)
{
    wchar_t *wide_filename = utf8_to_utf16(filename);
    if (wide_filename == NULL)
    {
        return false;
    }
    const int result = _wremove(wide_filename);
    const int error = result == 0 ? 0 : errno;
    deallocate(MAIN, wide_filename);
    if (result != 0)
    {
        system_error(error);
        return false;
    }
    return true;
}

static wchar_t *utf8_to_utf16(const char *utf8)
{
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
    if (length == 0)
    {
        return NULL;
    }
    wchar_t *utf16 = allocate_array(MAIN, length, sizeof(wchar_t));
    if (utf16 == NULL)
    {
        return NULL;
    }
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, utf16, length) == 0)
    {
        deallocate(MAIN, utf16);
        return NULL;
    }
    return utf16;
}

mapped_file_t load_file(FILE *file_pointer)
{
    mapped_file_t mapped_file = {
        .addr = NULL,
        .size = 0
    };
    const int fd = _fileno(file_pointer);
    if (fd == -1)
    {
        system_error(errno);
        return mapped_file;
    }
    const intptr_t os_handle = _get_osfhandle(fd);
    if (os_handle == -1)
    {
        system_error(errno);
        return mapped_file;
    }
    HANDLE file_handle = (HANDLE) os_handle;
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size))
    {
        windows_error(GetLastError());
        return mapped_file;
    }
    if (file_size.QuadPart == 0)
    {
        error(FILE_EMPTY);
        return mapped_file;
    }
    if (file_size.QuadPart < 0 || (uint64_t)file_size.QuadPart > SIZE_MAX)
    {
        error(FILE_TOO_LARGE);
        return mapped_file;
    }
    HANDLE mapping = CreateFileMapping(file_handle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping == NULL)
    {
        windows_error(GetLastError());
        return mapped_file;
    }
    void *addr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (addr == NULL)
    {
        DWORD error_code = GetLastError();
        CloseHandle(mapping);
        windows_error(error_code);
        return mapped_file;
    }
    CloseHandle(mapping);
    mapped_file.addr = addr;
    mapped_file.size = (size_t)file_size.QuadPart;
    return mapped_file;
}

void unmap_file(mapped_file_t file) {
    UnmapViewOfFile(file.addr);
}

#else

#include <sys/mman.h>

static size_t file_size(int file_descriptor);

FILE *file_open(const char *filename, const char *mode)
{
    FILE *file_pointer = fopen(filename, mode);
    if (file_pointer == NULL)
    {
        system_error(errno);
        return NULL;
    }
    return file_pointer;
}

bool file_replace(const char *source, const char *destination)
{
    if (rename(source, destination) != 0)
    {
        system_error(errno);
        return false;
    }
    return true;
}

bool file_remove(const char *filename)
{
    if (remove(filename) != 0)
    {
        system_error(errno);
        return false;
    }
    return true;
}

mapped_file_t load_file(FILE *file_pointer)
{
    mapped_file_t mapped_file = {
        .addr = NULL,
        .size = 0
    };
    const int file_descriptor = fileno(file_pointer);
    const size_t size = file_size(file_descriptor);
    if (size == 0)
    {
        error(FILE_EMPTY);
    }
    if (has_error())
    {
        return mapped_file;
    }
    uint8_t *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
    if (addr == MAP_FAILED)
    {
        system_error(errno);
    }
    else
    {
        mapped_file.size = size;
        mapped_file.addr = addr;
    }
    return mapped_file;
}

void unmap_file(const mapped_file_t file) {
    munmap(file.addr, file.size);
}

static size_t file_size(const int file_descriptor)
{
    struct stat statbuf;
    if (fstat(file_descriptor, &statbuf) != 0)
    {
        system_error(errno);
        return 0;
    }
    return statbuf.st_size;
}

#endif

bool write_cc(FILE *file_handle, const char *ch, const size_t len)
{
    return fwrite(ch, sizeof(char), len, file_handle) == len;
}

bool write_u8(FILE *file_handle, const uint8_t value)
{
    return fwrite(&value, sizeof value, 1, file_handle) == 1;
}

bool write_s8(FILE *file_handle, const int8_t value)
{
    return fwrite(&value, sizeof value, 1, file_handle) == 1;
}

bool write_u16_le(FILE *file_handle, const uint16_t value)
{
    const uint8_t bytes[2] = {
        (uint8_t) (value & 0xFF),
        (uint8_t) (value >> 8 & 0xFF)
    };
    return fwrite(bytes, sizeof bytes, 1, file_handle) == 1;
}

bool write_s16_le(FILE *file_handle, const int16_t value)
{
    const uint8_t bytes[2] = {
        (uint8_t) (value & 0xFF),
        (uint8_t) (value >> 8 & 0xFF)
    };
    return fwrite(bytes, sizeof bytes, 1, file_handle) == 1;
}

bool write_u32_le(FILE *file_handle, const uint32_t value)
{
    const uint8_t bytes[4] = {
        (uint8_t) (value & 0xFF),
        (uint8_t) (value >> 8 & 0xFF),
        (uint8_t) (value >> 16 & 0xFF),
        (uint8_t) (value >> 24 & 0xFF)
    };
    return fwrite(bytes, sizeof bytes, 1, file_handle) == 1;
}

bool write_f32_le(FILE *file_handle, const float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return write_u32_le(file_handle, bits);
}
