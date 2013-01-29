/*
Eagles Bulletin Board System
Copyright (C) 1995, Ray Rocker, rocker@datasync.com

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

#ifndef NAMELEN
#define NAMELEN            12
#endif
#ifndef PATHLEN
#define PATHLEN            255
#endif
#ifndef HOME
#define HOME               "home"
#endif
#ifndef PASSFILE
#define PASSFILE           "etc/passwds"
#endif
#ifndef PF_USERID_OFFSET
#define PF_USERID_OFFSET   0
#endif

#define STATFILE      "stats"
#define MAXSTATSUSERS 20
#define NUMSTATS      4

long statvars[NUMSTATS];

#define STAT_LOGIN        0
#define STAT_POST         1
#define STAT_POSTWAR      2
#define STAT_CHAT         3
