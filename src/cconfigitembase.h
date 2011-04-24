/**************************************************************************
*   Original Author: Daniel Muller (dan at verliba dot cz) 2003-05        *
*                                                                         *
*   Copyright (C) 2006-2011 by Verlihub Project                           *
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

#ifndef CCONFIGITEMBASE_H
#define CCONFIGITEMBASE_H
#include <string>
#include <iostream>

using namespace std;
#ifndef _WIN32
#define __int64 long long
#endif


/**
 * a Typed template for a config Item, provides all the methods for convetiong with string, affecting values, ans streaming
 */

#define DeclarecConfigItemBaseT(TYPE,TypeID,Suffix) \
class cConfigItemBase##Suffix : public cConfigItemBase { \
public: \
	cConfigItemBase##Suffix(TYPE &var): cConfigItemBase((void*)&var, 0){}; \
	cConfigItemBase##Suffix(TYPE &var, string const &Name): cConfigItemBase((void*)&var, 0){this->mName=Name;}; \
	virtual ~cConfigItemBase##Suffix(){}; \
	virtual TYPE & Data(){return *(TYPE*)mAddr;}; \
	virtual cConfigItemBase##Suffix &operator=(TYPE const &i){*(TYPE*)Address() = i; return *this;};\
	virtual nEnums::tItemType GetTypeID(){ return TypeID;} \
	virtual bool IsEmpty();\
	virtual std::istream &ReadFromStream(std::istream& is); \
	virtual std::ostream &WriteToStream (std::ostream& os); \
	virtual void ConvertFrom(const std::string &str); \
	virtual void ConvertTo(std::string &str); \
};

namespace nVerliHub {
	namespace nEnums {
		typedef enum
		{
			eIT_VOID, // unspecified
			eIT_BOOL,
			eIT_CHAR,
			eIT_INT,
			eIT_UINT, // unsigned int
			eIT_LONG,
			eIT_ULONG,
			eIT_PCHAR, // null terminated string
			eIT_TIMET,
			eIT_LLONG, // long long
			eIT_ULLONG,
			eIT_AR_BYTE, // array of bytes
			eIT_STRING,  // std::string
			eIT_DOUBLE
		} tItemType;
	};
	namespace nConfig {

/**
a base class for all kinds of config items, used by cConfigBase

@author Daniel Muller
*/
class cConfigItemBase
{
	friend class mConfigBase;
public:
	/** constructors */
	cConfigItemBase(void *addr=0, void *base=0) :
		mAddr(addr),
		mBase(base)
	{};

	virtual ~cConfigItemBase(){};

	/** setting value */
	template <class TYPE> cConfigItemBase &operator=(const TYPE &i){*(TYPE*)Address() = i; return *this;};
	/** cast operator */
	template <class TYPE> operator TYPE(){return *(TYPE*)mAddr;}

	/** input /output */
	virtual istream &ReadFromStream(istream &)=0;
	virtual ostream &WriteToStream (ostream &)=0;
	friend istream &operator >> (istream &is, cConfigItemBase &ci){return ci.ReadFromStream(is);}
	friend ostream &operator << (ostream &os, cConfigItemBase &ci){return ci.WriteToStream (os);}

	/** conversion of a string to a value */
	virtual void ConvertFrom(const string &) = 0;
	virtual void ConvertTo(string &) = 0;
	virtual bool IsEmpty() = 0;
	virtual nEnums::tItemType GetTypeID() = 0;
	void *mAddr;
	string mName;
	char mBuf[32];
public:
	virtual void *Address(){return mAddr;}
	void *mBase;

};
	DeclarecConfigItemBaseT(bool, nEnums::eIT_BOOL,Bool);
	DeclarecConfigItemBaseT(char, nEnums::eIT_CHAR,Char);
	DeclarecConfigItemBaseT(int, nEnums::eIT_INT,Int);
	DeclarecConfigItemBaseT(unsigned, nEnums::eIT_UINT,UInt);
	DeclarecConfigItemBaseT(long, nEnums::eIT_LONG,Long);
	DeclarecConfigItemBaseT(unsigned long, nEnums::eIT_ULONG, ULong);
	DeclarecConfigItemBaseT(double, nEnums::eIT_DOUBLE,Double);
	DeclarecConfigItemBaseT(char*, nEnums::eIT_PCHAR,PChar);
	DeclarecConfigItemBaseT(string, nEnums::eIT_STRING,String);
	DeclarecConfigItemBaseT(__int64, nEnums::eIT_LLONG,Int64);

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
