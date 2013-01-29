/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    Eagles Bulletin Board System (3.x)
    Copyright (C) 1995, Ray Rocker, rocker@datasync.com

    EBBS Modules
    Copyright (C) 1999, Paul Snow, psnow@nipha.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "modcommon.h"
#ifdef MOD_VOTE

#include <sys/file.h>
#include <sys/stat.h>
#include <dirent.h>

struct _sendmsgstruct {
  int btype;
  HEADER hdr;
  RNAME username;
  NAMELIST to_list;
  PATH destfile;
  char *srcfile;
  LONG errcode;
};

#ifndef AIX
extern char *index();
#endif

NAME poll_author;

#define VOTEDIR       "vote"
#define VOTEPREFIX    "control"
#define VOTECONTROL   "vote/control"
#define VOTECREATE    "vote/create"
#define VOTEDESC      "vote/desc"
#define VOTECOMMENTS  "vote/comments"
#define VOTEFILE      "vote"
#define VOTEBALLOT    "vote/ballots"
#define VOTERESULTS   "vote/results"
#define VOTEINSTR     "etc/voteinstr.txt"
#define VOTECOUNT     "vote/count"
#define VOTERANK      "vote/rank"


#ifndef MOD_EXTRAS
#define MAXPOLLS 9
#define MAXUSERPOLLS 1
#define POSTVOTE 1
#define VOTEBOARD "Polls"
int activepolls[MAXPOLLS+1];
#else
/* if maxpolls get set above 25, you must change this */
#define MAXPOLLS 25
int activepolls[MAXPOLLS+1];
#endif

typedef struct _VCNTRL {
  time_t opentime;
  time_t closetime;
  NAME author;
  TITLE title;
  int allowcom;
  int votecount;
  char choices[9][TITLELEN];
} VCNTRL;

VCNTRL vcntrl;

#define VOTEFILERR   -2
#define INVALIDVOTE  -1
#define NOVOTEOPEN    0
#define VALIDVOTE     1
#define USERVOTED     2
#define CLOSEVOTE     3
#define NOAUTHOR      4
#define NOTITLE       5
#define NOCHOICES     6
#define NOCHOICES2    7

valid_vote(ballotnum)
{
/*
 * Return codes:
 * 
 *  VALIDVOTE   - everything is ok, user has not voted.
 *  USERVOTED   - everything is ok, but user has voted.
 *  NOVOTEOPEN  - vote was closed.
 *  INVALIDVOTE - something wrong, votefiles deleted.
 * 
 *  If VALIDVOTE or USERVOTED vcntrl array is filled.
 * 
 */
  struct stat status;
  PATH votefile;
  int retc;
  sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
  if(stat(votefile,&status))
    return remove_vote_files(ballotnum);
  sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
  if(stat(votefile,&status))
    return remove_vote_files(ballotnum);
  if((retc = read_vote_control(ballotnum)) != VALIDVOTE)
    return retc;
  if(valid_vote_control() != VALIDVOTE) {
    if(retc == CLOSEVOTE) {
      doclosepolls(ballotnum,0);
      return NOVOTEOPEN;
    }
    else
      return remove_vote_files(ballotnum);
  }
  return VALIDVOTE;
}

valid_vote_control()
{
/*
 * Return codes:
 * 
 *  VALIDVOTE   - everything is ok, user has not voted.
 *  INVALIDVOTE - something is very wrong, abort.
 *  CLOSEVOTE   - vote needs to be closed.
 *  NOAUTHOR    - Author field blank.
 *  NOTITLE     - Title field blank.
 *  NOCHOICES   - Choice #1 blank.
 *  NOCHOICES2  - Choice #2 blank.
 * 
 */

  if(vcntrl.opentime && vcntrl.closetime) {
    if((vcntrl.opentime >= vcntrl.closetime))
      return INVALIDVOTE;
    if(vcntrl.closetime < time(0))
      return CLOSEVOTE;
    if(vcntrl.author[0] == '\0')
      return NOAUTHOR;
    if(vcntrl.title[0] == '\0')
      return NOTITLE;
    if(vcntrl.choices[0][0] == '\0')
      return NOCHOICES;
    if(vcntrl.choices[1][0] == '\0')
      return NOCHOICES2;
    return VALIDVOTE;
  }
  else
    return INVALIDVOTE;
}

