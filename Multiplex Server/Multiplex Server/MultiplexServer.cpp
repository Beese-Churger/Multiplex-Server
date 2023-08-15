#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <vector>

#include "packet_manager.h"

/// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE     1024
#define PORT_NUMBER 7890
#define FROM_SERVER "SERVER"



struct PartyMembers
{
	enum Rank
	{
		LEADER,
		MEMBER,
	};

	Rank rank;
	int clientid;
};

struct Party
{
	std::vector<PartyMembers*>MemberList;
	PartyMembers* Leader;
	int PartyID;
};


std::vector<Party*> partylist;

void no_rank(SOCKET ClientSocket)
{
	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", "YOU DON'T HAVE PERMISSION TO DO THAT");

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void send_party_invite(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket)
{
	int MsgPos;
	int FD_Index;
	char TargetSocket[BUFSIZ];
	char FailMessage[] = "<INVALID COMMAND. Type /help for a list of available commands>";
	int FailMessageLength = strlen(FailMessage);


	for (MsgPos = 3; MsgPos < MessageLength; ++MsgPos)
	{
		if ('\0' == Message[MsgPos])
		{
			TargetSocket[MsgPos - 1] = '\0';
			break;
		}
		TargetSocket[MsgPos - 3] = Message[MsgPos];
	}
	if (MsgPos == MessageLength)
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);
		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientSocket, encodedMSG, PacketDataLength, 0);
		return;
	}

	for (FD_Index = 1; FD_Index < ReadFds.fd_count; ++FD_Index)
	{
		if (ReadFds.fd_array[FD_Index] == atoi(TargetSocket))
		{
			char PacketBuffer[BUFSIZE];
			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
			CPacket::packet_add_data(PacketBuffer, "MSG", ">> PARTY INVITE SENT");
			char encodedMSG[BUFSIZE];
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			int PacketDataLength = strlen(encodedMSG);

			send(ClientSocket, encodedMSG, PacketDataLength, 0);

			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "PARTYINV", " >> PARTY INVITE FROM ");
			CPacket::packet_add_data(PacketBuffer, "FROM", ClientSocket);
			char PartyMSG[BUFSIZE];
			sprintf_s(PartyMSG, "(To accept, type /p accept or /p decline to decline)");
			CPacket::packet_add_data(PacketBuffer, "MSG", PartyMSG);
			memset(encodedMSG, '\0', BUFSIZE);
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			PacketDataLength = strlen(encodedMSG);

			send(ReadFds.fd_array[FD_Index], encodedMSG, PacketDataLength, 0);

			break;
		}
	}
	if (FD_Index == ReadFds.fd_count)
	{
		/*char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);
		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientSocket, encodedMSG, PacketDataLength, 0);*/
	}
}

void add_party_member(SOCKET ClientAccepted, Party* party, int ClientSent)
{
	PartyMembers* member = new PartyMembers();
	member->rank = PartyMembers::Rank::MEMBER;
	member->clientid = ClientAccepted;
	party->MemberList.push_back(member);

	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);

	char joinMSG[BUFSIZE];
	sprintf_s(joinMSG, "YOU HAVE SUCCESSFULLY JOINED [%d]'S PARTY!", ClientSent);
	CPacket::packet_add_data(PacketBuffer, "MSG", joinMSG);
	CPacket::packet_add_data(PacketBuffer, "PARTYID", party->PartyID);

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientAccepted, encodedMSG, PacketDataLength, 0);


	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);

	memset(joinMSG, '\0', BUFSIZE);
	sprintf_s(joinMSG, "[%d] HAS JOINED YOUR PARTY!", ClientAccepted);
	CPacket::packet_add_data(PacketBuffer, "MSG", joinMSG);

	memset(encodedMSG, '\0', BUFSIZE);
	CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	PacketDataLength = strlen(encodedMSG);
	for (int i = 0; i < party->MemberList.size(); ++i)
	{
		if (party->MemberList[i]->clientid != ClientAccepted)
		{
			send(party->MemberList[i]->clientid, encodedMSG, PacketDataLength, 0);
		}
	}
}

