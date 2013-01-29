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
#ifdef MOD_QUERY

extern int _query_if_logged_in __P((int, USEREC *, int *));
AdvQuery()
{
  NAME namebuf;
  ACCOUNT acct;
  PATH planfile;
  char buf[80];
  FILE *fp;
  int i, rc, in_now = 0, firstsig = 6;
  bbs_acctnames(&acctlist, NULL);
  move(3,0);
  clrtobot();
  prints("<Enter Userid>\n");    
  move(2,0);
  namecomplete(NULL, acctlist, "Query who: ", namebuf, sizeof(NAME));
  move(2,0);
  clrtoeol();
  move(3,0);
  if (namebuf[0] == '\0' || !is_in_namelist(acctlist, namebuf)) {
    if (namebuf[0]) {
      bbperror(S_NOSUCHUSER, NULL);
    }
    return PARTUPDATE;
  }

  if ((rc = bbs_query(namebuf, &acct)) != S_OK) {
    bbperror(rc, NULL);
    return PARTUPDATE;
  }

  bbs_enum_users(20, 0, acct.userid, _query_if_logged_in, &in_now);
#ifdef MOD_STATS
  get_user_stats(acct.userid);
#endif
  clear();
#ifdef MOD_STATS
  prints("%s (%s): %d logins, %d posts, %d postwar\n", 
   acct.userid,acct.username,statvars[0],statvars[1],statvars[2]);
#else
  prints("%s (%s):\n",acct.userid,acct.username);
#endif
  if (acct.lastlogin == 0)
    prints("Never logged in.\n");
  else prints("%s from %s %s %s", in_now ? "On" : "Last login", acct.fromhost,
              in_now ? "since" : "at", ctime((time_t *)&acct.lastlogin));

  if (acct.realname[0] != '\0') {
    prints("Real name: %s\n", acct.realname);
    firstsig++;
  }

  if (bbs_get_plan(acct.userid, planfile) != S_OK) {
    prints("No plan.\n");
    pressreturn();
  }
  else {
    /* For now, just print one screen of the plan. In the future maybe
       prompt to ask if they want to page thru the whole plan, since
       we have it. */
    if (fp = fopen(planfile, "r")) {
#ifdef MOD_ANIM
      animate_plan(fp);
#else
      prints("Plan:\n");
      for (i=firstsig; i<t_lines; i++) {
        if (!fgets(buf, sizeof buf, fp)) break;
        prints("%s", buf);
      }
      pressreturn();
#endif
      fclose(fp);
    }
    else {
      prints("No plan.\n");
      pressreturn();
    }
  }
  return FULLUPDATE;
}


#endif
