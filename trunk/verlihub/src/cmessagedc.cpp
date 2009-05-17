/***************************************************************************
*   Original Author: Daniel Muller (dan at verliba dot cz) 2003-05        *
*                                                                         *
*   Copyright (C) 2006-2009 by Verlihub Project                           *
*   devs at verlihub-project dot org                                      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include "cmessagedc.h"
#include "cprotocommand.h"
#include <iostream>

using namespace std;
namespace nDirectConnect {
namespace nProtocol {

//#define __msBuffSize 10240
//const int cMessageDC::msBuffSize=__msBuffSize;
//char *cMessageDC::mBuffer = new char[__msBuffSize];
//int cMessageDC::msCounterMessageDC  = 0;

// these strings correspond to the enum tDCMsg in th .h file
cProtoCommand /*cMessageDC::*/sDC_Commands[]=
{
	cProtoCommand(string("$GetINFO ")),  // check: logged_in(FI), nick
	cProtoCommand(string("$Search Hub:")), // check: nick, delay //this must be first!! before the nex one
	cProtoCommand(string("$Search ")), // check: ip, delay
	cProtoCommand(string("$SR ")), // check: nick
	cProtoCommand(string("$MyINFO ")), // check: after_nick, nick, share_min_max
	cProtoCommand(string("$Key ")),
	cProtoCommand(string("$ValidateNick ")),
	cProtoCommand(string("$MyPass ")),
	cProtoCommand(string("$Version ")),
	cProtoCommand(string("$GetNickList")), // 
	cProtoCommand(string("$ConnectToMe ")), // check: ip, nick
	cProtoCommand(string("$MultiConnectToMe ")),  // not implemented
	cProtoCommand(string("$RevConnectToMe ")), // check: nick, other_nick
	cProtoCommand(string("$To: ")), // check: nick, other_nick
	cProtoCommand(string("<")), // check: nick, delay, size, line_count
	cProtoCommand(string("$Quit ")), // no chech necessary
	cProtoCommand(string("$OpForceMove $Who:")), // check: op, nick
	cProtoCommand(string("$Kick ")), // check: op, nick, conn
	cProtoCommand(string("$MultiSearch Hub:")), // check: nick, delay
	cProtoCommand(string("$MultiSearch ")),  // check: ip, delay
	cProtoCommand(string("$Supports ")),
	cProtoCommand(string("$NetInfo ")),
	cProtoCommand(string("$Ban ")),
	cProtoCommand(string("$TempBan ")),
	cProtoCommand(string("$UnBan ")),
	cProtoCommand(string("$GetBanList")),
	cProtoCommand(string("$WhoIP ")),
	cProtoCommand(string("$Banned ")),
	cProtoCommand(string("$SetTopic ")),
	cProtoCommand(string("$GetTopic ")),
	cProtoCommand(string("$BotINFO "))
};

using namespace ::nDirectConnect::nProtocol::nEnums;

cMessageDC::cMessageDC() : cMessageParser(10)
{
	SetClassName("MessageDC");
}

cMessageDC::~cMessageDC(){}

/** parses the string and sets the state variables */
int cMessageDC::Parse()    // this function call too many comparisons, it's to optimize by a tree
{// attention, AreYou returns true if the first part matches, so, somemessages may be confused
	for(int i=0; i < eDC_UNKNOWN; i++)
	{
		if ( sDC_Commands[i].AreYou(mStr) )  
		{
			mType=tDCMsg(i);
			mKWSize=sDC_Commands[i].mBaseLength;
			mLen=mStr.size();
			break;
		}
	}
	if(mType == eMSG_UNPARSED) mType=eDC_UNKNOWN;
	return mType;
}


