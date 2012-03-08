/**************************************************************************
*   Original Author: Frog (frg at otaku-anime dot net) 2007-209           *
*                                                                         *
*   Copyright (C) 2010-2011 by Verlihub Project                           *
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
#ifndef CPIPYTHON_H
#define CPIPYTHON_H

#include "cpythoninterpreter.h"
#include "cconsole.h"
#include "src/cconndc.h"
#include "src/cvhplugin.h"
#include "src/cserverdc.h"
#include "src/cuser.h"
#include "src/script_api.h"
#include <iostream>
#include <vector>
#include <dlfcn.h>

#define PYTHON_PI_IDENTIFIER "Python"
#define PYTHON_PI_VERSION "1.1"

using std::vector;
namespace nVerliHub {
	namespace nPythonPlugin {

class cpiPython : public nPlugin::cVHPlugin
{
public:
	cpiPython();
	virtual ~cpiPython();
	bool RegisterAll();
	virtual void OnLoad(nSocket::cServerDC *);
	virtual bool OnNewConn(nSocket::cConnDC *);
	virtual bool OnCloseConn(nSocket::cConnDC *);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgPM(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMCTo(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSearch(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSR(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnFirstMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgValidateNick(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAny(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAnyEx(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSupport(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgBotINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgVersion(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyPass(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgRevConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnUnknownMsg(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnOperatorCommand(nSocket::cConnDC *, std::string *);
	virtual bool OnOperatorKicks(cUser *, cUser *, std::string *);
	virtual bool OnOperatorDrops(cUser *, cUser *);
	virtual bool OnValidateTag(nSocket::cConnDC *, cDCTag *);
	virtual bool OnUserCommand(nSocket::cConnDC *, std::string *);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnTimer(long msec);
	virtual bool OnNewReg(nTables::cRegUserInfo *);
	virtual bool OnNewBan(nTables::cBan *);

	bool AutoLoad();
	const char *GetName (const char *path);
	int SplitMyINFO(const char *msg, const char **nick, const char **desc, const char **tag, const char **speed, const char **mail, const char **size);
	const char *GetConf(const char *conf, const char *var);
	bool SetConf(const char *conf, const char *var, const char *val);
	w_Targs* SQL (int id, w_Targs* args);
	void LogLevel(int);
	bool IsNumber( const char* s );
	int char2int( char c );
	cPythonInterpreter *GetInterpreter(int id);
	bool CallAll(int func, w_Targs* args);
	int Size() { return mPython.size(); }

	void Empty()
	{
		tvPythonInterpreter::iterator it;
		for(it = mPython.begin(); it != mPython.end(); ++it)
		{
			if(*it != NULL) delete *it;
			*it = NULL;
		}
		mPython.clear();
	}

	void AddData(cPythonInterpreter *ip)
	{
		mPython.push_back(ip);
	}

	cPythonInterpreter * operator[](int i)
	{
		if(i < 0 || i > Size()) return NULL;
		return mPython[i];
	}

	cConsole mConsole;
	nMySQL::cQuery *mQuery;
	typedef vector<cPythonInterpreter *> tvPythonInterpreter;
	tvPythonInterpreter mPython;
	string mScriptDir;
	bool online;

	static void*        lib_handle;
	static w_TBegin     lib_begin;
	static w_TEnd       lib_end;
	static w_TReserveID lib_reserveid;
	static w_TLoad      lib_load;
	static w_TUnload    lib_unload;
	static w_THasHook   lib_hashook;
	static w_TCallHook  lib_callhook;
	static w_THookName  lib_hookname;
	static w_Tpack      lib_pack;
	static w_Tunpack    lib_unpack;
	static w_TLogLevel  lib_loglevel;
	static w_Tpackprint lib_packprint;

	static string botname;
	static string opchatname;
	static int log_level;
	static nSocket::cServerDC *server;
	static cpiPython *me;
};
	}; // namespace nPythonPlugin

extern "C" w_Targs* _SendDataToUser    (int id, w_Targs* args);  //(char *data, char *nick);
extern "C" w_Targs* _SendDataToAll     (int id, w_Targs* args);  //(char *data, long min_class, long max_class);
extern "C" w_Targs* _SendPMToAll       (int id, w_Targs* args);  //(char *data, char *from, long min_class, long max_class);
extern "C" w_Targs* _mc                (int id, w_Targs* args);  //(char *data);
extern "C" w_Targs* _usermc            (int id, w_Targs* args);  //(char *data, char *nick);
extern "C" w_Targs* _classmc           (int id, w_Targs* args);  //(char *data, long min_class, long max_class);
extern "C" w_Targs* _pm                (int id, w_Targs* args);  //(char *data, char *nick);
extern "C" w_Targs* _CloseConnection   (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _GetMyINFO         (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _SetMyINFO         (int id, w_Targs* args);  //(char *nick, char *desc, char *speed, char *email, char *size);
extern "C" w_Targs* _GetUserClass      (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _GetNickList       (int id, w_Targs* args);  //();
extern "C" w_Targs* _GetOpList         (int id, w_Targs* args);  //();
extern "C" w_Targs* _GetUserHost       (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _GetUserIP         (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _GetUserCC         (int id, w_Targs* args);  //(char *nick);
extern "C" w_Targs* _GetIPCC           (int id, w_Targs* args);  //(char *ip);
extern "C" w_Targs* _GetIPCN           (int id, w_Targs* args);  //(char *ip);
extern "C" w_Targs* _Ban               (int id, w_Targs* args);  //(char *nick, long howlong, long bantype);
extern "C" w_Targs* _KickUser          (int id, w_Targs* args);  //(char *op, char *nick, char *data);
extern "C" w_Targs* _ParseCommand      (int id, w_Targs* args);  //(char *data);
extern "C" w_Targs* _SetConfig         (int id, w_Targs* args);  //(char *conf, char *var, char *val);
extern "C" w_Targs* _GetConfig         (int id, w_Targs* args);  //(char *conf, char *var);
extern "C" w_Targs* _AddRobot          (int id, w_Targs* args);  //(char *nick, long uclass, char *desc, char *speed, char *email, char *share);
extern "C" w_Targs* _DelRobot          (int id, w_Targs* args);  //(char *robot);
extern "C" w_Targs* _SQL               (int id, w_Targs* args);  //(char *query, long limit);
extern "C" w_Targs* _GetUsersCount     (int id, w_Targs* args);  //();
extern "C" w_Targs* _GetTotalShareSize (int id, w_Targs* args);  //();
extern "C" w_Targs* _UserRestrictions  (int id, w_Targs* args);  //(char *nick, char *gagtime, char *nopmtime, char *nosearchtime, char *noctmtime);
extern "C" w_Targs* _Topic             (int id, w_Targs* args);  //(char* topic);
}; // namespace nVerliHub
#define w_ret0    cpiPython::lib_pack("l", (long)0)
#define w_ret1    cpiPython::lib_pack("l", (long)1)
#define w_retnone cpiPython::lib_pack("")

// logging levels:
// 0 - plugin critical errors, interpreter errors, bad call arguments
// 1 - callback / hook logging - only their status
// 2 - all function parameters and return values are printed
// 3 - debugging info is printed
#define log(...)                                        { printf( __VA_ARGS__ ); fflush(stdout); }
#define log1(...) { if (cpiPython::log_level > 0) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log2(...) { if (cpiPython::log_level > 1) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log3(...) { if (cpiPython::log_level > 2) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log4(...) { if (cpiPython::log_level > 3) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log5(...) { if (cpiPython::log_level > 4) { printf( __VA_ARGS__ ); fflush(stdout); }; }

#define dprintf(...) { printf("%s:%u: "__FILE__, __LINE__); printf( __VA_ARGS__ ); fflush(stdout); }

#endif
