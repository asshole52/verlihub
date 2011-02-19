/***************************************************************************
 *   Copyright (C) 2004 by Daniel Muller                                   *
 *   dan at verliba dot cz                                                 *
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cmsglist.h"
#include <stdlib.h>
#include "src/ctime.h"
#include "src/cserverdc.h"
#include "src/i18n.h"

using namespace nUtils;
namespace nMessanger
{

cMsgList::cMsgList(cServerDC *server): 
	cConfMySQL(server->mMySQL),
	mCache(server->mMySQL,"pi_messages", "receiver", "date_sent"),
	mServer(server)
{
	AddFields();
}


cMsgList::~cMsgList()
{}

void cMsgList::CleanUp()
{
	mQuery.Clear();
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << 
		" WHERE (date_expires > date_sent) AND (date_expires < " << mServer->mTime.Sec() << ")";
	mQuery.Query();
	mQuery.Clear();
}

void cMsgList::AddFields()
{
	// this is useful for the parent class
	mMySQLTable.mName = "pi_messages";

	// add every field you want to be able to load simply
	// add also primary key fields
	// the string constans here must correspont to field names
	AddCol("sender","varchar(32)","",false,mModel.mSender);
	AddPrimaryKey("sender");
	AddCol("date_sent","int(11)","",false, mModel.mDateSent);
	AddPrimaryKey("date_sent");
	AddCol("sender_ip","varchar(15)","",true, mModel.mSenderIP);
	AddCol("receiver","varchar(32)","",false, mModel.mReceiver );
	AddCol("date_expires","int(11)","0",true, mModel.mDateExpires );
	AddCol("subject","varchar(128)","",true, mModel.mSubject );
	AddCol("body","text","",true, mModel.mBody );
	
	mMySQLTable.mExtra="PRIMARY KEY (sender, date_sent)";
	SetBaseTo(&mModel);
}

int cMsgList::CountMessages(const string &nick, bool sender)
{
	// quick cache response
	if(!sender && mCache.IsLoaded() && !mCache.Find(nick)) return 0;
	// if found in cache then count it..
	mQuery.Clear();
	mQuery.OStream() << "SELECT COUNT(body) FROM " << this->mMySQLTable.mName << " WHERE " << (sender ? "sender" : "receiver") << "='";
	WriteStringConstant(mQuery.OStream(), nick );
	mQuery.OStream() << "'";
	mQuery.Query();
	int n = 0;
	MYSQL_ROW row;
	if(mQuery.StoreResult() && ( NULL != (row = mQuery.Row())))
		n = atoi(row[0]);
	mQuery.Clear();
	return n;
}

bool cMsgList::AddMessage( sMessage &msg )
{
	if (mCache.IsLoaded()) mCache.Add(msg.mReceiver);
	SetBaseTo(&msg);
	return SavePK();
}

int cMsgList::PrintSubjects(ostream &os, const string &nick, bool IsSender)
{
	int n = 0;
	mQuery.Clear();
	SelectFields(mQuery.OStream());
	mQuery.OStream() << "WHERE "  << (IsSender ? "sender" : "receiver") << "='";
	WriteStringConstant(mQuery.OStream(), nick );
	mQuery.OStream() << "'";
	db_iterator it;
	SetBaseTo(&mModel);
	for(it = db_begin(); it != db_end(); ++it) {
		os << mModel.AsSubj() << endl;
		n++;
	}
	mQuery.Clear();
	return 0;
}

void cMsgList::DeliverOnline(cUser *dest, sMessage &msg) 
{
	string omsg;
	ostringstream os;
	os << msg.AsOnline();
	cDCProto::Create_PM(omsg, msg.mSender, dest->mNick, 
			msg.mSender, os.str() );
	dest->mxConn->Send(omsg, true);
}

int cMsgList::DeliverMessagesForUser(cUser *dest)
{
	db_iterator it;
	int n = 0;
	long max_date = 0;
	
	mQuery.Clear();
	SelectFields(mQuery.OStream());
	mQuery.OStream() << "WHERE "  << "receiver" << "='" ;
	WriteStringConstant(mQuery.OStream(),dest->mNick);
	mQuery.OStream()<< "'";

	SetBaseTo(&mModel);
	
	for( it = db_begin(); it != db_end(); ++it, ++n ) {
		if (mModel.mDateSent > max_date)
			max_date = mModel.mDateSent;
		DeliverModelToUser(dest);
	}

	mQuery.Clear();
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << " WHERE receiver = '" ;
	WriteStringConstant(mQuery.OStream(),dest->mNick);
	mQuery.OStream() << "' AND date_sent <= " << max_date;
	mQuery.Query();
	
	return n;
}

int cMsgList::DeliverMessagesSinceSync(unsigned sync)
{
	db_iterator it;
	int n = 0;
	cUser *user = NULL;
	long max_date = 0;
	cQuery DelQ(mQuery);
	
	SetBaseTo(&mModel);
	mQuery.Clear();
	SelectFields(mQuery.OStream());
	mQuery.OStream() << "WHERE date_sent >=" << sync;


	for(it = db_begin(); it != db_end(); ++it, ++n ) {
		if (!user || user->mNick != mModel.mReceiver)
			user = mServer->mUserList.GetUserByNick(mModel.mReceiver);

		if(user) {
			DeliverModelToUser(user);
			DelQ.Clear();
			DelQ.OStream() << "DELETE FROM " << mMySQLTable.mName;
			WherePKey(DelQ.OStream());
			DelQ.Query();
		}
	}
	
	DelQ.Clear();
	mQuery.Clear();
	return n;
}

int cMsgList::DeliverModelToUser(cUser *dest)
{
	string omsg;
	ostringstream os;
	bool SenderOffline;
	
	os.str("");
	omsg.erase();
	SenderOffline =  (NULL == mServer->mUserList.GetUserByNick(mModel.mSender));

	if (SenderOffline) {
		omsg += "$Hello ";
		omsg += mModel.mSender;
		omsg += "|";
	}
	
	os << mModel.AsDelivery();
	
	cDCProto::Create_PM(omsg, mModel.mSender, dest->mNick, mModel.mSender, os.str());
	
	if (SenderOffline) {
		omsg += "|$Quit ";
		omsg += mModel.mSender;
	}
	
	dest->mxConn->Send(omsg, true);
	return 0;
}

void cMsgList::UpdateCache()
{
	unsigned LastSync = mCache.GetSync();
	mCache.Update();
	DeliverMessagesSinceSync(LastSync);
	mCache.Sync();

}

ostream & operator << (ostream &os, sMessage &Msg)
{
	cTime date_sent(Msg.mDateSent,0);
	switch (Msg.mPrintType) {
		case sMessage::AS_SUBJECT:
			os << autosprintf(_("From: %s To: %s\nDate: %s\n Subject: %s\n"), Msg.mSender.c_str(), Msg.mReceiver.c_str(), date_sent.AsDate().AsString().c_str(), Msg.mSubject.c_str());
		break;
		case sMessage::AS_BODY:
			os << autosprintf(_("From: %s To: %s\nDate: %s\n Subject: %s\n%s\n----\n"), Msg.mSender.c_str(), Msg.mReceiver.c_str(), date_sent.AsDate().AsString().c_str(), Msg.mSubject.c_str(), Msg.mBody.c_str());
		break;
		case sMessage::AS_DELIVERY:
			os << "\r\n" << autosprintf(_("#OFFLINE MESSAGE# [%s]\nSubject: %s -------------------------\n%s"), date_sent.AsDate().AsString().c_str(), Msg.mSubject.c_str(), Msg.mBody.c_str());
		break;
		case sMessage::AS_ONLINE:
			os << Msg.mBody;
		break;
		default: break;
	}
	return os;
}

};
