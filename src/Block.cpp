#include "Arduino.h"
#include "Block.h"

//Block::Block(byte _id, byte _quantity, byte _channel, byte _instrument)
Block::Block(byte _id, byte _quantity, byte _channel, byte _instrument, const std::string& _string_name)
{
  id = _id;
  quantity = _quantity;
  channel = _channel;
  instrument = _instrument;
  isConnected = false;
  string_name = _string_name;
  libmapper = false;
  for (int i = 0; i < MAX_ARRAY_SIZE; i++) {
    values[i] = BlockValue();
  }
}
