# Netchat

Netchat is an IRC-like chat client and server written in C++, mostly as a programming exercise.
I don't think anyone will ever have any need to use it, unless you're a weirdo, or I extend it to be cooler than other messaging solutions.

But feel free to browse the code! I'm learning modern C++, (Which, considering the libraries I chose, which are C libraries, make them odd choices for this exercise) and this was just a project for some practice. Even though I'm not using many modern C++ features lol. I'll tack more modern C++ stuff on as time passes.

## Building

It is targeting C++ 23 because of std::println, so you need a compiler that supports it.

It depends upon [dyad.c](https://github.com/rxi/dyad) for networking and [ncurses](https://github.com/mirror/ncurses) for the client CLI.
Maybe at some point I'll give it a GUI if I get bored.

CMake will fetch dyad.c for you, but currently ncurses needs to be installed on the system manually, so install whatever the development version of ncurses is for your OS through your package manager.

``` sh
git clone https://github.com/Trigex/Netchat
cd Netchat
mkdir build && cd build
cmake ..
make

# That should produce this binary if all went well!
./Netchat
```

## Usage

The server needs to be running obviously, so spin up the server like so
``` sh
./Netchat -server
```

And connect with the client

``` sh
./Netchat -client
```

Currently, the client's server address and the server's port number are hardcoded preprocessor statements, I'll make those CLI flags when I get the chance.

*Enjoy talking to yourself!*
