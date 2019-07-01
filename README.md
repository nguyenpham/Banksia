# Banksia
==============
A chess tournament manager for chess engines, using standard C++ 11


Overview
-----------

Banksia (a name of an Australian native wildflowers) is an open source chess tournament manager for chess engines. The code is written in C++ (using standad C++11 library). It can be compiled and run in some popurlar OSs such as Windows (tested, for both 32 and 64 bit app), MacOS (tested), Linux (not tested).

To manage engines, games and the complicated events / relationships between them, the app uses a timer with few c++ callback functions. I believe it is one of the simplest way thus it is not hard for programmers to follow, understand, maintain and modify.


Some features
-----------
- Command line interface (cli)
- Small, fast
- Simple and short (in term of design and implementation)
- Support chess engines with UCI protocol
- Tournament: concurrently, roundrobin, edp opening, ponderable
- Controlled mainly by 2 json files (one for configurations of engines, one for tournament management). That is very flexible, easy way to setup and change
- Controlable by keyboard when game playing (type anything from keyboard to display the help)
- Written in standard C++11
- Open source, MIT licence


Compile
----------
There are some project files for building by Visual Studio or Xcode.

If you want to compile those code manually, use g++ to compile and link as bellow:

    g++ -std=c++11 -c ../src/3rdparty/json/*.cpp -O3 -DNDEBUG
    g++ -std=c++11 -c ../src/3rdparty/process/process.cpp ../src/3rdparty/process/process_unix.cpp -O3 -DNDEBUG
    g++ -std=c++11 -c ../src/base/*.cpp -O3 -DNDEBUG
    g++ -std=c++11 -c ../src/chess/*.cpp -O3 -DNDEBUG
    g++ -std=c++11 -c ../src/game/*.cpp -O3 -DNDEBUG
    g++ -std=c++11 -c ../src/*.cpp -O3 -DNDEBUG
    g++ -o banksia *.o


Using
-------
Banksia requires two json files to work. Almost all fields are self-explaination via meanings of their names.

1) a json file to store engine's configurations

The file has all configurations including engines' options thus users can control all details.

2) a json file to store information about tournament such as match type, path of engine configure json file (json file 1), log path

Some important fields:
- type: type of tournament. At the moment it accepts only value 'roundrobin'
- time control: second is the unit for all value fields. You can use fractions (such as 1.5 as 1.5 second) for getting more precision.
- players: names of players will participate the tournament. They must be named in file json 1.

      "players" : [ "stockfish", "gaviota", "fruit" ]

Run the app in a console as bellow:
    
     banksia -jsonpath c:\tour\tour.json

There are two json files come with the project as an example.

When working, the app may display some information into screen as well as saving into some log files (depends on tournament's json file):
- results
- engine input / output log
- game pgn file


Working
---------
- Improve interface
- Support some diffirent opening book formats such as pgn and Polyglot
- Support some different tournament types: knockout, swiss
- Support Winboard protocol (not soon)
- Support other chess variants (not soon event it is designed for multi variants - at least from verion 1.0)


History
--------
- 1 July 2019: first release, version 0.01


Terms of use
---------------

The project is released under the liberal [MIT license](http://en.wikipedia.org/wiki/MIT_License), so basically you can use it with almost no restrictions.


Credits
--------

Banksia was written by Nguyen Hong Pham (axchess at yahoo dot com).


