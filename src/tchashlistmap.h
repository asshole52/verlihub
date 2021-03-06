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
#ifndef NUTILSTCHASHLISTMAP_H
#define NUTILSTCHASHLISTMAP_H
#include <list>
#include <map>
#include <string>
#include "cobj.h"

using namespace std;

namespace nVerliHub {
	namespace nUtils {

/** hashing structure */
template <class HashType = unsigned long> class hHashStr
{
	HashType __stl_hash_string(const char* __s) const
	{
		HashType __h = 0;
		for ( ; *__s; ++__s)
		__h = 33*__h + *__s;

		return HashType(__h);
	}
	public:
	HashType operator() (const string &s) const {return __stl_hash_string(s.c_str());}
};

typedef unsigned long ulong;
/**
provides the functionality of hash_map and a linked list

@author Daniel Muller
*/
template <class DataType, class KeyType=ulong> class tcHashListMap : public cObj
{
public:
	// single linked list is good for interating over all
	typedef list<DataType, allocator<DataType> > tUserSList;
	// invariant iterator ... good for insertion etc..
	typedef typename tUserSList::iterator iterator;

	//typedef typename pair<std::string, iterator> tKeyDataPair;
	typedef pair<KeyType,iterator> tHashDataPair;

	// hash of key (lowercase nick) -> mapped to iterator
	typedef map<KeyType,iterator> tUserHash;
	typedef typename tUserHash::iterator tUHIt;


	size_t size(){ return mUserHash.size();}
	tcHashListMap():cObj("tcHashListMap"){}
	~tcHashListMap(){}

private:
	tUserSList mUserList;
	tUserHash  mUserHash;
	hHashStr<KeyType> mHasher;

public:

	iterator begin(){ return mUserList.begin();}
	iterator end()  { return mUserList.end();  }

	KeyType   Key2Hash(const string &Key){ return mHasher(Key); }

	bool     AddWithHash(DataType Data, const KeyType &Hash);
	bool     RemoveByHash(const KeyType &Hash);
	bool     ContainsHash(const KeyType &Hash);
	DataType GetByHash(const KeyType &Hash);
	virtual void OnAdd(DataType){};
	virtual void OnRemove(DataType){};
};

template < class DataType, class KeyType >
bool tcHashListMap<DataType,KeyType>::ContainsHash(const KeyType &Hash)
{
	return mUserHash.find(Hash) != mUserHash.end();
}

template < class DataType, class KeyType >
DataType tcHashListMap<DataType,KeyType>::GetByHash(const KeyType &Hash)
{
	tUHIt uhit = mUserHash.find(Hash);
	if( uhit !=  mUserHash.end()) return *(uhit->second);
	return NULL;
}

template < class DataType, class KeyType >
bool tcHashListMap<DataType,KeyType>::AddWithHash(DataType Data, const KeyType &Hash)
{
	if(ContainsHash(Hash)) { if(Log(0))LogStream() << "Trying to add " << Hash << " twice" << endl; return false; }

	iterator ulit = mUserList.insert(mUserList.begin(), Data);
	if(ulit == mUserList.end()) { if(Log(0))LogStream() << "Can't add " << Hash << " into the list" << endl; return false; 	}

	pair<tUHIt,bool> P = mUserHash.insert(tHashDataPair(Hash,ulit));
	if ( P.second ) {
		OnAdd(Data);
		if(Log(3))LogStream() << "Successfully added " << Hash << endl;
	}
	else {
		if(Log(0))LogStream() << "Can't add " << Hash << endl;
		mUserList.erase(ulit);
		return false;
	}

	// debug check
	// if(!ContainsHash(Hash)) { if(Log(0))LogStream() << "ERROR adding " << Hash << endl; return false; }

	return true;
}

template < class DataType, class KeyType >
bool tcHashListMap<DataType,KeyType>::RemoveByHash(const KeyType &Hash)
{
	tUHIt uhit = mUserHash.find(Hash);
	if( uhit !=  mUserHash.end())
	{
		// if(Log(0))LogStream() << (*(uhit->second))->mNick << " to remove :" << Hash << endl;
		OnRemove(*(uhit->second));
		mUserList.erase( uhit->second);
		mUserHash.erase( uhit );
		if(Log(3))LogStream() << "Removed " << Hash << " successfully" << endl;
		return true;
	}
	if(Log(3)) LogStream() << "Removing Data that doesn't exist :" << Hash << endl;
	return false;
}


	}; // namespace nUtils
}; // namespace nVerliHub
#endif
