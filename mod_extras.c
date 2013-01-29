/*
Eagles Bulletin Board System
Copyright (C) 1995, Ray Rocker, rocker@datasync.com

EBBS Modules
Copyright (C) 1999, Paul Snow, psnow@nipha.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "modcommon.h"
#ifdef MOD_EXTRAS

#include <sys/stat.h>

extern int _match_profile_key();
extern int _profile_upd();
extern int _profile_format();

/* 
   Modules System Config. Bunch of system config information stored one
    per line, XXXX=data, where XXXX is one of the following:
*/

#define MODCONFIGFILE	"etc/modconfig"


/*ARGSUSED*/
_init_mod_config_func(indx, rec, arg)
int indx;
char *rec;
void *arg;
{
  char *equals;
  int i;
  strip_trailing_space(rec);

  if ((equals = strchr(rec, '=')) == NULL) return S_OK;
  *equals++ = '\0';
  strip_trailing_space(rec);
  strip_trailing_space(equals);
  while (*rec && isspace(*rec)) rec++;
  while (*equals && isspace(*equals)) equals++;

  if (!strcasecmp(rec, "cloak")) {
    if (toupper(*equals) == 'N') modserver.cloak = 1;
    else if (toupper(*equals) == 'S') modserver.cloak = 2;
    else modserver.cloak = 0;
  }
  else if (!strcasecmp(rec, "usersonline")) {
    if (toupper(*equals) == 'Y') modserver.usol = 1;
    else modserver.usol = 0;
  }
#ifdef MOD_POSTWAR
  else if (!strcasecmp(rec, "postwarboard")) {
    strncpy(modserver.postwarboard, equals, NAMELEN);
  }
#endif
#ifdef MOD_VOTE
  else if (!strcasecmp(rec, "maxpolls")) {
    modserver.maxpolls = atoi(equals);
  }
  else if (!strcasecmp(rec, "maxuserpolls")) {
    modserver.maxuserpolls = atoi(equals);
  }
  else if (!strcasecmp(rec, "postvote")) {
    if (toupper(*equals) == 'Y') modserver.postvote = 1;
    else modserver.postvote = 0;
  }
  else if (!strcasecmp(rec, "voteboard")) {
    strncpy(modserver.voteboard, equals, NAMELEN);
  }
#endif
  return S_OK;
}

init_mod_config()
{
  bzero(&modserver,sizeof(modserver));
  _record_enumerate(MODCONFIGFILE, 0, _init_mod_config_func, NULL);
  return S_OK;
}

void
display_users_online()
{
  int i, ucount = 0;
  time_t now;

  if(!modserver.usol)
    return;
  if(usermodcfg.usol[0] == 'N')
    return;
  if (global_ulist == NULL) {
    if (SetupGlobalList() == -1) return;
  }
  for (i=0; i<global_ulist_sz; i++) {
    global_ulist[i].wasfound = global_ulist[i].found;
    global_ulist[i].found = 0;
  }
  bbs_enum_users(global_ulist_sz, 0, NULL, FillShortUserList, NULL);
  ucount = 0;
  for (i=0; i<global_ulist_sz; i++)
    if (global_ulist[i].found) ucount++;
  move(t_lines-1, 0);
  clrtoeol();
  move(t_lines-1, 4);
  time(&now);
  prints("%d user%s online at %s", ucount, (ucount==1?"":"s"),Ctime(&now));
}

mod_can_cloak()
{
  char sysopaccount[NAMELEN+1];
  char myaccount[NAMELEN+1];

  if(!modserver.cloak)
    return 1;
  if(modserver.cloak == 1) 
    return 0;

  sprintf(sysopaccount,"%s",SYSOP_ACCOUNT);
  sprintf(myaccount,"%s", my_userid());
  if(strlen(sysopaccount) == strlen(myaccount))
    if(!strncmp(sysopaccount,myaccount,strlen(sysopaccount)))
      return 1;
  return 0;
}

ModCloak()
{
  if(mod_can_cloak())
    return ToggleCloak();
  move(4,0);
  prints("Sorry, General Purpose Cloak has been disabled.\n");
  clrtobot();
  return PARTUPDATE;
}

DispServInfo()
{
  move(3,0);
  clrtobot();
  prints("System Configuration\n\n");
  prints("Login as New          : %s\n",(server.newok ? "Yes" : "No"));
  prints("Max Logons per Userid : %d\n",server.maxlogons);
  prints("Max Signature lines   : %d\n",server.maxsiglines);
  prints("Show Real Names       : %s\n",(server.queryreal ? "Yes" : "No"));
  prints("Idle Timeout (seconds): %d\n\n",server.idletimeout);
  prints("Max Online Users      : %d\n",server.maxusers);
  prints("Reserved Slots        : %d\n",server.reservedslots);
  prints("User Table Size       : %d\n\n",server.maxutable);
  prints("Logfile name          : %s\n",server.logfile);
  prints("Logging Level         : %d\n",server.loglevel);
  prints("UUEncoder             : %s\n",server.encodebin);
  prints("Locale                : %s\n",server.locale);

  return PARTUPDATE;
}