read_vote_control(ballotnum)
{
  FILE *cfp;
  PATH votefile;
  NAME userid;
  char inbuf[80];
  int count;
  time_t votetime;
  char *tmpptr;

  bzero(&vcntrl, sizeof(vcntrl));
  sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
  if ((cfp = fopen(votefile, "r")) == NULL) {
    return NOVOTEOPEN;
  }
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  vcntrl.opentime = (time_t)atol(inbuf);
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  vcntrl.closetime = (time_t)atol(inbuf);
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
  sprintf(vcntrl.author,"%s",inbuf);
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
  sprintf(vcntrl.title,"%s",inbuf);
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  vcntrl.allowcom = atoi(inbuf);
  if(fgets(inbuf, sizeof(inbuf), cfp) == NULL) {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  vcntrl.votecount = atoi(inbuf);
  count = 0;
  if (fgets(inbuf, sizeof(inbuf), cfp) != NULL) {
    sprintf(vcntrl.choices[count],"%s",inbuf);
    count++;
  }
  else  {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  if (fgets(inbuf, sizeof(inbuf), cfp) != NULL) {
    sprintf(vcntrl.choices[count],"%s",inbuf);
    count++;
  }
  else  {
    fclose(cfp);
    return remove_vote_files(ballotnum);
  }
  while (fgets(inbuf, sizeof(inbuf), cfp) != NULL) {
    sprintf(vcntrl.choices[count],"%s",inbuf);
    count++;
    if(count >= 9) break;
  }
  fclose(cfp);
  sprintf(userid,"%s",my_userid());
  get_vote_time(userid, &votetime, ballotnum);
  if(votetime > vcntrl.opentime) {
    return USERVOTED;
  }
  return VALIDVOTE;
}

remove_vote_files(ballotnum)
int ballotnum;
{
  PATH votefile;

  sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTECONTROL,ballotnum);
  unlink(votefile);
  sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTEDESC,ballotnum);
  unlink(votefile);
  sprintf(votefile,"%s.%d",VOTECOMMENTS,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTECOMMENTS,ballotnum);
  unlink(votefile);
  sprintf(votefile,"%s.%d",VOTEBALLOT,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTEBALLOT,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTERESULTS,ballotnum);
  unlink(votefile);

  sprintf(votefile,"temp%s.%d",VOTECOUNT,ballotnum);
  unlink(votefile);
  sprintf(votefile,"temp%s.%d",VOTERANK,ballotnum);
  unlink(votefile);

  return NOVOTEOPEN;
}

write_vote_control(ballotnum)
{
/*
 * Return codes:
 * 
 *  VALIDVOTE   - vote control file written successfully.
 *  VOTEFILERRR - vote control file not written.
 *  valid_vote_control() errors.
 * 
 */

  FILE *fp;
  PATH votefile;
  char outbuf[80];
  int count, retc;

  if(((retc = valid_vote_control()) == VALIDVOTE)) {
    sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
    if((fp = fopen(votefile, "w")) != NULL) {
      sprintf(outbuf, "%lu\n", vcntrl.opentime);
      fputs(outbuf, fp);
      sprintf(outbuf, "%lu\n", vcntrl.closetime);
      fputs(outbuf, fp);
      sprintf(outbuf, "%s\n", vcntrl.author);
      fputs(outbuf, fp);
      sprintf(outbuf, "%s\n", vcntrl.title);
      fputs(outbuf, fp);
      sprintf(outbuf, "%d\n", vcntrl.allowcom);
      fputs(outbuf, fp);
      sprintf(outbuf, "%d\n", vcntrl.votecount);
      fputs(outbuf, fp);
      count = 0;
      while(vcntrl.choices[count][0] != '\0') {
        sprintf(outbuf, "%s", vcntrl.choices[count]);
        fputs(outbuf, fp);
        count++;
        if(count >= 9)
          break;
      }
      fclose(fp);
      return VALIDVOTE;
    }
    return VOTEFILERR;
  }
  return retc;
}

get_vote_files()
{
  DIR *votedir;
  PATH votedirname, votefilename;
  char ctlprefix[80];
  struct dirent *ent;
  struct stat status;
  char *tmpptr;
  char tmpbuf[20];
  int t, retc;

  sprintf(votedirname,"temp%s",VOTEDIR);
  if((votedir = opendir(votedirname)) == NULL) {
    if(!stat(votedirname,&status)) {
      sprintf(votefilename,"temp%s.file",VOTEDIR);
      rename(votedirname,votefilename);
    }
    mkdir(votedirname, 0700);
  }
  sprintf(votedirname,"%s",VOTEDIR);
  sprintf(ctlprefix,"%s",VOTEPREFIX);
  bzero(&activepolls, sizeof(activepolls));
  if((votedir = opendir(votedirname)) == NULL) {
    if(!stat(votedirname,&status)) {
      sprintf(votefilename,"%s.file",VOTEDIR);
      rename(votedirname,votefilename);
    }
    mkdir(votedirname, 0700);
  }
  else {
    for(ent=readdir(votedir);ent!=NULL;ent=readdir(votedir)) {
      if(!strcmp(ent->d_name,"."))
        continue ;
      if(!strcmp(ent->d_name,".."))
        continue ;
      if(strncmp(ent->d_name,ctlprefix,strlen(ctlprefix)))
        continue ;
      sprintf(votefilename,"%s/%s",votedirname,ent->d_name) ;
      if(stat(votefilename,&status))
        continue ;
      if((status.st_mode & S_IFDIR))
        continue ;
      sprintf(tmpbuf,"%s",ent->d_name);
      if(tmpptr = rindex(tmpbuf,'.')) {
        tmpptr++;
        if(t = atoi(tmpptr)) {
          if(retc = valid_vote(t)) {
            activepolls[t] = retc;
            activepolls[0]++;
          }
        }
      }
    }
    closedir(votedir) ;
  }
  return 0;
}

