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
#ifdef MODULES

#define MODMAIN 1

#include "modcommon.h"

USERDATA module_mydata;
NAMELIST acctlist;
#ifdef MOD_EXTRAS
MODACCOUNT usermodcfg;
MODSERVER modserver;
#endif
/* some useful common functions */
int
ci_strncmp(s1,s2,n)
register char *s1,*s2 ;
register int n ;
{
    for(;n;s1++,s2++,n--) {
        if(*s1=='\0' && *s2 == '\0')
          break ;
        if((isalpha(*s1)?*s1|0x20:*s1) != (isalpha(*s2)?*s2|0x20:*s2))
          return 1 ;
    }
    return 0 ;
}


mod_get_mydata()
{
  bzero(&module_mydata,sizeof(module_mydata));
  utable_get_record(my_utable_slot(), &module_mydata);
}

module_uncloak()
{
  mod_get_mydata();
  if (module_mydata.u.flags & FLG_CLOAK) {
    bbs_toggle_cloak();
    return 1;
  }
  return 0;
}

init_modules() 
{
#ifdef MOD_EXTRAS
  init_mod_config();
#endif
  return 1;
}

init_modules_after_login() 
{
  struct stat statb;

  unlink(c_tempfile);
#ifdef MOD_EXTRAS
  if(!mod_can_cloak())
    module_uncloak();
  init_user_modcfg();
#endif
#ifdef MOD_STATS
  init_stats();
#endif
#ifdef MOD_VOTE
  disp_newvotes();
#endif

  if(!stat(c_tempfile,&statb))
   More(c_tempfile);
  unlink(c_tempfile);
  return 1;
}

#endif

