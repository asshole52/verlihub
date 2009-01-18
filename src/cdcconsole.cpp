/***************************************************************************
                          cdcconsole.cpp  -  description
                             -------------------
    begin                : Wed Jul 2 2003
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

#include "cserverdc.h"
#include "cdcconsole.h"
#include "cuser.h"
#include "ckicklist.h"
#include "ckick.h"
#include "cinterpolexp.h"
#include "cban.h"
#include "cbanlist.h"
#include "cpenaltylist.h"
#include "ctime.h"
#include "creglist.h"
#include <sstream>
#include <iomanip>
#include "ccommand.h"
#include "ctriggers.h"
#include "ccustomredirects.h"
#define BAN_EREASON "Please provide a valid reason"

using nUtils::cTime;

namespace nDirectConnect {

using namespace nTables;

cPCRE cDCConsole::mIPRangeRex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)((\\/(\\d+))|(\\.\\.|-)(\\d+\\.\\d+\\.\\d+\\.\\d+))?$",0);

cDCConsole::cDCConsole(cServerDC *s, cMySQL &mysql):
	cDCConsoleBase(s),
	mServer(s),
	mTriggers(NULL),
	mRedirects(NULL),
	mCmdr(this),
	mUserCmdr(this),
 	mCmdBan(int(eCM_BAN),".(del|rm|un|info|list|ls)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ?", "(\\S+)( (.*)$)?", &mFunBan),
	mCmdGag(int(eCM_GAG),".(un)?(gag|nochat|nopm|noctm|nosearch|kvip|maykick|noshare|mayreg|mayopchat) ", "(\\S+)( (\\d+\\w))?", &mFunGag),
	mCmdTrigger(int(eCM_TRIGGER),".(ft|trigger)(\\S+) ", "(\\S+) (.*)", &mFunTrigger),
	mCmdSetVar(int(eCM_SET),".(set|=) ", "(\\[(\\S+)\\] )?(\\S+) (.*)", &mFunSetVar),
	mCmdRegUsr(int(eCM_REG),".r(eg)?(n(ew)?(user)?|del(ete)?|pass(wd)?|(en|dis)able|(set)?class|(protect|hidekick)(class)?|set|=|info) ", "(\\S+)( (((\\S+) )?(.*)))?", &mFunRegUsr),
	mCmdRaw(int(eCM_RAW),".proto(\\S+)_(\\S+) ","(.*)", &mFunRaw),
	mCmdCmd(int(eCM_CMD),".cmd(\\S+)","(.*)", &mFunCmd),
	mCmdWho(int(eCM_WHO),".w(ho)?(\\S+) ","(.*)", &mFunWho),
	mCmdKick(int(eCM_KICK),".(kick|drop|flood) ","(\\S+)( (.*)$)?", &mFunKick, eUR_KICK ),
	mCmdInfo(int(eCM_INFO),".(\\S+)info ?", "(\\S+)?", &mFunInfo),
	mCmdPlug(int(eCM_PLUG),".plug(in|out|list|reg|reload) ","(\\S+)( (.*)$)?", &mFunPlug),
	mCmdReport(int(eCM_REPORT),"\\+report ","(\\S+)( (.*)$)?", &mFunReport),
	mCmdBc(int(eCM_BROADCAST),".(bc|broadcast|oc|ops|regs|guests|vips|cheefs|admins|masters)( |\\r\\n)","(.*)$", &mFunBc), // |ccbc|ccbroadcast
	mCmdRedirConnType(int(eCM_CONNTYPE),".(\\S+)conntype ?","(.*)$",&mFunRedirConnType),
	mCmdRedirTrigger(int(eCM_TRIGGERS),".(\\S+)trigger ?","(.*)$",&mFunRedirTrigger),
	mCmdCustomRedir(int(eCM_CUSTOMREDIR),".(\\S+)redirect ?","(.*)$",&mFunCustomRedir),
	mCmdGetConfig(int(eCM_GETCONFIG),".(gc|getconfig) ?","(\\[(\\S+)\\])?", &mFunGetConfig),
	mConnTypeConsole(this),
	mTriggerConsole(NULL),
	mRedirectConsole(NULL)
{
	mTriggers = new cTriggers(mServer);	
	mTriggers->OnStart();
	mTriggerConsole = new cTriggerConsole(this);
	
	mRedirects = new cRedirects(mServer);
	mRedirects->OnStart();
	mRedirectConsole = new cRedirectConsole(this);

	mFunRedirConnType.mConsole = &mConnTypeConsole;
	mFunRedirTrigger.mConsole = mTriggerConsole;
	mFunCustomRedir.mConsole = mRedirectConsole;
	mCmdr.Add(&mCmdBan);
	mCmdr.Add(&mCmdGag);
	mCmdr.Add(&mCmdTrigger);
	mCmdr.Add(&mCmdSetVar);
	mCmdr.Add(&mCmdRegUsr);
	mCmdr.Add(&mCmdInfo);
	mFunInfo.mInfoServer.SetServer(mOwner);
	mCmdr.Add(&mCmdRaw);
	mCmdr.Add(&mCmdWho);
	mCmdr.Add(&mCmdKick);
	mCmdr.Add(&mCmdPlug);
	mCmdr.Add(&mCmdCmd);
	mCmdr.Add(&mCmdBc);
	mCmdr.Add(&mCmdRedirConnType);
	mCmdr.Add(&mCmdRedirTrigger);
	mCmdr.Add(&mCmdCustomRedir);
	mCmdr.Add(&mCmdGetConfig);
	mCmdr.InitAll(this);
	mUserCmdr.Add(&mCmdReport);
	mUserCmdr.InitAll(this);
}

cDCConsole::~cDCConsole(){
	if (mTriggers) delete mTriggers;
	mTriggers = NULL;
	if (mTriggerConsole) delete mTriggerConsole;
	mTriggerConsole = NULL;
	if (mRedirects) delete mRedirects;
	mRedirects = NULL;
	if (mRedirectConsole) delete mRedirectConsole;
	mRedirectConsole = NULL;
}

/** act on op's command */
int cDCConsole::OpCommand(const string &str, cConnDC * conn)
{
	istringstream cmd_line(str);
	string cmd;
	ostringstream os;
	cmd_line >> cmd;

	if(!conn || !conn->mpUser) return 0;
	tUserCl cl=conn->mpUser->mClass;

	switch(cl)
	{
		case eUC_MASTER:
			if( cmd == "!quit"      ) return CmdQuit(cmd_line,conn,0);
			if( cmd == "!restart"   ) return CmdQuit(cmd_line,conn,1);
			if( cmd == "!dbg_hash"  ) {
				mOwner->mUserList.DumpProfile(cerr);
				return 1;
		 	}
			if( cmd == "!core_dump"   ) return CmdQuit(cmd_line,conn,-1);
			if( cmd == "!hublist"   ) { mOwner->RegisterInHublist(mOwner->mC.hublist_host, mOwner->mC.hublist_port, conn); return 1;}
		case eUC_ADMIN:
			if( cmd == "!userlimit" || cmd=="!ul" ) return CmdUserLimit(cmd_line,conn);
			if( cmd == "!reload"    || cmd=="!re" ) return CmdReload(cmd_line,conn);
		case eUC_CHEEF:
			if( cmd == "!ccbroadcast" || cmd=="!ccbc" ) return CmdCCBroadcast(cmd_line,conn,eUC_NORMUSER,eUC_MASTER);
			if( cmd == "!class"                   ) return CmdClass     (cmd_line, conn);
			if( cmd == "!protect"                 ) return CmdProtect   (cmd_line, conn);
		case eUC_OPERATOR:
			if( cmd == "!topic"      || cmd=="!hubtopic" ) return CmdTopic(cmd_line,conn);
			if( cmd == "!getip"      || cmd=="!gi" ) return CmdGetip(cmd_line,conn);
			if( cmd == "!gethost"    || cmd=="!gh" ) return CmdGethost(cmd_line,conn);
			if( cmd == "!getinfo"    || cmd=="!gn" ) return CmdGetinfo(cmd_line,conn);
			if( cmd == "!help"       || cmd=="!?"  ) return CmdHelp(cmd_line,conn);
			if( cmd == "!hideme"     || cmd=="!hm" ) return CmdHideMe    (cmd_line, conn);
			if( cmd == "!hidekick"   || cmd=="!hk" ) return CmdHideKick  (cmd_line, conn);
			if( cmd == "!unhidekick" || cmd=="!uhk") return CmdUnHideKick(cmd_line, conn);
			if( cmd == "!commands"   || cmd=="!cmds") return CmdCmds(cmd_line,conn);

			try
			{
				if(mCmdr.ParseAll(str, os, conn) >= 0)
				{
					mOwner->DCPublicHS(os.str().c_str(),conn);
					return 1;
				}
			}
			catch(const char *ex)
			{
				if(Log(0)) LogStream() << "Exception in commands: " << ex << endl;
			}
			catch (...)
			{
				if(Log(0)) LogStream() << "Exception in commands.." << endl;
			}

		break;
		default: return 0;
		break;
	}
	if (mTriggers->DoCommand(conn,cmd, cmd_line, *mOwner))
		return 1;
	return 0;
}


