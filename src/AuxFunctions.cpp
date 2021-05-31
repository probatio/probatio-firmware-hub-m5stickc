#include "AuxFunctions.h"

WiFiUDP udp;

int cycle = 0;

byte buffer[BUFFER_SIZE];
int indexBuffer = 0;

boolean loopCycleControl = true;
int loopCycleTime = 100;

int quantBlocksConnected = 0;

bool isConnected = false;

std::string c_dual_buttons_1 = "dual_buttons_1";
std::string c_obsf_24 = "obsf_24";
std::string c_obsf_30 = "obsf_30";
std::string c_cherries_1 = "cherries_1";
std::string c_dual_pots_1 = "dual_pots_1";
std::string c_tap_1 = "tap_1";
std::string c_tap_2 = "tap_2";
std::string c_dual_pistons_1 = "dual_pistons_1";
std::string c_joystick_1 = "joystick_1";
std::string c_four_pistons_1 = "four_pistons_1";
std::string c_four_thumbs_1 = "four_thumbs_1";
std::string c_dof4_joystick_1 = "dof4_joystick_1";
std::string c_dof4_joystick_2 = "dof4_joystick_2";
std::string c_crank_1 = "crank_1";
std::string c_dial_1 = "dial_1";
std::string c_debug = "debug";

// Block dual_buttons_1(BLOCK_DUAL_BUTTONS_1, SIZE_BLOCK_DUAL_BUTTONS_1, 0, 0);
// Block obsf_24(BLOCK_OBSF_24, SIZE_BLOCK_OBSF_24, 0, 0);
// Block obsf_30(BLOCK_OBSF_30, SIZE_BLOCK_OBSF_30, 0, 0);
// Block cherries_1(BLOCK_CHERRIES_1, SIZE_BLOCK_CHERRIES_1, 0, 0);
// Block dual_pots_1(BLOCK_DUAL_POTS_1, SIZE_BLOCK_DUAL_POTS_1, 0, 0);
// Block tap_1(BLOCK_TAP_1, SIZE_BLOCK_TAP_1, 0, 0);
// Block tap_2(BLOCK_TAP_2, SIZE_BLOCK_TAP_2, 0, 0);
// Block dual_pistons_1(BLOCK_DUAL_PISTONS_1, SIZE_BLOCK_DUAL_PISTONS_1, 0, 0);
// Block joystick_1(BLOCK_JOYSTICK_1, SIZE_BLOCK_JOYSTICK_1, 0, 0);
// Block four_pistons_1(BLOCK_FOUR_PISTONS_1, SIZE_BLOCK_FOUR_PISTONS_1, 0, 0);
// Block four_thumbs_1(BLOCK_FOUR_THUMBS_1, SIZE_BLOCK_FOUR_THUMBS_1, 0, 0);
// Block dof4_joystick_1(BLOCK_DOF4_JOYSTICK_1, SIZE_BLOCK_DOF4_JOYSTICK_1, 0, 0);
// Block dof4_joystick_2(BLOCK_DOF4_JOYSTICK_2, SIZE_BLOCK_DOF4_JOYSTICK_2, 0, 0);
// Block crank_1(BLOCK_CRANK_1, SIZE_BLOCK_CRANK_1, 0, 0);
// Block dial_1(BLOCK_DIAL_1, SIZE_BLOCK_DIAL_1, 0, 0);
// Block debug(BLOCK_DEBUG, SIZE_BLOCK_DEBUG, 0, 0);

Block dual_buttons_1(BLOCK_DUAL_BUTTONS_1, SIZE_BLOCK_DUAL_BUTTONS_1, 0, 0, c_dual_buttons_1);
Block obsf_24(BLOCK_OBSF_24, SIZE_BLOCK_OBSF_24, 0, 0, c_obsf_24);
Block obsf_30(BLOCK_OBSF_30, SIZE_BLOCK_OBSF_30, 0, 0, c_obsf_30);
Block cherries_1(BLOCK_CHERRIES_1, SIZE_BLOCK_CHERRIES_1, 0, 0, c_cherries_1);
Block dual_pots_1(BLOCK_DUAL_POTS_1, SIZE_BLOCK_DUAL_POTS_1, 0, 0, c_dual_pots_1);
Block tap_1(BLOCK_TAP_1, SIZE_BLOCK_TAP_1, 0, 0, c_tap_1);
Block tap_2(BLOCK_TAP_2, SIZE_BLOCK_TAP_2, 0, 0, c_tap_2);
Block dual_pistons_1(BLOCK_DUAL_PISTONS_1, SIZE_BLOCK_DUAL_PISTONS_1, 0, 0, c_dual_pistons_1);
Block joystick_1(BLOCK_JOYSTICK_1, SIZE_BLOCK_JOYSTICK_1, 0, 0, c_joystick_1);
Block four_pistons_1(BLOCK_FOUR_PISTONS_1, SIZE_BLOCK_FOUR_PISTONS_1, 0, 0, c_four_pistons_1);
Block four_thumbs_1(BLOCK_FOUR_THUMBS_1, SIZE_BLOCK_FOUR_THUMBS_1, 0, 0, c_four_thumbs_1);
Block dof4_joystick_1(BLOCK_DOF4_JOYSTICK_1, SIZE_BLOCK_DOF4_JOYSTICK_1, 0, 0, c_dof4_joystick_1);
Block dof4_joystick_2(BLOCK_DOF4_JOYSTICK_2, SIZE_BLOCK_DOF4_JOYSTICK_2, 0, 0, c_dof4_joystick_2);
Block crank_1(BLOCK_CRANK_1, SIZE_BLOCK_CRANK_1, 0, 0, c_crank_1);
Block dial_1(BLOCK_DIAL_1, SIZE_BLOCK_DIAL_1, 0, 0, c_dial_1);
Block debug(BLOCK_DEBUG, SIZE_BLOCK_DEBUG, 0, 0, c_debug);

