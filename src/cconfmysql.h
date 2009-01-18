/***************************************************************************
                          cconfmysql.h  -  description
                             -------------------
    begin                : Sun Jun 29 2003
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

#ifndef CCONFMYSQL_H
#define CCONFMYSQL_H

#include "cconfigbase.h"
#include <iostream>
#include <iomanip>
#include "cmysql.h"
#include "cquery.h"

using namespace nMySQL;
using namespace nConfig;

namespace nConfig
{

/**
  * a Typed config Item for mysql - not users due to compiling problems
*/

#define DeclareMySQLItemClass(TYPE,Suffix) \
class cConfigItemMySQL##Suffix : public cConfigItemBase##Suffix \
{ \
public: \
	cConfigItemMySQL##Suffix(TYPE &var): cConfigItemBase##Suffix(var){}; \
	virtual std::ostream &WriteToStream (std::ostream& os); \
};

DeclareMySQLItemClass(string,String);
DeclareMySQLItemClass(char *,PChar);

/** Unused structure
*/
class cMySQLItemCreator : public cBasicItemCreator
{
public:
	cMySQLItemCreator(){};
	virtual cConfigItemBaseString* NewItem(string &var){ return new cConfigItemMySQLString(var);};
	virtual cConfigItemBasePChar * NewItem(char * &var){ return new cConfigItemMySQLPChar(var);};
};


class cMySQLColumn
{
public:
	cMySQLColumn();
	~cMySQLColumn();
	void AppendDesc(ostream &os) const;
	void ReadFromRow(const MYSQL_ROW &row);
	string mName;
	string mType;
	string mDefault;
	bool mNull;
	bool operator!=(const cMySQLColumn &col) const;
};

class cMySQLTable : public cObj
{
public:
	cMySQLTable(cMySQL &);
	~cMySQLTable();
	vector<cMySQLColumn> mColumns;
	const cMySQLColumn * GetColumn(const string &) const;
	bool GetDescription(const string &);
	bool CreateTable();
	bool AutoAlterTable(const cMySQLTable &original);
	void AppendColumnDesc(const cMySQLColumn &col, ostream &os);
	string mName;
	string mExtra;
	cQuery mQuery;
};

/**a mysql configuratin class
  *@author Daniel Muller
  */

class cConfMySQL : public cConfigBase //<sMySQLItemCreator>
{
public:
	cConfMySQL(cMySQL &mysql);
	~cConfMySQL();

	cMySQL &mMySQL;
	cQuery mQuery;
	/** loads data from the mysql result */
	virtual int Load();
	virtual int Load(cQuery &);
	virtual int Save();
	/** do mysql query */
	int StartQuery(string query);
	int StartQuery();
	int StartQuery(cQuery &);
	int EndQuery();
	int EndQuery(cQuery &);
	void AddPrimaryKey(const char*);
	void WherePKey(ostream &os);
	void AllFields(ostream &, bool DoF=true, bool DoV=false, bool IsAff = false, string joint=string(", "));
	void AllPKFields(ostream &, bool DoF=true, bool DoV=false, bool IsAff = false, string joint=string(", "));
	void SelectFields(ostream &);
	void UpdateFields(ostream &os);
	bool UpdatePKVar(const char* var_name, string &new_val);
	bool UpdatePKVar(const char *);
	bool UpdatePKVar(cConfigItemBase *);
	bool UpdatePK();
	bool UpdatePK(cQuery &);
	bool LoadPK();
	bool SavePK(bool dup=false);
	static void WriteStringConstant(ostream &os, const string &str);
	void CreateTable();
	template <class T> void AddCol(const char *colName, const char *colType, const char *colDefault, bool colNull, T &var)
	{
		cMySQLColumn col;
		col.mName = colName;
		col.mType = colType;
		col.mDefault = colDefault;
		col.mNull = colNull;
		mMySQLTable.mColumns.push_back(col);
		Add(colName, var);
	}
	void DeletePK();

protected: // Protected attributes

	tItemHash mPrimaryKey;
	/**  */
	int ok;
	// number of collumns
	unsigned mCols;
	cMySQLTable mMySQLTable;

	/// UF to make equations and lists of values or fiel names
	struct ufEqual
	{
		ostream &mOS;
		string mJoint;
		bool start;
		bool mDoField, mDoValue;
		bool mIsAffect;
		ufEqual (ostream &os, string joint, bool DoF = true, bool DoV = true, bool IsAff = true):
			mOS(os), mJoint(joint), start(true),
			mDoField(DoF), mDoValue(DoV), mIsAffect(IsAff) {};
		ufEqual (ufEqual const &eq):
			mOS(eq.mOS),mJoint(eq.mJoint),start(eq.start),
			mDoField(eq.mDoField),mDoValue(eq.mDoValue),mIsAffect(eq.mIsAffect){}

		void operator()(cConfigItemBase* item);
	};

	/**
	  * Unary Function that loads current result's row into currently pointed structure
	*/
	struct ufLoad
	{
		string mEmpty;
		MYSQL_ROW mRow;
		int i;
		ufLoad(MYSQL_ROW row):mRow(row),i(0){}

		void operator()(cConfigItemBase* item)
		{
			if(mRow[i])
				item->ConvertFrom(mRow[i]);
			else
				item->ConvertFrom(mEmpty);
			i++;
		}
	};

public:
	/** database result iterator
	  * very useful
	  */
	struct db_iterator
	{
		cConfMySQL *mConf;
		cQuery *mQuery;
		db_iterator( cConfMySQL *conf = NULL ) : mConf(conf), mQuery(conf?&conf->mQuery:NULL){};
		db_iterator( cConfMySQL *conf, cQuery *query ) : mConf(conf), mQuery(query){};
		db_iterator &operator++();
		db_iterator &operator= (const db_iterator &it ){mConf = it.mConf; mQuery = it.mQuery; return *this;}
		db_iterator &operator= (cConfMySQL *conf){mConf = conf; mQuery=conf?&conf->mQuery:NULL; return *this;}
		bool operator==(db_iterator &it ){return (mConf == it.mConf) && (mQuery == it.mQuery);}
		bool operator!=(db_iterator &it ){return (mConf != it.mConf) || (mQuery != it.mQuery);}
	};

	db_iterator &db_begin(cQuery &);
	db_iterator &db_begin();
	db_iterator &db_end(){return mDBEnd;}
private:
	db_iterator mDBBegin;
	db_iterator mDBEnd;
};

};

#endif
