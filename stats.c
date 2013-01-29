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
#ifdef MOD_STATS

#include <math.h>

extern char *ctime();

extern char *my_userid();

extern int input_active;

typedef struct statrec {
    NAME   userid;
    int    statvars[NUMSTATS];
    time_t lastlogin;
} ACCTSTAT;

ACCTSTAT stat_array[MAXSTATSUSERS];

PATH statfile;

int chat_idle;

#define STAT_ROUND_MIN 1


init_stats() {
  int i;
  char initbuf[80];
  for (i=0;i<NUMSTATS;i++) statvars[i] = 0;
  do_stats(STAT_LOGIN);
  return 1;
}

do_stats(statnum)
int statnum;
{
  get_user_stats(my_userid());
  statvars[statnum]++;
  write_user_stats(my_userid());
  return 1;
}

local_bbs_get_statfile(userid,buf)
char *userid;
char *buf;
{
  get_home_directory(userid,buf);
  strcat(buf, "/");
  strcat(buf, STATFILE);
}

write_user_stats(userid)
char *userid;
{
  PATH statfile;
  char suserid[NAMELEN+1];
  int i;
  bzero(&statfile,sizeof(statfile));
  sprintf(suserid,"%s",userid);
  local_bbs_get_statfile(suserid,statfile);
  write_stat_file(statfile);
  bzero(&statfile,sizeof(statfile));
}

get_user_stats(userid)
char *userid;
{
  PATH statfile;
  char suserid[NAMELEN+1];
  int i;
  bzero(&statfile,sizeof(statfile));
  for(i=0;i<NUMSTATS;i++) statvars[i] = 0;
  sprintf(suserid,"%s",userid);
  local_bbs_get_statfile(suserid,statfile);
  read_stat_file(statfile);
  bzero(&statfile,sizeof(statfile));
}

read_stat_file(statfile)
char *statfile;
{
  FILE *fp;
  PATH statfilepath;
  char inbuf[80];
  int i;
  bzero(&statfilepath,sizeof(statfilepath));
  bzero(&inbuf,sizeof(inbuf));
  i = 0;
  sprintf(statfilepath,"%s",statfile);
  if ((fp = fopen(statfilepath, "r")) != NULL) {
    while(fgets(inbuf,sizeof(inbuf),fp) != NULL) {
      statvars[i] = atol(inbuf);
      i++;
      if(i >= NUMSTATS) break;
    }
    fclose(fp);
  } 
  bzero(&statfilepath,sizeof(statfilepath));  
  bzero(&inbuf,sizeof(inbuf));
}

write_stat_file(statfile)
char *statfile;
{
  FILE *fp;
  PATH statfilepath;
  char inbuf[10];
  int i;
  bzero(&statfilepath,sizeof(statfilepath));
  bzero(&inbuf,sizeof(inbuf));
  i = 0;
  sprintf(statfilepath,"%s",statfile);
  if ((fp = fopen(statfilepath, "w")) != NULL) {
    for(i=0;i<NUMSTATS;i++) {
      sprintf(inbuf,"%d\n",statvars[i]);
      fputs(inbuf,fp);
    }
    fclose(fp);
  }
  bzero(&statfilepath,sizeof(statfilepath));  
  bzero(&inbuf,sizeof(inbuf));
}

lastlogin_sort(statent1,statent2)
struct statrec *statent1;
struct statrec *statent2;
{
	if(statent1->lastlogin > statent2->lastlogin)
	  return 0;
	return 1;
}

stat_sort(statent1,statent2)
struct statrec *statent1;
struct statrec *statent2;
{
	if(statent1->statvars[currstatnum] > statent2->statvars[currstatnum])
	  return 0;
	return 1;
}

_enum_stats(indx, rec, en)
int indx;
char *rec;
struct enumstruct *en;
{
  ACCTSTAT statent;
  memset(&statent, 0, sizeof statent);
  passent_to_stats(rec, &statent);
  get_user_stats(statent.userid);
  memcpy(&statent.statvars,&statvars,sizeof(statvars));
  get_lastlog_time(statent.userid, &statent.lastlogin);
  if (en->fn(indx, &statent, en->arg) == ENUM_QUIT) return ENUM_QUIT;
  return S_OK;
}

/*ARGSUSED*/
local_bbs_enum_stats(chunk, startrec, enumfn, arg)
SHORT chunk;
SHORT startrec;
int (*enumfn)();
void *arg;
{
  struct enumstruct en;
  en.fn = enumfn;
  en.arg = arg;
  _record_enumerate(PASSFILE, startrec, _enum_stats, &en);
  return S_OK;
}

passent_to_stats(rec, statent)
char *rec;
ACCTSTAT *statent;
{
  statent->userid[NAMELEN] = '\0';
  strncpy(statent->userid, rec+PF_USERID_OFFSET, NAMELEN);
  strip_trailing_space(statent->userid);
  return S_OK;
}