void create_party(int ClientSent)
{
	
	Party* party = new Party();
	party->PartyID = ClientSent;
	PartyMembers* leader = new PartyMembers();
	leader->rank = PartyMembers::Rank::LEADER;
	leader->clientid = ClientSent;
	party->Leader = leader;
	party->MemberList.push_back(leader);

	//store existing parties
	partylist.push_back(party);

	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", "PARTY HAS BEEN CREATED! Type /help for party commands");
	CPacket::packet_add_data(PacketBuffer, "PARTYID", party->PartyID);
	
	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	//std::string clientid = ClientSent;
	send(ClientSent, encodedMSG, PacketDataLength, 0);
}

void accept_party_invite(SOCKET ClientAccepted, int ClientSent, int partyid)
{
	if (partyid == 1)
	{
		for (int i = 0; i < partylist.size(); ++i)
		{
			if(partylist[i]->PartyID == ClientSent)
				add_party_member(ClientAccepted, partylist[i], ClientSent);
		}
		
	}
		
	else
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", "YOU ARE ALREADY IN A PARTY");

		char encodedMSG[BUFSIZE];
		int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientAccepted, encodedMSG, PacketDataLength, 0);
	}
}

void decline_party_invite(fd_set ReadFds, SOCKET ClientDecline, int ClientSent, int partyID)
{
	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);

	char joinMSG[BUFSIZE];
	sprintf_s(joinMSG, "YOU HAVE DECLINED [%d]'S PARTY INVITE", ClientSent);
	CPacket::packet_add_data(PacketBuffer, "MSG", joinMSG);

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientDecline, encodedMSG, PacketDataLength, 0);

	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);

	memset(joinMSG, '\0', BUFSIZE);
	sprintf_s(joinMSG, "[%d] HAS DECLINED THE PARTY INVITE", ClientDecline);
	CPacket::packet_add_data(PacketBuffer, "MSG", joinMSG);

	memset(encodedMSG, '\0', BUFSIZE);
	CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	PacketDataLength = strlen(encodedMSG);

	send(ClientSent, encodedMSG, PacketDataLength, 0);
	for (int j = 0; j < partylist.size(); ++j)
	{
		if (partylist[j]->PartyID == partyID)
		{
			for (int i = 0; i < partylist[j]->MemberList.size(); ++i)
			{
				if (partylist[j]->MemberList[i]->clientid != ClientDecline)
				{
					send(partylist[j]->MemberList[i]->clientid, encodedMSG, PacketDataLength, 0);
				}
			}
		}
	}
}

void list_members(SOCKET ClientSocket, int partyid)
{
	std::string temp = "=== List of members in Party with id [" + std::to_string(partyid) + "] ===\n";
	char tempname[BUFSIZE];
	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				sprintf_s(tempname, "%d: [%d]\n", partylist[i]->MemberList[j]->rank, partylist[i]->MemberList[j]->clientid);
				temp += tempname;
			}
		}
	}
	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", temp.c_str());

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void leave_party(SOCKET ClientSocket, int partyid)
{
	int todel = 0;
	int todelparty = 0;
	bool erase = false;
	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				char temp[BUFSIZE];

				if (partylist[i]->MemberList[j]->clientid == ClientSocket)
				{
					todel = j;

					sprintf_s(temp, "YOU HAVE LEFT THE PARTY [%d]", partyid);

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);
					CPacket::packet_add_data(PacketBuffer, "PARTYID", 1);

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);

					send(ClientSocket, encodedMSG, PacketDataLength, 0);
				}
				else
				{
					sprintf_s(temp, "[%d] HAS LEFT THE PARTY", ClientSocket);

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					

					if (partylist[i]->MemberList.size() - 1 <= 1)
					{
						sprintf_s(temp, "[%d] HAS LEFT THE PARTY. THERE ARE NO MORE MEMBERS LEFT IN THE PARTY, DISBANDING", ClientSocket);
						CPacket::packet_add_data(PacketBuffer, "PARTYID", 1);
						erase = true;
						todelparty = i;
					}
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);

					send(partylist[i]->MemberList[j]->clientid, encodedMSG, PacketDataLength, 0);
				}
			}
			partylist[i]->MemberList.erase(partylist[i]->MemberList.begin() + todel);
		}
	}
	if(erase)
		partylist.erase(partylist.begin() + todelparty);
}

