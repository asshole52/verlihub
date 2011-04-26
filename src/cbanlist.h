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

#ifndef NDIRECTCONNECTCBANLIST_H
#define NDIRECTCONNECTCBANLIST_H
#include "cconfmysql.h"
#include "cban.h"
#include "ckick.h"
#include <string>
#include <iostream>
#include "thasharray.h"

using std::string;
using std::ostream;

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nTables {
		class cUnBanList;
		/// @addtogroup Core
		/// @{
		/**
		 * The banlist manager that is capable of:
		 * - keeping a list of ban
		 * - adding new ban
		 * - deleting existing ban
		 * - checking if IP address, nickname, share, etc. are banned
		 * Every ban entity is an instance of cBan class
		 * and it interacts with banlist table in a MySQL database.
		 *
		 * The class also keeps a list of temporary bans for IP address
		 * and nickname that is not stored in database.
		 * @author Daniel Muller
		 */
		class cBanList : public nConfig::cConfMySQL
		{
			friend class nVerliHub::nSocket::cServerDC;

			/**
			 * Structure that describes a temporary ban.
			 */
			struct sTempBan
			{
				sTempBan(long u, const string &reason) : mUntil(u), mReason(reason)
				{};
				/// Time when the ban will expired.
				long mUntil;
				/// Reason of the ban
				string mReason;
			};
			public:
				/**
				 * Class constructor.
				 * @param server Pointer to a cServerDC instance.
				 */
				cBanList(nSocket::cServerDC*);

				/**
				 * Class destructor.
				 */

				~cBanList();

				/**
				 * Add a new ban to banlist table.
				 * If the ban already exists, the entry found is updated.
				 * @param ban A cBan instance.
				 */
				void AddBan(cBan &ban);

				/**
				* Temporary ban a nickname with the given reason.
				* This will add a new entry in mTempNickBanlist list.
				* @param ip The nickname to ban.
				* @param until The end of the ban.
				* @param reason The reason of the ban.
				*/
				void AddNickTempBan(const string &nick, long until, const string &reason);

				/**
				 * Temporary ban an IP address with the given reason.
				 * This will add a new entry in mTempIPBanlist list.
				 * @param ip The IP address in network byte order.
				 * @param until The end of the ban.
				 * @param reason The reason of the ban.
				 */
				void AddIPTempBan(unsigned long ip, long until, const string &reason);

				/**
				 * Populate a stream to build a WHERE condition for a SELECT statement.
				 * @param os The stream where to write the condition.
				 * @param value The value to be matched.
				 * @param mask The bit-mask of ban type.
				 */
				bool AddTestCondition(ostream &os, const string &value, int mask);

				/**
				 * Delete ban entries that are 30 days old.
				 */
				virtual void Cleanup();

				/**
				 * Delete all ban entries matching the given conditions.
				 * Flags is a bit-mask of tBanFlags enumerations and it controls
				 * how to build the WHERE condition to delete ban entries:
				 * - tBanFlags::eBF_IP: match bans by IP address
				 * - tBanFlags::eBF_NICK: match bans by nickname
				 * Both conditions can be specified.
				 * @param ip The IP address.
				 * @param nick The nickname.
				 * @param mask The bit-mask for conditions.
				 * @return The number of deleted entries.
				 */
				int DeleteAllBansBy(const string &ip, const string &nick, int mask);

				/**
				 * Delete a given ban.
				 * @param ban An instance of cBan class to delete.
				 */
				void DelBan(cBan &ban);

				/**
				 * Delete a temporary ban on IP address in mTempIPBanlist list
				 * @param ip The IP address.
				 */
				void DelIPTempBan(unsigned long ip);

				/**
				 * Delete a temporary ban on nickname in mTempNickBanlist list
				 * @param ip The nickname.
				 */
				void DelNickTempBan(const string &nick);

				/**
				 * Extract the level domain substring from the given host.
				 * @param hostname The hostname.
				 * @param result The string where to store the result.
				 * @param level The level domain substring to extract.
				 * @return True if the substring can be extracted or false otherwise.
				 */
				bool GetHostSubstring(const string &hostname, string &dest, int level);

				/**
				 * Check if an IP address is temporary banned.
				 * Search if done in mTempIPBanlist list.
				 * @param ip The IP address.
				 * @return The end of the ban if the ban if found or zero otherwise.
				 */
				long IsIPTempBanned(unsigned long ip);

				/**
				 * Check if a nickname is temporary banned.
				 * Search if done in mTempNickBanlist list.
				 * @param ip The nickname.
				 * @return The end of the ban if the ban if found or zero otherwise.
				 */
				long IsNickTempBanned(const string &nick);

				/**
				 * Convert an Internet address in standard format (dotted string)
				 * representation to Internet network address.
				 * @param ip The IP address in standard format representation.
				 * @return The Internet network address.
				 * * @see Num2Ip()
				 */
				static unsigned long Ip2Num(const string &ip);

				/**
				 * Fetch a list of ban entries sorted by ban time
				 * and output it to the given stream.
				 * @param os The destination stream.
				 * @param count The number of entries to select.
				 */
				void List(ostream &os, int count);

				/**
				 * Load a ban entry into the given cBan instance.
				 * The cBan instance must set the primary key of
				 * the entry in banlis table.
				 * @param ban cBan instance.
				 * @return True if the entry if found or false otherwise.
				 */
				bool LoadBanByKey(cBan &ban);

				/**
				 * Fill the given cBan instance from the given
				 * user connection.
				 * If connection is valid, the cBan instance will contain the
				 * IP address, host, reason of the ban, the banner nickname,
				 * the nickname of the user and the start and end date of the ban.
				 * @param ban cBan instance where to store the result.
				 * @param connection The user connection.
				 * @param nick_op The banner nickname.
				 * @param reason The reason of the ban.
				 * @param length The time of the ban.
				 * @param mask The bit-mask for ban type.
				 * @see tBanFlags
				 */
				void NewBan(cBan &ban, nSocket::cConnDC *connection, const string &nickOp, const string &reason, unsigned length, unsigned mask);

				/**
				* Fill the given cBan instance from the given
				* cKick class instance.
				* The cBan instance will contain the IP address, host,
				* reason of the ban, the banner nickname, the nickname,
				* email address and share of the user and the start and
				* end date of the ban.
				* @param ban cBan instance where to store the result.
				* @param kick cKick instance.
				* @param length The time of the ban.
				* @param mask The bit-mask for ban type.
				* @see tBanFlags
				*/
				void NewBan(cBan &ban, const cKick &kick, long length, int mask);

				/**
				 * Convert an Internet network address to its Internet
				 * standard format (dotted string) representation.
				 * @param ip A proper address representation.
				 * @return Internet IP address as a string.
				 * @see Ip2Num()
				 */
				static void Num2Ip(unsigned long num, string &ip);

				/**
				 * Remove temporary ban entries for banned IP address
				 * and nickname.
				 * The search is done in both mTempNickBanlist and
				 * mTempIPBanlist lists.
				 * @param before Delete all ban entries that expires before this date.
				 * @return The number of removed entries.
				 */
				int RemoveOldShortTempBans(long before);

				/**
				 * Set unbanlist manager instance.
				 * @see cUnBanList
				 */
				void SetUnBanList(cUnBanList *UnBanList)
				{
					mUnBanList = UnBanList;
				}

				/**
				 * Found a ban entry with the given user connection
				 * and nickname.
				 * The WHERE condition to query a ban entry is build
				 * with AddTestCondition() call and it depends on the
				 * value of the given bit-mask.
				 * If the ban entry found is loaded into cBan instance.
				 * @param ban The cBan instance where to load the ban entry.
				 * @param connection The connection of the user.
				 * @param nickname The nickname.
				 * @param mask The bit-mask for conditions.
				 * @see tBanFlags
				 */
				bool TestBan(cBan &, nSocket::cConnDC *connection, const string &nickname, unsigned mask);

				/**
				 * Remove ban entries and create new unban entries for each found ban.
				 * Flags is a bit-mask of tBanFlags enumerations and it controls
				 * how to build the WHERE condition to delete the ban entries.
				 * See AddTestCondition() for more information.
				 * By default ban entries are deleted from banlist table unless you speficy
				 * the deleteEntry param.
				 * @param os The stream where to output the unban entries.
				 * @param value The value for WHERE condition.
				 * @param reason The unban reason.
				 * @param nickOp The unbanner nickname.
				 * @param mask The bit-mask for the condition.
				 * @param deleteEntry Set to true if you want to delete ban entries from banlist table
				 * @return The number of ban entries found
				 * @see AddTestCondition()
				 */
				int Unban(ostream &os, const string &value, const string &reason, const string &nickOp, int mask, bool deleteEntry = true);

				/**
				 * Syncronize the given cBan instance with the
				 * ban entry in banlis table.
				 * @param ban The cBan instance.
				 * @return Always zero.
				 */
				int UpdateBan(cBan &);
			protected:
				/// cBan instance.
				/// This is the model of the table and
				/// the attribute is used to fetch new ban entry with LoadPK() call
				/// or add a new one with SavePK() call.
				/// @see SavePK()
				/// @see LoadPK()
				cBan mModel;
			private:
				/// Pointer to unbanlist manager.
				cUnBanList *mUnBanList;

				/// Define a list of temporary bans.
				typedef tHashArray<sTempBan *> tTempNickBans;

				/// List of temporary bans on nickname.
				tTempNickBans mTempNickBanlist;

				/// List of temporary bans on IP address.
				tTempNickBans mTempIPBanlist;

				/// Pointer to cServerDC instance.
				nSocket::cServerDC* mS;

		};

		/**
		 * The unbanlist manager that has the same feature of banlist manager.
		 * The same table of cBanList class is used to store a list of unban entries.
		 * Every unban entity is an instance of cUnBan class
		 * and it interacts with banlist table in a MySQL database.
		 * @author Daniel Muller
		 */
		class cUnBanList : public cBanList
		{
			public:
				/**
				 * Class constructor.
				 * @param server Pointer to a cServerDC instance.
				 */
				cUnBanList(nSocket::cServerDC*);

				/**
				 * Class destructor.
				 */
				~cUnBanList();

				/**
				 * Delete unban entries that are 30 days old.
				 */
				virtual void Cleanup();
			protected:
				/// cUnBan instance.
				/// This is the model of the table and
				/// the attribute is used to fetch new unban entry with LoadPK() call
				/// or add a new one with SavePK() call.
				/// @see SavePK()
				/// @see LoadPK()
				cUnBan mModelUn;
		};
		/// @}
	}; // namesapce nTables
}; // namespace nVerliHub

#endif
