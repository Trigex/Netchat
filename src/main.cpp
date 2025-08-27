#include <print>
#include <string>
#include <csignal>
#include <unistd.h>
#include "Server.hpp"
#include "Client.hpp"

#define PRINT_USAGE_AND_EXIT(programName, isFailure) std::println(stderr, "Usage: {} [-server || -client] (Launches Netchat in server or client mode, respectively)", programName); std::exit(isFailure ? EXIT_FAILURE : EXIT_SUCCESS);
#define PORT 8080 // Move to cli argument later
#define ADDRESS "127.0.0.1"

volatile sig_atomic_t gSignalStatus;

void signalHandler(int signal)
{
    std::println("Received SIGINT signal, shutting down gracefully...");
    gSignalStatus = signal;
}

void exceptionHandler(const std::exception& e)
{
    std::println(stderr, "An unhandled exception occurred: ({})", e.what());
    std::println("Terminating Netchat...");
    std::exit(EXIT_FAILURE);
}

void startServerMode()
{
    try
    {
        Server server(PORT);
        server.start();
    } catch (const std::runtime_error& e)
    {
        exceptionHandler(e);
    }
}

void startClientMode()
{
    try
    {
        Client client(ADDRESS, PORT);
        client.run();
    } catch (const std::runtime_error& e)
    {
        exceptionHandler(e);
    }
}

int main(const int argc, char *argv[])
{
    // Register signal handler for SIGINT (Ctrl + C) so server / client can exit gracefully
    std::signal(SIGINT, signalHandler);

    const char *programPathname = argv[0];

    if (argc < 2) // No argument provided! GTFO
    {
        PRINT_USAGE_AND_EXIT(programPathname, true);
    }

    // Accept all forms of argument because why not, one dash, two dashes, single character, who cares
    if (const std::string modeArgument(argv[1]); modeArgument == "-server" || modeArgument == "--server" || modeArgument == "-s") // Running as a server
    {
        startServerMode();
    } else if (modeArgument == "-client" || modeArgument == "--client" || modeArgument == "-c") // Running as a client
    {
        startClientMode();
    } else if (modeArgument == "-help" || modeArgument == "--help" || modeArgument == "-h")
    {
        PRINT_USAGE_AND_EXIT(programPathname, false); // False, because it's not a failure exit if you're just invoking help duh
    } else
    {
        PRINT_USAGE_AND_EXIT(programPathname, true);
    }

    return EXIT_SUCCESS;
}