void disband_party(SOCKET ClientSocket, int partyid)
{
	int todelparty = 0;
	bool erase = false;
	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				if (ClientSocket == partylist[i]->MemberList[j]->clientid)
				{
					if (partylist[i]->MemberList[j]->rank != PartyMembers::Rank::LEADER)
					{
						no_rank(ClientSocket);
						return;
					}
					
					erase = true;
					todelparty = i;
				}
				if (erase)
				{
					char temp[BUFSIZE];

					sprintf_s(temp, "PARTY HAS BEEN DISBANDED BY THE OWNER");

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);
					CPacket::packet_add_data(PacketBuffer, "PARTYID", 1);

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);

					send(partylist[i]->MemberList[j]->clientid, encodedMSG, PacketDataLength, 0);
				}
			}
			partylist[i]->MemberList.clear();
		}
	}
	if (erase)
		partylist.erase(partylist.begin() + todelparty);
}

void party_kick(SOCKET ClientSocket, char Message[], int MessageLength, int partyid)
{
	int todel = 0;
	int todelparty = 0;
	bool erase = false;
	bool kicked = false;

	int MsgPos;
	int FD_Index;
	char TargetSocket[BUFSIZ];

	for (MsgPos = 8; MsgPos < MessageLength; ++MsgPos)
	{
		if (' ' == Message[MsgPos])
		{
			TargetSocket[MsgPos - 1] = '\0';
			break;
		}
		TargetSocket[MsgPos - 8] = Message[MsgPos];
	}

	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				char temp[BUFSIZE];
				if (partylist[i]->MemberList[j]->clientid == atoi(TargetSocket))
				{
					if (atoi(TargetSocket) == ClientSocket)
					{
						sprintf_s(temp, "YOU CAN'T KICK YOURSELF");

						char PacketBuffer[BUFSIZE];
						memset(PacketBuffer, '\0', BUFSIZE);
						CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
						CPacket::packet_add_data(PacketBuffer, "MSG", temp);

						char encodedMSG[BUFSIZE];
						int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
						int PacketDataLength = strlen(encodedMSG);

						send(ClientSocket, encodedMSG, PacketDataLength, 0);
						return;
					}
					if (partylist[i]->MemberList[j]->rank == PartyMembers::Rank::LEADER)
					{
						no_rank(ClientSocket);
						return;
					}
					else
					{
						todel = j;

						sprintf_s(temp, "YOU HAVE BEEN KICKED FROM THE PARTY [%d]", partyid);

						char PacketBuffer[BUFSIZE];
						memset(PacketBuffer, '\0', BUFSIZE);
						CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
						CPacket::packet_add_data(PacketBuffer, "MSG", temp);
						CPacket::packet_add_data(PacketBuffer, "PARTYID", 1);
						char encodedMSG[BUFSIZE];
						int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
						int PacketDataLength = strlen(encodedMSG);

						send(partylist[i]->MemberList[j]->clientid, encodedMSG, PacketDataLength, 0);
						kicked = true;
					}
				}
				else
				{
					if (kicked)
					{
						sprintf_s(temp, "[%d] HAS BEEN KICKED FROM THE PARTY", atoi(TargetSocket));

						char PacketBuffer[BUFSIZE];
						memset(PacketBuffer, '\0', BUFSIZE);
						CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);


						if (partylist[i]->MemberList.size() - 1 <= 1)
						{
							sprintf_s(temp, "[%d] HAS BEEN KICKED FROM THE PARTY. THERE ARE NO MORE MEMBERS LEFT IN THE PARTY, DISBANDING", atoi(TargetSocket));
							CPacket::packet_add_data(PacketBuffer, "PARTYID", 1);
							erase = true;
							todelparty = i;
						}
						CPacket::packet_add_data(PacketBuffer, "MSG", temp);

						char encodedMSG[BUFSIZE];
						int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
						int PacketDataLength = strlen(encodedMSG);

						send(partylist[i]->MemberList[j]->clientid, encodedMSG, PacketDataLength, 0);
					}
				}
			}
			partylist[i]->MemberList.erase(partylist[i]->MemberList.begin() + todel);
		}
	}
	if (erase)
		partylist.erase(partylist.begin() + todelparty);
}

