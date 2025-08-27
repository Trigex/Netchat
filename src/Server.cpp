#include "Server.hpp"
#include <csignal>
#include <iostream>
#include <ranges>
#include <stdexcept>

extern volatile sig_atomic_t gSignalStatus;

// Helper function to trim leading/trailing whitespace (like \r\n from netcat)
static std::string trim(const std::string& str)
{
    const std::string WHITESPACE = " \n\r\t\f\v";
    const size_t first = str.find_first_not_of(WHITESPACE);
    if (std::string::npos == first) {
        return str;
    }
    const size_t last = str.find_last_not_of(WHITESPACE);
    return str.substr(first, (last - first + 1));
}

// Check if a string is empty, not including control characters
bool isEffectivelyEmpty(const std::string& str)
{
    for (const char c : str) {
        if (!std::iscntrl(static_cast<unsigned char>(c))) {
            // Found a non-control character, so the string is not effectively empty
            return false;
        }
    }
    // No non-control characters were found
    return true;
}

/**
 * @brief Retrieve a pointer to a User from a dyad_Event (Function should only should be used in event handlers attached to a client dyad_Stream)
 * @param event The event pointer passed by the event handler
 * @return Pointer to the User attached to that client stream
 */
User* getUserFromEvent(const dyad_Event *event)
{
    return static_cast<User*>(event->udata);
}

/**
 * @brief Retrieve a pointer to a Server instance from a dyad_Event (Function should only should be used in event handlers attached to the server dyad_Stream)
 * @param event The event pointer passed by the event handler
 * @return Pointer to the Server attached to the server stream
 */
Server* getServerFromEvent(const dyad_Event *event)
{
    return static_cast<Server*>(event->udata);
}

Server::Server(const int port) : m_Stream(nullptr), m_Port(port), m_Running(false) {m_Ids = 0;}

void Server::start()
{
    std::println("Starting server...");
    std::println("Initializing Dyad...");
    dyad_init();

    // Create main listening stream
    m_Stream = dyad_newStream();
    if (!m_Stream)
    {
        throw std::runtime_error("Error: dyad_newStream() failed.");
    }

    // Add event listeners for the server stream
    dyad_addListener(m_Stream, DYAD_EVENT_ERROR, onError, this);
    dyad_addListener(m_Stream, DYAD_EVENT_ACCEPT, onAccept, this);

    // Start listening for connections on the specified port
    std::println("Server starting on port {}...", m_Port);
    if (const int result = dyad_listen(m_Stream, m_Port); result < 0)
    {
        cleanup();
        throw std::runtime_error("Error: dyad_listen() failed.");
    }

    m_Running = true;
    std::println("Server is running. Waiting for connections.");

    // Time dyad_update() blocks for
    dyad_setUpdateTimeout(0.1);

    // Main event loop
    /* Run server loop while more than 1 stream is active (including the server's stream), and while we haven't explicitly shut down the server (m_Running).
     * It also depends on gSignalStatus, which is non 0 when a SIGINT is invoked (Ctrl +C), for graceful shutdown */
    while (dyad_getStreamCount() > 0 && m_Running && gSignalStatus == 0)
    {
        // dyad_update() processes all pending network events.
        dyad_update();
    }

    std::println("Shutting down server...");
    // Shutting down server, clear user list and shutdown dyad
    cleanup();
}

void Server::stop()
{
    // Setting m_running to false will cause the main loop in start() to exit.
    m_Running = false;
}

void Server::cleanup()
{
    // Free User pointer memory
    for (const auto &val: m_Users | std::views::values)
    {
        delete val;
    }

    // Clear user list
    m_Users.clear();
    // dyad shutdown does the networking cleanup work
    dyad_shutdown();
}

User* Server::addUser(const std::string &nickname, const dyad_Event *event)
{
    auto* server = getServerFromEvent(event);
    auto* user = new User{m_Ids++, dyad_getAddress(event->remote), nickname, event->remote, server};
    std::println("Adding user: {}", user->toString());
    server->m_Users[user->id] = std::move(user);
    return user;
}

void Server::removeUser(const unsigned int id)
{
    try
    {
        auto* user = m_Users.at(id);
        std::println("Removing user: {}", user->toString());
        dyad_end(user->userStream);
        delete user;
        m_Users.erase(id);
    } catch (const std::out_of_range& e)
    {
        std::println(stderr, "Error deleting User: {}", e.what());
    }
}

void Server::broadcast(const std::string &message)
{
    const std::string formattedMsg = message + "\r\n";

    for (const auto user: m_Users | std::views::values)
    {
        if (!user) continue;
        if (dyad_getState(user->userStream) == DYAD_STATE_CONNECTED)
        {
            if (!user->nick.empty()) // Only broadcast to users with nicknames
            {
                dyad_write(user->userStream, formattedMsg.c_str(), static_cast<int>(formattedMsg.length()));
            }
        }
    }
}

// (Comment to make ReSharper stop bugging me about making event handler parameters const, they can't be!)
// ReSharper disable CppParameterMayBeConstPtrOrRef
#pragma region Static event handlers

#pragma region Event handlers attached to Server stream

void Server::onAccept(dyad_Event *event)
{
    // Server object
    const auto server = getServerFromEvent(event);

    // Client connected
    const char* clientIp = dyad_getAddress(event->remote);
    std::println("[+] Client connected: {}:{}", clientIp, dyad_getPort(event->remote));

    // Create user for client
    auto* user = server->addUser("", event);

    // Add listeners to client's stream to handle its events
    dyad_addListener(event->remote, DYAD_EVENT_DATA, onData, user);
    dyad_addListener(event->remote, DYAD_EVENT_CLOSE, onClose, user);

    dyad_writef(event->remote, "Welcome to the chat! Please enter your nickname: ");
}

void Server::onError(dyad_Event *event)
{
    std::println(stderr, "Server error: {}", event->msg);
}

#pragma endregion

#pragma region Event handlers attached to individual client streams

void Server::onData(dyad_Event *event)
{
    auto* user = getUserFromEvent(event);
    if (!user) return;

    const std::string recvData = trim(std::string(event->data, event->size));

    if (user->nick == "")
    {
        // Nickname entry message
        user->nick = recvData;

        std::println("Client {} (id: {}) set nickname to {}", dyad_getAddress(user->userStream), user->id, user->nick);
        const std::string joinMsg = "'" + user->nick + "'" + " has joined the chat";
        user->server->broadcast(joinMsg);
    } else // User already has nickname, so this should be a chat message
    {
        if (!recvData.empty())
        {
            const std::string chatMsg = "[" + user->nick + "]: " + recvData;
            user->server->broadcast(chatMsg);
        } else if (isEffectivelyEmpty(recvData))
        {
            const std::string errorMsg = "[Server] Please send non-blank messages!";
            dyad_write(user->userStream, errorMsg.c_str(), errorMsg.length());
        }
    }

    std::println("Received data: {} (size: {})", trim(std::string(event->data, event->size)), event->size);
}

void Server::onClose(dyad_Event *event)
{
    const auto* user = getUserFromEvent(event);
    user->server->removeUser(user->id);
    std::println("[-] Client disconnected");
}

#pragma endregion

#pragma endregion