char *
pollauthor(ballotnum)
{
  PATH votefile;
  FILE *cfp;
  char inbuf[80];
  char *tmpptr;

  bzero(&poll_author,sizeof(poll_author));
  sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
  if ((cfp = fopen(votefile, "r")) != NULL) {
    fgets(inbuf, sizeof(inbuf), cfp);
    fgets(inbuf, sizeof(inbuf), cfp);
    fgets(inbuf, sizeof(inbuf), cfp);
    if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
      sprintf(poll_author,"%s",inbuf);
    fclose(cfp);
  }
  return poll_author;
}

/* call get_vote_files after done with info obtained in get_my_polls */
get_my_polls()
{
  int i, uidlen;
  NAME author;
  NAME userid;

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif

  sprintf(userid,"%s",my_userid());
  uidlen = strlen(userid);
  get_vote_files();
  if(activepolls[0]) {
    /* check which polls are userid's */
    for(i=1;i<=maxpoll;i++) {
      if(activepolls[i]) {
        sprintf(author,"%s",pollauthor(i));
        if(strlen(author) == uidlen) {
          if(strncmp(author,userid,uidlen)) {
            activepolls[i] = 0;
            activepolls[0]--;
          }
        }
        else {
          activepolls[i] = 0;
          activepolls[0]--;
        }
      }
    }
  }
  return;
}

get_vote_file(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, VOTEFILE);
}

get_vote_time(userid, tbuf, ballotnum)
char *userid;
LONG *tbuf;
int ballotnum;
{
  PATH votefile;
  int i;
  FILE *fp;
  char votetime[20];

  *tbuf = 0;
  get_vote_file(userid, votefile);
  if((fp = fopen(votefile, "r")) == NULL)
    return 0;
  i = 1;
  while((fgets(votetime,sizeof(votetime),fp)) != NULL) {
    if(i == ballotnum) {
      *tbuf = atol(votetime);
      break;
    }
    i++;
  }
  fclose(fp);
  return 0;
}

set_vote_time(ballotnum)
int ballotnum;
{
  PATH votefile;
  FILE *fp;
  int i,j,fd;
  char votetime[20];
  char writeout[21];
  NAME userid;

  sprintf(userid,"%s",my_userid());
  get_vote_file(userid, votefile);
  unlink(c_tempfile);
  rename(votefile,c_tempfile);
  if ((fd = open(votefile, O_WRONLY|O_CREAT, 0600)) == 0) {
    prints("Can't open your voting registration! Aborting...\n");
    rename(c_tempfile,votefile);
    return 1;
  }
  flock(fd, LOCK_EX);
  i = 1;
  if((fp = fopen(c_tempfile, "r")) != NULL) {
    while((fgets(votetime,sizeof(votetime),fp)) != NULL) {
      if(i == ballotnum) 
        sprintf(writeout,"%d\n",time(0));
      else
        sprintf(writeout,"%s",votetime);
      write(fd, writeout, strlen(writeout));
      i++;
    }
  }
  if(i <= ballotnum) {
    if(i == ballotnum) {
      sprintf(writeout,"%d\n",time(0));
      write(fd, writeout, strlen(writeout));
    }
    else {
      while(i < ballotnum) {
        sprintf(writeout,"0\n");
        write(fd, writeout, strlen(writeout));
        i++;
      }
      sprintf(writeout,"%d\n",time(0));
      write(fd, writeout, strlen(writeout));
    }
  }
  flock(fd, LOCK_UN);
  close(fd);
  unlink(c_tempfile);
  return 0;
}    

