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
#ifdef MOD_POSTWAR

#ifndef MOD_EXTRAS
#define POSTWAR_BOARD "TheZone"
#endif
#define NEW_SKIP      004

struct readnewstruct {
  int nummsgs;
  int numread;
  int openflags;
  int dispflags;
  int excode;
  char *thread;
};

extern NewPostReadfn(int, HEADER *, struct readnewstruct *);

postwar_mode()
{
  NAME postwarboard;
  int rc = 0;
#ifdef MOD_EXTRAS
  extern MODSERVER modserver;
  bzero(&postwarboard,sizeof(postwarboard));
  sprintf(postwarboard,"%s",modserver.postwarboard);
#else
  bzero(&postwarboard,sizeof(postwarboard));
  sprintf(postwarboard,POSTWAR_BOARD);
#endif
  if(strlen(currboard) == strlen(postwarboard))
    if(!strncmp(currboard, postwarboard,strlen(currboard)))
      rc = 1;
  bzero(&postwarboard,sizeof(postwarboard));
  return rc;
}

do_postwar()
{
  int rc, in_postwar_read, in_postwar = 1;
  int openflags, newmsgs;
  struct readnewstruct rns;
  char msgbuf[80], ans[4];

  while(in_postwar) {
    bbs_set_mode(M_POSTWAR);
    rc = (GenericPost(0));
    bbs_set_mode(M_POSTWAR);
    in_postwar_read = 1;
    while(in_postwar_read) {
      move(t_lines-1,0);
      clrtoeol();
      getdata(t_lines-1, 0, "Post again, Read new posts, Exit PostWar Mode? [R]: ", 
       ans, sizeof(ans), DOECHO, 0);
        switch (*ans) {
        case 'e': case 'E': 
          in_postwar = in_postwar_read = 0;
          break;
        case 'p': case 'P':
          in_postwar_read = 0;
          break;
        default: {
          clear();
          CloseBoard();
          newmsgs = OpenBoard(&openflags, 1, NULL);
          if (newmsgs > 0) {
            rns.nummsgs = newmsgs;
            rns.numread = 0;
            rns.openflags = openflags;
            rns.dispflags = NEW_SKIP;
            rns.thread = NULL;
            bbs_enum_headers(10, 0, 1, NewPostReadfn, &rns);
          }
          else {
            sprintf(msgbuf, "No New Posts on %s\n", currboard);
            move(t_lines/2, t_columns/2-10);
            prints(msgbuf);
          }
        }
      }
    }
  }
  return rc;
}

PwPost()
{
  int rc;
  if(postwar_mode())
   rc = do_postwar();
  else
    return (GenericPost(1));
  return rc;
}

/*ARGSUSED*/
PwPostMessage(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int rc;
  if (!BITISSET(openflags, OPEN_POST)) {
    bell();
    return DONOTHING;
  }
  if(postwar_mode())
   rc = do_postwar();
  else
    return (GenericPost(0));   /* no need to check post privilege -- we do */
  return rc;
}

#endif