/** act on usr's command */
int cDCConsole::UsrCommand(const string & str, cConnDC * conn)
{
	istringstream cmd_line(str);
	ostringstream os;
	string cmd;
	if (mOwner->mC.disable_usr_cmds)
	{
		mOwner->DCPublicHS("This functionality is currently disabled.",conn);
		return 1;
	}
	cmd_line >> cmd;

	switch(conn->mpUser->mClass)
	{
		case eUC_MASTER:
		case eUC_ADMIN:
		case eUC_CHEEF:
		case eUC_OPERATOR:
		case eUC_VIPUSER:
		case eUC_REGUSER:
			if( cmd == "+kick" ) return CmdKick( cmd_line, conn);
		case eUC_NORMUSER:
			if( cmd == "+passwd" ) return CmdRegMyPasswd( cmd_line, conn);
			if( cmd == "+help"     ) return CmdHelp(cmd_line , conn);
			if( cmd == "+myinfo" ) return CmdMyInfo( cmd_line, conn);
			if( cmd == "+myip" ) return CmdMyIp( cmd_line, conn);
			if( cmd == "+me" ) return CmdMe( cmd_line, conn);
			if( cmd == "+regme" ) return CmdRegMe( cmd_line, conn);
			if( cmd == "+chat" ) return CmdChat( cmd_line, conn, true);
			if( cmd == "+nochat" ) return CmdChat( cmd_line, conn, false);
			if(mUserCmdr.ParseAll(str, os, conn) >= 0)
			{
				mOwner->DCPublicHS(os.str().c_str(),conn);
				return 1;
			}
		break;
		default: break;
	}

	if (mTriggers->DoCommand(conn,cmd, cmd_line, *mOwner))
		return 1;
	return 0;
}


/** get user's ip */
int cDCConsole:: CmdGetip(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;

	while(cmd_line.good())
	{
		cmd_line >> s;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user && user-> mxConn )
			os << mOwner->mL.user << ": " << s << mOwner->mL.ip << ": " << user->mxConn->AddrIP() << endl;
		else
			os << mOwner->mL.user << ": " << s << mOwner->mL.not_in_userlist << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;

}

/** list commands */
int cDCConsole::CmdCmds(istringstream &cmd_line , cConnDC *conn)
{
	ostringstream os;
	string omsg;
	os << "\r\n[::] Full list of commands: \r\n";
	mCmdr.List(& os);
	mOwner->mP.EscapeChars(os.str(), omsg);
	mOwner->DCPublicHS(omsg.c_str(),conn);
	return 1;
}

