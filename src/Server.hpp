#pragma once
#include <string>
#include <unordered_map>
#include "dyad.h"
#include "User.hpp"

/// Static ID field that gets incremented with every new user connection, giving each user a unique ID
static unsigned int m_Ids;

class Server
{
public:
    /**
     * @brief Constructs a Server instance.
     * @param port The port number the server will listen on.
     */
    explicit Server(int port);
    /**
     * @brief Starts the server, initializes Dyad, and enters the main event loop.
    */
    void start();
    /**
     * @brief Shuts down the server gracefully.
     */
    void stop();

    /**
     * @brief Cleanup allocated resources
     */
    void cleanup();
private:
    /**
     * @brief Add a user to the User list (m_Users)
     * @return Pointer to the created user
     */
    User* addUser(const std::string &nickname, const dyad_Event *event);
    void removeUser(const unsigned int id);

    void broadcast(const std::string& message);
    // -- Event handlers
    /**
     * @brief Called when a new client connects to the server.
     * @param event The Dyad event structure containing connection details.
     */
    static void onAccept(dyad_Event* event);
    /**
     * @brief Called when an error occurs on a stream.
     * @param event The Dyad event structure containing the error message.
     */
    static void onError(dyad_Event* event);
    /**
     * @brief Called when a client sends data to the server.
     * @param event The Dyad event structure containing the received data.
     */
    static void onData(dyad_Event* event);
    /**
     * @brief Called when a client's connection is closed.
     * @param event The Dyad event structure.
     */
    static void onClose(dyad_Event* event);
private:
    /// Central server stream
    dyad_Stream *m_Stream;
    /// Port the server socket is bound to
    int m_Port;
    /// Is the server currently running?
    bool m_Running;
    /// std::unordered_map of pointers to connected users, with the user's ID as the key
    std::unordered_map<unsigned int, User*> m_Users;
};