dovote(ballotnum)
int ballotnum;
{
  FILE *cfp, *tfp;
  PATH votefile, tempvotefile;
  NAME userid;
  NAME author;
  TITLE title;
  char inbuf[80], choices[10], vote[2];
  int fd, count, aborted, votecount, allowcom = 0;
  time_t opentime, closetime, votetime;
  struct stat statb;
  char *tmpptr;

  int retc, y, x;
  char ans[4];

  retc = valid_vote(ballotnum);
  switch(retc) {
    case VALIDVOTE: {
      break;
    }
    case USERVOTED: {
      prints("\n\nThe polls are open but you've already voted in this poll!\n");
      clrtobot();
      pressreturn();
      clear();
      return FULLUPDATE;
    }
    default: {
      prints("\n\nSorry, the polls are not open at this time.\n");
      clrtobot();
      pressreturn();
      clear();
      return FULLUPDATE;
    }
  }
  sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
  More(votefile, YEA);
  clear();
  move(0,0);
  standout();
  prints("THE BALLOT - %s (%s)",vcntrl.title,vcntrl.author);
  standend();
  move(2,0);
  if(vcntrl.votecount == 0) {
    prints("To cast your vote, enter the digit next to your selection.\n");
    prints("Any other key aborts voting.\n");
  }
  else if(vcntrl.votecount > 0) {
    prints("You may choose up to %d selections.  Enter the digit next to\n",
            vcntrl.votecount);
    prints("your selections one at a time.  Enter 0 to end your selections\n");
    prints("before choosing %d. Any other key aborts voting.\n",
            vcntrl.votecount);
  }
  else {
    prints("You may rank up to %d selections.  Enter the digit next to\n",
            abs(vcntrl.votecount));
    prints("your selections one at a time in the order you want.  Enter 0 to end your\n");
    prints("selections before choosing %d. Any other key aborts voting.\n",
            abs(vcntrl.votecount));
  }
  prints("This poll will close at: %s", ctime(&vcntrl.closetime));
  if(vcntrl.allowcom)
    prints("Leave an anonymous comment after you vote.");
  bzero(&choices, sizeof(choices));
  move(7,0);
  count = 0;
  while(vcntrl.choices[count][0] != '\0') {
    sprintf(vote,"%d",count + 1);
    choices[count] = vote[0];
    prints("%d) %s",count + 1,vcntrl.choices[count]);
    count++;
    if(count >= 9) break;
  }
  sprintf(votefile,"%s.%d",VOTEBALLOT,ballotnum);

  if(vcntrl.votecount == 0) {
    while(1) {
      vote[0] = vote[1] = '\0';
      getdata(count+9, 0, "Enter your choice: ", vote, 2, DOECHO, NULL);
      if (vote[0] == '\0' || index(choices, vote[0]) == NULL) {
        move(count+11, 0);
        prints("Invalid choice...voting aborted.\n");
        pressreturn();
        clear();
        return FULLUPDATE;
      }
      getdata(count+10, 0, "Please confirm your choice (Y/N) [N]: ",
              ans, 2, DOECHO, NULL);
      if(ans[0] == 'y' || ans[0] == 'Y')
        break;
      move(count+9,0);
      clrtobot();
    }
    if ((fd = open(votefile, O_WRONLY|O_CREAT|O_APPEND, 0600)) == 0) {
      move(count+12, 0);
      prints("Can't open the ballot box! Aborting...\n");
      pressreturn();
      clear();
      return FULLUPDATE;
    }
    if(!set_vote_time(ballotnum)) {
      flock(fd, LOCK_EX);
      write(fd, vote, 1);
      flock(fd, LOCK_UN);
      fstat(fd, &statb);
      close(fd);
      prints("Ballot recorded! (%d cast so far)\n", statb.st_size);
    }
  }
  if(vcntrl.votecount < 0) {
     /* not implemented yet */
    return;
  }
  if(vcntrl.votecount > 0) {
     /* not implemented yet */
    return;
  }
  if(vcntrl.allowcom) {
    getyx(&y,&x);
    y++;
    getdata(y, 0, "Include an anonymous comment (y/n) [N]: ",
            vote, 2, DOECHO, NULL);
    if(vote[0] == 'Y' || vote[0] == 'y') {
      unlink(c_tempfile);
      aborted = Edit(c_tempfile);
      if (stat(c_tempfile, &statb) != 0 || statb.st_size == 0)
        aborted = 1;
      if(!aborted) {
        sprintf(tempvotefile,"temp%s.%d.%s",VOTECOMMENTS,ballotnum,my_userid());
        if(tfp = fopen(tempvotefile, "w")) {
          sprintf(votefile,"%s.%d",VOTECOMMENTS,ballotnum);
          if(!stat(votefile,&statb)) {
            suckinfile(tfp, votefile);
            fprintf(tfp,
                   "\n-------------------------------------------\n");
          }
          suckinfile(tfp, c_tempfile);
          fclose(tfp);
          rename(tempvotefile,votefile);
        }
        unlink(c_tempfile);
        clear();
        prints("Comment recorded!\n");
      }
    }
  }
  pressreturn();
  clear();
  return FULLUPDATE;
}

donotvote(ballotnum)
int ballotnum;
{

  int y,x,retc;
  char vote[2];

  retc = valid_vote(ballotnum);
  switch(retc) {
    case VALIDVOTE: {
      break;
    }
    case USERVOTED: {
      prints("\n\nThe polls are open but you've already voted in this poll!\n");
      clrtobot();
      pressreturn();
      clear();
      return FULLUPDATE;
    }
    default: {
      prints("\n\nSorry, the polls are not open at this time.\n");
      clrtobot();
      pressreturn();
      clear();
      return FULLUPDATE;
    }
  }

  getyx(&y,&x);
  y++;
  prints("You are about to give up your vote on poll #%d\n",ballotnum);
  y++;
  getdata(y, 0, "Are you sure, you cannot change your mind (y/n) [N]: ",
                  vote, 2, DOECHO, NULL);
  if(vote[0] == 'Y' || vote[0] == 'y') {
    if(set_vote_time(ballotnum))
      prints("Can't mark you as voted in this poll! Aborting...\n");
    else
      prints("You are marked as voted in this poll!\n");
  }
  else
    prints("Action Aborted!\n");
  pressreturn();
  clear();
  return FULLUPDATE;
}

closepolls()
{
  int i;
#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif
  if(maxpoll < 1) maxpoll = 1;
  for(i=1;i<=maxpoll;i++)
    doclosepolls(i,0);
  return 0;
}