#define SYSCFGHELP   "etc/serverinfo.hlp"

local_bbs_get_syscfghelp(buf)
char *buf;
{
  strcpy(buf, SYSCFGHELP);
  return S_OK;
}

SysConfigHelp()
{
  PATH syscfghelp;
  int rc;
  clear();
  if ((rc = local_bbs_get_syscfghelp(syscfghelp)) != S_OK) {
    bbperror(rc, "System config help file is unavailable");
    pressreturn();
  }
  if (More(syscfghelp, 1)) {
    prints("System config help file is unavailable.");
    pressreturn();
  }
  return FULLUPDATE;
}

DispModInfo()
{
  move(3,0);
  clrtobot();
  prints("System Modules Configuration\n\n");
  prints("Cloak                 : %s\n",
(modserver.cloak ? ((modserver.cloak == 1) ? "Yes" : "Sysop Only") : "No"));
  prints("Display Users Online  : %s\n",(modserver.usol ? "Yes" : "No"));
#ifdef MOD_POSTWAR
  prints("Postwar Board         : %s\n\n",modserver.postwarboard);
#endif
#ifdef MOD_VOTE
  prints("Max Polls             : %d\n",modserver.maxpolls);
  prints("Max User Polls        : %d\n",modserver.maxuserpolls);
  prints("Post Vote             : %s\n",(modserver.postvote ? "Yes" :"No"));
  prints("Vote Board            : %s\n\n",modserver.voteboard);
#endif
  return PARTUPDATE;
}

#define MODCFGHELP   "etc/modcfginfo.hlp"

local_bbs_get_modcfghelp(buf)
char *buf;
{
  strcpy(buf, MODCFGHELP);
  return S_OK;
}

ModConfigHelp()
{
  PATH modcfghelp;
  int rc;
  clear();
  if ((rc = local_bbs_get_modcfghelp(modcfghelp)) != S_OK) {
    bbperror(rc, "Module config help file is unavailable");
    pressreturn();
  }
  if (More(modcfghelp, 1)) {
    prints("Module config help file is unavailable.");
    pressreturn();
  }
  return FULLUPDATE;
}

/* 
   ModProfile. Bunch of module configuration stored one per line, 
   XXXX=data, where XXXX is USOL, ANIM, VOTE, MONT, XMSG, WELC
*/

#define MODPROFILE       "modprofile"

#define MODPROF_USOL  "USOL"
#define DEFAULT_USOL  "Y"
#ifdef MOD_ANIM
#define MODPROF_ANIM  "ANIM"
#define DEFAULT_ANIM  "Y"
#endif
#ifdef MOD_VOTE
#define MODPROF_VOTE  "VOTE"
#define DEFAULT_VOTE  "Y"
#endif
#ifdef MOD_MONITOR
#define MODPROF_MONT  "MONT"
#define DEFAULT_MONT  "L"
#endif
#ifdef MOD_XMESSAGE
#define MODPROF_XMSG  "XMSG"
#define DEFAULT_XMSG  "Y"
#endif
#ifdef MOD_WELCOME
#define MODPROF_WELC  "WELC"
#define DEFAULT_WELC  "Y"
#endif


struct modprofile_data {
  char *key;
  char *data;
};

get_modprofile_file(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, MODPROFILE);
}

set_modprofile_data(userid, key, data)
char *userid;
char *key;
char *data;
{
  int rc;
  PATH modprofile;
  struct stat stbuf;

  get_modprofile_file(userid, modprofile);

  if(data[0] == '\0') {
    if(!strcmp(key, MODPROF_USOL))
      strcpy(data,DEFAULT_USOL);
#ifdef MOD_ANIM
    else if(!strcmp(key, MODPROF_ANIM))
      strcpy(data,DEFAULT_ANIM);
#endif
#ifdef MOD_VOTE
    else if(!strcmp(key, MODPROF_VOTE))
      strcpy(data,DEFAULT_VOTE);
#endif
#ifdef MOD_MONITOR
    else if(!strcmp(key, MODPROF_MONT))
      strcpy(data,DEFAULT_MONT);
#endif
#ifdef MOD_XMESSAGE
    else if(!strcmp(key, MODPROF_XMSG))
      strcpy(data,DEFAULT_XMSG);
#endif
#ifdef MOD_WELCOME
    else if(!strcmp(key, MODPROF_WELC))
      strcpy(data,DEFAULT_WELC);
#endif
  }
  if (data[0] == '\0') {
    _record_delete(modprofile, _match_profile_key, key);
    return S_OK;
  }
  rc = _record_replace(modprofile, _match_profile_key, key, _profile_upd, data);
  if (rc != S_OK) {
    struct modprofile_data pd;
    pd.key = key;
    pd.data = data;
    rc = _record_add(modprofile, _match_profile_key, key, _profile_format, &pd);
  }
  return rc;
}

