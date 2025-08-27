#include "Client.hpp"
#include <thread>
#include <print>

// Helper function to trim leading/trailing whitespace (like \r\n from netcat)
static std::string trim(const std::string &str)
{
    const std::string WHITESPACE = " \n\r\t\f\v";
    size_t first = str.find_first_not_of(WHITESPACE);
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(WHITESPACE);
    return str.substr(first, (last - first + 1));
}

extern volatile sig_atomic_t gSignalStatus;

Client::Client(const std::string &host, const int port)
    : m_Host(host), m_Port(port), m_Running(false),
      m_UIDirty(true), m_BorderWindow(nullptr), m_OutputWindow(nullptr),
      m_InputWindow(nullptr), m_ServerStream(nullptr)
{
    initNcurses();
}

Client::~Client()
{
}

void Client::run()
{
    // Initialize networking
    dyad_init();
    m_ServerStream = dyad_newStream();

    // Add event handlers to stream
    dyad_addListener(m_ServerStream, DYAD_EVENT_CONNECT, onConnect, this);
    dyad_addListener(m_ServerStream, DYAD_EVENT_ERROR, onError, this);
    dyad_addListener(m_ServerStream, DYAD_EVENT_DATA, onData, this);
    dyad_addListener(m_ServerStream, DYAD_EVENT_CLOSE, onClose, this);

    if (dyad_connect(m_ServerStream, m_Host.c_str(), m_Port) < 0)
    {
        cleanupNcurses();
        dyad_shutdown();
        throw std::runtime_error("Failed to connect to server (Error: dyad_connect() failed)");
    }

    dyad_setUpdateTimeout(0);

    m_Running = true;
    addMessage("Connecting to " + m_Host + "...");

    while (m_Running && gSignalStatus == 0)
    {
        dyad_update();

        int ch = wgetch(m_InputWindow);
        if (ch != ERR)
        {
            handleInput(ch);
        }

        if (m_UIDirty)
        {
            drawUI();
            m_UIDirty = false; // UI has been redrawn
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent 100% CPU usage
    }

    dyad_shutdown();
    cleanupNcurses();
}

void Client::stop()
{
    m_Running = false;
}

void Client::initNcurses()
{
    initscr(); // Start curses mode
    cbreak(); // Line buffering disabled
    noecho(); // Don't echo() while doing getch
    keypad(stdscr, TRUE); // Enable function keys
    curs_set(1); // Show the cursor

    // Create windows
    recreateWindows();
}

void Client::recreateWindows()
{
    int height, width;
    getmaxyx(stdscr, height, width);

    m_BorderWindow = newwin(height, width, 0, 0);
    m_OutputWindow = newwin(height - 3, width - 2, 1, 1);
    m_InputWindow = newwin(1, width - 2, height - 2, 1);

    scrollok(m_OutputWindow, TRUE); // Allow output window to scroll
    nodelay(m_InputWindow, TRUE); // Make getch non-blocking
}

void Client::cleanupNcurses()
{
    if (m_InputWindow) delwin(m_InputWindow);
    if (m_OutputWindow) delwin(m_OutputWindow);
    if (m_BorderWindow) delwin(m_BorderWindow);
    if (isendwin() == FALSE)
    {
        endwin(); // end curses mode
    }
}

void Client::sendMessage()
{
    if (!m_InputBuffer.empty() && dyad_getState(m_ServerStream) == DYAD_STATE_CONNECTED)
    {
        dyad_write(m_ServerStream, m_InputBuffer.c_str(), static_cast<int>(m_InputBuffer.length()));
        dyad_write(m_ServerStream, "\n", 1);
        m_InputBuffer.clear();
    }
}

void Client::onConnect(dyad_Event *e)
{
    auto *client = static_cast<Client *>(e->udata);
    client->addMessage("Successfully connected to server!");
}

void Client::onError(dyad_Event *e)
{
    auto *client = static_cast<Client *>(e->udata);
    client->addMessage("Error: " + std::string(e->msg));
    client->m_Running = false;
}

void Client::onData(dyad_Event *e)
{
    auto *client = static_cast<Client *>(e->udata);
    std::string msg = trim(std::string(e->data, e->size));
    client->addMessage(msg);
}

void Client::onClose(dyad_Event *e)
{
    auto *client = static_cast<Client *>(e->udata);
    client->addMessage("Disconnected from server. Press any key to exit.");
    client->m_Running = false;
}

void Client::drawUI()
{
    int width, height;
    getmaxyx(stdscr, height, width);

    // Border and title
    box(m_BorderWindow, 0, 0);
    mvwprintw(m_BorderWindow, 0, 2, " Netchat Client ");
    wnoutrefresh(m_BorderWindow);

    // Output window (Chat history)
    werase(m_OutputWindow);
    int outHeight, outWidth;
    getmaxyx(m_OutputWindow, outHeight, outWidth);
    int startLine = m_Messages.size() > outHeight ? m_Messages.size() - outHeight : 0;
    for (int i = 0; i < outHeight && (startLine + i) < m_Messages.size(); ++i)
    {
        mvwprintw(m_OutputWindow, i, 0, m_Messages[startLine + i].c_str());
    }
    wnoutrefresh(m_OutputWindow); // Draw to virtual screen

    // Input window
    werase(m_InputWindow);
    mvwprintw(m_InputWindow, 0, 0, "> %s", m_InputBuffer.c_str());
    wnoutrefresh(m_InputWindow); // Draw to virtual screen

    // Update the physical screen once with all changes
    doupdate();
}

void Client::handleInput(int ch)
{
    if (ch == KEY_RESIZE)
    {
        cleanupNcurses();
        endwin();
        refresh();
        recreateWindows();
        m_UIDirty = true;
    }
    else if (ch == '\n' || ch == KEY_ENTER)
    {
        sendMessage();
        m_UIDirty = true;
    } else if (ch == KEY_BACKSPACE || ch == 127)
    {
        if (!m_InputBuffer.empty())
        {
            m_InputBuffer.pop_back();
            m_UIDirty = true;
        }
    } else if (isprint(ch))
    {
        m_InputBuffer += static_cast<char>(ch);
        m_UIDirty = true;
    }
}

void Client::addMessage(const std::string &msg)
{
    m_Messages.emplace_back(msg);
    m_UIDirty = true;
}