doclosepolls(ballotnum,forceclose)
int ballotnum;
int forceclose;
{
  FILE *tfp;
  char inchar;
  struct stat statb;
  char *tmpptr;
  struct stat status;
  PATH votefile, tempvotefile;
  int retc, bfd, totvotes, counts[11], i;
  NAME voteboard;
  BOARD board;
  char *bname;

#ifndef MOD_EXTRAS
  int postvote = POSTVOTE;

  sprintf(voteboard,"%s",VOTEBOARD);
#else
  int postvote = modserver.postvote;

  sprintf(voteboard,"%s",modserver.voteboard);
#endif

  bname = voteboard;
  if(_lookup_board(bname, &board) != S_OK)
    postvote = 0;
  sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
  if(stat(votefile,&status)) {
    return remove_vote_files(ballotnum);
  }
  sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
  if(stat(votefile,&status)) {
    return remove_vote_files(ballotnum);
  }

  retc = read_vote_control(ballotnum);
  if((retc != VALIDVOTE) && (retc != USERVOTED))
    return retc;
  /* vote files already removed in read_vote_control */

  retc = valid_vote_control();
  if((retc == VALIDVOTE) || (retc == CLOSEVOTE)) {
    if(((retc == VALIDVOTE) && forceclose) || (retc == CLOSEVOTE)) {
      sprintf(votefile,"%s.%d",VOTEBALLOT,ballotnum);
      if(stat(votefile,&status))
        return remove_vote_files(ballotnum);
      sprintf(tempvotefile,"temp%s.%d",VOTEBALLOT,ballotnum);
      rename(votefile, tempvotefile);
      sprintf(votefile,"%s.%d",VOTECONTROL,ballotnum);
      sprintf(tempvotefile,"temp%s.%d",VOTECONTROL,ballotnum);
      rename(votefile, tempvotefile);
      sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
      sprintf(tempvotefile,"temp%s.%d",VOTEDESC,ballotnum);
      rename(votefile, tempvotefile);
      sprintf(votefile,"%s.%d",VOTECOMMENTS,ballotnum);
      sprintf(tempvotefile,"temp%s.%d",VOTECOMMENTS,ballotnum);
      rename(votefile, tempvotefile);
      sprintf(tempvotefile,"temp%s.%d",VOTEBALLOT,ballotnum);
      for (bfd = 0; bfd < 10; bfd++) counts[bfd] = 0;
      if ((bfd = open(tempvotefile, O_RDONLY)) != -1) {
        while (read(bfd, &inchar, 1) == 1) {
          counts[(int)(inchar - '0')]++;
        }
        close(bfd);
      }
      sprintf(tempvotefile,"temp%s.%d",VOTERESULTS,ballotnum);
      if (tfp = fopen(tempvotefile, "w")) {
        totvotes = 0;
        vcntrl.closetime = time(0);
        fprintf(tfp,
"-------------------------------------------------------------------------\n\n");
        fprintf(tfp, "** %s - opened by %s\n", vcntrl.title, vcntrl.author);
        fprintf(tfp, "** Polls opened: %s", ctime(&vcntrl.opentime));
        fprintf(tfp, "** Polls closed: %s", ctime(&vcntrl.closetime));
        if(forceclose)
          fprintf(tfp, "** Closed by : %s\n", my_userid());
        fprintf(tfp, "\n** Synopsis of ballot issue:\n\n");
        sprintf(tempvotefile,"temp%s.%d",VOTEDESC,ballotnum);
        suckinfile(tfp, tempvotefile);
        fprintf(tfp, "** Results:\n\n");
        for(i=0;i<9;i++) {
          if(vcntrl.choices[i][0] == '\0')
            break;
          else {
            fprintf(tfp, "%1d) %3d - %s",i+1,counts[i+1],vcntrl.choices[i]);
            totvotes += counts[i+1];
          }
        }
      }
      fprintf(tfp, "\nTotal votes cast = %d\n\n", totvotes);
      sprintf(tempvotefile,"temp%s.%d",VOTECOMMENTS,ballotnum);
      if(!stat(tempvotefile,&statb)) {
        fprintf(tfp,"Comments:\n-------------------------\n\n");
        suckinfile(tfp, tempvotefile);
      }
      sprintf(votefile,"%s",VOTERESULTS);
      sprintf(tempvotefile,"temp%s.%d",VOTERESULTS,ballotnum);
      if(!postvote)
        suckinfile(tfp, votefile);
      fclose(tfp);
      if(!postvote)
        rename(tempvotefile,votefile);
      else 
        post_vote_file(ballotnum,vcntrl.author,vcntrl.title);
      remove_vote_files(ballotnum);
    }
  }
  else
    return remove_vote_files(ballotnum);
  return 0;
}

suckinfile(fp, fname)
FILE *fp;
char *fname;
{
  char inbuf[256];
  FILE *sfp;
  if ((sfp = fopen(fname, "r")) == NULL) return -1;
  while (fgets(inbuf, sizeof(inbuf), sfp) != NULL)
    fputs(inbuf, fp);
  fclose(sfp);
  return 0;
}


