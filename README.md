# Banksia - a Chess tournament manager


Overview
-----------

Banksia (a name of an Australian native wildflowers) is an open source chess tournament manager for chess engines. The code is written in C++ (using standad C++11 library). It can be compiled and run in some popurlar systems such as Windows (tested, for both 32 and 64 bit app), MacOS (tested), Linux (not tested).

To manage engines, games and the complicated events / relationships between them, the app uses a timer with few c++ callback functions. I believe it is one of the simplest way thus the code is not hard for programmers to follow, understand, maintain and modify.

![Demo](banksia.jpg)

Some features
-----------
- Command line interface (cli)
- Small, fast
- Simple and short (in term of design and implementation)
- Support chess engines with UCI and Winboard protocols
- Support opening book formats edp, pgn, bin (Polyglot)
- Tournament: concurrency, round robin, ponderable
- Controlled mainly by 2 json files (one for configurations of engines, one for tournament management). That is very flexible, easy way to setup and change
- Controllable by keyboard when games playing (type anything from keyboard to display the help)
- Written in standard C++11
- Open source, MIT license


Compile
----------
There are some project files for building by Visual Studio or Xcode.

If you want to compile those code manually, use g++ to compile and link as bellow:

    g++ -std=c++11 -c ../src/3rdparty/process/process.cpp ../src/3rdparty/process/process_unix.cpp -O2 -DNDEBUG
    g++ -std=c++11 -c ../src/3rdparty/json/*.cpp -O2 -DNDEBUG
    g++ -std=c++11 -c ../src/base/*.cpp -O2 -DNDEBUG
    g++ -std=c++11 -c ../src/chess/*.cpp -O2 -DNDEBUG
    g++ -std=c++11 -c ../src/game/*.cpp -O2 -DNDEBUG
    g++ -std=c++11 -c ../src/*.cpp -O2 -DNDEBUG
    g++ -o banksia *.o

In MS Windows, the first line needed to change to:

    g++ -std=c++11 -c ../src/3rdparty/process/process.cpp ../src/3rdparty/process/process_win.cpp -O2 -DNDEBUG


Note: you may help me to create a makefile too ;)

Using
-------
Banksia requires two json files to work. Almost all fields in those files are self-explaination via meanings of their names.

1) a json file to store engine's configurations

Configuration of each engine may includ options thus users can control all details.

2) a json file to store information about tournament such as match type, path of engine configuration json file (json file 1), log path...

Some important fields:
- type: type of tournament. At the moment it accepts only value 'roundrobin'
- time control: second is the unit for all value fields. You can use fractions (such as 1.5 as 1.5 second) for being more precision.
- players: names of players will participate the tournament. They must be listed in json file 1.

      "players" : [ "stockfish", "gaviota", "fruit" ]

Run the app in a console as bellow:
    
     banksia -jsonpath c:\tour\tour.json

There are two json files come with the project as an example.

When working, the app may display some information into screen as well as saving into some log files (controlled by tournament's json file 2):
- results
- engine input / output log
- game pgn file


Bellow is screen of a tournament between 3 chess engines:

![Demo](demo.png)

Working
---------
- Improve interface
- Auto create or update json files
- Support some different tournament types: knockout, swiss
- Support other chess variants (not soon event it is designed for multi-variants)


History
--------
- 10 July 2019: v1.50, fixed some problems of Winboard protocol,  support Polyglot's opening books
- 8 July 2019: v1.00, fixed bugs, improve interface, support Winboard protocol
- 3 July 2019: v0.03, fixed bugs, improve interface, support pgn opening
- 1 July 2019: version 0.01, first release


Terms of use
---------------

The project is released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
--------

Banksia was written by Nguyen Hong Pham (axchess at yahoo dot com).


