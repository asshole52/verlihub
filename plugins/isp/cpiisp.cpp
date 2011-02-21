#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cpiisp.h"
#include <stringutils.h>

using namespace nStringUtils;
using namespace nDirectConnect::nProtocol::nEnums;

cpiISP::cpiISP()
{
	mName = "ISP";
	mVersion = VERSION;
	mCfg= NULL;
}

void cpiISP::OnLoad(cServerDC *server)
{
	if (!mCfg) mCfg = new cISPCfg(server);
	mCfg->Load();
	mCfg->Save();
	tpiISPBase::OnLoad(server);
}

cpiISP::~cpiISP() {
	if (mCfg != NULL) delete mCfg;
	mCfg = NULL;
}

bool cpiISP::RegisterAll()
{
	RegisterCallBack("VH_OnParsedMsgMyINFO");
	RegisterCallBack("VH_OnParsedMsgValidateNick");
	RegisterCallBack("VH_OnOperatorCommand");
	return true;
}

bool cpiISP::OnParsedMsgMyINFO(cConnDC * conn, cMessageDC *msg) 
{ 
	cISP *isp;
	if (conn->mpUser && (conn->GetTheoricalClass() <= mCfg->max_check_isp_class))
	{
		isp = mList->FindISP(conn->AddrIP(), conn->mCC);
		if (!isp)
		{
			if (!mCfg->allow_all_connections)
			{
				// no results;
				mServer->DCPublicHS(mCfg->msg_no_isp, conn);
				conn->CloseNice(500);
				return false;
			} else return true;
		}
		

		if (!conn->mpUser->mInList)
		{
			if (conn->GetTheoricalClass() <= mCfg->max_check_conn_class)
			{
				if (!isp->CheckConn(msg->ChunkString(eCH_MI_SPEED)))
				{
					string omsg = isp->mConnMessage;
					string pattern;
					cDCProto::EscapeChars(isp->mConnPattern, pattern);
					ReplaceVarInString(omsg,"pattern",omsg,pattern);
					mServer->DCPublicHS(omsg, conn);
					conn->CloseNice(500);
					return false;
				}
			}

			int share_sign = isp->CheckShare(conn->GetTheoricalClass(), conn->mpUser->mShare, mCfg->unit_min_share_bytes, mCfg->unit_max_share_bytes);
			if (share_sign)
			{
				mServer->DCPublicHS((share_sign>0)?mCfg->msg_share_more:mCfg->msg_share_less, conn);
				conn->CloseNice(500);
				return false;
			}
		}
	
		if (conn->GetTheoricalClass() <= mCfg->max_insert_desc_class)
		{
			string &desc = msg->ChunkString(eCH_MI_DESC);
			string desc_prefix;
			if (isp->mAddDescPrefix.length() > 0)
			{
				ReplaceVarInString(isp->mAddDescPrefix,"CC",desc_prefix,conn->mCC);
				ReplaceVarInString(desc_prefix,"CLASS",desc_prefix,conn->GetTheoricalClass());
				desc = desc_prefix + desc;
				msg->ApplyChunk(eCH_MI_DESC);
			}
		}
	}
	return true;
}

bool cpiISP::OnParsedMsgValidateNick(cConnDC * conn, cMessageDC *msg)
{
	cISP *isp;	
	if (conn->GetTheoricalClass() <= mCfg->max_check_nick_class)
	{
		string & nick = msg->ChunkString(eCH_1_PARAM);
		isp = mList->FindISP(conn->AddrIP(),conn->mCC);
		// TEST cout << "Checking nick: " << nick << endl;
		if (isp && !isp->CheckNick(nick, conn->mCC))
		{
			string omsg;
			//cout << "ISP - got a wrong nick" << endl;
			ReplaceVarInString(isp->mPatternMessage, "pattern", omsg, isp->mNickPattern);
			ReplaceVarInString(omsg, "nick", omsg, nick); 
			ReplaceVarInString(omsg, "CC", omsg, conn->mCC); 
			
			mServer->DCPublicHS(omsg,conn);
			conn->CloseNice(500);
			return false;
		}
		//mServer->DCPublicHS(isp->mName, conn);
	}
	return true;
}

bool cpiISP::OnOperatorCommand(cConnDC *conn, string *str)
{
	if( mConsole.DoCommand(*str, conn) ) return false;
	return true;
}

REGISTER_PLUGIN(cpiISP);