post_vote_file(ballotnum,author,title)
int ballotnum;
char *author;
char *title;
{
  PATH votefile;
  NAME voteboard;
  struct _sendmsgstruct sm;
  
#ifndef MOD_EXTRAS
  sprintf(voteboard,"%s",VOTEBOARD);
#else
  sprintf(voteboard,"%s",modserver.voteboard);
#endif
  sprintf(votefile,"temp%s.%d",VOTERESULTS,ballotnum);
  sm.btype = BOARD_POST;
  memset(&sm.hdr, 0, sizeof sm.hdr);
  strncpy(sm.hdr.owner, author, sizeof sm.hdr.owner);
  sprintf(sm.username, "Auto Posted at Poll Closure");
  strncpy(sm.hdr.title, title, TITLELEN);
  sm.to_list = NULL;
  sm.srcfile = votefile;
  sm.destfile[0] = '\0';
  _do_message(0, voteboard, &sm);
}

display_polls(alv)
int alv;
{
  int i, allowcom;
  FILE *cfp;
  PATH votefile;
  NAME userid;
  NAME author;
  TITLE title;
  char inbuf[80], alvoted[5];
  time_t opentime, closetime, votetime;
  char *tmpptr;
#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif

  clear();
  prints("Active Polls\n");
  for(i=1;i<=maxpoll;i++) {
    if(activepolls[i]) {
      if(alv) {
        if(activepolls[i] == USERVOTED)
          sprintf(alvoted,"*");
        else
          sprintf(alvoted,"%d",i);
      }
      else
        sprintf(alvoted,"%d",i);
      valid_vote(i);
      prints("%c%3s) %s (%s)\n",(vcntrl.allowcom ? '+' : ' '),
       alvoted,vcntrl.title,vcntrl.author);
    }
  }
}

Vote()
{
  int retc, i, dvote;
  char inbuf[80];

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif
  bbs_set_mode(M_VOTING);
  closepolls();
  get_vote_files();
  if(!activepolls[0]) {
    clear();
    prints("Sorry, the polls are not open at this time.\n");
    pressreturn();
    return FULLUPDATE;
  }
  if(activepolls[0] == 1) {
    for(i=0;i<=maxpoll;i++) {
      if(activepolls[i]) {
        retc = dovote(activepolls[i]);
        break;
      }
    }
  }
  else {
    display_polls(1);
    getdata(activepolls[0]+3,0,"Enter number of poll: ",inbuf,4,DOECHO,NULL);
    dvote = atoi(inbuf);
    if(dvote && activepolls[dvote])
      retc = dovote(dvote);
    else {
      prints("\nInvalid poll number.\n");
      pressreturn();
      return FULLUPDATE;
    }
  }
  bbs_set_mode(M_UNDEFINED);
  return retc;
}

NoVote()
{
  int retc, i, dvote;
  char inbuf[80];

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif
  bbs_set_mode(M_VOTING);
  closepolls();
  get_vote_files();
  if(!activepolls[0]) {
    clear();
    prints("Sorry, the polls are not open at this time.\n");
    pressreturn();
    return FULLUPDATE;
  }
  if(activepolls[0] == 1) {
    for(i=0;i<=maxpoll;i++) {
      if(activepolls[i]) {
        retc = donotvote(activepolls[i]);
        break;
      }
    }
  }
  else {
    display_polls(1);
    getdata(activepolls[0]+3,0,"Enter number of poll: ",inbuf,4,DOECHO,NULL);
    dvote = atoi(inbuf);
    if(dvote && activepolls[dvote])
      retc = donotvote(dvote);
    else {
      prints("\nInvalid poll number.\n");
      pressreturn();
      return FULLUPDATE;
    }
  }
  bbs_set_mode(M_UNDEFINED);
  return retc;
}

VoteResults()
{
  PATH votefile;
  NAME voteboard;
  BOARD board;
  char *bname;

#ifndef MOD_EXTRAS
  int postvote = POSTVOTE;

  sprintf(voteboard,"%s",VOTEBOARD);
#else
  int postvote = modserver.postvote;

  sprintf(voteboard,"%s",modserver.voteboard);
#endif

  bname = voteboard;
  if(_lookup_board(bname, &board) != S_OK)
    postvote = 0;
  if(postvote) {
      move(4,0);
      prints("Poll results are posted on the board '%s'.\n",voteboard);
      clrtobot();
      pressreturn();
  }
  else {
    sprintf(votefile,"%s",VOTERESULTS);
    if(More(votefile, YEA) == -1) {
      move(4,0);
      prints("No poll results are available at this time.\n");
      clrtobot();
      pressreturn();
    }
  }
  return FULLUPDATE;
}

display_vote_control(descrdone,daysopen)
int descrdone;
int daysopen;
{
  int i;
  char polltype[40];

  if(!vcntrl.votecount)
    sprintf(polltype,"Single Choice");
  else if(vcntrl.votecount > 0)
    sprintf(polltype,"Multiple Choices");
  else 
    sprintf(polltype,"Choice Ranking");
  clear();
  standout();
  prints("Open The Polls");
  standend();
  move(2,0);
  prints("A. Poll Title        : %s\n",vcntrl.title);
  prints("B. Allow Comments    : %s\n",((vcntrl.allowcom) ? "Yes" : "No"));
  prints("C. Poll Type         : %s\n",polltype);
  prints("D. Days Poll is Open : %d\n",daysopen);
  prints("E. Edit Description    %s\n",
((descrdone) ? "(Description created)" : "(Description must be created)"));
  prints("Poll Choices: (Must have at least 2)\n");
  for(i=0;i<9;i++) {
    if(vcntrl.choices[i][0] == '\0')
      prints(" %d.\n",i+1);
    else
      prints(" %d. %s",i+1,vcntrl.choices[i]);
  }
  prints("Q. Quit Poll Creation\n");
  prints("S. Save Poll\n");
  return 0;
}

