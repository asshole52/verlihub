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
#include "cconfigitembase.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include "stringutils.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	namespace nConfig {

//// ConvertFrom

void cConfigItemBaseBool	::ConvertFrom(const std::string &str){*this = (0!=atoi(str.c_str()));}
void cConfigItemBaseInt		::ConvertFrom(const std::string &str){*this = atoi(str.c_str());}
void cConfigItemBaseUInt	::ConvertFrom(const std::string &str){*this = atol(str.c_str());}
void cConfigItemBaseLong	::ConvertFrom(const std::string &str){*this = atol(str.c_str());}
void cConfigItemBaseInt64	::ConvertFrom(const std::string &str){*this = StringAsLL(str);}
void cConfigItemBaseULong	::ConvertFrom(const std::string &str){*this = strtoul(str.c_str(),NULL,10);}
void cConfigItemBaseDouble	::ConvertFrom(const std::string &str){*this = atof(str.c_str());}
void cConfigItemBaseChar	::ConvertFrom(const std::string &str){*this = *str.c_str();}
void cConfigItemBaseString	::ConvertFrom(const std::string &str){*this = str;}
void cConfigItemBasePChar	::ConvertFrom(const std::string &str)
{
	char *data = this->Data() ;
	if( data ) delete data;
	data = new char[str.size()+1];
	memcpy(data,str.data(),str.size()+1);
	*this = data;
}

//
//// ConvertTo
//

void cConfigItemBaseBool	::ConvertTo(std::string &str){str = (this->Data())?"1":"0";}
void cConfigItemBaseInt		::ConvertTo(std::string &str){sprintf(mBuf,"%d",this->Data()); str = mBuf;}
void cConfigItemBaseUInt	::ConvertTo(std::string &str){sprintf(mBuf,"%u",this->Data()); str = mBuf;}
void cConfigItemBaseLong	::ConvertTo(std::string &str){sprintf(mBuf,"%ld",this->Data()); str = mBuf;}
void cConfigItemBaseInt64	::ConvertTo(std::string &str){sprintf(mBuf,"%lld",this->Data()); str = mBuf;}
void cConfigItemBaseULong	::ConvertTo(std::string &str){sprintf(mBuf,"%lu",this->Data()); str = mBuf;}
void cConfigItemBaseDouble	::ConvertTo(std::string &str){sprintf(mBuf,"%f",this->Data()); str = mBuf;}
void cConfigItemBaseChar	::ConvertTo(std::string &str){sprintf(mBuf,"%c",this->Data()); str = mBuf;}
void cConfigItemBaseString	::ConvertTo(std::string &str){str = this->Data();}
void cConfigItemBasePChar	::ConvertTo(std::string &str){str = this->Data();}

//
//// ReadFromStream
//

#define DefineReadFromStreamMethod(TYPE,Suffix) \
std::istream &cConfigItemBase##Suffix::ReadFromStream(std::istream& is){ return is >> (this->Data()); }

DefineReadFromStreamMethod(bool,Bool);
DefineReadFromStreamMethod(int,Int);
DefineReadFromStreamMethod(unsigned,UInt);
DefineReadFromStreamMethod(char,Char);
DefineReadFromStreamMethod(char*,PChar);
DefineReadFromStreamMethod(double,Double);
DefineReadFromStreamMethod(long,Long);
DefineReadFromStreamMethod(unsigned long,ULong);

std::istream &cConfigItemBaseInt64::ReadFromStream(std::istream& is)
{
	string tmp;
	is >> tmp;
	this->ConvertFrom(tmp);
	return is;
}

std::istream &cConfigItemBaseString::ReadFromStream(std::istream& is)
{
	string str;
	Data() = "";
	is >> Data() >> str;
	for(  ; str.size() && (str[0] != '#') ;is >> str )
	{
		Data() += ' ';
		Data() += str;
		str = "";
	}
	return is ;
}
//
//// WriteToStream
//
#define DefineWriteToStreamMethod(TYPE,Suffix) \
std::ostream &cConfigItemBase##Suffix::WriteToStream(std::ostream& os){ return os << (this->Data()); }

DefineWriteToStreamMethod(bool,Bool);
DefineWriteToStreamMethod(int,Int);
DefineWriteToStreamMethod(unsigned,UInt);
DefineWriteToStreamMethod(char,Char);
DefineWriteToStreamMethod(char*,PChar);
DefineWriteToStreamMethod(double,Double);
DefineWriteToStreamMethod(string,String);
DefineWriteToStreamMethod(long,Long);
DefineWriteToStreamMethod(unsigned long,ULong);
std::ostream &cConfigItemBaseInt64::WriteToStream(std::ostream& os)
{
	string tmp;
	this->ConvertTo(tmp);
	os << tmp;
	return os;
}

//
//// IsEmpty
//
bool cConfigItemBaseBool	::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseInt		::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseUInt	::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseLong	::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseInt64	::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseULong	::IsEmpty(){ return !this->Data();}
bool cConfigItemBaseChar	::IsEmpty(){ return !this->Data();}
bool cConfigItemBasePChar	::IsEmpty(){ return !this->Data() || !*(this->Data());}
bool cConfigItemBaseDouble	::IsEmpty(){ return this->Data() == 0.;}
bool cConfigItemBaseString	::IsEmpty(){ return !this->Data().size();}

	}; // namespace nConfig
}; // namespace nVerliHub