/*ARGSUSED*/
CommonStatFunc(indx, statent, info)
int indx;
ACCTSTAT *statent;
struct enum_info *info;
{
  int minstat;
  if (info->count < MAXSTATSUSERS) {
    memcpy(&stat_array[info->count],statent,sizeof(struct statrec));
  }
  else {
    if(info->count == MAXSTATSUSERS)
      qsort(stat_array,MAXSTATSUSERS,sizeof(struct statrec),stat_sort);
    minstat = stat_array[MAXSTATSUSERS-1].statvars[currstatnum];
    if(statent->statvars[currstatnum] > minstat) {
      memcpy(&stat_array[MAXSTATSUSERS-1],statent,sizeof(struct statrec));
      qsort(stat_array,MAXSTATSUSERS,sizeof(struct statrec),stat_sort);
    }
  }
  info->count++;
  return S_OK;
}

CommonStat()
{
  int i, statretr, x, y;
  struct enum_info info;
  info.count = 0;
  clear();
  move(12,32);
  prints("Calculating...");
  refresh();
  
  local_bbs_enum_stats(0, 0, CommonStatFunc, &info);
  clear();
  switch(currstatnum) {
    case STAT_LOGIN: {
      prints("\n%-14s %-7s %-7s %-13s    %s\n\n",
       "User Id","Logins","Posts","Postwar Posts","Last Login");
      break;
    }
    case STAT_POST: {
      prints("\n%-14s %-7s %-13s %-7s    %s\n\n",
       "User Id","Posts","Postwar Posts","Logins","Last Login");
      break;
    }
    case STAT_POSTWAR: {
      prints("\n%-14s %-13s %-7s %-7s    %s\n\n",
       "User Id","Postwar Posts","Posts","Logins","Last Login");
      break;
    }
    case STAT_CHAT: {
      prints("\n%-14s %-12s %-7s  %s\n\n",
       "User Id","Chat Entries","Logins","Last Login");
      break;
    }
  }
  statretr = (info.count < MAXSTATSUSERS) ? info.count-1 : MAXSTATSUSERS-1;
  for(i=0;i<statretr;i++) {
    if(stat_array[i].statvars[currstatnum] == 0) break;
    switch(currstatnum) {
      case STAT_LOGIN: {
        prints("%-14s  %-7d %-7d %-13d    %s",
         stat_array[i].userid, 
         stat_array[i].statvars[STAT_LOGIN],
         stat_array[i].statvars[STAT_POST], 
         stat_array[i].statvars[STAT_POSTWAR],
         (stat_array[i].lastlogin == 0) ? "\n":ctime(&stat_array[i].lastlogin));
        break;
      }
      case STAT_POST: {
        prints("%-14s  %-7d %-13d %-7d    %s",
         stat_array[i].userid, 
         stat_array[i].statvars[STAT_POST], 
         stat_array[i].statvars[STAT_POSTWAR],
         stat_array[i].statvars[STAT_LOGIN],
         (stat_array[i].lastlogin == 0) ? "\n":ctime(&stat_array[i].lastlogin));
        break;
      }
      case STAT_POSTWAR: {
        prints("%-14s  %-13d %-7d %-7d    %s",
         stat_array[i].userid, 
         stat_array[i].statvars[STAT_POSTWAR],
         stat_array[i].statvars[STAT_POST], 
         stat_array[i].statvars[STAT_LOGIN],
         (stat_array[i].lastlogin == 0) ? "\n":ctime(&stat_array[i].lastlogin));
        break;
      }
      case STAT_CHAT: {
        prints("%-14s  %-12d %-7d  %s",
         stat_array[i].userid, 
         stat_array[i].statvars[STAT_CHAT],
         stat_array[i].statvars[STAT_LOGIN],
         (stat_array[i].lastlogin == 0) ? "\n":ctime(&stat_array[i].lastlogin));
        break;
      }
    }
  }
  clrtobot();
  return PARTUPDATE;
}

LastLoginStatFunc(indx, statent, info)
int indx;
ACCTSTAT *statent;
struct enum_info *info;
{
  int minstat;
  if (info->count < MAXSTATSUSERS) {
    memcpy(&stat_array[info->count],statent,sizeof(struct statrec));
  }
  else {
    if(info->count == MAXSTATSUSERS)
      qsort(stat_array,MAXSTATSUSERS,sizeof(struct statrec),lastlogin_sort);
    minstat = stat_array[MAXSTATSUSERS-1].lastlogin;
    if(statent->lastlogin > minstat) {
      memcpy(&stat_array[MAXSTATSUSERS-1],statent,sizeof(struct statrec));
      qsort(stat_array,MAXSTATSUSERS,sizeof(struct statrec),lastlogin_sort);
    }
  }
  info->count++;
  return S_OK;
}

