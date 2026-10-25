#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "app.hpp"
#include "exceptions.hpp"

namespace {

void printUsage() {
    std::cout << "Usage: playlist-builder [--port N] [--host ADDR] [--data FILE] [--public DIR] [--samples DIR]\n"
                 "Defaults: --port 8080 --host 127.0.0.1 --data data/library.json --public public "
                 "--samples data/samples\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);

    int port = 8080;
    std::string host = "127.0.0.1";
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
        host = "0.0.0.0";
    }
    std::string dataFile = "data/library.json";
    std::string publicDir = "public";
    std::string samplesDir = "data/samples";

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto value = [&]() -> std::string {
                if (i + 1 >= argc) throw BadRequestException("Missing value after " + arg);
                return argv[++i];
            };
            if (arg == "--port") port = std::stoi(value());
            else if (arg == "--host") host = value();
            else if (arg == "--data") dataFile = value();
            else if (arg == "--public") publicDir = value();
            else if (arg == "--samples") samplesDir = value();
            else if (arg == "--help" || arg == "-h") {
                printUsage();
                return 0;
            } else {
                throw BadRequestException("Unknown option " + arg);
            }
        }

        if (!std::filesystem::is_regular_file(std::filesystem::path(publicDir) / "index.html")) {
            throw StorageException("Cannot find " + publicDir +
                                   "/index.html. Run the server from the project folder or pass --public.");
        }

        PlaylistApp app(dataFile, publicDir, samplesDir);
        HttpServer server(host, port, [&app](const HttpRequest& request) {
            HttpRequest copy = request;
            return app.handle(copy);
        });

        std::cout << "Playlist Builder: " << app.library().size() << " media items, "
                  << app.playlistCount() << " playlists (" << dataFile << ")\n"
                  << "Open http://" << (host == "0.0.0.0" ? "localhost" : host) << ":" << port
                  << " in your browser. Press Ctrl+C to stop." << std::endl;
        server.run();
    } catch (const AppException& e) {
        std::cerr << e.type() << ": " << e.what() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        printUsage();
        return 1;
    }
    return 0;
}