collapse_choices()
{
  int i, count;
  char choices[9][TITLELEN];
  count = 0;
  bzero(&choices, sizeof(choices));
  for(i=0;i<9;i++) {
    if(vcntrl.choices[i][0] != '\0') {
      strncpy(choices[count],vcntrl.choices[i],TITLELEN-1);
      count++;
    }
  }
  bcopy(&choices,&vcntrl.choices,sizeof(choices));
  return 0;
}

OpenVote()
{
  struct stat statb;
  FILE *fp;
  PATH votefile;
  TITLE title;
  char inbuf[TITLELEN+1];
  char *newline;
  int numselections = 0, aborted;
  int i, ballotnum = 0;
  time_t closetime, numseconds;
  int y,x,retc,daysopen,descrdone,voteready;
  char ans[3], ans2[4], vote[2];
  char choices[25];

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
  int maxuserpolls = MAXUSERPOLLS;
  if(maxuserpolls > MAXPOLLS)
    maxuserpolls = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  int maxuserpolls = modserver.maxuserpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
  if(maxuserpolls > MAXPOLLS)
    maxuserpolls = MAXPOLLS;
#endif
  if(maxpoll < 1) maxpoll = 1;
  closepolls(); /* close any polls scheduled to close */
  for(i=1;i<=maxpoll;i++) {
    sprintf(votefile,"%s.%d",VOTECONTROL,i);
    if(stat(votefile,&statb)) {
      sprintf(votefile,"%s.%d",VOTECREATE,i);
      if(stat(votefile,&statb)) {
        fp = fopen(votefile, "w");
        fclose(fp);
        ballotnum = i;
        break;
      }
      else {
        if((statb.st_ctime + 7200) < time(0)) {
          unlink(votefile);
          fp = fopen(votefile, "w");
          fclose(fp);
          ballotnum = i;
          break;
        }
      }
    }
  }
  if (!ballotnum) {
    prints("\n\nSorry, the maximum number of polls (%d) is already open.\n\n",
            maxpoll);
    prints("A poll must close first, or the maximum number of polls\n");
    prints(" must be increased before starting a new poll.\n");
    clrtobot();
    pressreturn();
    clear();
    return FULLUPDATE;
  }
  if(!HasPerm(C_VOTEADMIN)) {
    get_my_polls();
    if(activepolls[0] >= maxuserpolls) {
      sprintf(votefile,"%s.%d",VOTECREATE,ballotnum);
      unlink(votefile);
      prints("\n\nSorry, the maximum number of open polls that you can have\n");
      prints(" (%d) is already open.\n\n",maxuserpolls);
      prints("You must close one of your polls first before starting a new poll.\n");
      clrtobot();
      pressreturn();
      clear();
      return FULLUPDATE;
    }
  }

/* Set up vote control */
  bzero(&vcntrl, sizeof(vcntrl));
  daysopen = 10;
  vcntrl.opentime = time(0);
  vcntrl.closetime = time(0) + (86400 * daysopen);
  sprintf(vcntrl.author, "%s", my_userid());
  sprintf(vcntrl.title, "Poll #%d",ballotnum); 
  vcntrl.allowcom = 1;
  vcntrl.votecount = 0;
  descrdone = 0;
  voteready = 0;
  sprintf(choices,"AaBbCcDdEeQqSs123456789");
  while(!voteready) {
    aborted = 0;
    display_vote_control(descrdone,daysopen);
    getyx(&y,&x);
    getdata(y+2,0,"Item to change: [A-E, 1-9, (Q)uit, (S)ave: ",
            ans,2,DOECHO,NULL);
    if (ans[0] == '\0' || index(choices, ans[0]) == NULL) {
      prints("Invalid selection\n");
      pressreturn();
    }
    else {
      switch(ans[0]) {
        case 'A':
        case 'a':
          getdata(y+3,0,"Title of Poll: ",title,TITLELEN,DOECHO,NULL);
          if(title[0] != '\0')
            sprintf(vcntrl.title,"%s",title);
          break;
        case 'B':
        case 'b':
          getdata(y+3,0,"Allow Comments: (Y/N) [Y]: ",ans2,3,DOECHO,NULL);
          if(ans2[0] == 'N' || ans2[0] == 'n')
            vcntrl.allowcom = 0;
          else
            vcntrl.allowcom = 1;
          break;
        case 'C':
        case 'c':
          move(y+3,0);
          prints("This option is not yet implemented\n");
          pressreturn();
          break;
        case 'D':
        case 'd':
          getdata(y+3,0,"Days to keep poll open: (1-999) [10]: ",
                  ans2,3,DOECHO,NULL);
          if(ans2[0] != '\0') {
            i = atoi(ans);
            if((i > 0) && (i < 1000))
              daysopen = i;
          }
          break;
        case 'E':
        case 'e':
          sprintf(votefile,"%s.%d",VOTEDESC,ballotnum);
            aborted = Edit(votefile);
            if(!aborted)
              descrdone = 1;
            if (stat(votefile, &statb) != 0 || statb.st_size == 0)
              descrdone = 0;
          aborted = 0;
          break;
        case 'Q':
        case 'q':
          aborted = 1;
          voteready = 1;
          break;
        case 'S':
        case 's':
          vcntrl.opentime = time(0);
          vcntrl.closetime = time(0) + (86400 * daysopen);
          collapse_choices();
          if(!descrdone) {
            prints("You must create a description first.\n");
            pressreturn();
            break;
          }
          if((retc = valid_vote_control()) == VALIDVOTE)
            voteready = 1;
          else {
            if((retc == NOCHOICES) || (retc == NOCHOICES2)) {
              prints("You must have at least 2 choices in a poll\n");
              pressreturn();
              break;
            }
            else {
              aborted = 1;
              voteready = 1;
              prints("Major blowup, Aborting....\n");
              pressreturn();
            }
          }
          break;
        default:
          i = atoi(ans);
          getdata(y+3,0,"Enter Poll choice : ",vcntrl.choices[i-1],
                  TITLELEN, DOECHO, NULL);
          if(vcntrl.choices[i-1] != '\0')
            strcat(vcntrl.choices[i-1],"\n");
          break;
      }
    }
    if(voteready)
      break;
  }
  sprintf(votefile,"%s.%d",VOTECREATE,ballotnum);
  unlink(votefile);
  if(aborted) {
    prints("Open polls ABORTED.\n");
    pressreturn();
    remove_vote_files(ballotnum);
    return FULLUPDATE;
  }
  write_vote_control(ballotnum);
  bbslog(2, "OPENVOTE #%d by %s\n", ballotnum,my_userid());
  sprintf(votefile,"%s.%d",VOTECOMMENTS,ballotnum);
  unlink(votefile);
  sprintf(votefile,"%s.%d",VOTEBALLOT,ballotnum);
  unlink(votefile);
  prints("The polls are open!\n");
  pressreturn();
  clear();
  return FULLUPDATE;
}

