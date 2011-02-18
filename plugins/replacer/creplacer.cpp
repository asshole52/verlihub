/***************************************************************************
 *   Copyright (C) 2004 by Pralcio                                         *
 *   based on "Forbid" code made by                                        *
 *   Dan Muller & bourne                             *
 *   dan at verliba dot cz & bourne at freemail dot hu                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "creplacer.h"
#include "src/cconfigitembase.h"
#include "src/cserverdc.h"

namespace nDirectConnect {
namespace nTables {

cReplacer::cReplacer( cServerDC *server )
 : cConfMySQL(server->mMySQL) , mS(server)
{
	SetClassName("nDC::cReplacer");
	mMySQLTable.mName="pi_replacer"; // made change
	Add("word",mModel.mWord);
	AddPrimaryKey("word");
	Add("rep_word",mModel.mRepWord); // made change
	Add("afclass",mModel.mAfClass);
	SetBaseTo(&mModel);
}

cReplacer::~cReplacer()
{
}

void cReplacer::CreateTable(void)
{
	cQuery query(mMySQL);
	query.OStream() <<
		"CREATE TABLE IF NOT EXISTS " << mMySQLTable.mName << " ("
		"word varchar(30) not null primary key,"
		"rep_word varchar(30) not null," // made change
		"afclass tinyint default 4" // affected class. normal=1, vip=2, cheef=3, op=4, admin=5, master=10
		")";
	query.Query();
}

void cReplacer::Empty()
{
	//SetBaseTo( &mModel);
	mData.clear();
}

int cReplacer::LoadAll()
{
	Empty();
	SelectFields(mQuery.OStream());
	int n=0;
	db_iterator it;

	PrepareNew();
	for(it = db_begin(); it != db_end(); ++it)
	{
		n++;
		if(Log(3)) LogStream() << "Loading :" << mData.back()->mWord << endl;
		if(Log(3)) LogStream() << "Loading :" << mData.back()->mRepWord << endl; //made change
		if(!mData.back()->PrepareRegex())
		{
			if(Log(3)) LogStream() << "Error in regex :" << mData.back()->mWord << endl;
		} else {
			PrepareNew();
		}
	}
	mQuery.Clear();
	DeleteLast();
	return n;
}

int cReplacer::DeleteLast()
{
	if( !mData.size() ) return 0;
	SetBaseTo(&mModel);
	delete mData.back();
	mData.pop_back();
}

void cReplacer::PrepareNew()
{
	cReplacerWorker *tr = new cReplacerWorker;
	SetBaseTo(tr);
	mData.push_back(tr);
}

cReplacerWorker * cReplacer::operator[](int i)
{
	if( i < 0 || i >= Size() ) return NULL;
	return mData[i];
}

int cReplacer::AddReplacer(cReplacerWorker &fw)
{
	SetBaseTo(&fw);
	return SavePK();
}

void cReplacer::DelReplacer(cReplacerWorker &fw)
{
	SetBaseTo(&fw);
	DeletePK();
}

string cReplacer::ReplacerParser(const string & str, cConnDC * conn)
{
	string lcstr(str);
	string::size_type idx;
	string t_word;
	string r_word;
	string temp(str);
	bool find_loop;

	transform(lcstr.begin(), lcstr.end(), lcstr.begin(), ::tolower);

	tDataType::iterator it;
	for( it = mData.begin(); it != mData.end(); ++it )
	{
		if((*it)->CheckMsg(lcstr))
		{
			if((*it)->mAfClass >= conn->mpUser->mClass)
			{
				t_word = (*it)->mWord;
				r_word = (*it)->mRepWord;
				find_loop = true;
				while (find_loop)
				{
					idx = temp.find(t_word.data());
					if ( idx != string::npos )
					{
						temp.replace(idx,t_word.length(),r_word.data(),r_word.length());
					}
					else find_loop = false;
				}
			}
		}
	}
	return temp;
}

cReplaceCfg::cReplaceCfg(cServerDC *s) : mS(s)
{
}

int cReplaceCfg::Load()
{
	mS->mSetupList.LoadFileTo(this,"pi_replacer");
	return 0;
}

int cReplaceCfg::Save()
{
	mS->mSetupList.SaveFileTo(this,"pi_replacer");
	return 0;
}

};
};