void party_transfer(SOCKET ClientSocket, char Message[], int MessageLength, int partyid)
{
	int MsgPos;
	int FD_Index;
	char TargetSocket[BUFSIZ];

	for (MsgPos = 12; MsgPos < MessageLength; ++MsgPos)
	{
		if (' ' == Message[MsgPos])
		{
			TargetSocket[MsgPos - 1] = '\0';
			break;
		}
		TargetSocket[MsgPos - 12] = Message[MsgPos];
	}

	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			bool found = false;
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				char temp[BUFSIZE];
				if (partylist[i]->MemberList[j]->clientid == atoi(TargetSocket))
				{
					if (ClientSocket == partylist[i]->MemberList[j]->clientid)
					{
						if (partylist[i]->MemberList[j]->rank != PartyMembers::Rank::LEADER)
						{
							no_rank(ClientSocket);
							return;
						}
					}
					
					sprintf_s(temp, "YOU HAVE BEEN PROMOTED TO PARTY LEADER");

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);
					partylist[i]->Leader->rank = PartyMembers::Rank::MEMBER;
					partylist[i]->MemberList[j]->rank = PartyMembers::Rank::LEADER;
					partylist[i]->Leader = partylist[i]->MemberList[j];

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);
					found = true;
					send(ClientSocket, encodedMSG, PacketDataLength, 0);
					
				}
				if(!found)
				{
					sprintf_s(temp, "YOU HAVE BEEN DEMOTED TO MEMBER");

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);

					send(ClientSocket, encodedMSG, PacketDataLength, 0);
				}
				else
				{
					sprintf_s(temp, "[%d] HAS BEEN PROMOTED TO PARTY LEADER", partylist[i]->MemberList[j]->clientid);

					char PacketBuffer[BUFSIZE];
					memset(PacketBuffer, '\0', BUFSIZE);
					CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
					CPacket::packet_add_data(PacketBuffer, "MSG", temp);

					char encodedMSG[BUFSIZE];
					int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
					int PacketDataLength = strlen(encodedMSG);

					send(partylist[i]->MemberList[j]->clientid, encodedMSG, PacketDataLength, 0);
				}
			}
		}
	}
}

