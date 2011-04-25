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

#ifndef CASYNCSOCKETSERVER_H
#define CASYNCSOCKETSERVER_H

#include "cconnchoose.h"

#if USE_SELECT
#include "cconnselect.h"
#else
#include "cconnpoll.h"
#endif
#include "ctimeout.h"
#include <list>
#include "cobj.h"
//#include "cconndc.h" // added
#include "casyncconn.h"
#include "cmeanfrequency.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	namespace nSocket {
		/// @addtogroup Core
		/// @{
		/**
		* General asynchronous (non-blocking) socket server with some basic functionality.
		*
		* @author Daniel Muller
		*/
		class cAsyncSocketServer : public cObj
		{
			public:
				friend class nVerliHub::nSocket::cAsyncConn;

				/// Define a list of connections.
				typedef list<cAsyncConn*> tConnList;

				/// Define an iterator to a list of connections.
				typedef tConnList::iterator tCLIt;

				/**
				* Class constructor.
				* Create a server and start listening on given port.
				* @param port The port to listen on.
				*/
				cAsyncSocketServer(int port=0);

				/**
				* Class destructor.
				*/
				virtual ~cAsyncSocketServer();

				/**
				* Stop main process loop and delete all connections.
				*/
				void close();

				/**
				* Return the port on which the server is listening on.
				* @return The port.
				*/
				virtual unsigned int getPort() const;

				/**
				 * Create a new connection and handle it.
				 * @param OnPort The port to listen on.
				 * @param UDP True if it is an UDP connection.
				 * @return The new connection.
				 * @see ListenWithConn()
				 */
				virtual cAsyncConn * Listen(int OnPort, bool UDP = false);

				/**
				 * Handle the given connection and listen on the given port.
				 * @param connection The connection to listen on.
				 * @param OnPort The port to listen on.
				 * @param UDP True if it is an UDP connection.
				 * @return The new connection.
				 */
				virtual cAsyncConn * ListenWithConn(cAsyncConn *connection, int OnPort, bool UDP=false);

				/**
				* This event is triggered when a connection is closed.
				* @param conn The closed connection closed.
				*/
				void OnConnClose(cAsyncConn*);

				/**
				* Event handler function called every period of time by OnTimerBase().
				* The frequency of this method is called depends on
				* the value of timer_serv_period config variable.
				* @param now The current time.
				* @return Always zero.
				* @see OnTimerBase()
				*/
				virtual int OnTimer(cTime &now);

				/**
				* This event is trigger every N seconds.
				* This method is responsable of calling cAsyncConn::OnTimerBase()
				* for every connections the server is handling.
				* @param now Current time.
				* @return Always zero.
				*/
				int OnTimerBase(cTime &now);

				/**
				* Start the main loop.
				* This �s the method that calls OnTimerBase() and TimeStep()
				* Run it until it is stopped or paused.
				* @see TimeStep()
				* @see OnTimerBase()
				* @return The error code.
				*/
				int run();

				/**
				 *
				 * Set the port to listen on
				 * @param newPort The new port.
				 */
				virtual void setPort(const unsigned int newPort);

				/**
				 * Start the socket to listen on.
				 * @param OverrideDefaultPort Use this port instead of mPort.
				 * @return -1 on failure.
				 */
				virtual int StartListening(int OverrideDefaultPort=0);

				/**
				* Stop main loop and set error code.
				* @param code The error code.
				*/
				void stop(int);

				/**
				 * Stop handling the given connection.
				 * @param connection The connection.
				 * @return True if succesfull or false otherwise.
				 */
				virtual bool StopListenConn(cAsyncConn *connection);

				/**
				* This method handles all connections and
				* iterate over them with cConnChoose instance.
				* Handle new incoming connection, take care of existing ones
				* and close those connections that are not actived anymore.
				*/
				void TimeStep();

				/// The address to listen on
				string mAddr;

				/// The port to listen on
				unsigned int mPort;

				/// Connection period to call event time method cAsyncConn::OnTimerBase().
				/// This value controls how often cAsyncConn::OnTimerBase() is called.
				int timer_conn_period;

				/// Server period to call event time method OnTimerBase().
				/// This value controls how often OnTimerBase() is called.
				int timer_serv_period;

				/// Delay in milliseconds to wait in main loop for every iteration.
				int mStepDelay;

				/// Maximum size of the buffer for cAsyncConn::SetLineToRead() method.
				unsigned long mMaxLineLength;

				/// Use reverse DNS lookup feature when there is a new connection.
				int mUseDNS;

				/// The current time.
				cTime mTime;

				/// Measure the frequency of the server.
				nUtils::cMeanFrequency<unsigned ,21> mFrequency;

		protected:
			/// Indicate if the main loop is running.
			bool mbRun;

			/// List of connections that the server is handling.
			/// The list contains pointers to cAsyncConn instance.
			tConnList mConnList;

			#if !USE_SELECT
				/// Connection chooser for poll.
				cConnPoll mConnChooser;
			#else
				/// Connection chooser for select.
				cConnSelect mConnChooser;
			#endif

			/// True if Windows sockets is initialized.
			static bool WSinitialized;

			/// Pointer to connection factory instance.
			cConnFactory *mFactory;

			/**
			* Add the connection to the server so it can be processed.
			* @param conn The connection to add.
			*/
			virtual void addConnection(cAsyncConn *);

			/**
			* Return true if the server accepts new incoming connections.
			* @return True if the server accepts a new connection or false otherwise.
			*/
			virtual bool AllowNewConn()
			{
				return true;
			};

			/**
			* Remove the connection from the server.
			* The pointer to the connection will be deleted and not valid anymore.
			* @param conn The connection to remove.
			*/
			void delConnection(cAsyncConn *);

			/**
			* Create a new string buffer for the given connection.
			* @param conn The connection.
			* @return Pointer to the new buffer.
			*/
			virtual string * FactoryString(cAsyncConn *conn);

			/**
			* Perform input operation on the given connection.
			* This method will read all data from the connection
			* and return the number of read bytes or a negative number if an error occurred.
			* @param conn The connection.
			* @return Number of read bytes.
			*/
			virtual int input(cAsyncConn *conn);

			/**
			* This event is triggered when there is a new incoming message
			* for a connection.
			* @param conn The connection that has new incoming message.
			* @param message Pointer to the new message.
			*/
			virtual void OnNewMessage(cAsyncConn *, string *);

			/**
			* Perform output operation on the given connection.
			* This method will send all data from the connection
			* and return the number of sent bytes or a negative number if an error occurred.
			* @param conn The connection.
			* @return Number of sent bytes.
			*/
			int output(cAsyncConn * conn);

			/**
			* This event is triggered when there is a new incoming connection.
			* @param conn The new connection.
			* @return Zero if connection is accepted or a negative number otherwise.
			*/
			virtual int OnNewConn(cAsyncConn*);

			/// Structure that contains timers
			struct sTimers
			{
				/// Server time of the last call of OnTimerBase
				cTime main;
				/// Connection time of the last call of cAsyncConn::OnTimerBase
				cTime conn;
			};
			/// Timer structure
			sTimers mT;

			/// Errore code when main loop stops
			int mRunResult;
		private:
			/// Pointer to the connection that server is currently handling
			cAsyncConn * mNowTreating;
		};
	}; // namespace nSocket
}; // namespace nVerliHub
#endif
