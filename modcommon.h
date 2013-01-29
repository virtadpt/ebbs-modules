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
#include "modules.h"
#include "../client.h"
#include "mod_server.h"
#include <sys/stat.h>
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <signal.h>
#include <time.h>

struct shorturec {
  NAME userid;
  LONG pid;
  SHORT flags;
  SHORT mode;
  short wasfound;
  short found;
  short active;
} *global_ulist;

extern struct shorturec *global_ulist;
extern int global_ulist_sz;
extern int FillShortUserList();
extern char *Ctime __P((time_t *));
extern SERVERDATA server;
extern char c_tempfile[];

#ifndef MODMAIN
extern USERDATA module_mydata;
extern NAMELIST acctlist;
#endif

#ifdef MOD_EXTRAS

typedef struct _MODACCOUNT {
  char usol[2];
#ifdef MOD_ANIM
  char anim[2];
#endif
#ifdef MOD_VOTE
  char vote[2];
#endif
#ifdef MOD_MONITOR
  char monitor[2];
#endif
#ifdef MOD_XMESSAGE
  char xmessage[2];
#endif
#ifdef MOD_WELCOME
  char welcome[2];
#endif
} MODACCOUNT;

#ifndef MODMAIN
extern MODACCOUNT usermodcfg;
#endif

typedef struct _MODSERVER {
    int  cloak;
    int  usol;
#ifdef MOD_POSTWAR
    NAME postwarboard;
#endif
#ifdef MOD_VOTE
    int  maxpolls;
    int  maxuserpolls;
    int  postvote;
    NAME voteboard;
#endif
} MODSERVER;

#ifndef MODMAIN
extern MODSERVER modserver;
#endif
#endif

#ifdef MOD_STATS
# include "stats.h"
  int currstatnum;
  time_t lasttime;
  void chat_alarm();
#endif

