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
#include "ctime.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sstream>
#include <string.h>
#if defined _WIN32
#include <windows.h>
#endif
#include "i18n.h"

using namespace std;

namespace nVerliHub {
	namespace nUtils {

cTime::~cTime()
{
}

string cTime::AsString() const
{
	ostringstream os;
	os << (*this);
	return os.str();
}

std::ostream & operator<< (std::ostream &os, const cTime &t)
{
	#ifdef WIN32
		static char *buf;
	#else
		#define CTIME_BUFFSIZE 26
		static char buf[CTIME_BUFFSIZE + 1];
	#endif

	long n, rest, i;
	ostringstream ostr;

	switch (t.mPrintType) {
		case 1:
			#ifdef WIN32
				buf = ctime((const time_t*) & (t.tv_sec));
			#else
				strftime(buf, CTIME_BUFFSIZE + 1, "%Y/%m/%d %H:%M:%S", localtime((const time_t*) & (t.tv_sec)));
			#endif

			buf[strlen(buf)] = 0;
			os << buf;
		break;
		case 2:
			rest = t.tv_sec;
			i = 0;
			n = rest / (24 * 3600 * 7);
			rest %= (24 * 3600 * 7);
			if ((n > 0) && (++i <= 2)) ostr << " " << autosprintf(((n == 1) ? _("%ld week") : _("%ld weeks")), n);
			n = rest / (24 * 3600);
			rest %= (24 * 3600);
			if ((n > 0) && (++i <= 2)) ostr << " " << autosprintf(((n == 1) ? _("%ld day") : _("%ld days")), n);
			n = rest / 3600;
			rest %= 3600;
			if ((n > 0) && (++i <= 2)) ostr << " " << autosprintf(((n == 1) ? _("%ld hour") : _("%ld hours")), n);
			n = rest / 60;
			rest %= 60;
			if ((n > 0) && (++i <= 2)) ostr << " " << autosprintf(((n == 1) ? _("%ld min") : _("%ld mins")), n);
			n = rest;
			rest = 0;
			if ((n > 0) && (++i <= 2)) ostr << " " << autosprintf(((n == 1) ? _("%ld sec") : _("%ld secs")), n);
			if ((((int)t.tv_usec / 1000) > 0) && (++i <= 2)) ostr << " " << autosprintf(_("%d ms"), (int)t.tv_usec / 1000);
			//if ((((int)t.tv_usec % 1000) > 0) && (++i <= 2)) ostr << " " << autosprintf(_("%d �s"), (int)t.tv_usec % 1000);

			if (ostr.str().empty())
				os << autosprintf(_("%d ms"), 0); // _("%d �s")
			else
				os << ostr.str().substr(1); // strip space
		break;
		default:
			os << t.tv_sec << " " << ((t.tv_sec == 1) ? _("sec") : _("secs")) << t.tv_usec << " " << _("�s");
		break;
	}

	return os;
};

	}; // namespace nUtils
}; // namespace nVerliHub
