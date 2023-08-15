#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>

class CPacket
{
public:
	static int packet_add_data(char Buffer[], const char DataName[], const int Value);
	static int packet_add_data(char Buffer[], const char DataName[], const char Value[]);

	static int packet_parser_get_data(const char Packet[], const char DataName[], std::string& DataString);

	static int packet_parser_data(const char Packet[], const char DataName[]);
	static int packet_parser_data(const char Packet[], const char DataName[], char Buffer[], int& BufferSize);

	static int packet_encode(char Packet[], const char PacketID[], const char PacketData[]);
	static int packet_decode(const char Packet[], char PacketID[], char PacketData[]);
};
