#pragma once

#include <stdexcept>
#include <string>

// Root of the application's exception hierarchy. Every error the user can
// cause maps to an HTTP status so the web layer can report it uniformly.
class AppException : public std::runtime_error {
public:
    AppException(const std::string& message, int status)
        : std::runtime_error(message), status_(status) {}

    virtual ~AppException() = default;

    int status() const noexcept { return status_; }
    virtual std::string type() const { return "AppException"; }

private:
    int status_;
};

class InvalidMediaException : public AppException {
public:
    explicit InvalidMediaException(const std::string& message)
        : AppException(message, 400) {}
    std::string type() const override { return "InvalidMediaException"; }
};

class BadRequestException : public AppException {
public:
    explicit BadRequestException(const std::string& message)
        : AppException(message, 400) {}
    std::string type() const override { return "BadRequestException"; }
};

class NotFoundException : public AppException {
public:
    explicit NotFoundException(const std::string& message)
        : AppException(message, 404) {}
    std::string type() const override { return "NotFoundException"; }
};

class DuplicateException : public AppException {
public:
    explicit DuplicateException(const std::string& message)
        : AppException(message, 409) {}
    std::string type() const override { return "DuplicateException"; }
};

class MethodNotAllowedException : public AppException {
public:
    explicit MethodNotAllowedException(const std::string& message)
        : AppException(message, 405) {}
    std::string type() const override { return "MethodNotAllowedException"; }
};

class EmptyPlaylistException : public AppException {
public:
    explicit EmptyPlaylistException(const std::string& playlistName)
        : AppException("Playlist \"" + playlistName + "\" has no tracks to export", 422) {}
    std::string type() const override { return "EmptyPlaylistException"; }
};

class StorageException : public AppException {
public:
    explicit StorageException(const std::string& message)
        : AppException(message, 500) {}
    std::string type() const override { return "StorageException"; }
};
