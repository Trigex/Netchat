#pragma once
#include <format>

class Server;

struct User
{
    unsigned int id;
    const char* address;
    std::string nick;
    dyad_Stream *userStream;
    Server *server;
    ///< Pointer to the server instance this user is linked to, so in event handlers for a client dyad_Stream, we still have access to server instance functions (Only way to pass instance data to the event handlers being event->udata)

    std::string toString()
    {
        return std::format("Nickname: \"{}\", ID: {}, IP Address: {}", nick, id, address);
    }
};