void party_chat(SOCKET ClientSocket)
{
	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", "SWITCHED TO PARTY CHAT");
	CPacket::packet_add_data(PacketBuffer, "CHATID", 2);

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void all_chat(SOCKET ClientSocket)
{
	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", "SWITCHED TO ALL CHAT");
	CPacket::packet_add_data(PacketBuffer, "CHATID", 2);

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void no_party(SOCKET ClientSocket)
{
	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", "YOU ARE NOT IN A PARTY");

	char encodedMSG[BUFSIZE];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void send_welcome_message(SOCKET ClientSocket)
{
	char WelcomeMessage[100];
	int WelcomeMessageLength;

	sprintf_s(WelcomeMessage, "=== Welcome to Jonathan's I / O multiplexing server! Your Session ID is [%d] ===" , ClientSocket);
	WelcomeMessageLength = strlen(WelcomeMessage);

	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", WelcomeMessage);
	CPacket::packet_add_data(PacketBuffer, "SESSIONID", ClientSocket);


	char encodedMSG[1024];
	int result = CPacket::packet_encode(encodedMSG, "Welcome", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void session_info_message(fd_set ReadFds, SOCKET ClientSocket)
{
	char InfoMessage[100];
	char HelpMessage[100];

	std::string totalOnline = " ";
	for (int i = 1; i < ReadFds.fd_count; ++i)
	{
		if (ClientSocket == ReadFds.fd_array[i])
		{
			continue;
		}

		std::string toShow = "[" + std::to_string(ReadFds.fd_array[i]) + "]";
		totalOnline += toShow;
		
	}
	sprintf_s(InfoMessage, "=== Connected with %d other client(s) ===", ReadFds.fd_count - 2);
	sprintf_s(HelpMessage, "=== Type /help for useful commands ===");
	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", InfoMessage);
	CPacket::packet_add_data(PacketBuffer, "HELPINFO", HelpMessage);

	char encodedMSG[1024];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void send_notice_message(fd_set ReadFds, SOCKET ClientSocket)
{
	char InfoMessage[100];
	int InfoMessageLength;

	for (int i = 1; i < ReadFds.fd_count; ++i)
	{
		if (ClientSocket == ReadFds.fd_array[i])
		{
			continue;
		}

		sprintf_s(InfoMessage, "=== New client has connected. Session ID is [%d] ===", ClientSocket);
		InfoMessageLength = strlen(InfoMessage);

		char PacketBuffer[1024];
		memset(PacketBuffer, '\0', 1024);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", InfoMessage);


		char encodedMSG[1024];
		int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ReadFds.fd_array[i], encodedMSG, PacketDataLength, 0);
	}
}

void show_online(fd_set ReadFds, SOCKET ClientSocket)
{
	char InfoMessage[100];
	char HelpMessage[100];

	sprintf_s(InfoMessage, "=== Connected with %d other client(s)", ReadFds.fd_count - 2);
	std::string totalOnline;
	totalOnline += InfoMessage;

	for (int i = 1; i < ReadFds.fd_count; ++i)
	{
		if (ClientSocket == ReadFds.fd_array[i])
		{
			continue;
		}
		char toShow[BUFSIZE];
		sprintf_s(toShow, ", [%d]", ReadFds.fd_array[i]);

		totalOnline += toShow;

	}
	totalOnline += " ===";
	char PacketBuffer[1024];
	memset(PacketBuffer, '\0', 1024);
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", totalOnline.c_str());

	char encodedMSG[1024];
	int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

void whisper_to_one(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket)
{
	int MsgPos;
	int FD_Index;
	char TargetSocket[BUFSIZ];
	char FailMessage[] = "<Fail to send a message>";
	int FailMessageLength = strlen(FailMessage);
	char SuccessMessage[BUFSIZE];
	int SuccessMessageLength;
	char WhisperMessage[BUFSIZE];
	int WhisperMessageLength;

	for (MsgPos = 3; MsgPos < MessageLength; ++MsgPos)
	{
		if (' ' == Message[MsgPos])
		{
			TargetSocket[MsgPos - 1] = '\0';
			break;
		}
		TargetSocket[MsgPos - 3] = Message[MsgPos];
	}
	if (MsgPos == MessageLength)
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);
		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientSocket, encodedMSG, PacketDataLength, 0);
		return;
	}

	for (FD_Index = 1; FD_Index < ReadFds.fd_count; ++FD_Index)
	{
		if (ReadFds.fd_array[FD_Index] == atoi(TargetSocket))
		{
			char PacketBuffer[BUFSIZE];
			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "WHISPER", " >> WHISPER FROM ");
			CPacket::packet_add_data(PacketBuffer, "FROM", ClientSocket);
			CPacket::packet_add_data(PacketBuffer, "MSG", &Message[MsgPos + 1]);
			char encodedMSG[BUFSIZE];
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			int PacketDataLength = strlen(encodedMSG);

			send(ReadFds.fd_array[FD_Index], encodedMSG, PacketDataLength, 0);

			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "WHISPER", " >> WHISPER TO ");
			CPacket::packet_add_data(PacketBuffer, "FROM", ReadFds.fd_array[FD_Index]);
			CPacket::packet_add_data(PacketBuffer, "MSG", &Message[MsgPos + 1]);
			memset(encodedMSG, '\0', BUFSIZE);
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			PacketDataLength = strlen(encodedMSG);

			send(ClientSocket, encodedMSG, PacketDataLength, 0);

			break;
		}
	}
	if (FD_Index == ReadFds.fd_count)
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);
		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientSocket, encodedMSG, PacketDataLength, 0);
	}
}

