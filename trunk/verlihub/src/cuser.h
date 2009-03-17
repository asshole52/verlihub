/***************************************************************************
                          cuser.h  -  description
                             -------------------
    begin                : Thu May 8 2003
    copyright            : (C) 2003 by Daniel Muller
    email                : dan at verliba dot cz
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CUSER_H
#define CUSER_H

#include <string>
#include "cobj.h"
#include "cconndc.h"
#include "cfreqlimiter.h"
#include "cpenaltylist.h"

using namespace std;
using namespace nUtils;

namespace nDirectConnect
{

namespace nEnums {

/** several types of users with some differences .. for later
	everyone is allowed to create no more then he is -1
*/
typedef enum tUserCl
{
	eUC_PINGER = -1, /// pinger user
	eUC_NORMUSER = 0, ///< Regular user
	eUC_REGUSER = 1, ///< Registered user
	eUC_VIPUSER = 2, ///< VIP user
	eUC_OPERATOR = 3, ///< Operator
	eUC_CHEEF = 4, ///< Cheef operator
	eUC_ADMIN = 5,///< Hub Admin
	eUC_MASTER = 10 ///< Hub master, creates aminds, etc...
};

/** user rights, there will be defaults for every class, but they can be changed */
typedef enum
{
	eUR_NOINFO  = 0x000001, //< can login without user info
	eUR_NOSHARE = 0x000002, //< can login with less than share limit
	eUR_CHAT 	= 0x000004, //<  can talk in the main chat
	eUR_SEARCH  = 0x000008, //< can search
	eUR_STPM    = 0x000010, //< stealth PM (with other nick, that doesn't exist or not registered)
	eUR_OPCHAT	= 0x000020, //< can opchat
	eUR_REDIR   = 0x000040, //< can op force move to a selected hublist
	eUR_REDIRANY= 0x000080, //< can op force move to any hub
	eUR_KICK    = 0x000100, //< can kick (with a previous chat message)
	eUR_DROP    = 0x000200, //< can drop users (without the chat message)
	eUR_TBAN  	= 0x000400, //< can use tban up to a configurable limit
	eUR_PBAN 	= 0x000800, //< can ban longer than the tban limit
	eUR_GETIP  	= 0x001000, //< get user's ip
	eUR_FULL1   = 0x002000, //< connection on almost full hub
	eUR_FULL2   = 0x004000, //< connection on completely full hub (someone is doconnected)
	eUR_MASSMSG = 0x008000, //<
	eUR_MASSRED = 0x010000, //< masss redirect
	eUR_S_MAXU  = 0x020000, //< set max users
	eUR_S_MINS  = 0x040000, //< set minshare
	eUR_S_HUBN  = 0x080000, //< set hubname
	eUR_S_REDI	= 0x100000,  //< set redirhub(s) etc...
	eUR_CTM     = 0x200000,  // start download
	eUR_PM      = 0x400000,   // private messages
	eUR_REG     = 0x800000 //< can create or edit registered users (lowr classes)
} tUserRights;

typedef enum
{
	eFH_SEARCH,
	eFH_CHAT,
	eFH_PM,
	eFH_LAST_FH
} tFloodHashes;

typedef enum
{
	eFC_PM,
	eFC_LAST_FC
} tFloodCounters;
};

using namespace ::nDirectConnect::nTables;
using namespace ::nDirectConnect::nEnums;

/** I should define for each class of users a mask of rights that they can't get ad of
 * those that they always get.. this should be configurable 2DO TODO*/

class cConnDC;
class cServerDC;


/**
 * Basic class for users, every users must have at least this info..
 * @author Daniel Muller
 * */
class cUserBase : public cObj
{
public:
	cUserBase();
	cUserBase(const string &nick);
	virtual ~cUserBase();
		virtual bool CanSend(); 
	virtual void Send(string &data, bool pipe, bool cache=true);
public: // Public attributes
	/** user's  nickname */
	string mNick;
	string mMyINFO;
	string mMyINFO_basic;
	/** the uers's class */
	nDirectConnect::nEnums::tUserCl mClass;
	/** if the user was already added to the list 
	 * - it's more like is the Hello command was sent... */
	bool mInList;
};

/**Any type of dc user, contains info abou the connected users
  *@author Daniel Muller
  */
class cUser : public cUserBase
{
public:
	cUser();
	/** constructor with a nickname */
	cUser(const string &nick);
	virtual ~cUser();
	virtual bool CanSend(); 
	virtual void Send(string &data, bool pipe, bool cache=true);
	/** check for the right to ... */
	inline int HaveRightTo(unsigned int mask){ return mRights & mask; }
	/** return tru if user needs a password and the password is correct */
	bool CheckPwd(const string &pwd);
	/** perform a registration: set class, rights etc... precondition: password was al right */
	void Register();
public: // Public attributes
	/** pointer to the connection */
	cConnDC * mxConn;
	/** pointer to the srever
		this is extern pointer do not delete */
	cServerDC * mxServer;
	/** the part of MyINFO command, that is common for all */
	string mEmail;
	