/** get user's host */
int cDCConsole::CmdGethost(istringstream &cmd_line , cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;
	while(cmd_line.good())
	{
		cmd_line >> s;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user && user->mxConn)
		{
			if(!mOwner->mUseDNS)
				user->mxConn->DNSLookup();
			os << mOwner->mL.user << ": " << s << " " << mOwner->mL.host << ": " << user->mxConn->AddrHost() << endl;
		}
		else     os << mOwner->mL.user << ": " << s << mOwner->mL.not_in_userlist << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

/** get user's host and ip */
int cDCConsole::CmdGetinfo(istringstream &cmd_line , cConnDC *conn )
{
	ostringstream os;
	string s;
	cUser * user;
	while(cmd_line.good())
	{
		cmd_line >> s;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user && user->mxConn)
		{
			if(!mOwner->mUseDNS)
				user->mxConn->DNSLookup();
			os << mOwner->mL.user << ": " << s
				 << " " << mOwner->mL.ip << ": " << user->mxConn->AddrIP()
				 << " " << mOwner->mL.host << ": " << user->mxConn->AddrHost()
				 << " " << "CC: " << user->mxConn->mCC << endl;
		}
		else     os << mOwner->mL.user << ": " << s << mOwner->mL.not_in_userlist << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

/** quit program */
int cDCConsole::CmdQuit(istringstream &, cConnDC * conn, int code)
{
	ostringstream os;
	if(conn->Log(1)) conn->LogStream() << "Stopping hub with code " << code << " .";
	os << "[::] Stopping Hub...";
	mOwner->DCPublicHS(os.str(),conn);
	if (code >= 0) {
		mOwner->stop(code);
	} else {
		*(int*)1 = 0;
	}
	return 1;
}

bool cDCConsole::cfGetConfig::operator()()
{
	ostringstream os;
/*TODO change to usercan*/;
	if (mConn->mpUser->mClass < eUC_ADMIN) {
		*mOS << "no rights ";
		return false;
	}
	string file;
	cConfigBaseBase::tIVIt it;
	const int max = 60;
	const int width = 5;
	GetParStr(2, file);
	if(!file.size()) 
	{
		for(it = mS->mC.mvItems.begin();it != mS->mC.mvItems.end();it++)
			os << "\r[::]  " << setw(width) << setiosflags(ios::left) << mS->mC.mhItems.GetByHash(*it)->mName << setiosflags(ios::right) <<"    =   " << *(mS->mC.mhItems.GetByHash(*it)) << "\r\n";
		} else {
		mS->mSetupList.OutputFile(file.c_str(), os);
	}
	mS->DCPrivateHS(os.str(),mConn);
	return true;
}

/** show all variables along with their values */
int cDCConsole::CmdGetconfig(istringstream & , cConnDC * conn)
{
	ostringstream os;
	cConfigBaseBase::tIVIt it;
	for(it = mOwner->mC.mvItems.begin();it != mOwner->mC.mvItems.end(); it++)
		os << setw(20) << mOwner->mC.mhItems.GetByHash(*it)->mName << " = " << *(mOwner->mC.mhItems.GetByHash(*it)) << "\r\n";
	mOwner->DCPrivateHS(os.str(),conn);
	return 1;
}

/** send help message corresponding to connection */
int cDCConsole::CmdHelp(istringstream &, cConnDC * conn)
{
	if(!conn || !conn->mpUser) return 1;
	string file;
	mTriggers->TriggerAll(cTrigger::eTF_HELP, conn);
	return 1;
}

int cDCConsole::CmdCCBroadcast(istringstream & cmd_line, cConnDC * conn, int cl_min, int cl_max)
{
	string start, end, str, cc_zone;
	ostringstream ostr;
	string tmpline;
	
	// test for existence of parameter
	cmd_line >> cc_zone;
	
	getline(cmd_line,str);
	while(cmd_line.good())
	{
		tmpline="";
		getline(cmd_line,tmpline);
		str += "\r\n" + tmpline;
	}
	
	if(! str.size())
	{
		ostr << "Usage example: !ccbc :US:GB: <message>. Please type !help for more info" << endl;
                mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	mOwner->mP.Create_PMForBroadcast(start,end,mOwner->mC.hub_security, conn->mpUser->mNick ,str);
	mOwner->SendToAllWithNickCC(start,end, cl_min, cl_max, cc_zone);
	if ( mOwner->LastBCNick != "disable")
		mOwner->LastBCNick = conn->mpUser->mNick;
	return 1;
}

int cDCConsole::CmdMyInfo(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg;
	os << "\r\n[::] Your info: \r\n";
	conn->mpUser->DisplayInfo(os, eUC_OPERATOR);
	omsg = os.str();
	mOwner->DCPublicHS(omsg,conn);
	return 1;
}

int cDCConsole::CmdMyIp(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg;
	os << "\r\n[::] Your IP: " << conn->AddrIP();
	omsg = os.str();
	mOwner->DCPublicHS(omsg,conn);
	return 1;
}

int cDCConsole::CmdMe(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string query,text;
	string tmpline;
	
	getline(cmd_line,text);
	if ((mOwner->mC.disable_me_cmd) || (mOwner->mC.mainchat_class > 0 && conn->mpUser->mClass < eUC_REGUSER))
	{
		mOwner->DCPublicHS("This functionality is currently disabled.",conn);
		return 1;
	}
	while(cmd_line.good())
	{
		tmpline="";
		getline(cmd_line,tmpline);
		text += "\r\n" + tmpline;
	}
	
	if(conn->mpUser->mClass < eUC_VIPUSER && !cDCProto::CheckChatMsg(text,conn)) 
		return 0;
	
	os << "** " <<  conn->mpUser->mNick<< text << "";
	string msg = os.str();
	mOwner->mUserList.SendToAll(msg, true);
	os.str(mOwner->mEmpty);
	return 1;
}

int cDCConsole::CmdChat (istringstream & cmd_line, cConnDC * conn, bool switchon)
{
	if(!conn->mpUser) {
		// this should never occur
		return 0;
	}
	if (switchon && !mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick))
	{
		mOwner->mChatUsers.Add(conn->mpUser);
	}
	else if (!switchon && mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick))
	{
		mOwner->mChatUsers.Remove(conn->mpUser);
	}
	return 1;
}

int cDCConsole::CmdRegMe(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg, regnick, prefix;
	if (mOwner->mC.disable_regme_cmd)
		{
		mOwner->DCPublicHS("This functionality is currently disabled.",conn);
		return 1;
		}
	if(mOwner->mC.autoreg_class > 3)
	{
	mOwner->DCPublicHS("Registration failed; please contact an operator for more help.",conn);
	return 1;
	}
	__int64 user_share, min_share;	
	
	if(mOwner->mC.autoreg_class >= 0) {
		
		if(!conn->mpUser) {
			// this should never occur
			return 0;
		}
		
		// reg user's online nick
		regnick = conn->mpUser->mNick;
		prefix= mOwner->mC.nick_prefix_autoreg;
		ReplaceVarInString(prefix,"CC",prefix, conn->mCC);
		
		if( prefix.size() && StrCompare(regnick,0,prefix.size(),prefix) !=0 ) {
			ReplaceVarInString(mOwner->mL.autoreg_nick_prefix, "prefix", omsg, prefix);
			ReplaceVarInString(omsg, "nick", omsg, conn->mpUser->mNick);
			mOwner->DCPublicHS(omsg,conn);
			return 0;
		}
		
		user_share = conn->mpUser->mShare / (1024*1024);
		min_share = mOwner->mC.min_share_reg;
		if( mOwner->mC.autoreg_class == 2) min_share = mOwner->mC.min_share_vip;
		if( mOwner->mC.autoreg_class >= 3) min_share = mOwner->mC.min_share_ops;
		
		if( user_share < min_share ) {
			ReplaceVarInString(mOwner->mC.autoreg_min_share, "min_share", omsg, min_share);
			mOwner->DCPublicHS(omsg,conn);
			return 0;
		}
		
		cUser *user = mServer->mUserList.GetUserByNick(regnick);
		cRegUserInfo ui;
		bool RegFound = mOwner->mR->FindRegInfo(ui, regnick);
		
		if (RegFound) {
			omsg = mOwner->mL.autoreg_already_reg;
			mOwner->DCPublicHS(omsg,conn);
			return 0;
		}
		
		if(user && user->mxConn)
		{
			string text;
			getline(cmd_line,text);
		
			if( text.size() < mOwner->mC.password_min_len ) {
				omsg = mOwner->mL.pwd_min;
				mOwner->DCPublicHS(omsg,conn);
				return 0;
			}
			
			// @dReiska: addreg with pass
			// second param is NULL because there is no OP
			if ( mOwner->mR->AddRegUser(regnick, NULL, mOwner->mC.autoreg_class) ) {
				// @dReiska: lets strip space from beginning
				text = text.substr(1);
				
				if(mOwner->mR->ChangePwd(regnick, text, 0)) {
					// sent the report to the opchat
					os << "A new user has been registered with class " << mOwner->mC.autoreg_class;
					mOwner->ReportUserToOpchat(conn, os.str(), false);
					os.str(mOwner->mEmpty);
					// sent the message to the user
					ReplaceVarInString(mOwner->mL.autoreg_success, "password", omsg, text);
					ReplaceVarInString(omsg, "regnick", omsg, regnick);
				} else {
					omsg = mOwner->mL.autoreg_error;
					mOwner->DCPublicHS(omsg,conn);
					return false;	
				}
			} else {
				omsg = mOwner->mL.autoreg_error;
				mOwner->DCPublicHS(omsg,conn);
				return false;	
			}
		}
		
		mOwner->DCPublicHS(omsg,conn);
		return 1;
		
	} else {
		
		//-- to opchat
		string text, tmpline;
		
		getline(cmd_line,text);
		while(cmd_line.good())
		{
			tmpline="";
			getline(cmd_line,tmpline);
			text += "\r\n" + tmpline;
		}
		
		os << "REGME: '" << text <<"'.";
		mOwner->ReportUserToOpchat(conn, os.str(), mOwner->mC.dest_regme_chat);
		//-- to user
		os.str(mOwner->mEmpty);
		os << "Thank you, your request has been sent.";
		omsg = os.str();
		mOwner->DCPublicHS(omsg,conn);
		return 1;
	}
	
}

/** hub topic by H_C_K */
int cDCConsole::CmdTopic(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string omsg,topic;
	getline(cmd_line,topic);
	if (conn->mpUser->mClass < mOwner->mC.topic_mod_class)
	{
		mOwner->DCPublicHS("You do not have permissions to change the topic.",conn);
		return 1;
	}	
	if (topic.length() > 255)
	{
		os <<"Topic must be max 255 characters long. Your topic was "
			<< topic.length() 
			<<" characters long.";
		mOwner->DCPublicHS(os.str().data(),conn);
		return 1;
	}

	mOwner->mC.hub_topic = topic;

	cDCProto::Create_HubName(omsg, mOwner->mC.hub_name, topic);
	mOwner->SendToAll(omsg, eUC_NORMUSER, eUC_MASTER);
	if (topic.length())	omsg = mOwner->mL.msg_topic_set;
	else omsg = mOwner->mL.msg_topic_reset;
	ReplaceVarInString(omsg,"user", omsg, conn->mpUser->mNick);
	ReplaceVarInString(omsg,"topic", omsg, topic);
	mOwner->DCPublicHSToAll(omsg);
	return 1;
} 

int cDCConsole::CmdKick(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg, OtherNick, Reason;
	string tmpline;
	
	if( conn && conn->mpUser && conn->mpUser->Can(eUR_KICK, mOwner->mTime.Sec()))
	{
		cmd_line >> OtherNick;
		getline(cmd_line,Reason);
		while(cmd_line.good())
		{
			tmpline="";
			getline(cmd_line,tmpline);
			Reason += "\r\n" + tmpline;
		}
		if (Reason[0] == ' ') Reason = Reason.substr(1);
		if (Reason.size() > 3)
		{
			mOwner->DCKickNick(&os, conn->mpUser, OtherNick, Reason,
				cServerDC::eKCK_Drop|cServerDC::eKCK_Reason|cServerDC::eKCK_PM|cServerDC::eKCK_TBAN);
		}
	}
	else
	{
		os << "You cannot kick anyone!!" ;
	}
	omsg = os.str();
	mOwner->DCPublicHS(omsg,conn);
	return 1;
}

int cDCConsole::CmdRegMyPasswd(istringstream & cmd_line, cConnDC * conn)
{
	string str;
	int crypt = 0;
	ostringstream ostr;
	cRegUserInfo ui;

	if(!mOwner->mR->FindRegInfo(ui,conn->mpUser->mNick))
		return 0;

	if(!ui.mPwdChange)
	{
		ostr << mOwner->mL.pwd_cannot;
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}

	cmd_line >> str >> crypt;
	if(str.size() < mOwner->mC.password_min_len)
	{
		string str;
		ReplaceVarInString(mOwner->mL.pwd_min,"length",str, mOwner->mC.password_min_len);
		mOwner->DCPrivateHS(str,conn);
		mOwner->DCPublicHS(str,conn);
		return 1;
	}
	if(!mOwner->mR->ChangePwd(conn->mpUser->mNick, str,crypt))
	{
		ostr << mOwner->mL.pwd_set_error;
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}

	ostr << mOwner->mL.pwd_success;
	mOwner->DCPrivateHS(ostr.str(),conn);
	mOwner->DCPublicHS(ostr.str(),conn);
	conn->ClearTimeOut(eTO_SETPASS);
	return 1;
}

/** banlist * /
int cDCConsole::CmdBanList(istringstream & cmd_line, cConnDC * conn, int bantype, bool filter)
{
	string ipnick;
	ostringstream omsg;
	cmd_line >> ipnick;
	if(filter)
	{
		if(!ipnick.size())
		{
			omsg << "use with a paramater";
			mOwner->DCPublicHS(omsg.str(), conn);
			return 1;
		}
	}
	omsg << "Sorry un available...";
	mOwner->DCPublicHS(omsg.str(), conn);
	return 1;
	// @todo GetBanList mOwner->mBanList->GetBanList(conn, bantype, filter? &ipnick :NULL);
	return 1;
}*/

/** This will hide cmd actions by a given user or op.*/
int cDCConsole::CmdHideMe(istringstream & cmd_line, cConnDC * conn)
{
	int cls = -1;
	cmd_line >> cls;
	ostringstream omsg;
	if(cls < 0)
	{
		omsg << "Please use: !hideme <class>\r\n where <class> is the maximum class of users, that may not see your cmd actions." << endl;
		mOwner->DCPublicHS(omsg.str(),conn);
		return 1;
	}
	if(cls > conn->mpUser->mClass) cls = conn->mpUser->mClass;
	conn->mpUser->mHideKicksForClass = cls;
	omsg << "Your command actions are now hidden from users with class below" << cls << ".";
	mOwner->DCPublicHS(omsg.str(),conn);
	return 1;
}


/*!
    \fn cDCConsole::CmdUserLimit(istringstream & cmd_line, cConnDC * conn)
    \param
    usage: !userlimit <max_users> [<minutes>=60]
 */
int cDCConsole::CmdUserLimit(istringstream & cmd_line, cConnDC * conn)
{
	string str;
	ostringstream ostr;
	int minutes = 60, maximum = -1;
	cmd_line >> maximum >> minutes;

	if( maximum < 0 )
	{
		ostr << "Type !help for more information: (usage !userlimit <max_users> [<minutes>=60])";
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	// 60 steps at most
	cInterpolExp *fn = new
		cInterpolExp(mOwner->mC.max_users_total, maximum, (60*minutes) / mOwner->timer_serv_period ,(6*minutes) / mOwner->timer_serv_period);
	mOwner->mTmpFunc.push_back((cTempFunctionBase *)fn);

	ostr << "Starting to update max_users variable to: " << maximum
		<< " (Duration: " << minutes << " minutes)";
	mOwner->DCPublicHS(ostr.str(), conn);

	return 1;
}

/** class user (change temporarily his class)
	!class <nick> <new_class_0_to_5>
*/
int cDCConsole::CmdClass(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;
	int nclass = 3, oclass, mclass = conn->mpUser->mClass;

	cmd_line >> s >> nclass;

	if(!s.size() || nclass < 0 || nclass > 5 || nclass >= mclass)
	{
		os << "Use !class <nick> [<class>=3]. Please type !help for more info." << endl
			<< "Max class is " << mclass << endl;
		mOwner->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}


	user = mOwner->mUserList.GetUserByNick(s);

	if( user && user->mxConn )
	{
		oclass = user->mClass;
		if( oclass < mclass )
		{
			os << mOwner->mL.user << ": " << s << " temp changing class to " << nclass << endl;
			user->mClass = (tUserCl) nclass;
			if ((oclass < eUC_OPERATOR) && (nclass >= eUC_OPERATOR))
			{
				mOwner->mOpchatList.Add(user);
				if (!(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys))
				{
					mOwner->mOpList.Add(user);
					mOwner->mUserList.SendToAll(mOwner->mOpList.GetNickList(), false);
				}
			}
			else if ((oclass >= eUC_OPERATOR) && (nclass < eUC_OPERATOR))
			{
				mOwner->mOpchatList.Remove(user);
				mOwner->mOpList.Remove(user);
				mOwner->mUserList.SendToAll(mOwner->mOpList.GetNickList(), false);
			}
		}
		else
		{
			os << "You haven't rights to change class of " << s << "." << endl;
		}
	}
	else
	{
		os <<  mOwner->mL.user << ": " << s << mOwner->mL.not_in_userlist << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

/** hidekick users until reconnect
	# usage !hidekick <nick> ...
*/
int cDCConsole::CmdHideKick(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;

	while(cmd_line.good())
	{
		cmd_line >> s;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user && user-> mxConn && user->mClass < conn->mpUser->mClass)
		{
			os << mOwner->mL.user << ": " << s << " kicks are now hidden." << endl;
			user->mHideKick = true;
		}
		else
		{
			os << mOwner->mL.user << ": " << s << mOwner->mL.not_in_userlist << endl;
		}
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

/** unhidekick user
	usage: !unhidekick <nick> ...
*/
int cDCConsole::CmdUnHideKick(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;

	while(cmd_line.good())
	{
		cmd_line >> s;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user && user-> mxConn && user->mClass < conn->mpUser->mClass)
		{
			os << mOwner->mL.user << ": " << s << " will show kick messages to chat" << endl;
			user->mHideKick = false;
		}
		else
			os << mOwner->mL.user << ": " << s << " not found in nicklist (or no rights)." << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

/** protect user against kicks
	!protect <nick> [<against_class>=your_class-1]
*/
int cDCConsole::CmdProtect(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;
	int oclass, mclass = conn->mpUser->mClass, nclass = mclass -1;
	if(nclass > 5) nclass = 5;

	cmd_line >> s >> nclass;

	if(!s.size() || nclass < 0 || nclass > 5 || nclass >= mclass)
	{
		os << "Use !protect <nick> [<againstclass>=your_class-1]. Please type !help for more info." << endl
			<< "Max class is " << mclass-1 << endl;
		mOwner->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}


	user = mOwner->mUserList.GetUserByNick(s);

	if( user && user->mxConn )
	{
		oclass = user->mClass;
		if( oclass < mclass )
		{
			os << mOwner->mL.user << ": " << s << " temporarily changing protection to " << nclass << endl;
			user->mProtectFrom = nclass;
		}
		else
			os << "You don't have enough privileges to protect " << s << "." << endl;
	}
	else
	{
		os << mOwner->mL.user << ": " << s << " not found in nicklist." << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

int cDCConsole::CmdReload(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;

	os << "Reloading triggers ,configuration and reglist cache." << endl;
	mTriggers->ReloadAll();
	mOwner->mC.Load();
	mOwner->DCPublicHS(os.str().c_str(),conn);
	if (mOwner->mC.use_reglist_cache) mOwner->mR->UpdateCache();
	//mOwner->mConnTypes->ReloadAll();
	return 1;
}

bool cDCConsole::cfReport::operator()()
{
	ostringstream os;
	string omsg, nick, reason;
	cUser *user;
	enum { eREP_ALL, eREP_NICK, eREP_RASONP, eREP_REASON };

	GetParOnlineUser(eREP_NICK, user, nick);
	GetParStr(eREP_REASON, reason);

	//-- to opchat
	os << "REPORT: user '" << nick <<"' ";
	if (user && user->mxConn)
	{
		os << "IP= '" << user->mxConn->AddrIP() << "' HOST='" << user->mxConn->AddrHost() << "' ";
	} else os << "which is offline ";
	os << "Reason='" << reason  << "'. reporter";
	mS->ReportUserToOpchat(mConn, os.str(), mS->mC.dest_report_chat);
	//-- to sender
	*mOS << "Thanx, your report has been accepted. ";
	return true;
}

bool cDCConsole::cfRaw::operator()()
{
	enum { eRW_ALL, eRW_USR, eRW_HELLO, eRW_PASSIVE , eRW_ACTIVE};
	static const char * actionnames [] = { "all","user","hello","active" };
	static const int actionids [] = { eRW_ALL, eRW_USR, eRW_HELLO, eRW_ACTIVE };

	enum { eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_ANY , eRC_REDIR, eRC_PM, eRC_CHAT };
	static const char * cmdnames [] = { "hubname","hello","quit", "any", "redir", "pm", "chat" };
	static const int cmdids [] = { eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_ANY , eRC_REDIR , eRC_PM, eRC_CHAT};

	int Action = -1;
	int CmdID = -1;

	string tmp;

   //@todo eventualy use cUser::Can
	if (this->mConn->mpUser->mClass < eUC_ADMIN) return false;
	mIdRex->Extract(1,mIdStr,tmp);
	Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action <0) return false;

	mIdRex->Extract(2,mIdStr,tmp);
	CmdID  = this->StringToIntFromList(tmp, cmdnames, cmdids, sizeof(cmdnames)/sizeof(char*));
	if (CmdID <0) return false;

	string theCommand, endOfCommand;
	string param, nick;
	GetParStr(1,param);
	bool WithNick = false;

	switch (CmdID)
	{
		case eRC_HUBNAME: theCommand = "$HubName "; break;
		case eRC_HELLO: theCommand = "$Hello "; break;
		case eRC_QUIT: cDCProto::Create_Quit(theCommand , ""); break;
		case eRC_REDIR: theCommand = "$ForceMove "; break;
		case eRC_PM:
			mS->mP.Create_PMForBroadcast(
				theCommand,
				endOfCommand,
				mS->mC.hub_security,
				mConn->mpUser->mNick,
				param);
			WithNick = true;
		break;
		case eRC_CHAT: theCommand = "<" + mConn->mpUser->mNick +"> "; break;
		case eRC_ANY:
			cDCProto::UnEscapeChars(param, param);
			break;
		default : return false; break;
	}

	if (!WithNick)
	{
		theCommand += param;
		theCommand += "|";
	}

	cUser *target_usr = NULL;
	switch (Action)
	{
		case eRW_ALL: if(!WithNick) mS->mUserList.SendToAll(theCommand);
			else mS->mUserList.SendToAllWithNick(theCommand, endOfCommand); break;
		case eRW_HELLO: if(!WithNick) mS->mHelloUsers.SendToAll(theCommand);
			else mS->mHelloUsers.SendToAllWithNick(theCommand, endOfCommand);break;
		case eRW_ACTIVE: if(!WithNick) mS->mActiveUsers.SendToAll(theCommand);
			else mS->mActiveUsers.SendToAllWithNick(theCommand, endOfCommand); break;
		case eRW_USR:
			target_usr = mS->mUserList.GetUserByNick(nick);
			if (target_usr && target_usr->mxConn)
			{
				if( WithNick)
				{
					theCommand += nick;
					theCommand += endOfCommand;
				}
				target_usr->mxConn->Send(theCommand);
			}
		break;
		default: return false; break;
	}
	return true;
}

bool cDCConsole::cfBan::operator()()
{

	static const char *bannames[]={"nick", "ip", "nickip", "", "range", "host1", "host2" , "host3", "hostr1",  "share", "prefix"};
	static const int banids[]= {cBan::eBF_NICK, cBan::eBF_IP, cBan::eBF_NICKIP, cBan::eBF_NICKIP, cBan::eBF_RANGE,
      cBan::eBF_HOST1, cBan::eBF_HOST2, cBan::eBF_HOST3, cBan::eBF_HOSTR1, cBan::eBF_SHARE, cBan::eBF_PREFIX };

	enum { BAN_BAN, BAN_UNBAN, BAN_INFO, BAN_LIST };
	static const char *prefixnames[]={"add", "new", "rm", "del", "un", "info", "check", "list", "ls" };
	static const int prefixids[]= { BAN_BAN, BAN_BAN, BAN_UNBAN, BAN_UNBAN, BAN_UNBAN, BAN_INFO, BAN_INFO, BAN_LIST, BAN_LIST};

	if (this->mConn->mpUser->mClass < eUC_OPERATOR) return false;
	
	cBan Ban(mS);
	int BanType = cBan::eBF_NICKIP;
	cKick Kick;
	time_t BanTime = 0;
	string tmp;
	int Count = 0;
	int MyClass = 0;
	if( !mConn->mpUser ) return false;
	MyClass = mConn->mpUser->mClass;
	if( MyClass < eUC_OPERATOR ) return false;

	//"!(un)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ", "(\\S+)( (.*)$)?"

	enum { BAN_PREFIX = 1, BAN_TYPE = 2, BAN_LENGTH = 4, BAN_THIS = 6, BAN_WHO = 1, BAN_REASON = 3};
	bool IsNick = false;
	bool IsPerm = !mIdRex->PartFound(BAN_LENGTH);

	int BanAction = BAN_BAN;
	if( mIdRex->PartFound(BAN_PREFIX) )
	{
		mIdRex->Extract( BAN_PREFIX, mIdStr, tmp);
		BanAction = this->StringToIntFromList(tmp, prefixnames, prefixids, sizeof(prefixnames)/sizeof(char*));
		if (BanAction < 0) return false;
	}

	if(mIdRex->PartFound(BAN_TYPE))
	{
		mIdRex->Extract( BAN_TYPE, mIdStr, tmp);
		BanType = this->StringToIntFromList(tmp, bannames, banids, sizeof(bannames)/sizeof(char*));
		if (BanType < 0) return false;
	}

	if (BanType == cBan::eBF_NICK) IsNick = true;

	if ( mIdRex->PartFound(BAN_THIS) )
		IsNick = ((0 == mIdRex->Compare( BAN_TYPE, mIdStr, "nick")) || (BanType == cBan::eBF_NICK));

	string Who;
	GetParUnEscapeStr(BAN_WHO, Who);

	if(!IsPerm)
	{
		mIdRex->Extract(BAN_LENGTH, mIdStr,tmp);
		if(tmp != "perm")
		{
			BanTime = mS->Str2Period(tmp, *mOS);
			if(BanTime < 0)
			{
				(*mOS) << "Please provide a valid ban time";
				return false;
			}
		} else IsPerm = true;
	}

	bool unban = (BanAction == BAN_UNBAN);
	cUser *user = NULL;
	int BanCount = 100;

	switch (BanAction)
	{
	case BAN_UNBAN:
	case BAN_INFO:
		if (unban)
		{
			if( !GetParStr(BAN_REASON,tmp))
			{
				(*mOS) << BAN_EREASON;
				return false;
			}
			#ifndef WITHOUT_PLUGINS
			if(!mS->mCallBacks.mOnUnBan.CallAll(Who, mConn->mpUser->mNick, tmp)) {
				(*mOS) << "Action has been discarded by plugin";
				return false;	
			}
			#endif
			(*mOS) << "Unbanning...\r\n";
		}

		if(BanType == cBan::eBF_NICKIP)
		{
			Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, cBan::eBF_NICK, unban);
			Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, cBan::eBF_IP, unban);
			if(!unban)
			{
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, cBan::eBF_RANGE, false);
				string Host;
				if (mConn->DNSResolveReverse(Who, Host)) {
					Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, cBan::eBF_HOSTR1, false);
					Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, cBan::eBF_HOST3, false);
					Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, cBan::eBF_HOST2, false);
					Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, cBan::eBF_HOST1, false);
				}
			}
		}
		else if(BanType == cBan::eBF_NICK) {
			Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, cBan::eBF_NICK, unban);
			Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, cBan::eBF_NICKIP, unban);
		}
		else
		{
			Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, BanType, unban);
		}
		(*mOS) << endl << "Total : " << Count << " bans.";
		break;
	case BAN_BAN:
		Ban.mNickOp = mConn->mpUser->mNick;
		mParRex->Extract(BAN_REASON, mParStr,Ban.mReason);
		Ban.mDateStart = cTime().Sec();
		if(BanTime)
			Ban.mDateEnd = Ban.mDateStart+BanTime;
		else
			Ban.mDateEnd = 0;
		Ban.SetType(BanType);

		switch (BanType)
		{
		case cBan::eBF_NICKIP:
		case cBan::eBF_NICK:
		case cBan::eBF_IP:
			if( mS->mKickList->FindKick(Kick, Who, mConn->mpUser->mNick, 3000, true, true, IsNick) )
			{
				mS->mBanList->NewBan(Ban, Kick, BanTime, BanType );
				if( mParRex->PartFound(BAN_REASON) )
				{
					mParRex->Extract(BAN_REASON, mParStr,tmp);
					Ban.mReason += "\r\n";
					Ban.mReason += tmp;
				}
			} else {
				if ( !mParRex->PartFound(BAN_REASON) )
				{
					(*mOS) << BAN_EREASON;
					return false;
				}
				if (BanType == cBan::eBF_NICKIP) BanType = cBan::eBF_IP;
				mParRex->Extract(BAN_REASON, mParStr,Kick.mReason);
				Kick.mOp = mConn->mpUser->mNick;
				Kick.mTime = cTime().Sec();

				if (BanType == cBan::eBF_NICK)
					Kick.mNick = Who;
				else
					Kick.mIP = Who;

				mS->mBanList->NewBan(Ban, Kick, BanTime, BanType);
			}
			break;

		case cBan::eBF_HOST1:
		case cBan::eBF_HOST2:
		case cBan::eBF_HOST3:
		case cBan::eBF_HOSTR1:
			if ( !mParRex->PartFound(BAN_REASON) )
			{
				(*mOS) << BAN_EREASON;
				return false;
			}
			if ( MyClass < (eUC_ADMIN - (BanType - cBan::eBF_HOST1) ) ) //@todo rights
			{
				(*mOS) << "You have no rights for this ban";
				return false;
			}
			Ban.mHost = Who;
			Ban.mIP = Who;
			break;
		case cBan::eBF_RANGE:
			if(! cDCConsole::GetIPRange(Who, Ban.mRangeMin, Ban.mRangeMax) )
			{
				(*mOS) << "Unknown range format '" << Who << "'";
				return false;
			}
			Ban.mIP=Who;
			break;
		case cBan::eBF_PREFIX:
			if ( !mParRex->PartFound(BAN_REASON) )
			{
				(*mOS) << BAN_EREASON;
				return false;
			}
			Ban.mNick = Who;
		break;
		case cBan::eBF_SHARE:
		{
			if ( !mParRex->PartFound(BAN_REASON) )
			{
				(*mOS) << BAN_EREASON;
				return false;
			}
			istringstream is(Who);
			__int64 share;
			/*cConfigItemBaseInt64 *ci = new cConfigItemBaseInt64(share);
			ci->ConvertFrom(Who);
			delete ci;*/
			is >> share;
			Ban.mShare = share;
		}
		break;
		default: break;
		}
		#ifndef WITHOUT_PLUGINS
		if(!mS->mCallBacks.mOnNewBan.CallAll(&Ban)) {
			(*mOS) << "Action has been discarded by plugin";
			return false;	
		}
		#endif
		user = mS->mUserList.GetUserByNick(Ban.mNick);
		if (user != NULL)
		{
			mS->DCKickNick(mOS, mConn->mpUser, Ban.mNick, Ban.mReason, cServerDC::eKCK_Reason | cServerDC::eKCK_Drop);
		}

		mS->mBanList->AddBan(Ban);
		(*mOS) << "Adding ban: ";
		Ban.DisplayComplete(*mOS);
		break;
	case BAN_LIST:
		GetParInt(BAN_WHO,BanCount);
		mS->mBanList->List(*mOS,BanCount);
	break;
	default:(*mOS) << "This command has not implemented yet.\r\nAvailable command are: " << endl;
		return false; 
		break;
	}
	return true;
}

