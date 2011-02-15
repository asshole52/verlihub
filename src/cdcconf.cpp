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
#include "cdcconf.h"
#include <string>
using namespace std;

namespace nDirectConnect
{
namespace nTables
{

cDCConf::cDCConf( cServerDC &serv ): mS(serv)
{
	max_length[eDC_KEY]=128;
	max_length[eDC_VALIDATENICK]=64;
	max_length[eDC_MYPASS]=64;
	max_length[eDC_VERSION]=32;
	max_length[eDC_GETNICKLIST]=16;
	max_length[eDC_MYNIFO]=128;
	max_length[eDC_GETINFO]=10+max_nick;
	max_length[eDC_CONNECTTOME]=32*2*max_nick;
	max_length[eDC_MCONNECTTOME]=64;
	max_length[eDC_RCONNECTTOME]=64;
	max_length[eDC_TO]=2048;
	max_length[eDC_CHAT]=1024;
	max_length[eDC_QUIT]=64;
	max_length[eDC_OPFORCEMOVE]=512;
	max_length[eDC_KICK]=64;
	max_length[eDC_SEARCH_PAS]=256;
	max_length[eDC_SEARCH]=256;
	max_length[eDC_SR]=256;
	max_length[eDC_MSEARCH]=256;
	max_length[eDC_UNKNOWN]=8;
}

cDCConf::~cDCConf()
{
}

void cDCConf::AddVars()
{
	// Hub info and basic settings
	Add("hub_name",hub_name,string("VerliHub"));
	Add("hub_desc",hub_desc,string("No Description"));
	Add("hub_topic",hub_topic,"");
	Add("hub_category",hub_category,string(""));
	Add("hub_owner",hub_owner,string(""));
	Add("hub_version",hub_version,VERSION);
	Add("hub_version_special",hub_version_special,string(""));	
	Add("hub_security", hub_security, string("VerliHub"));
	Add("hub_security_desc", hub_security_desc, string("Hub-Security for broadcasting messages"));
	Add("opchat_name", opchat_name, string("OpChat"));
	Add("opchat_desc", opchat_desc, string("Operator chat - only for OPs"));
	Add("hub_host",hub_host,string(""));
	Add("listen_ip",mS.mAddr,string("0.0.0.0"));
	#if !defined _WIN32
	Add("listen_port",mS.mPort,4111);
	#else
	Add("listen_port",mS.mPort,411);
	#endif
	Add("extra_listen_ports", extra_listen_ports,string(""));
	// End hub info and basic settings
	
	// Hublist configuration
	Add("hublist_host",hublist_host,string(""));
	Add("hublist_port",hublist_port,2501);
	Add("hublist_send_minshare",hublist_send_minshare,true);
	Add("timer_hublist_period", mS.mHublistTimer.mMinDelay.tv_sec,(__typeof__( mS.mHublistTimer.mMinDelay.tv_sec)) 0); //@todo 3*3600
	// End hublist configuration

	// Max users configuration
	Add("max_users",max_users_total,6000);
	Add("max_extra_regs",max_extra_regs,25);
	Add("max_extra_vips",max_extra_vips,50);
	Add("max_extra_ops",max_extra_ops,100);
	Add("max_extra_cheefs",max_extra_cheefs,100);
	Add("max_extra_admins",max_extra_admins,200);
	Add("max_users0",max_users[0],600);
	Add("max_users1",max_users[1],5400);
	Add("max_users2",max_users[2],1000);
	Add("max_users3",max_users[3],1000);
	Add("max_users4",max_users[4],1000);
	Add("max_users5",max_users[5],1000);
	Add("max_users6",max_users[6],1000);
	// End max users configuration
	
	// Share configuration
	Add("min_share",min_share,(unsigned long)1*1024);
	Add("min_share_reg",min_share_reg,(unsigned long)1*1024);
	Add("min_share_vip",min_share_vip,(unsigned long)1*1024);
	Add("min_share_ops",min_share_ops,(unsigned long)0);
	Add("min_share_factor_passive", min_share_factor_passive, 1.0);
	Add("min_share_use_hub",min_share_use_hub,(unsigned long)0);
	Add("min_share_use_hub_reg",min_share_use_hub_reg,(unsigned long)0);
	Add("min_share_use_hub_vip",min_share_use_hub_vip,(unsigned long)0);
	Add("max_share",max_share,(unsigned long)10*1024*1024);
	Add("max_share_reg",max_share_reg,(unsigned long)10*1024*1024);
	Add("max_share_vip",max_share_vip,(unsigned long)10*1024*1024);
	Add("max_share_ops",max_share_ops,(unsigned long)10*1024*1024);
	// End share configuration
	
	// Search configuration
	Add("search_number", search_number, 1);
	Add("int_search",int_search,32);
	Add("int_search_pas",int_search_pas,48);
	Add("int_search_reg",int_search_reg,16);
	Add("int_search_reg_pass",int_search_reg_pass,48);
	Add("int_search_vip",int_search_vip,8);
	Add("int_search_op",int_search_op,1);
	Add("min_search_chars", min_search_chars, 5);
	Add("max_passive_sr",max_passive_sr,25);
	Add("delayed_search",delayed_search, 1);
	// End search configuration
	
	// Nicklist configuration
	Add("max_nick",max_nick,64u);
	Add("min_nick",min_nick,3u);
	Add("nick_chars",nick_chars, string(""));
	Add("nick_prefix",nick_prefix,string(""));
	Add("nick_prefix_cc", nick_prefix_cc, false);
	Add("nick_prefix_autoreg",nick_prefix_autoreg,string(""));
	Add("autoreg_class", autoreg_class, -1);
	Add("nicklist_on_login",nicklist_on_login, 1);
	Add("optimize_userlist",optimize_userlist,0);
	Add("ul_portion",ul_portion,50);
	// End nicklist configuration
	
	// Mainchat messages configuration
	Add("max_message_size",mS.mMaxLineLength, (unsigned long)10240);
	Add("max_chat_msg",max_chat_msg,256u);
	Add("max_chat_lines",max_chat_lines,5);
	Add("delayed_chat",delayed_chat, false);
	Add("int_chat_ms",int_chat_ms,1000);
	Add("chat_default_on",chat_default_on, true);
	Add("mainchat_class",mainchat_class,0);
	// End mainchat messages configuration
	
	// Private message configuration
	Add("max_flood_counter_pm",max_flood_counter_pm,5);
	Add("int_flood_pm_period", int_flood_pm_period, 5);
	Add("int_flood_pm_limit", int_flood_pm_limit, 5);
	// End private message configuration
	
	// User control configuration
	Add("classdif_reg", classdif_reg, 2);
	Add("classdif_kick", classdif_kick, 0);
	Add("classdif_pm",classdif_pm,10);
	//Add("classdif_search",classdif_search,10);
	Add("classdif_download",classdif_download,10);
	Add("min_class_use_hub",min_class_use_hub,0);
	Add("min_class_use_hub_passive",min_class_use_hub_passive,0);
	Add("min_class_register" , min_class_register , 4);
	Add("min_class_redir",min_class_redir,4);
	Add("min_class_bc", min_class_bc, 4);
	Add("min_class_bc_guests", min_class_bc_guests, 4);
	Add("min_class_bc_regs", min_class_bc_regs, 4);
	Add("min_class_bc_vips", min_class_bc_vips, 4);
	Add("bc_reply",mS.LastBCNick,mEmpty);
	Add("plugin_mod_class",plugin_mod_class,5);
	Add("topic_mod_class",topic_mod_class,5);
	Add("cmd_start_op",cmd_start_op,string("!"));
	Add("cmd_start_user",cmd_start_user,string("+"));
	Add("dest_report_chat", dest_report_chat, false);
	Add("dest_regme_chat", dest_regme_chat, false);
	Add("dest_drop_chat", dest_drop_chat, false);
	Add("disable_me_cmd", disable_me_cmd, false);
	Add("disable_regme_cmd", disable_regme_cmd, false);
	Add("disable_usr_cmds", disable_usr_cmds, false);
	Add("disable_report_cmd", disable_report_cmd, false);
	Add("allways_ask_password", allways_ask_password, false);
	Add("default_password_encryption",default_password_encryption,1); //eCRYPT_ENCRYPT
	Add("password_min_len", password_min_len, 6);
	Add("pwd_tmpban", pwd_tmpban, 10);
	Add("wrongpassword_report", wrongpassword_report, 1);
	Add("botinfo_report", botinfo_report, 0);
	Add("send_user_ip",send_user_ip,false);
	Add("send_user_info", send_user_info, true);
	// End user control configuration
	
	// Advanced hub configuration and tweaks
	Add("extended_welcome_message", extended_welcome_message, 0);
	Add("host_header", host_header, 1);
	Add("int_myinfo",int_myinfo,60);
	Add("int_nicklist",int_nicklist,60);
	Add("int_login",int_login, 60);
	Add("max_class_int_login",max_class_int_login,4);
	Add("tban_kick",tban_kick,300);
	Add("tban_max",tban_max,3600*24);
	Add("log_level",mS.msLogLevel, 0);
	Add("dns_lookup",mS.mUseDNS, 0);
	Add("report_dns_lookup",report_dns_lookup,0);
	Add("hide_all_kicks",hide_all_kicks,0);
	Add("timer_conn_period", mS.timer_conn_period, 4);
	Add("timer_serv_period", mS.timer_serv_period, 1);
	Add("min_frequency", min_frequency, 0.3);
	Add("step_delay", mS.mStepDelay, 50);
	Add("max_attempts_threshold", max_attempts_threshold, 0);
	Add("max_upload_kbps",max_upload_kbps, 2000000.0);
	Add("timer_reloadcfg_period", mS.mReloadcfgTimer.mMinDelay.tv_sec, (__typeof__( mS.mHublistTimer.mMinDelay.tv_sec))300); // 5 minutes
	Add("use_reglist_cache",use_reglist_cache, true);
	Add("use_penlist_cache",use_penlist_cache, true);
	Add("delayed_login" ,delayed_login,  1);
	Add("delayed_myinfo",delayed_myinfo, 1);
	Add("drop_invalid_key",drop_invalid_key, 0);
	Add("delayed_ping",delayed_ping,60);
	Add("disable_zlib", disable_zlib, 0);

	static const char *to_names[] = { "key", "nick", "login", "myinfo", "flush", "setpass"};
	double to_default[] = { 60. , 30., 600., 40., 30., 300. };
	string s_varname;
	for(int i = 0; i < 6; i ++) {
		s_varname = "timeout_";
		s_varname += to_names[i];
		Add(s_varname, timeout_length[i], to_default[i]);
	}
	// End advanced hub configuration and tweaks
	
	 // Tag configuration
	Add("show_tags",show_tags,2);
	Add("tag_allow_none",tag_allow_none, true);
	Add("tag_allow_unknown",tag_allow_unknown,1);
	Add("tag_allow_passive",tag_allow_passive, true);
	Add("tag_allow_sock5",tag_allow_sock5, true);
	Add("tag_sum_hubs", tag_sum_hubs, 2);
	Add("tag_min_class_ignore",tag_min_class_ignore ,(int)eUC_OPERATOR);
	Add("show_desc_len",show_desc_len,-1);
	Add("desc_insert_mode", desc_insert_mode, false);
	Add("show_email",show_email,1);
	Add("show_speed",show_speed,1);
	Add("tag_max_hs_ratio",tag_max_hs_ratio,100.0);
	Add("tag_max_hubs",tag_max_hubs,100);
	Add("tag_min_version",tag_min_version,-1);
	Add("tag_max_version",tag_max_version,-1);
	// End tag configuration
	
	// IP and zone configuration
	Add("cc_zone1",cc_zone[0],string(""));
	Add("cc_zone2",cc_zone[1],string(""));
	Add("cc_zone3",cc_zone[2],string(""));
	Add("ip_zone4_min",ip_zone4_min,string(""));
	Add("ip_zone4_max",ip_zone4_max,string(""));
	Add("ip_zone5_min",ip_zone5_min,string(""));
	Add("ip_zone5_max",ip_zone5_max,string(""));
	Add("ip_zone6_min",ip_zone6_min,string(""));
	Add("ip_zone6_max",ip_zone6_max,string(""));
	// End IP and zone configuration
	
	// Custom messages
	Add("msg_replace_ban",msg_replace_ban, string(""));
	Add("msg_welcome_guest",msg_welcome[0]);
	Add("msg_welcome_reg",msg_welcome[1]);
	Add("msg_welcome_vip",msg_welcome[2]);
	Add("msg_welcome_op",msg_welcome[3]);
	Add("msg_welcome_cheef",msg_welcome[4]);
	Add("msg_welcome_admin",msg_welcome[5]);
	Add("msg_welcome_master",msg_welcome[10]);
	// End cstom messages
}


/*!
    \fn nDirectConnect::nTables::cDCConf::Load()
 */
int cDCConf::Load()
{
	mS.mSetupList.LoadFileTo(this,mS.mDBConf.config_name.c_str());
	hub_version = VERSION;
	return 0;
}

/*!
    \fn nDirectConnect::nTables::cDCConf::Save()
 */
int cDCConf::Save()
{
	hub_version = VERSION;
	mS.mSetupList.SaveFileTo(this,mS.mDBConf.config_name.c_str());
	return 0;
}

};
};