void reply_to_one(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket, char TargetSocket[])
{
	int MsgPos;
	int FD_Index;
	char FailMessage[] = "<No one has sent you a whisper yet>";
	int FailMessageLength = strlen(FailMessage);
	char SuccessMessage[BUFSIZE];
	int SuccessMessageLength;
	char WhisperMessage[BUFSIZE];
	int WhisperMessageLength;

	if (TargetSocket == "")
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);

		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);
		send(ClientSocket, encodedMSG, PacketDataLength, 0);
		return;
	}

	for (FD_Index = 1; FD_Index < ReadFds.fd_count; ++FD_Index)
	{
		if (ReadFds.fd_array[FD_Index] == atoi(TargetSocket))
		{
			char PacketBuffer[BUFSIZE];
			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "WHISPER", " >> WHISPER FROM ");
			CPacket::packet_add_data(PacketBuffer, "FROM", ClientSocket);
			CPacket::packet_add_data(PacketBuffer, "MSG", &Message[3]);
			char encodedMSG[BUFSIZE];
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			int PacketDataLength = strlen(encodedMSG);

			send(ReadFds.fd_array[FD_Index], encodedMSG, PacketDataLength, 0);

			memset(PacketBuffer, '\0', BUFSIZE);
			CPacket::packet_add_data(PacketBuffer, "WHISPER", " >> WHISPER TO ");
			CPacket::packet_add_data(PacketBuffer, "FROM", TargetSocket);
			CPacket::packet_add_data(PacketBuffer, "MSG", &Message[3]);
			memset(encodedMSG, '\0', BUFSIZE);
			CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
			PacketDataLength = strlen(encodedMSG);

			send(ClientSocket, encodedMSG, PacketDataLength, 0);

			break;
		}
	}
	if (FD_Index == ReadFds.fd_count)
	{
		char PacketBuffer[BUFSIZE];
		memset(PacketBuffer, '\0', BUFSIZE);
		CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
		CPacket::packet_add_data(PacketBuffer, "MSG", FailMessage);
		char encodedMSG[BUFSIZE];
		CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
		int PacketDataLength = strlen(encodedMSG);

		send(ClientSocket, encodedMSG, PacketDataLength, 0);
	}
}

void send_to_all(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket)
{
	int i;

	for (i = 1; i < ReadFds.fd_count; ++i)
	{
		send(ReadFds.fd_array[i], Message, MessageLength, 0);
	}
}

void send_to_party(fd_set ReadFds, char Message[], int MessageLength, SOCKET ClientSocket, int partyid)
{
	for (int i = 0; i < partylist.size(); ++i)
	{
		if (partylist[i]->PartyID == partyid)
		{
			for (int j = 0; j < partylist[i]->MemberList.size(); ++j)
			{
				send(partylist[i]->MemberList[j]->clientid, Message, MessageLength, 0);	
			}
		}
	}
}

void ShowHelp(SOCKET ClientSocket)
{
	char HelpMsg[BUFSIZE];
	char PacketBuffer[BUFSIZE];
	memset(PacketBuffer, '\0', BUFSIZE);
	std::string temp;
	std::vector<std::string>HelpList;

	HelpList.push_back("*** General Commands ***\n");
	HelpList.push_back(" > /who             - Lists all online users\n");
	HelpList.push_back(" > /w [id]          - Whisper to specified ID\n");
	HelpList.push_back(" > /r               - Reply to the most recent whisper\n");
	HelpList.push_back(" > /chat a          - Switch chat mode to ALL\n");
	HelpList.push_back(" > /chat p          - Switch chat mode to PARTY\n");
	HelpList.push_back("          *** Party Commands ***\n");
	HelpList.push_back(" > /p create        - Create a party\n");
	HelpList.push_back(" > /p [id]          - Send a party invite to specified ID\n");
	HelpList.push_back(" > /p list          - Lists all players currently in the party\n");
	HelpList.push_back(" > /p leave         - Leaves the current party\n");
	HelpList.push_back(" > /p disband       - Disbands your party\n");
	HelpList.push_back(" > /p transfer [id] - Transfers the leader role to someone else in the party\n");
	HelpList.push_back(" > /p kick [id]     - Kicks a party member\n");
	for (int i = 0; i < HelpList.size(); ++i)
	{
		temp += HelpList[i];
	}
	char encodedMSG[1024];
	CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
	CPacket::packet_add_data(PacketBuffer, "MSG", temp.c_str());
	CPacket::packet_encode(encodedMSG, "HELPMSG", PacketBuffer);
	int PacketDataLength = strlen(encodedMSG);

	send(ClientSocket, encodedMSG, PacketDataLength, 0);
}