LastLoginStat()
{
  int i, statretr;
  struct enum_info info;
  info.count = 0;
  clear();
  move(12,32);
  prints("Calculating...");
  refresh();
  
  local_bbs_enum_stats(0, 0, LastLoginStatFunc, &info);
  clear();
  prints("\n%-14s %-7s %-7s %-13s    %s\n\n",
   "User Id","Logins","Posts","Postwar Posts","Last Login");
  statretr = (info.count < MAXSTATSUSERS) ? info.count-1 : MAXSTATSUSERS-1;
  for(i=0;i<statretr;i++) {
    if(stat_array[i].lastlogin == 0) break;
    prints("%-14s  %-7d %-7d %-13d    %s",
     stat_array[i].userid, stat_array[i].statvars[0],
     stat_array[i].statvars[1], stat_array[i].statvars[2],
     (stat_array[i].lastlogin == 0) ? "\n":ctime(&stat_array[i].lastlogin));
  }
  clrtobot();
  return PARTUPDATE;
}

LoginStat()
{
  currstatnum=STAT_LOGIN;
  CommonStat();
  return PARTUPDATE;
}

PostStat()
{
  currstatnum=STAT_POST;
  CommonStat();
  return PARTUPDATE;
}

PostwarStat()
{
  currstatnum=STAT_POST;
  CommonStat();
  return PARTUPDATE;
}

ChatStat()
{
  currstatnum=STAT_CHAT;
  CommonStat();
  return PARTUPDATE;
}

SetUserStats()
{
  NAME userid;
  char genbuf[256], ans[10];
  int x, y;

  bbs_acctnames(&acctlist, NULL);
  move(2,0);
  clrtobot();
  namecomplete(NULL, acctlist, "Userid: ", userid, sizeof(NAME)); 
  if (userid[0] == '\0' || !is_in_namelist(acctlist, userid)) {
    bbperror(S_NOSUCHUSER, NULL);
    return PARTUPDATE;
  }
  get_user_stats(userid);
  statdisplay(userid);

  getyx(&y, &x);
  getdata(y++,0,"Change user stats (Y/N)? [N]: ",ans,sizeof(ans),
          DOECHO, 0);
  if (ans[0] != 'Y' && ans[0] != 'y') {
    prints("User stats not changed.\n");
    pressreturn();
    return FULLUPDATE;
  }
  
  move(y,0);
  clrtobot();
  sprintf(genbuf, "New login count [%d]: ", statvars[STAT_LOGIN]);
  getdata(y, 0, genbuf, ans, sizeof(ans), DOECHO, 0);
  if (atol(ans) != 0 || ans[0] == '0')
    statvars[STAT_LOGIN] = atol(ans);

  move(y,0);
  clrtobot();
  sprintf(genbuf, "New post count [%d]: ", statvars[STAT_POST]);
  getdata(y, 0, genbuf, ans, sizeof(ans), DOECHO, 0);
  if (atol(ans) != 0 || ans[0] == '0')
    statvars[STAT_POST] = atol(ans);

  move(y,0);
  clrtobot();
  sprintf(genbuf, "New postwar count [%d]: ", statvars[STAT_POSTWAR]);
  getdata(y, 0, genbuf, ans, sizeof(ans), DOECHO, 0);
  if (atol(ans) != 0 || ans[0] == '0')
    statvars[STAT_POSTWAR] = atol(ans);

  move(y,0);
  clrtobot();
  sprintf(genbuf, "New times in chat count [%d]: ", statvars[STAT_CHAT]);
  getdata(y, 0, genbuf, ans, sizeof(ans), DOECHO, 0);
  if (atol(ans) != 0 || ans[0] == '0')
    statvars[STAT_CHAT] = atol(ans);

  move(3,0);
  clrtobot();
  statdisplay(userid);
  clrtobot();
  getyx(&y, &x);

  getdata(++y, 0, "Are you sure (Y/N)? [N]: ", ans, sizeof(ans),
   DOECHO, 0);
  if (ans[0] == 'Y' || ans[0] == 'y') {    
    write_user_stats(userid);
    prints("User stats were changed.\n");
  }
  else prints("User stats not changed.\n");
  
  pressreturn();
  return FULLUPDATE;
}


UserStatDisplay()
{
  NAME userid;
  bbs_acctnames(&acctlist, NULL);
  move(2,0);
  clrtobot();
  namecomplete(NULL, acctlist, "Userid: ", userid, sizeof(NAME)); 
  if (userid[0] == '\0' || !is_in_namelist(acctlist, userid)) {
    bbperror(S_NOSUCHUSER, NULL);
    return PARTUPDATE;
  }
  get_user_stats(userid);
  move(3,0);
  statdisplay(userid);
  return PARTUPDATE;
}


statdisplay(userid)
char *userid;
{
  time_t lastlogin;
  prints("\n%s\n", userid);
  get_lastlog_time(userid, &lastlogin);
  if (lastlogin)
    prints("Last login time : %s", ctime((time_t *)&lastlogin));
  else 
    prints("Never logged in.\n");
  prints("Logins          : %d\n", statvars[STAT_LOGIN]);
  prints("Posts           : %d\n", statvars[STAT_POST]);
  prints("Postwar Posts   : %d\n", statvars[STAT_POSTWAR]);
  prints("Chat Entries    : %d\n", statvars[STAT_CHAT]);
  clrtobot();
}        

StatChat()
{
    do_stats(STAT_CHAT);
    return Chat();
}

#endif