	/** active connection state */
	bool IsPassive;

	/** user's rights */
	unsigned long mRights;
	/** different time stamps */
	struct sTimes
	{
		/******** [s] ****/
		/** connection time */
		cTime connect;
		/** login time, user entered userlist */
		cTime login;
		/** last search */
		cTime search;
		/** my_info */
		cTime info;
		/** chat */
		cTime chat;
		/** getnicklist */
		cTime nicklist;	
		/** private message to any user*/
		cTime pm;
		sTimes():connect(0l),login(0l),search(0l),info(0l),chat(0l),nicklist(0l),pm(0l){}
	};
	sTimes mT;
	typedef tHashArray<void*>::tHashType tFloodHashType;
	tFloodHashType mFloodHashes[nDirectConnect::nEnums::eFH_LAST_FH];
 	int mFloodCounters[nDirectConnect::nEnums::eFC_LAST_FC];
  
	/** 0 means perm ban, otherwiese in seconds */
	long mBanTime;
	/** indicates whether user is to ban after the following kick */
	bool mToBan;
	/** minimal class users that can see this one */
	nDirectConnect::nEnums::tUserCl mVisibleClassMin;
	/** minimal class users that can see this one as operator */
	nDirectConnect::nEnums::tUserCl mOpClassMin;
	/** user's sharesize */
	__int64 mShare;
	/** the list of nicks queued to send either as to nicklist or myinfo. */
	string mQueueUL;
	/** ip, that user has for downloads ises in connect to me */
	string mIP4DL;
	/** specifies the one bove higher class of users can see kick chat mesages */
	int mHideKicksForClass;
	/** flood detectors */
	cFreqLimiter mFloodPM;
	/** chat discrimination */
	long mGag;
	long mNoPM;
	long mNoSearch;
	long mNoCTM;
	long mCanKick;
	long mCanDrop;
	long mCanTBan;
	long mCanPBan;
	long mCanShare0;
	long mCanReg;
	long mCanOpchat;

	/** kick messages hide from chat */
	bool mHideKick;
	/** hide share **/
	bool mHideShare;
	/** class protection against kicking */
	int mProtectFrom;

protected: // Protected attributes

	public: long ShareEnthropy(const string &sharesize);
	void DisplayInfo(ostream &os, int DisplClass);

	/*!
		\fn Can(unsigned Right, long now = 0, unsigned OtherClass = 0)
		return true if the user has given rights
	*/
	bool Can(unsigned Right, long now = 0, unsigned OtherClass = 0);
	void SetRight(unsigned Right, long until, bool allow=false);
	void ApplyRights(cPenaltyList::sPenalty &pen);
};

namespace nProtocol{ class cMessageDC; };
using nProtocol::cMessageDC;
class cUserCollection;
class cChatConsole;

class cUserRobot : public cUser
{
public:
	cUserRobot(cServerDC *server = NULL){mxServer = server;};
	virtual ~cUserRobot(){};
	/** constructor with a nickname */
	cUserRobot(const string &nick, cServerDC *server = NULL):cUser(nick){mxServer = server;};

	virtual bool ReceiveMsg(cConnDC *conn, cMessageDC *msg) = 0;
	bool SendPMTo(cConnDC *conn, const string &msg);
};

class cChatRoom : public cUserRobot
{
public:
	cChatRoom(const string &nick, cUserCollection *col, cServerDC *server = NULL);
	virtual ~cChatRoom();
	cUserCollection *mCol;
	virtual bool ReceiveMsg(cConnDC *conn, cMessageDC *msg);
	virtual void SendPMToAll(const string & Msg, cConnDC *FromConn);
	virtual bool IsUserAllowed(cUser *);
	cChatConsole *mConsole;
};

class cOpChat : public cChatRoom
{
public:
	cOpChat(cServerDC *server);
	virtual bool IsUserAllowed(cUser *);
};

class cMainRobot : public cUserRobot
{
public:
	cMainRobot(const string &nick, cServerDC *server = NULL):cUserRobot(nick,server){};
	virtual bool ReceiveMsg(cConnDC *conn, cMessageDC *msg);
};

namespace nPlugin{
	class cVHPlugin;
};
using nPlugin::cVHPlugin;

class cPluginRobot : public cUserRobot
{
public:
	cPluginRobot(const string &nick, cVHPlugin *pi, cServerDC *server = NULL);
	cVHPlugin *mPlugin;
	virtual bool ReceiveMsg(cConnDC *conn, cMessageDC *msg);
};


};


#endif