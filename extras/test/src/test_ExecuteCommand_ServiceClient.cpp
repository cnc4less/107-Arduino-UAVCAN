/**
 * This software is distributed under the terms of the MIT License.
 * Copyright (c) 2020 LXRobotics.
 * Author: Alexander Entinger <alexander.entinger@lxrobotics.com>
 * Contributors: https://github.com/107-systems/107-Arduino-UAVCAN/graphs/contributors.
 */

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <catch.hpp>

#include <test/util/Const.h>
#include <test/util/Types.h>
#include <test/util/micros.h>

#include <ArduinoUAVCAN.h>

/**************************************************************************************
 * GLOBAL CONSTANTS
 **************************************************************************************/

static CanardNodeID const REMOTE_NODE_ID = 27;

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

static util::CanFrameVect can_frame_vect;
static ExecuteCommand_1_0::Response::Status response_status = ExecuteCommand_1_0::Response::Status::INTERNAL_ERROR;

/**************************************************************************************
 * PRIVATE FUNCTION DEFINITION
 **************************************************************************************/

static bool transmitCanFrame(uint32_t const id, uint8_t const * data, uint8_t const len)
{
  util::CanFrame frame{id, std::vector<uint8_t>(data, data + len)};
  can_frame_vect.push_back(frame);
  return true;
}

void onExecuteCommand_1_0_Response_Received(CanardTransfer const & transfer, ArduinoUAVCAN & /* uavcan */)
{
  ExecuteCommand_1_0::Response const response = ExecuteCommand_1_0::Response::create(transfer);
  response_status = response.status();
}

/**************************************************************************************
 * TEST CODE
 **************************************************************************************/

TEST_CASE("A '435.ExecuteCommand.1.0' request is sent to a server", "[execute-command-client-01]")
{
  ArduinoUAVCAN uavcan(util::LOCAL_NODE_ID, util::micros, transmitCanFrame);

  std::string const cmd_1_param = "I want a double espresso with cream";
  ExecuteCommand_1_0::Request req_1(0xCAFE, reinterpret_cast<uint8_t const *>(cmd_1_param.c_str()), cmd_1_param.length());

  REQUIRE(uavcan.request<ExecuteCommand_1_0::Request, ExecuteCommand_1_0::Response>(req_1, REMOTE_NODE_ID, onExecuteCommand_1_0_Response_Received) == true);
  /* Transmit all the CAN frames. */
  while(uavcan.transmitCanFrame()) { }

  /* Verify the content of the CAN frames. */
  /*
   * pyuavcan call 27 435.uavcan.node.ExecuteCommand.1.0 '{"command": 0xCAFE, "parameter": "I want a double espresso with cream"}' --tr='CAN(can.media.socketcan.SocketCANMedia("vcan0",8),13)'
   */
  static util::CanFrameVect const EXPECTED_CAN_FRAMES_REQUEST_1 =
  {
    {0x136CCD8D, {0xFE, 0xCA, 0x23, 0x49, 0x20, 0x77, 0x61, 0xA0}},
    {0x136CCD8D, {0x6E, 0x74, 0x20, 0x61, 0x20, 0x64, 0x6F, 0x00}},
    {0x136CCD8D, {0x75, 0x62, 0x6C, 0x65, 0x20, 0x65, 0x73, 0x20}},
    {0x136CCD8D, {0x70, 0x72, 0x65, 0x73, 0x73, 0x6F, 0x20, 0x00}},
    {0x136CCD8D, {0x77, 0x69, 0x74, 0x68, 0x20, 0x63, 0x72, 0x20}},
    {0x136CCD8D, {0x65, 0x61, 0x6D, 0xC4, 0xC8, 0x40}},
  };

  REQUIRE(can_frame_vect.size() == EXPECTED_CAN_FRAMES_REQUEST_1.size());
  auto actual_1 = std::begin(can_frame_vect);
  std::for_each(std::begin(EXPECTED_CAN_FRAMES_REQUEST_1),
                std::end  (EXPECTED_CAN_FRAMES_REQUEST_1),
                [&actual_1](util::CanFrame const frame)
                {
                  REQUIRE(actual_1->id   == frame.id);
                  REQUIRE(actual_1->data == frame.data);
                  actual_1++;
                });

  /* Feed back the command response to the uavcan node. In a
   * real system the answer would come back from the remote node.
   */
  uint8_t const data_1[] = {0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xE0};
  uavcan.onCanFrameReceived(0x126CC69B, data_1, 8);

  /* Check if the expected response has been indeed received. */
  REQUIRE(response_status == ExecuteCommand_1_0::Response::Status::NOT_AUTHORIZED);

  /* Send a second request. */
  std::string const cmd_2_param = "I do not need coffee anymore";
  ExecuteCommand_1_0::Request req_2(0xDEAD, reinterpret_cast<uint8_t const *>(cmd_2_param.c_str()), cmd_2_param.length());

  REQUIRE(uavcan.request<ExecuteCommand_1_0::Request, ExecuteCommand_1_0::Response>(req_2, REMOTE_NODE_ID, onExecuteCommand_1_0_Response_Received) == true);
  /* Transmit all the CAN frames. */
  can_frame_vect.clear();
  while(uavcan.transmitCanFrame()) { }

  /* Verify the content of the CAN frames. */
  /*
   * pyuavcan call 27 435.uavcan.node.ExecuteCommand.1.0 '{"command": 0xDEAD, "parameter": "I do not need coffee anymore"}' --tr='CAN(can.media.socketcan.SocketCANMedia("vcan0",8),13)'
   */
  static util::CanFrameVect const EXPECTED_CAN_FRAMES_REQUEST_2 =
  {
    {0x136CCD8D, {0xAD, 0xDE, 0x1C, 0x49, 0x20, 0x64, 0x6F, 0xA1}},
    {0x136CCD8D, {0x20, 0x6E, 0x6F, 0x74, 0x20, 0x6E, 0x65, 0x01}},
    {0x136CCD8D, {0x65, 0x64, 0x20, 0x63, 0x6F, 0x66, 0x66, 0x21}},
    {0x136CCD8D, {0x65, 0x65, 0x20, 0x61, 0x6E, 0x79, 0x6D, 0x01}},
    {0x136CCD8D, {0x6F, 0x72, 0x65, 0x51, 0x31, 0x61}},
  };

  REQUIRE(can_frame_vect.size() == EXPECTED_CAN_FRAMES_REQUEST_2.size());
  auto actual_2 = std::begin(can_frame_vect);
  std::for_each(std::begin(EXPECTED_CAN_FRAMES_REQUEST_2),
                std::end  (EXPECTED_CAN_FRAMES_REQUEST_2),
                [&actual_2](util::CanFrame const frame)
                {
                  REQUIRE(actual_2->id   == frame.id);
                  REQUIRE(actual_2->data == frame.data);
                  actual_2++;
                });

  /* Feed back the command response to the uavcan node. In a
   * real system the answer would come back from the remote node.
   */
  uint8_t const data_2[] = {0x05, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xE1};
  uavcan.onCanFrameReceived(0x126CC69B, data_2, 8);

  /* Check if the expected response has been indeed received. */
  REQUIRE(response_status == ExecuteCommand_1_0::Response::Status::BAD_STATE);
}
