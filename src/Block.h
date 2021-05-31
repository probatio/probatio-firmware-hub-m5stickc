#ifndef Block_h
#define Block_h

#include "Arduino.h"
#include "BlockValue.h"

/*
Uses std::string, included on main.cpp
This whole code needs some cleaning, check my comment on
main.cpp
Edu Meneses - 2021/05/31 
*/
// #include <vector> 

#define MAX_ARRAY_SIZE 5

class Block
{
  public:
    boolean isConnected;
    boolean libmapper;
    byte id;
    byte quantity;
    BlockValue values[MAX_ARRAY_SIZE];
    byte channel;
    byte instrument;
    std::string string_name;
    //Block(byte id, byte quantity, byte channel, byte instrument);
    Block(byte id, byte quantity, byte channel, byte instrument, const std::string& _string_name);
    
  private:
};

#endif
