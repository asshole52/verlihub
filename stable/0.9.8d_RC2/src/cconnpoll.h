/***************************************************************************
 *   Copyright (C) 2003 by Dan Muller                                      *
 *   dan@verliba.cz                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "cconnchoose.h"

#ifndef NSERVERCCONNPOLL_H
#define NSERVERCCONNPOLL_H

#if !USE_SELECT
#include <sys/poll.h>

#if !defined _SYS_POLL_H_ && !defined _SYS_POLL_H && !defined pollfd && !defined _POLL_EMUL_H_
/** the poll file descriptor structure (where not defined) */
struct pollfd {
   int fd;           /* file descriptor */
   short events;     /* requested events */
   short revents;    /* returned events */
};
#endif

#include <vector>
using std::vector;

namespace nServer {

/**
polling connection chooser

@author Daniel Muller
*/
class cConnPoll : public cConnChoose
{
public:
	cConnPoll();
	~cConnPoll();

	/** Calls the poll function to determine non-blocking socket
	  * \sa cConnChoose::Choose
	  */
	virtual int Choose(cTime &tmout){ return this->poll(tmout.MiliSec()); };
	/// @see cConnChoose::OptIn
	virtual void OptIn ( tSocket, tChEvent);
	/// @see cConnChoose::OptOut
	virtual void OptOut( tSocket, tChEvent);
	virtual int OptGet( tSocket );
	/// @see cConnChoose::RevGet
	virtual int RevGet( tSocket );
	virtual bool RevTest( tSocket );
	virtual bool AddConn(cConnBase *);

	/** Wrapper for system defined pollfd, provides constructor, and reset methods
	  * @author Daniel Muller
	*/
	struct cPollfd: public pollfd
	{
		cPollfd(){reset();};
		void reset(){fd=-1;events=revents=0;};
	};

	virtual bool RevTest( cPollfd & );

	int poll(int wp_sec);
	typedef vector<cPollfd> tFDArray;

	cPollfd &FD(int sock){ return mFDs[sock];}
protected:
	tFDArray mFDs;

	const int mBlockSize;

};

};

#endif

#endif