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
#include "ccallbacklist.h"
#include "cpluginbase.h"
#include "cpluginmanager.h"
#ifdef WIN32
#pragma warning( disable : 4355)
#endif

namespace nVerliHub {
	namespace nPlugin {

cCallBackList::cCallBackList(cPluginManager *mgr, string id) :
mMgr(mgr),
mCallOne(mMgr,this),
mName(id)
{
	if(mMgr)
		mMgr->SetCallBack(id, this);
}

const string &cCallBackList::Name() const
{
	return mName;
}

cCallBackList::~cCallBackList()
{}

void cCallBackList::ufCallOne::operator()(cPluginBase *pi)
{
	if(mCall)
		mCall = mCBL->CallOne(pi);
	// If the plugin is not alive, unload it with plugin manager
	if(!pi->IsAlive())
		mMgr->UnloadPlugin(pi->Name());
}

bool cCallBackList::Register(cPluginBase *plugin)
{
	if(!plugin)
		return false;
	tPICont::iterator i = find(mPlugins.begin(), mPlugins.end(), plugin);
	if(i != mPlugins.end())
		return false;
	mPlugins.push_back(plugin);
	return true;
}

bool cCallBackList::Unregister(cPluginBase *plugin)
{
	if(!plugin)
		return false;
	tPICont::iterator i = find(mPlugins.begin(), mPlugins.end(), plugin);
	if(i == mPlugins.end())
		return false;
	mPlugins.erase(i);
	return true;
}

bool cCallBackList::CallAll()
{
	mCallOne.mCall = true;
	return for_each(mPlugins.begin() , mPlugins.end(), mCallOne).mCall;
}

void cCallBackList::ListRegs(ostream &os, const char *indent)
{
	for(tPICont::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
		os << indent << (*i)->Name() << "\r\n";
}
	}; // namespace nPlugin
}; // namespace nVerliHub
