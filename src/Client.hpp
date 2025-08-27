#pragma once
#include <dyad.h>
#include <ncurses.h>
#include <string>
#include <vector>

class Client
{
public:
    /**
     * @brief Constructs the chat client.
     * @param host The server hostname or IP address.
     * @param port The server port.
     */
    Client(const std::string& host, int port);
    /**
     * @brief Destructor to ensure ncurses is properly shut down.
     */
    ~Client();
    /**
     * @brief Starts the client, initializes networking, and enters the main UI loop.
     */
    void run();
    void stop();
private:
    // --- Ncurses UI Methods ---
    void initNcurses();
    void recreateWindows(); // Helper function for resizing
    void drawUI();
    void handleInput(int ch);
    void addMessage(const std::string& msg);
    void cleanupNcurses();

    // --- Networking Methods ---
    void sendMessage();

    // --- Dyad Static Callbacks ---
    static void onConnect(dyad_Event* e);
    static void onError(dyad_Event* e);
    static void onData(dyad_Event* e);
    static void onClose(dyad_Event* e);
private:
    const std::string m_Host;
    int m_Port;
    bool m_Running;
    bool m_UIDirty; // Flag to track if the UI needs a redraw

    // NCurses windows
    WINDOW* m_BorderWindow;
    WINDOW* m_OutputWindow;
    WINDOW* m_InputWindow;

    // Chat state
    std::vector<std::string> m_Messages;
    std::string m_InputBuffer;

    dyad_Stream* m_ServerStream;
};