/** splits message to it's important parts and stores their info in the chunkset mChunks */
bool cMessageDC::SplitChunks()
{
 	SetChunk(0,0,mStr.length()); // the zeroth chunk is everywhere the same

	// now try to find chunks one by one
	switch(mType)
	{
		case eDCE_SUPPORTS: break; // this has variable count of params
		case eDC_KEY:
		case eDC_VALIDATENICK:
		case eDC_MYPASS:
		case eDC_VERSION:
		case eDC_KICK:
		case eDC_QUIT:
		case eDCO_UNBAN:
		case eDCO_WHOIP:
		case eDCO_BANNED:
		case eDCO_SETTOPIC:
		case eDCB_BOTINFO:
			if (mLen < mKWSize) mError = 1;
			else SetChunk(eCH_1_PARAM,mKWSize,mLen-mKWSize);
			break;
		case eDC_GETINFO:
			if(!SplitOnTwo(mKWSize,' ', eCH_GI_OTHER , eCH_GI_NICK)) mError =1;
			break;
		case eDC_RCONNECTTOME:
			if(!SplitOnTwo(mKWSize,' ', eCH_RC_NICK, eCH_RC_OTHER)) mError =1;
			break;
		case eDC_CHAT:
			if(!SplitOnTwo(mKWSize,'>', eCH_CH_NICK, eCH_CH_MSG)) mError =1;
			if(!ChunkRedLeft( eCH_CH_MSG, 1)) mError =1;  // this is because of empty space after '>'
			break;
		case eDC_SEARCH_PAS:
			if(!SplitOnTwo(mKWSize,' ', eCH_PS_NICK, eCH_PS_QUERY)) mError =1;
			if(!SplitOnTwo('?',eCH_PS_QUERY, eCH_PS_SEARCHLIMITS, eCH_PS_SEARCHPATTERN,0)) mError =1;
			break;
		case eDC_CONNECTTOME:
			if(!SplitOnTwo(mKWSize,' ', eCH_CM_NICK, eCH_CM_ACTIVE)) mError =1;
			if(!SplitOnTwo(':', eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT)) mError =1;
			break;
		case eDC_TO:
			// $To: <other nick> From: <my nick> $<<nick>> <message>
			// eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG } ;
			//	if(!SplitOnTwo( mKWSize,' ', eCH_PM_TO, eCH_PM_FROM)) mError =1;
			//	ChunkRedLeft( eCH_PM_FROM, 6);  // skip the "From: " part
			//	if(!SplitOnTwo( ' ', eCH_PM_FROM, eCH_PM_FROM, eCH_PM_CHMSG)) mError =1;
			//	ChunkRedLeft( eCH_PM_CHMSG, 2);  // skip the "$<" part
			//	if(!SplitOnTwo( '>', eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG)) mError =1;
			//	ChunkRedLeft( eCH_PM_MSG, 1);  // skip the " " part (after nick)
			if(!SplitOnTwo( mKWSize," From: ", eCH_PM_TO, eCH_PM_FROM)) mError =1;
			if(!SplitOnTwo( " $<", eCH_PM_FROM, eCH_PM_FROM, eCH_PM_CHMSG)) mError =1;
			if(!SplitOnTwo( '>', eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG)) mError =1;
			if(!ChunkRedLeft( eCH_PM_MSG, 1)) mError = 1;  // skip the " " part (after nick)
		break;
		case eDC_MYNIFO:
			// $MyINFO $ALL <nick> <interest>$ $<speed>$<e-mail>$<sharesize>$
			// eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE
			if(!SplitOnTwo( mKWSize,' ', eCH_MI_DEST, eCH_MI_NICK)) mError =1;
			if(!SplitOnTwo( ' ', eCH_MI_NICK,eCH_MI_NICK, eCH_MI_INFO)) mError =1;
			if(!SplitOnTwo( '$', eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED)) mError =1;
			if(!ChunkRedLeft( eCH_MI_SPEED, 2)) mError = 1;
			if(!SplitOnTwo( '$', eCH_MI_SPEED, eCH_MI_SPEED, eCH_MI_MAIL)) mError =1;
			if(!SplitOnTwo( '$', eCH_MI_MAIL, eCH_MI_MAIL, eCH_MI_SIZE)) mError =1;
			if(!ChunkRedRight(eCH_MI_SIZE,1)) mError =1;
			break;
		case eDC_MCONNECTTOME: // not implemented
			break;
		case eDC_OPFORCEMOVE:
			 //$OpForceMove $Who:<victimNick>$Where:<newIp>$Msg:<reasonMsg>
			 //NICK DEST REASON
			if(!SplitOnTwo( mKWSize,'$', eCH_FM_NICK, eCH_FM_DEST)) mError =1;
			if(!ChunkRedLeft( eCH_FM_DEST, 6)) mError = 1;  // skip the "Where:" part
			if(!SplitOnTwo( '$', eCH_FM_DEST, eCH_FM_DEST,eCH_FM_REASON)) mError =1;
			if(!ChunkRedLeft( eCH_FM_REASON, 4)) mError = 1;  // skip the "Msg:" part
			break;
		case eDC_MSEARCH: // not implemented, but should be same as search
		case eDC_SEARCH: // active
			// $Search <ip>:<port> <searchstring>
			//enum {eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY}
			if(!SplitOnTwo( mKWSize,' ', eCH_AS_ADDR, eCH_AS_QUERY)) mError =1;
			if(!SplitOnTwo( ':', eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT)) mError =1;
			if(!SplitOnTwo('?',eCH_AS_QUERY, eCH_AS_SEARCHLIMITS, eCH_AS_SEARCHPATTERN,0)) mError =1;
			break;
		case eDC_SR:
			// search result $SR <resultNick> <filepath>^E<filesize> <freeslots>/<totalslots>^E<hubname> (<hubhost>[:<hubport>])^E<searchingNick>
			//               $SR <resultNick> <filepath>^E<filesize> <freeslots>/<totalslots>^ETTH:<TTH-ROOT> (<hub_ip>[:<hubport>])^E<searchingNick>
			//               $SR <resultNick> <filepath>^E           <freeslots>/<totalslots>^ETTH:<TTH-ROOT> (<hub_ip>:[hubport]>)^E<searchingNick>
			//           1)      ----FROM----|-----------------------------------(PATH)--------------------------------------------------------------
			//           2)                  |-------PATH-----------|----------------------(SLOTS)---------------------------------------------------
			//           3)                                         |-----------SLOTS---------|-------------(HUBINFO)--------------------------------
			//           4)                                                                   |-------------(HUBINFO)---------------|----TO----------
			//           5)
			// 
			// enum {eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO}
			if(!SplitOnTwo( mKWSize,' ', eCH_SR_FROM, eCH_SR_PATH)) mError =1;
			//if(!SplitOnTwo( ' ', eCH_SR_PATH, eCH_SR_PATH, eCH_SR_SLOTS)) mError =1;
			//if(!SplitOnTwo( ' ', eCH_SR_SLOTS, eCH_SR_SLOTS, eCH_SR_HUBINFO) mError =1;
			//if(!SplitOnTwo( ' ', eCH_SR_HUBINFO, eCH_SR_HUBINFO, eSR_TO) mError =1;
			
			if(!SplitOnTwo( 0x05, eCH_SR_PATH, eCH_SR_PATH,  eCH_SR_SIZE)) mError =1;
			if(!SplitOnTwo( 0x05, eCH_SR_SIZE, eCH_SR_HUBINFO, eCH_SR_TO, false)) mError =1;
			if(SplitOnTwo( 0x05,eCH_SR_HUBINFO, eCH_SR_SIZE, eCH_SR_HUBINFO))
			{
				if(!SplitOnTwo( ' ', eCH_SR_SIZE, eCH_SR_SIZE, eCH_SR_SLOTS)) mError =1;
				if(!SplitOnTwo( '/', eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO )) mError =1;
			}else
				SetChunk(eCH_SR_SIZE,0,0);
			break;
		case eDCM_NETINFO:
			if(!SplitOnTwo( mKWSize,'$', eCH_NI_HUBS, eCH_NI_SLOTS)) mError =1;
			if(!SplitOnTwo( '$', eCH_NI_SLOTS, eCH_NI_SLOTS,  eCH_NI_ACTIVE)) mError =1;
			break;
		case eDCO_BAN:
			if(!SplitOnTwo( mKWSize,'$', eCH_NB_NICK, eCH_NB_REASON)) mError =1;
			break;
		case eDCO_TBAN:
			if(!SplitOnTwo( mKWSize,'$', eCH_NB_NICK, eCH_NB_TIME)) mError =1;
			if(!SplitOnTwo( '$', eCH_NB_TIME, eCH_NB_TIME,  eCH_NB_REASON)) mError =1;
			break;
		default:
			break;
	}
	return mError;
}


};
};