/*ARGSUSED*/
_mod_profile_fill(indx, rec, modacct)
int indx;
char *rec;
MODACCOUNT *modacct;
{
  strip_trailing_space(rec);
  if (rec[4] != '=') return S_OK;
  rec[4] = '\0';
  if (!strcmp(rec, MODPROF_USOL)) 
    strncpy(modacct->usol, rec+5, 1);
#ifdef MOD_ANIM
  else if(!strcmp(rec, MODPROF_ANIM))
    strncpy(modacct->anim, rec+5, 1);
#endif
#ifdef MOD_VOTE
  else if(!strcmp(rec, MODPROF_VOTE))
    strncpy(modacct->vote, rec+5, 1);
#endif
#ifdef MOD_MONITOR
  else if(!strcmp(rec, MODPROF_MONT))
    strncpy(modacct->monitor, rec+5, 1);
#endif
#ifdef MOD_XMESSAGE
  else if(!strcmp(rec, MODPROF_XMSG))
    strncpy(modacct->xmessage, rec+5, 1);
#endif
#ifdef MOD_WELCOME
  else if(!strcmp(rec, MODPROF_WELC))
    strncpy(modacct->welcome, rec+5, 1);
#endif
  return S_OK;
}

enum_modprofile_data(userid, modacct)
char *userid;
MODACCOUNT *modacct;
{
  PATH modprofile;
  get_modprofile_file(userid, modprofile);
  _record_enumerate(modprofile, 0, _mod_profile_fill, modacct);
  if (modacct->usol[0] == '\0')
    strcpy(modacct->usol, DEFAULT_USOL);
#ifdef MOD_ANIM
  if (modacct->anim[0] == '\0')
    strcpy(modacct->anim, DEFAULT_ANIM);
#endif
#ifdef MOD_VOTE
  if (modacct->vote[0] == '\0')
    strcpy(modacct->vote, DEFAULT_VOTE);
#endif
#ifdef MOD_MONITOR
  if (modacct->monitor[0] == '\0')
    strcpy(modacct->monitor, DEFAULT_MONT);
#endif
#ifdef MOD_XMESSAGE
  if (modacct->xmessage[0] == '\0')
    strcpy(modacct->xmessage, DEFAULT_XMSG);
#endif
#ifdef MOD_WELCOME
  if (modacct->welcome[0] == '\0')
    strcpy(modacct->welcome, DEFAULT_WELC);
#endif

  return S_OK;
}

init_user_modcfg()
{
  NAME userid;
  sprintf(userid,"%s",my_userid());
  bzero(&usermodcfg,sizeof(usermodcfg));
  enum_modprofile_data(userid, &usermodcfg);
  write_user_modcfg(userid);
  return 0;
}

write_user_modcfg(userid)
char *userid;
{
  set_modprofile_data(userid,MODPROF_USOL,usermodcfg.usol);
#ifdef MOD_ANIM
  set_modprofile_data(userid,MODPROF_ANIM,usermodcfg.anim);
#endif
#ifdef MOD_VOTE
  set_modprofile_data(userid,MODPROF_VOTE,usermodcfg.vote);
#endif
#ifdef MOD_MONITOR
  set_modprofile_data(userid,MODPROF_MONT,usermodcfg.monitor);
#endif
#ifdef MOD_XMESSAGE
  set_modprofile_data(userid,MODPROF_XMSG,usermodcfg.xmessage);
#endif
#ifdef MOD_WELCOME
  set_modprofile_data(userid,MODPROF_WELC,usermodcfg.welcome);
#endif
}