CloseVote()
{
  int i, dvote;
  char inbuf[80];

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif
  closepolls();
  get_vote_files();
  if(!activepolls[0]) {
    clear();
    prints("Sorry, there are no open polls.\n");
    pressreturn();
    return FULLUPDATE;
  }
  display_polls(0);
  getdata(activepolls[0]+3,0,"Enter number of poll to close: ",
   inbuf,4,DOECHO,NULL);
  dvote = atoi(inbuf);
  if(dvote && activepolls[dvote]) {
    doclosepolls(dvote,1);
    prints("\nPoll # %d closed\n",dvote);
  }
  else
    prints("\nInvalid poll number.\n");
  pressreturn();
  return FULLUPDATE;
}

disp_newvotes()
{
  int i;
  FILE *tfp;
  PATH votefile;
  int newpolls = 0;
  struct stat statb;

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;

  if(usermodcfg.vote[0] == 'N')
    return 0;
#endif

  get_vote_files();
  if(activepolls[0]) {
    for(i=1;i<=maxpoll;i++) {
      if(activepolls[i] == VALIDVOTE)
        newpolls++;
    }
  }
  if(newpolls) {
    if(tfp = fopen(c_tempfile, "a")) {
      fprintf(tfp,"\nYou have not yet voted in the following polls:\n\n");
      for(i=1;i<=maxpoll;i++) {
        if(activepolls[i] == VALIDVOTE) {
          valid_vote(i);
          fprintf(tfp,"%c%3d) %s (%s)\n",(vcntrl.allowcom ? '+' : ' '),
           i,vcntrl.title,vcntrl.author);
        }
      }
      sprintf(votefile,"%s",VOTEINSTR);
      if(!stat(votefile,&statb))
        suckinfile(tfp, votefile);
      fclose(tfp);
    }
  }
  return newpolls;
}

CloseMyPolls()
{
  int i, dvote;
  char inbuf[80];

#ifndef MOD_EXTRAS
  int maxpoll = MAXPOLLS;
#else
  int maxpoll = modserver.maxpolls;
  if(maxpoll > MAXPOLLS)
    maxpoll = MAXPOLLS;
#endif
  closepolls();
  get_my_polls();
  if(!activepolls[0]) {
    clear();
    prints("Sorry, you do not have any open polls.\n");
    pressreturn();
    return FULLUPDATE;
  }
  display_polls(0);
  getdata(activepolls[0]+3,0,"Enter number of poll to close: ",
   inbuf,4,DOECHO,NULL);
  dvote = atoi(inbuf);
  if(dvote && activepolls[dvote]) {
    doclosepolls(dvote,1);
    prints("\nPoll # %d closed\n",dvote);
  }
  else
    prints("\nInvalid poll number.\n");
  pressreturn();
  get_vote_files();
  return FULLUPDATE;
}

#endif