Block* blocks[QUANTITY_BLOCKS] = {&dual_buttons_1, &obsf_24, &obsf_30, &cherries_1, &dual_pots_1, &tap_1, &tap_2, &dual_pistons_1, &joystick_1, &four_pistons_1, &four_thumbs_1, &dof4_joystick_1, &dof4_joystick_2, &crank_1, &dial_1, &debug};

/*
   =============
   I2C data requisition to blocks
   =============
*/

void requestI2C() {
  quantBlocksConnected = 0;
  for (int i = 0; i < QUANTITY_BLOCKS; i++) {
    bool isConnected = requestFromDevice(blocks[i]);
    if (isConnected) {
      quantBlocksConnected++;
    }
  }
}

bool requestFromDevice(Block * _block) {
  bool result = false;
  Wire.requestFrom((uint8_t)_block->id, (uint8_t)_block->quantity, (uint8_t)true);
  if (Wire.available())
  {
    _block->isConnected = true;
    result = true;
    for (int i = 0; i < _block->quantity; i++) {
      _block->values[i].setValue(Wire.read());
    }
  } else {
    _block->isConnected = false;
  }
  return result;
}

/*
   =============
   Populate buffer
   =============
*/

void formatBufferWithBlocks() {
  indexBuffer = 0;
  buffer[indexBuffer++] = 2;
  for (int i = 0; i < QUANTITY_BLOCKS; i++) {
    if (blocks[i]->isConnected) {
      addActiveBlockValues(blocks[i]->id, blocks[i]->quantity, blocks[i]->values);
    } else {
      addInactiveBlockMessage(blocks[i]->id, blocks[i]->quantity);
    }
  }
  buffer[indexBuffer++] = '\n';
}

void addActiveBlockValues(byte blockId, byte arraySize, BlockValue values[]) {
  buffer[indexBuffer++] = blockId;
  for (int i = 0; i < arraySize; i++) {
    buffer[indexBuffer++] = values[i].getValue();
  }
}

void addInactiveBlockMessage(byte blockId, byte quantity) {
  buffer[indexBuffer++] = 0;
  for (int i = 0; i < quantity; i++) {
    buffer[indexBuffer++] = 0;
  }
}

/*
   =============
   Send buffer as consolidated Serial message
   =============
*/

void sendConsolidatedSerialMessage() {
  Serial.write(buffer, BUFFER_SIZE);
}

/*
   =============
   Create and send individual OSC Messages
   =============
*/

void sendIndividualOSCMessages(char* deviceName, char* destIP, int32_t destPort) {
    IPAddress oscIP;
    IPAddress emptyIP(0,0,0,0);

    if (oscIP.fromString(destIP) && oscIP != emptyIP && WiFi.status() == WL_CONNECTED) {
        for (int i = 0; i < QUANTITY_BLOCKS; i++) {
            if (blocks[i]->isConnected) {
                //      String message_address = "/probatio_m5/";
                //      String block_address = String(blocks[i]->string_name);
                //      String composed_address = String(message_address + block_address);
                //      char char_address[composed_address.length()];
                //      composed_address.toCharArray(char_address, composed_address.length());
                OSCMessage oscMsg(deviceName);
                udp.beginPacket(oscIP, destPort);
                for (int j = 0; j < blocks[i]->quantity; i++) {
                    oscMsg.add((int)blocks[i]->values[j].getValue());
                }
                oscMsg.send(udp);
                udp.endPacket();
            }
        }
    }
}

/*
   =============
   Create and send consolidated OSC Message
   =============
*/

void sendConsolidatedOSCMessage(char* deviceName, char* destIP, int32_t destPort) {

    IPAddress oscIP;
    IPAddress emptyIP(0,0,0,0);

    if (oscIP.fromString(destIP) && oscIP != emptyIP && WiFi.status() == WL_CONNECTED) {

        OSCMessage oscMsg(deviceName);
        udp.beginPacket(oscIP, destPort);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            oscMsg.add((int)buffer[i]);
        }
        oscMsg.send(udp);
        udp.endPacket();
    }
}

/*
   =============
   DEBUG Dump Buffer
   =============
*/

void debugDumpBuffer() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    Serial.print(buffer[i], HEX);
  }
  Serial.println();
}