int main(int argc, char** argv)
{
	int          Port = PORT_NUMBER;
	WSADATA      WsaData;
	SOCKET       ServerSocket;
	SOCKADDR_IN  ServerAddr;

	unsigned int Index;
	int          ClientLen = sizeof(SOCKADDR_IN);
	SOCKET       ClientSocket;
	SOCKADDR_IN  ClientAddr;

	fd_set       ReadFds, TempFds;
	TIMEVAL      Timeout; // struct timeval timeout;

	char         Message[BUFSIZE];
	int          Return;

	char Packet[1024];
	char PacketID[1024];
	int PacketIDLength = -1;
	char PacketData[1024];
	int PacketDataLength = -1;

	if (2 == argc)
	{
		Port = atoi(argv[1]);
	}
	printf("Using port number : [%d]\n", Port);

	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
	{
		printf("WSAStartup() error!\n");
		return 1;
	}

	ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == ServerSocket)
	{
		printf("socket() error\n");
		return 1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr,
		sizeof(ServerAddr)))
	{
		printf("bind() error\n");
		return 1;
	}

	if (SOCKET_ERROR == listen(ServerSocket, 5))
	{
		printf("listen() error\n");
		return 1;
	}

	FD_ZERO(&ReadFds);
	FD_SET(ServerSocket, &ReadFds);

	while (1)
	{
		TempFds = ReadFds;
		Timeout.tv_sec = 5;
		Timeout.tv_usec = 0;

		if (SOCKET_ERROR == (Return = select(0, &TempFds, 0, 0, &Timeout)))
		{ // Select() function returned error.
			printf("select() error\n");
			return 1;
		}
		if (0 == Return)
		{ // Select() function returned by timeout.
			printf("Select returned timeout.\n");
		}
		else if (0 > Return)
		{
			printf("Select returned error!\n");
		}
		else
		{
			for (Index = 0; Index < TempFds.fd_count; Index++)
			{
				if (TempFds.fd_array[Index] == ServerSocket)
				{ // New connection requested by new client.
					ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &ClientLen);
					FD_SET(ClientSocket, &ReadFds);
					printf("New Client Accepted : Socket Handle [%d]\n", ClientSocket);

					send_welcome_message(ClientSocket);
					//help_message(ReadFds, ClientSocket);
					session_info_message(ReadFds, ClientSocket);
					send_notice_message(ReadFds, ClientSocket);
					
				}
				else
				{ // Something to read from socket.
					memset(Message, '\0', BUFSIZE);
					memset(Packet, '\0', BUFSIZE);
					memset(PacketID, '\0', BUFSIZE);
					memset(PacketData, '\0', BUFSIZE);

					// decode
					Return = recv(TempFds.fd_array[Index], Packet, BUFSIZE, 0);

					PacketDataLength = CPacket::packet_decode(Packet, PacketID, PacketData);
					CPacket::packet_parser_data(PacketData, "MSG", Message, PacketDataLength);
					int partyid = CPacket::packet_parser_data(PacketData, "PARTYID");
					int chatid = CPacket::packet_parser_data(PacketData, "CHATID");
					if (0 == Return)
					{ // Connection closed message has arrived.
						closesocket(TempFds.fd_array[Index]);
						char PacketBuffer[BUFSIZE];
						memset(PacketBuffer, '\0', BUFSIZE);
						CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
						char temp[BUFSIZE];
						sprintf_s(temp, "=== [%d] Has left the chatroom ===", TempFds.fd_array[Index]);
						CPacket::packet_add_data(PacketBuffer, "MSG", temp);
						char encodedMSG[BUFSIZE];
						CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
						int PacketDataLength = strlen(encodedMSG);
						send_to_all(ReadFds, encodedMSG, PacketDataLength, TempFds.fd_array[Index]);
						
						printf("Connection closed :Socket Handle [%d]\n", TempFds.fd_array[Index]);
						FD_CLR(TempFds.fd_array[Index], &ReadFds);
					}
					else if (0 > Return)
					{ // recv() function returned error.
						closesocket(TempFds.fd_array[Index]);
						char PacketBuffer[BUFSIZE];
						memset(PacketBuffer, '\0', BUFSIZE);
						CPacket::packet_add_data(PacketBuffer, "FROM", FROM_SERVER);
						char temp[BUFSIZE];
						sprintf_s(temp, "=== [%d] Has left the chatroom ===", TempFds.fd_array[Index]);
						CPacket::packet_add_data(PacketBuffer, "MSG", temp);
						char encodedMSG[BUFSIZE];
						CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
						int PacketDataLength = strlen(encodedMSG);
						send_to_all(ReadFds, encodedMSG, PacketDataLength, TempFds.fd_array[Index]);
						printf("Exceptional error :Socket Handle [%d]\n", TempFds.fd_array[Index]);
						FD_CLR(TempFds.fd_array[Index], &ReadFds);
					}
					else
					{ // Message recevied.
						const char* Str;

						//Whisper
						if ('/' == Message[0])
						{
							if (NULL != (Str = strstr(Packet, "/help"))) // show all commands
								ShowHelp(TempFds.fd_array[Index]);
							
							if (NULL != (Str = strstr(Packet, "/who"))) // show all clients online
								show_online(ReadFds, TempFds.fd_array[Index]);

							if (NULL != (Str = strstr(Packet, "/w"))) // whisper to player
								whisper_to_one(ReadFds, Message, Return, TempFds.fd_array[Index]);

							if (NULL != (Str = strstr(Packet, "/r"))) // reply to player
							{
								char sendTO[BUFSIZE];
								CPacket::packet_parser_data(PacketData, "SENDTO", sendTO, PacketDataLength);
								reply_to_one(ReadFds, Message, Return, TempFds.fd_array[Index], sendTO);
							}

							if (NULL != (Str = strstr(Packet, "/p create"))) // invite player to party
								create_party(TempFds.fd_array[Index]);

							if (NULL != (Str = strstr(Packet, "/p"))) // invite player to party
								send_party_invite(ReadFds, Message, Return, TempFds.fd_array[Index]);

							if (NULL != (Str = strstr(Packet, "/p accept"))) // accept most recent party invite
							{
								int invSender = CPacket::packet_parser_data(PacketData, "PARTYACCEPT");	
								accept_party_invite(TempFds.fd_array[Index], invSender, partyid);
							}

							if (NULL != (Str = strstr(Packet, "/p decline"))) // decline most recent party invite
							{
								int invSender = CPacket::packet_parser_data(PacketData, "PARTYDECLINE");
								decline_party_invite(ReadFds,TempFds.fd_array[Index], invSender, partyid);
							}

							if (NULL != (Str = strstr(Packet, "/p list"))) // show all members in a party
							{
								if (partyid != 1)
									list_members(TempFds.fd_array[Index], partyid);
								else
									no_party(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/p leave"))) // leave current party
							{
								if(partyid != 1)
									leave_party(TempFds.fd_array[Index], partyid);
								else
									no_party(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/p disband"))) // leave current party
							{
								if (partyid != 1)
									disband_party(TempFds.fd_array[Index], partyid);
								else
									no_party(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/p kick"))) // leave current party
							{
								if (partyid != 1)
									party_kick(TempFds.fd_array[Index], Message, Return, partyid);
								else
									no_party(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/chat a"))) // leave current party
							{
									all_chat(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/chat p"))) // leave current party
							{
								if (partyid != 1)
									party_chat(TempFds.fd_array[Index]);
								else
									no_party(TempFds.fd_array[Index]);
							}

							if (NULL != (Str = strstr(Packet, "/p transfer"))) // leave current party
							{
								if (partyid != 1)
									party_transfer(TempFds.fd_array[Index], Message, Return, partyid);
								else
									no_party(TempFds.fd_array[Index]);
							}
								
						}
						else
						{
							if (chatid == 1)
							{
								char PacketBuffer[BUFSIZE];
								memset(PacketBuffer, '\0', BUFSIZE);
								CPacket::packet_add_data(PacketBuffer, "FROM", TempFds.fd_array[Index]);
								CPacket::packet_add_data(PacketBuffer, "MSG", Message);
								char encodedMSG[BUFSIZE];
								CPacket::packet_encode(encodedMSG, "PACKET", PacketBuffer);
								int PacketDataLength = strlen(encodedMSG);
								send_to_all(ReadFds, encodedMSG, PacketDataLength, TempFds.fd_array[Index]);
							}
							if (chatid == 2)
							{
								char PacketBuffer[BUFSIZE];
								memset(PacketBuffer, '\0', BUFSIZE);
								CPacket::packet_add_data(PacketBuffer, "FROMPARTY", "Party >>");
								CPacket::packet_add_data(PacketBuffer, "FROM", TempFds.fd_array[Index]);
								CPacket::packet_add_data(PacketBuffer, "MSG", Message);

								char encodedMSG[BUFSIZE];
								int result = CPacket::packet_encode(encodedMSG, "INFO", PacketBuffer);
								int PacketDataLength = strlen(encodedMSG);

								send_to_party(ReadFds, encodedMSG, PacketDataLength, TempFds.fd_array[Index], partyid);
							}
						}
					}
				}
			}
		}
	}

	WSACleanup();

	return 0;
}