UserModInfo()
{
  NAME userid;
  int y,x,changesettings;
  char ans[25];
  char promptstr[50];

  changesettings = 0;
  sprintf(userid,"%s",my_userid());
  clear();
  prints("Change User Configuration for Modules\n\n");
  do_user_mod_info();
  getyx(&y, &x);
  y++;
  sprintf(promptstr,"Change settings (Y/N) [N]: ");
  getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
  if(ans[0] == 'y' || ans[0] == 'Y') {
    sprintf(promptstr,"Display Users Online        : (Y/N) [%s]: ",
     usermodcfg.usol);
    y++;
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    if(ans[0] != '\0') {
      if(ans[0] == 'y' || ans[0] == 'Y') {
        usermodcfg.usol[0] = 'Y';
        changesettings = 1;
      }
      else if(ans[0] == 'n' || ans[0] == 'N') {
        usermodcfg.usol[0] = 'N';
        changesettings = 1;
      }
    }
#ifdef MOD_ANIM
    sprintf(promptstr,"Show Animations             : (Y/N) [%s]: ",
     usermodcfg.anim);
    y++;
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    if(ans[0] != '\0') {
      if(ans[0] == 'y' || ans[0] == 'Y') {
        usermodcfg.anim[0] = 'Y';
        changesettings = 1;
      }
      else if(ans[0] == 'n' || ans[0] == 'N') {
        usermodcfg.anim[0] = 'N';
        changesettings = 1;
      }
    }
#endif
#ifdef MOD_VOTE
    sprintf(promptstr,"New Vote Notification       : (Y/N) [%s]: ",
     usermodcfg.vote);
    y++;
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    if(ans[0] != '\0') {
      if(ans[0] == 'y' || ans[0] == 'Y') {
        usermodcfg.vote[0] = 'Y';
        changesettings = 1;
      }
      else if(ans[0] == 'n' || ans[0] == 'N') {
        usermodcfg.vote[0] = 'N';
        changesettings = 1;
      }
    }
#endif
#ifdef MOD_MONITOR
    sprintf(promptstr,"Default Monitor Mode        : (L/S/O) [%s]: ",
     usermodcfg.monitor);
    y++;
    move(y+1,0);
    prints("       (L)ong, (S)hort, (O)ld Style\n");
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    y++;
    if(ans[0] != '\0') {
      if(ans[0] == 'l' || ans[0] == 'L') {
        usermodcfg.monitor[0] = 'L';
        changesettings = 1;
      }
      else if(ans[0] == 's' || ans[0] == 'S') {
        usermodcfg.monitor[0] = 'S';
        changesettings = 1;
      }
      else if(ans[0] == 'o' || ans[0] == 'O') {
        usermodcfg.monitor[0] = 'O';
        changesettings = 1;
      }
    }
#endif
#ifdef MOD_XMESSAGE
    sprintf("Receive X-Messages          : (Y/N) [%s]: ",
     usermodcfg.xmessage);
    y++;
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    if(ans[0] != '\0') {
      if(ans[0] == 'y' || ans[0] == 'Y') {
        usermodcfg.xmessage[0] = 'Y';
        changesettings = 1;
      }
      else if(ans[0] == 'n' || ans[0] == 'N') {
        usermodcfg.xmessage[0] = 'N';
        changesettings = 1;
      }
    }
#endif
#ifdef MOD_WELCOME
    sprintf(promptstr,"Show Board Welcome Screens  : (Y/N) [%s]: ",
     usermodcfg.welcome);
    y++;
    getdata(y,0,promptstr, ans, sizeof(ans), DOECHO, 0);
    if(ans[0] != '\0') {
      if(ans[0] == 'y' || ans[0] == 'Y') {
        usermodcfg.welcome[0] = 'Y';
        changesettings = 1;
      }
      else if(ans[0] == 'n' || ans[0] == 'N') {
        usermodcfg.welcome[0] = 'N';
        changesettings = 1;
      }
    }
#endif
    move(t_lines - 2,0);
    if(changesettings) {
      prints("Settings Updated!\n");
      write_user_modcfg(userid);
    }
    else
      prints("Settings not updated.\n");
  }
  return PARTUPDATE;
}

do_user_mod_info()
{
  prints("Display Users Online        : %s\n",
   ((usermodcfg.usol[0] == 'Y') ? "Yes" : "No"));
#ifdef MOD_ANIM
  prints("Show Animations             : %s\n",
   ((usermodcfg.anim[0] == 'Y') ? "Yes" : "No"));
#endif
#ifdef MOD_VOTE
  prints("New Vote Notification       : %s\n",
   ((usermodcfg.vote[0] == 'Y') ? "Yes" : "No"));
#endif
#ifdef MOD_MONITOR
  prints("Default Monitor Mode        : %s\n",
   ((usermodcfg.monitor[0] == 'S') ? "Short" : 
    ((usermodcfg.monitor[0] == 'L') ? "Long" : "Old Style")));
#endif
#ifdef MOD_XMESSAGE
  prints("Receive X-Messages          : %s\n",
   ((usermodcfg.xmessage[0] == 'Y') ? "Yes" : "No"));
#endif
#ifdef MOD_WELCOME
  prints("Show Board Welcome Screens  : %s\n",
   ((usermodcfg.welcome[0] == 'Y') ? "Yes" : "No"));
#endif
  return PARTUPDATE;
}

FuncTest()
{
  pressreturn();
}

#endif
