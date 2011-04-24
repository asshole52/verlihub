/**************************************************************************
*   Original Author: Frog (frg at otaku-anime dot net) 2007-209           *
*                                                                         *
*   Copyright (C) 2010-2011 by Verlihub Project                           *
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
#include "cpipython.h"
#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "cconsole.h"
#include "cpythoninterpreter.h"
#include "src/stringutils.h"
#include "src/i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nPythonPlugin {

cConsole::cConsole(cpiPython *pyt) :
	mPython(pyt),
	mCmdPythonScriptAdd(1,"!pyload ", "(\\S+)", &mcfPythonScriptAdd),
	mCmdPythonScriptGet(0,"!pylist", "", &mcfPythonScriptGet),
	mCmdPythonScriptDel(2,"!pyunload ", "(\\S+)", &mcfPythonScriptDel),
	mCmdPythonScriptRe(3,"!pyreload ", "(\\S+)", &mcfPythonScriptRe),
	mCmdPythonScriptLog(4,"!pylog ", "(\\d+)", &mcfPythonScriptLog),
	mCmdr(this)
{
	mCmdr.Add(&mCmdPythonScriptAdd);
	mCmdr.Add(&mCmdPythonScriptDel);
	mCmdr.Add(&mCmdPythonScriptGet);
	mCmdr.Add(&mCmdPythonScriptRe);
	mCmdr.Add(&mCmdPythonScriptLog);
}

cConsole::~cConsole()
{
}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	if(mCmdr.ParseAll(str, os, conn) >= 0)
	{
		mPython->mServer->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfGetPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}
	(*mOS) << _("Loaded Python scripts:") << "\r\n";

	(*mOS) << "\n ";
	(*mOS) << setw(6) << setiosflags(ios::left) << "ID";
	(*mOS) << toUpper(_("Script")) << "\n";
	(*mOS) << " " << string(6+20,'=') << endl;

	for(int i = 0; i < GetPI()->Size(); i++) {
		(*mOS) << " " << setw(6) << setiosflags(ios::left) << GetPI()->mPython[i]->id << GetPI()->mPython[i]->mScriptName << "\r\n";
	}
	return true;
}

bool cConsole::cfDelPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1,scriptfile);

	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	bool found = false;
	bool number = false;
	int num = 0;
	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;
	for(it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;
		if((number && num == li->id) || (!number && StrCompare(li->mScriptName,0,li->mScriptName.size(),scriptfile) == 0)) {
			found = true;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str()) << " ";
			delete li;
			GetPI()->mPython.erase(it);

			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str()) << " ";
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it is not running."), scriptfile.c_str()) << " ";
	}
	return true;
}

bool cConsole::cfAddPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1, scriptfile);

	if(!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	cPythonInterpreter *ip = new cPythonInterpreter(scriptfile);
	if(!ip) {
		(*mOS) << _("Failed to allocate new Python interpreter.");
		return true;
	}

	GetPI()->mPython.push_back(ip);
	if(ip->Init())
		(*mOS) << autosprintf(_("Script %s is now running."), ip->mScriptName.c_str()) << " ";
	else {
		(*mOS) << autosprintf(_("Script %s not found or could not be parsed!."), scriptfile.c_str()) << " ";
		GetPI()->mPython.pop_back();
		delete ip;
	}

	return true;
}

bool cConsole::cfLogPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}
	string level;
	GetParStr(1, level);
	ostringstream ss;
   	ss << cpiPython::log_level;
   	string oldValue = ss.str();
	ss.str("");
	ss << level;
   	string newValue = ss.str();
	(*mOS) << autosprintf(_("Updated %s.%s from '%s' to '%s'"), "pi_lua", "log_level", oldValue.c_str(), newValue.c_str()) << "\r\n";
	cpiPython::me->LogLevel( atoi(level.c_str()) );
	return true;
}

bool cConsole::cfReloadPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1,scriptfile);

	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	bool found = false;
	bool number = false;
	int num = 0;
	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;
	string name;
	for(it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;
		if((number && num == li->id) || (!number && StrCompare(li->mScriptName,0,li->mScriptName.size(),scriptfile) == 0)) {
			found = true;
			name = li->mScriptName;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str()) << " ";
			delete li;
			GetPI()->mPython.erase(it);
			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str()) << " ";
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it is not running."), scriptfile.c_str()) << " ";
		return false;
	} else {
		cPythonInterpreter *ip = new cPythonInterpreter(name);
		if(!ip) {
			(*mOS) << _("Failed to allocate new Python interpreter.");
			return true;
		}

		GetPI()->mPython.push_back(ip);
		if(ip->Init())
			(*mOS) << autosprintf(_("Script %s is now running."), ip->mScriptName.c_str()) << " ";
		else {
			(*mOS) << autosprintf(_("Script %s not found or could not be parsed!."), scriptfile.c_str()) << " ";
			GetPI()->mPython.pop_back();
			delete ip;
		}
		return true;
	}
}

	}; // namespace nPythonPlugin
}; // namespace nVerliHub
