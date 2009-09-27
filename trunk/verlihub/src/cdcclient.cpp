/***************************************************************************
*   Original Author: Daniel Muller (dan at verliba dot cz) 2003-05        *
*                                                                         *
*   Copyright (C) 2006-2009 by Verlihub Project                           *
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
#include "cserverdc.h"
#include "cdcclient.h"
#include "cconndc.h"
#include "stringutils.h"
#include <stdlib.h>

using namespace nStringUtils;
namespace nDirectConnect {
	namespace nTables {
		
		/**
		
		Class constructor
		
		*/
	  
		cDCClient::cDCClient()
		{
			mName = "Unknown";
			mEnable = 1;
			mMinVersion = -1;
			mMaxVersion = -1;
			mBan = false;
		}
		
		/**
		
		Class destructor
		
		*/
		
		cDCClient::~cDCClient(){}
		
		/**
		
		This function is called when cRedirect object is created. Here it is not useful so the body is empty
		
		*/
		
		void cDCClient::OnLoad()
		{}
		
		/**
		
		Redefine << operator to describe a redirect and show its status
		
		@param[in,out] os The stream where to store the description.
		@param[in,out] tr The cRedirect object that describes the redirect
		@return The stream
		*/
		
		ostream &operator << (ostream &os, cDCClient &tr)
		{
			os << "\r" << tr.mName << "\t - Tag ID: " << tr.mTagID;
			os << " (";
			if(tr.mEnable) os << "Enable";
			else os << "Disable";
			os << ")";
			return os;
		}
	};
};
 