bool cDCConsole::cfInfo::operator()()
{
	enum {eINFO_SERVER };
	static const char * infonames [] = { "hub","server" };
	static const int infoids [] = { eINFO_SERVER, eINFO_SERVER };

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);

	int InfoType = this->StringToIntFromList(tmp, infonames, infoids, sizeof(infonames)/sizeof(char*));
	if (InfoType < 0) return false;

	int MyClass = mConn->mpUser->mClass;
	if( MyClass < eUC_VIPUSER ) return false;

	switch(InfoType)
	{
		case eINFO_SERVER: mInfoServer.Output(*mOS, MyClass); break;
		default : (*mOS) << "This command has not implemented yet.\r\nAvailable command is: !hubinfo (alias of !serverinfo)" << endl;
			return false;
	}

	return true;
}

bool cDCConsole::cfTrigger::operator()()
{
	string ntrigger;
	string text, cmd;
	if(mConn->mpUser->mClass < eUC_MASTER) return false;
   	mIdRex->Extract(2,mIdStr,cmd);
	enum {eAC_ADD, eAC_DEL, eAC_EDIT, eAC_DEF, eAC_FLAGS};
	static const char *actionnames[]={"new","add","del","edit","def", "setflags"};
	static const int actionids[]={eAC_ADD, eAC_ADD, eAC_DEL, eAC_EDIT, eAC_DEF, eAC_FLAGS};
	int Action = this->StringToIntFromList(cmd, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	mParRex->Extract(1,mParStr,ntrigger);
	mParRex->Extract(2,mParStr,text);
	int i;
	int flags = 0;
	istringstream is(text);
	bool result = false;
	cTrigger *tr;

	switch(Action)
	{
	case eAC_ADD:
		tr = new cTrigger;
		tr->mCommand = ntrigger;
		tr->mDefinition = text;
		break;

		case eAC_EDIT:
		for( i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i )
		{
			if( ntrigger == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand )
			{
				mS->SaveFile(((*((cDCConsole*)mCo)->mTriggers)[i])->mDefinition,text);
				result = true;
				break;
			}
		}
		break;
	case eAC_FLAGS:
		flags = -1;
		is >> flags;
		if (flags >= 0) {
			for( i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i )
			{
				if( ntrigger == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand )
				{
				(*((cDCConsole*)mCo)->mTriggers)[i]->mFlags = flags;
				((cDCConsole*)mCo)->mTriggers->SaveData(i);
				result = true;
				break;
				}
			}
		}

		break;
	default: (*mOS) << "Not implemented" << endl;
		break;
	};
		return result;
}

bool cDCConsole::cfSetVar::operator()()
{
	string file(mS->mDBConf.config_name),var,val, fake_val;
	bool DeleteItem = false;
	

	//@todo use cUser::Can
	//@todo per-variable rights
	if(mConn->mpUser->mClass < eUC_ADMIN) return false;

	if (mParRex->PartFound(2)) mParRex->Extract(2,mParStr,file);
	mParRex->Extract(3,mParStr,var);
	mParRex->Extract(4,mParStr,val);

	cConfigItemBase *ci = NULL;
	if (file == mS->mDBConf.config_name)
	{
		ci = mS->mC[var];

		if( !ci )
		{
			(*mOS) << "Undefined variable: " << var;
			return false;
		}
	} else {
		DeleteItem = true;
		ci = new cConfigItemBaseString(fake_val,var);
		mS->mSetupList.LoadItem(file.c_str(), ci);
	}

	if (ci)
	{
		(*mOS) << "Changing [" << file << "] " << var << " from: '" << *ci << "'";
		ci->ConvertFrom(val);
		(*mOS) << " => '" << *ci << "'";
		mS->mSetupList.SaveItem(file.c_str(), ci);
		if(DeleteItem) delete ci;
		ci = NULL;
	}

	return true;
}


bool cDCConsole::cfGag::operator()()
{
	string cmd, nick, howlong;
	time_t period = 24*3600*7;
	time_t Now = 1;

	bool isUn = false;

   //@todo use cUser::Can
	if(mConn->mpUser->mClass < eUC_OPERATOR) return false;

	isUn = mIdRex->PartFound(1);
	mIdRex->Extract(2, mIdStr, cmd);

	mParRex->Extract(1, mParStr, nick);
	if (mParRex->PartFound(3))
	{
		mParRex->Extract(3, mParStr, howlong);
		period = mS->Str2Period(howlong, *mOS);
		if (!period) return false;
	}

	cPenaltyList::sPenalty penalty;
	penalty.mNick = nick;

	if(!isUn) Now = cTime().Sec() + period;

	enum {eAC_GAG, eAC_NOPM, eAC_NODL, eAC_NOSEARCH, eAC_KVIP, eAC_NOSHARE, eAC_CANREG, eAC_OPCHAT};
	static const char *actionnames[]={"gag","nochat","nopm","noctm","nodl","nosearch","kvip", "maykick", "noshare", "mayreg", "mayopchat"};
	static const int actionids[]={eAC_GAG, eAC_GAG,eAC_NOPM, eAC_NODL, eAC_NODL, eAC_NOSEARCH, eAC_KVIP, eAC_KVIP, eAC_NOSHARE, eAC_CANREG, eAC_OPCHAT};
	int Action = this->StringToIntFromList(cmd, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	switch(Action)
	{
		case eAC_GAG: penalty.mStartChat = Now; break;
		case eAC_NOPM: penalty.mStartPM = Now; break;
		case eAC_NODL: penalty.mStartCTM = Now; break;
		case eAC_NOSEARCH: penalty.mStartSearch = Now; break;
		case eAC_KVIP: penalty.mStopKick = Now; break;
		case eAC_OPCHAT: penalty.mStopOpchat = Now; break;
		case eAC_NOSHARE: penalty.mStopShare0 = Now; break;
		case eAC_CANREG: penalty.mStopReg = Now; break;
		default: return false;
	};

	bool ret = false;
	if(!isUn) ret = mS->mPenList->AddPenalty(penalty);
	else ret = mS->mPenList->RemPenalty(penalty);

	cUser *usr = mS->mUserList.GetUserByNick(nick);
	if (usr != NULL)
	{
		switch(Action)
		{
			case eAC_GAG: usr->SetRight(eUR_CHAT, penalty.mStartChat, isUn); break;
			case eAC_NOPM: usr->SetRight(eUR_PM, penalty.mStartPM, isUn); break;
			case eAC_NODL: usr->SetRight(eUR_CTM, penalty.mStartCTM, isUn); break;
			case eAC_NOSEARCH: usr->SetRight(eUR_SEARCH, penalty.mStartSearch, isUn); break;
			case eAC_NOSHARE: usr->SetRight(eUR_NOSHARE, penalty.mStopShare0, isUn); break;
			case eAC_CANREG: usr->SetRight(eUR_REG, penalty.mStopReg, isUn); break;
			case eAC_KVIP: usr->SetRight(eUR_KICK, penalty.mStopKick, isUn); break;
			case eAC_OPCHAT: usr->SetRight(eUR_OPCHAT, penalty.mStopOpchat, isUn); break;
			default: break;
		};
	}

	(*mOS) << penalty ;
	if (ret) (*mOS) << " saved OK ";
	else (*mOS) << " save error ";
	return true;
}

bool cDCConsole::cfCmd::operator()()
{
	enum { eAC_LIST };
	static const char * actionnames [] = { "list", "lst" };
	static const int actionids [] = { eAC_LIST, eAC_LIST };

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	
	switch(Action)
	{
//		case eAC_LIST: this->mS->mCo.mCmdr.List(mOS); break;
		default: return false;
	}
	return true;
}

bool cDCConsole::cfWho::operator()()
{
	enum { eAC_IP, eAC_RANGE };
	static const char * actionnames [] = { "ip" , "range", "subnet" };
	static const int actionids [] = { eAC_IP, eAC_RANGE, eAC_RANGE };

   //@todo use cUser::Can 
	if (this->mConn->mpUser->mClass < eUC_OPERATOR) return false;

   string tmp;
	mIdRex->Extract(2,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	string separator("\r\n\t");
	string userlist;

	mParRex->Extract(0, mParStr,tmp);
	unsigned long ip_min, ip_max;
	int cnt = 0;

	switch(Action)
	{
		case eAC_IP:
			ip_min = cBanList::Ip2Num(tmp);
			ip_max = ip_min;
			cnt = mS->WhoIP(ip_min, ip_max, userlist, separator, true);
			break;
		case eAC_RANGE:
			if(! cDCConsole::GetIPRange(tmp, ip_min, ip_max) ) return false;
			cnt = mS->WhoIP(ip_min, ip_max, userlist, separator, false);
			break;
		default: return false;
	}


	if(!cnt) (*mOS) << "No user with " << tmp;
	else (*mOS) << "Users with " << actionnames[Action] << " " << tmp << ":\r\n\t" << userlist << "Total: " << cnt;
	return true;
}

bool cDCConsole::cfKick::operator()()
{
	enum { eAC_KICK, eAC_DROP, eAC_FLOOD };
	static const char * actionnames [] = { "kick", "drop", "flood" };
	static const int actionids [] = { eAC_KICK, eAC_DROP, eAC_FLOOD };

	if (this->mConn->mpUser->mClass < eUC_VIPUSER) return false;
	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	string nick, text;

	mParRex->Extract(1,mParStr,nick);

	ostringstream os;
	string CoolNick, ostr;
	int i;
	cUser *other;

	switch(Action)
	{
		case eAC_KICK:
			if (!mParRex->PartFound(2))
			{
				(*mOS) << "What about the reason ??" << endl;
				return false;
			}
			mParRex->Extract(2,mParStr,text);
		case eAC_DROP:
			mS->DCKickNick(mOS, this->mConn->mpUser, nick, text,
				(Action == eAC_KICK)?
				(cServerDC::eKCK_Drop|cServerDC::eKCK_Reason|cServerDC::eKCK_PM|cServerDC::eKCK_TBAN):
				(cServerDC::eKCK_Drop|cServerDC::eKCK_Reason));
		break;
		case eAC_FLOOD:
			text += "\r\n";
			other = mS->mUserList.GetUserByNick(nick);
			if (
				other && other->mxConn &&
				(other->mClass < mConn->mpUser->mClass) &&
				(other->mProtectFrom < mConn->mpUser->mClass))
			{
				for (i = 0; i < 10000; i++)
				{
					os.str(string(""));
					os << 1000+rand()%9000 << "Flood" << i;
					CoolNick = os.str();
					os.str(string(""));
					os << "$Hello " << CoolNick << "|";
					mS->mP.Create_PM(ostr, CoolNick, nick, CoolNick, text);
					os << ostr << "|";
					ostr = os.str();
					other->mxConn->Send(ostr, false);
				}
			}
		break;
		default: (*mOS) << "Not implemented" << endl;
		return false;
	};
	return true;
}

bool cDCConsole::cfPlug::operator()()
{
	enum { eAC_IN, eAC_OUT, eAC_LIST, eAC_REG, eAC_RELAOD };
	static const char * actionnames [] = { "in","out","list","reg","reload", NULL};
	static const int actionids [] = { eAC_IN, eAC_OUT, eAC_LIST, eAC_REG, eAC_RELAOD };

	if (this->mConn->mpUser->mClass < mS->mC.plugin_mod_class)
	{
		(*mOS) << "No rights to use plugins";
		return false;
	}

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	switch (Action)
	{
	case eAC_LIST:
		(*mOS) << "Loaded plugins: \r\n";
		mS->mPluginManager.List(*mOS);
		break;
	case eAC_REG:
		(*mOS) << "Available callbacks: \r\n";
		mS->mPluginManager.ListAll(*mOS);
		break;
	case eAC_OUT:
		if (mParRex->PartFound(1))
		{
			mParRex->Extract(1, mParStr, tmp);
			if(!mS->mPluginManager.UnloadPlugin(tmp)) return false;
		}
		break;
	case eAC_IN:
		if (mParRex->PartFound(1))
		{
			mParRex->Extract(1, mParStr, tmp);
			if(!mS->mPluginManager.LoadPlugin(tmp))
			{
				(*mOS) << mS->mPluginManager.GetError() << "\r\n";
				return false;
			}
		}
		break;
	case eAC_RELAOD:
		if (GetParStr(1, tmp))
		{
			if(!mS->mPluginManager.ReloadPlugin(tmp))
			{
				(*mOS) << mS->mPluginManager.GetError() << "\r\n";
				return false;
			}
		}
		break;
	default: break;
	}
	return true;
}

bool cDCConsole::cfRegUsr::operator()()
{
	enum { eAC_NEW, eAC_DEL, eAC_PASS, eAC_ENABLE, eAC_DISABLE, eAC_CLASS, eAC_PROTECT, eAC_HIDEKICK, eAC_SET, eAC_INFO };
	static const char * actionnames [] = { "n","new","newuser", "del","delete", "pass","passwd", "enable",
		"disable", "class", "setclass", "protect", "protectclass", "hidekick", "hidekickclass", "set","=",
		"info"  };
	static const int actionids [] = { eAC_NEW, eAC_NEW, eAC_NEW, eAC_DEL, eAC_DEL, eAC_PASS, eAC_PASS, eAC_ENABLE,
		eAC_DISABLE, eAC_CLASS, eAC_CLASS, eAC_PROTECT, eAC_PROTECT, eAC_HIDEKICK, eAC_HIDEKICK, eAC_SET, eAC_SET,
		eAC_INFO };

	if (this->mConn->mpUser->mClass < eUC_OPERATOR) return false;

	string tmp;
	mIdRex->Extract(2,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

//	"!r(eg)?(\S+) ", "(\\S+)( (((\\S+) )?(.*)))?"
	string nick, par, field;

	int ParClass = 1;
	int MyClass = this->mConn->mpUser->mClass;
	if ((Action != eAC_INFO) && (! this->mConn->mpUser->Can(eUR_REG,mS->mTime.Sec()))) return false;

	mParRex->Extract(1,mParStr,nick);
	bool WithPar = false;

 	WithPar=mParRex->PartFound(3);
	if( Action != eAC_SET && WithPar)	mParRex->Extract(3,mParStr,par);

	if( Action == eAC_SET )
	{
		WithPar = WithPar && mParRex->PartFound(5);
		if( !WithPar )
		{
			(*mOS) << "Missing parameter(s)";
			return false;
  		}
		mParRex->Extract(5,mParStr,field);
		if(WithPar) mParRex->Extract(6,mParStr,par);
 	}


	cUser *user = mS->mUserList.GetUserByNick(nick);
	cRegUserInfo ui;
	ostringstream ostr;
	bool RegFound = mS->mR->FindRegInfo(ui, nick);
	bool authorized = false;

	// check rights

	if (RegFound)
	{
		if (mS->mC.classdif_reg > 10)
		{	(*mOS) << "Invalid classdif_reg value. Must be between 1 and 5. Please correct this first!";
			return false;
		}
		if ((MyClass < eUC_MASTER) && !(
			(MyClass >= (ui.mClass+mS->mC.classdif_reg) &&
			MyClass >= (ui.mClassProtect)) ||
			((Action == eAC_INFO) &&(MyClass >= (ui.mClass - 1)))
		 ))
		{
			(*mOS) << "Insufficient rights!";
			return false;
		}
	}

	switch(Action)
	{
		case eAC_CLASS: case eAC_PROTECT: case eAC_HIDEKICK: case eAC_NEW:
		 	std::istringstream lStringIS(par);
			lStringIS >> ParClass;
		break;
	};

	switch(Action)
	{
		case eAC_SET: authorized = RegFound && (( MyClass >= eUC_ADMIN ) && (MyClass > ui.mClass) && (field != "class")); break;
		case eAC_NEW:
			authorized = !RegFound  && (MyClass >= (ParClass + mS->mC.classdif_reg));
			break;
		case eAC_PASS: case eAC_HIDEKICK: case eAC_ENABLE: case eAC_DISABLE: case eAC_DEL:
			authorized = RegFound && (MyClass >= (ui.mClass+mS->mC.classdif_reg));
			break;
		case eAC_CLASS:
			authorized = RegFound && (MyClass >= (ui.mClass+mS->mC.classdif_reg)) && (MyClass >= (ParClass + mS->mC.classdif_reg));
			break;
		case eAC_PROTECT:
			authorized = RegFound && (MyClass >= (ui.mClass+mS->mC.classdif_reg)) && (MyClass >= (ParClass + 1));
			break;
		case eAC_INFO : authorized = RegFound && (MyClass >= eUC_OPERATOR); break;
	};

	if (MyClass == eUC_MASTER) authorized = RegFound || (!RegFound && (Action == eAC_NEW));

	if (!authorized)
	{
		if (!RegFound) *mOS << " No user '" << nick << "' is registered in database..";
		else if (Action == eAC_NEW) *mOS << "User '" << nick << "' already exists";
		else *mOS << "Insufficient rights!";
		return false;
	}

 	if( Action >= eAC_CLASS && Action <= eAC_SET && !WithPar)
 	{
 		(*mOS) << "Missing Parameter";
 		return false;
  }

	switch (Action)
	{
	case eAC_NEW: // new
		if (RegFound)
		{
			(*mOS) << "The user is already registered in the database";
			return false;
		}
		#ifndef WITHOUT_PLUGINS
		if(!mS->mCallBacks.mOnNewReg.CallAll(nick,ParClass)) {
			(*mOS) << "Action has been discarded by plugin";
			return false;	
		}
		#endif
		
		if (mS->mR->AddRegUser(nick, mConn, ParClass))
		{
			if(user && user->mxConn)
			{
				ostr.str(mS->mEmpty);
				ostr << mS->mL.pwd_setup;
				mS->DCPrivateHS(ostr.str(), user->mxConn);
			}
			(*mOS) << "User has been added; please tell him to change his password";
		}
		else
		{
			(*mOS) << "Error adding a new user";
			return false;
		}
		break;
	case eAC_DEL: // delete
		#ifndef WITHOUT_PLUGINS
		if(!mS->mCallBacks.mOnDelReg.CallAll(nick,ui.mClass)) {
			(*mOS) << "Action has been discarded by plugin";
			return false;	
		}
		#endif
		if (mS->mR->DelReg(nick))
		{
			(*mOS) << "Deleted user '" << nick << "' from database\r\n"
				<< "User's info: " << ui <<"\r\n";
		}
		else
		{
			(*mOS) << "Error deleting user '" << nick << "'\r\n"
				<< "User's info: " << ui <<"\r\n";
			return false;
		}
		break;
	case eAC_PASS: // pass
		if (WithPar)
		{
			#if ! defined _WIN32
			if ( mS->mR->ChangePwd(nick, par, 1) )
			#else
			if ( mS->mR->ChangePwd(nick, par, 0) )
			#endif
				(*mOS) << "Success";
			else
			{
				(*mOS) << "Error";
				return false;
			}
		} else {
			field="pwd_change";
			par="1";
			ostr << mS->mL.pwd_can;
		}
		break;
	case eAC_CLASS: // class
		#ifndef WITHOUT_PLUGINS
		cout << "Here we go " << endl;
		if(!mS->mCallBacks.mOnUpdateClass.CallAll(nick,ui.mClass, ParClass)) {
			(*mOS) << "Action has been discarded by plugin";
			return false;	
		}
		#endif
		field="class";
		ostr << "Your class has been changed to :" << par << "; changes will take effect on next login.";
		break;
	case eAC_ENABLE: //enable
		field="enabled";
		par="1";
		WithPar= true;
		ostr << "Your registration has been enabled";
		break;
	case eAC_DISABLE: // disable
		field="enabled";
		par="0";
		WithPar= true;
		ostr << "Your registration has been disabled";
		break;
	case eAC_PROTECT: // protect
		field="class_protect";
		ostr << "You are now on protected by level: " << par;
		if (user) user->mProtectFrom = ParClass;
		break;
	case eAC_HIDEKICK: // hidekick
		field="class_hidekick";
		ostr << "You are now (in DB) on the hidekick level: " << par;
		break;
	case eAC_SET: break; // set
	case eAC_INFO: (*mOS) << ui << endl; break;
	default:
		mIdRex->Extract(1,mIdStr,par);
		(*mOS) << "The command '" << par << "' hasn't implemented yet";
		return false;
		break;
	}

	if( (WithPar && (Action >=eAC_ENABLE) && (Action <=eAC_SET)) || ( (Action==eAC_PASS) && !WithPar ))
	{
		if (mS->mR->SetVar(nick, field, par))
  	{
  		(*mOS) << "Updated variable '" << field << "' to value " << par << " for user " << nick;
			if(user && user->mxConn && ostr.str().size()) mS->DCPrivateHS(ostr.str(), user->mxConn);
    }
    else
    {
			(*mOS) << "Error setting variable '" << field << "' to value " << par << " for user " << nick;
     		return false;
  	}
	}
	(*mOS) << "OK";
	return true;
}

bool cDCConsole::cfRedirToConsole::operator()()
{
	if (this->mConn->mpUser->mClass < eUC_OPERATOR) return false;
	if(this->mConsole != NULL)
		return mConsole->OpCommand(mIdStr + mParStr, mConn);
	else return false;
}

/** broadcast command */
bool cDCConsole::cfBc::operator()()
{
	enum { eBC_BC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC };
	enum { eBC_ALL, eBC_MSG };
	const char *cmds[] = { "bc","broadcast","oc","ops","guests","regs","vips","cheefs","admins",",masters","ccbc","ccbroadcast", NULL};
	const int nums[] = {eBC_BC,eBC_BC, eBC_OC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC, eBC_CC };
	string message;
	int cmdid;
	
	if (!GetIDEnum(1,cmdid, cmds, nums)) return false;
	
	GetParStr(eBC_MSG, message);
	//rights to broadcast
	int MinClass = mS->mC.min_class_bc;
	int MaxClass = eUC_MASTER;
	int MyClass = this->mConn->mpUser->mClass;
	int AllowedClass = eUC_MASTER;
	switch(cmdid)
	{
		case eBC_BC:
			MinClass = eUC_NORMUSER;
			MaxClass = eUC_MASTER;
			AllowedClass = mS->mC.min_class_bc;
		break;
		case eBC_GUEST:
			MinClass = eUC_NORMUSER; 
			MaxClass = eUC_NORMUSER;
			AllowedClass = mS->mC.min_class_bc_guests;
		break;
		case eBC_REG: 
			MinClass = eUC_REGUSER;
			MaxClass = eUC_REGUSER;
			AllowedClass = mS->mC.min_class_bc_regs;
		break;
		case eBC_VIP: 
			MinClass = eUC_VIPUSER;
			MaxClass = eUC_VIPUSER;
			AllowedClass = mS->mC.min_class_bc_vips;
		break;
		case eBC_OC: 
			MinClass = eUC_OPERATOR;
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_OPERATOR;
		break; 
		case eBC_CHEEF:
			MinClass = eUC_CHEEF; 
			MaxClass = eUC_ADMIN;
			AllowedClass = eUC_OPERATOR;
		break;
		case eBC_ADMIN: 
			MinClass = eUC_ADMIN; 
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_ADMIN;
		break;
		case eBC_MASTER:
			MinClass = eUC_MASTER; 
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_ADMIN;
		break;
		case eBC_CC:
		break;
		default: break;
	}
	
	if (MyClass < AllowedClass)
	{
		*mOS << "You do not have permissions to broadcast to this class.";
		return false;
	}
	
	string start, end;
	mS->mP.Create_PMForBroadcast(start,end,mS->mC.hub_security, this->mConn->mpUser->mNick ,message);
	cTime TimeBefore, TimeAfter;
	if ( mS->LastBCNick != "disable") mS->LastBCNick = mConn->mpUser->mNick;
	int count = mS->SendToAllWithNick(start,end, MinClass, MaxClass);
	TimeAfter.Get();
	*mOS << "Message delivered to " << count << " users in : " << (TimeAfter-TimeBefore).AsPeriod();
	return true;
}

};


/*!
    \fn nDirectConnect::cDCConsole::GetIPRange(const string &range, unsigned long &from, unsigned long &to)
 */
bool nDirectConnect::cDCConsole::GetIPRange(const string &range, unsigned long &from, unsigned long &to)
{
	//"^(\\d+\\.\\d+\\.\\d+\\.\\d+)((\\/(\\d+))|(\\.\\.|-)(\\d+\\.\\d+\\.\\d+\\.\\d+))?$"
	enum {R_IP1 = 1, R_RANGE = 2, R_BITS=4, R_DOTS = 5, R_IP2 = 6};
	if (!mIPRangeRex.Exec(range)) return false;
	string tmp;
	// easy : from..to
	if (mIPRangeRex.PartFound(R_RANGE))
	{
		if (mIPRangeRex.PartFound(R_DOTS))
		{
			mIPRangeRex.Extract(R_IP1, range, tmp);
			from = cBanList::Ip2Num(tmp);
			mIPRangeRex.Extract(R_IP2, range, tmp);
			to = cBanList::Ip2Num(tmp);
			return true;
		}
		// the more complicated 1.2.3.4/16 style mask
		else
		{
			unsigned long mask = 0;
			unsigned long addr1 = 0;
			unsigned long addr2 = 0xFFFFFFFF;
			mIPRangeRex.Extract(0, range, tmp);
			from = cBanList::Ip2Num(tmp);
			int i;
			i = tmp.find_first_of("/\\");
			istringstream is(tmp.substr(i+1));
			mask = from;
			is >> i;
			addr1 = mask & (0xFFFFFFFF << (32-i));
			addr2 = addr1 + (0xFFFFFFFF >> i);
			from = addr1;
			to   = addr2;
			return true;
		}
	} else
	{
		mIPRangeRex.Extract(R_IP1, range, tmp);
		from = cBanList::Ip2Num(tmp);
		to = from;
		return true;
	}
	return false;
}


