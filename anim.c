/*
Eagles Bulletin Board System
Copyright (C) 1995, Ray Rocker, rocker@datasync.com

EBBS Modules
Copyright (C) 1999, Paul Snow, psnow@nipha.com

This program is free software; you can redistribute it and/or
modify
it under the terms of the GNU General Public License as published
by
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
#ifdef MOD_ANIM

#define STRLEN 80
#include <math.h>
#include <stdio.h>   
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#if USES_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifndef MAXINT
#define __BITSPERBYTE 8
#define __BITS(type)  (__BITSPERBYTE * (int)sizeof(type))
#ifndef BITS
#define BITS(type) __BITS(type)
#endif
#ifndef BITSPERBYTE
#define BITSPERBYTE  __BITSPERBYTE
#endif
#define INTBITS     __BITS(int)
#define MININT      (1 << (INTBITS - 1))
#define MAXINT      (~MININT)
#endif


int animline;
int animchar;

addm_io(fd,timeout)
int fd ;
int timeout ;
{
    extern int i_newfd;
    extern struct timeval i_to;
    extern struct timeval *i_top;
    i_newfd = fd ;
    if(timeout) {
        i_to.tv_sec = 0 ;
        i_to.tv_usec = 1000 * timeout ;
        i_top = &i_to ;
    } else i_top = NULL ;
}

char *strip_function(ptr)
char *ptr;
{
	char *newptr;
	char *tmpptr;
	if((newptr = index(ptr, '('))) {
	    newptr++;
	    if(*newptr == '"')
		newptr++;
	    if((tmpptr = rindex(newptr, ')'))) {
		*(tmpptr) = '\0';
		tmpptr--;
		if(*tmpptr == '"')
		    *(tmpptr) = '\0';
	    }
	    return newptr;
	}
	else return ptr;
}

char *second_arg(ptr)
char *ptr;
{
	while(*ptr != ',') {
	    if(*ptr == '\0')
		return ptr;
	    else ptr++;
	}
	ptr++;
	return ptr;
}

int
do_anim(str,offset,endanimline)
char *str;
int offset;
int endanimline;
{
	/*  Animation Primitives
		move(x,y)
		msleep(x)
		sleep(x)
		clear()
		clrtobot()
		clrtoeol()
		print("str")
		prints("str")
		printa("str")
		printb("str")
		printclr("str")
	*/
	int i, stringlen;
	char *newstr;
	stringlen = strlen(str);
	if(stringlen < 5)
	    return 0;
	if(*str == ' ')
	    return 0;
	if(*str == 'm' || *str == 'M') {
	    if(!ci_strncmp(str,"move(",5)) {
		newstr = strip_function(str);
		animline = atoi(newstr) - 1 + offset;
		if(animline >= endanimline)
		    animline = endanimline - 1;
		if(animline < offset)
		    animline = offset;
		animchar = atoi(second_arg(newstr)) - 1;
		if(animchar > 79)
		    animchar = 79;
		if(animchar < 0)
		    animchar = 0;
		return 1;
	    }
	    if(stringlen < 7)
		return 0;
	    if(!ci_strncmp(str,"msleep(",7)) {
		newstr = strip_function(str);
	 	i = atoi(newstr);
		if(i > 10000) i = 10000;
		if(i) usleep(i * 1000);
		return 1;
	    }
	    return 0;
	} /* *str == 'm' */

	if(*str == 's' || *str == 'S') {
	    if(stringlen < 6)
		return 0;
	    if(!ci_strncmp(str,"sleep(",6)) {
		newstr = strip_function(str);
		i = atoi(newstr);
		if(i > 10) i = 10;
		if(i) sleep(i);
		return 1;
	    }
	    return 0;
	} /* *str == 's' */

	if(*str == 'p' || *str == 'P') {
	    if(stringlen < 6)
		return 0;
	    if(!ci_strncmp(str,"print(",6)) {
		newstr = strip_function(str);
		move(animline,animchar);
		prints("%s",newstr);
		move(endanimline,0);
		refresh(); 
		return 1;
	    }
	    if(stringlen < 7)
		return 0;
	    if(!ci_strncmp(str,"prints(",7)) {
		newstr = strip_function(str);
		move(animline,animchar);
		prints("%s",newstr);
		move(endanimline,0);
		refresh(); 
		animchar = animchar + strlen(newstr);
		if(animchar > 79) {
		    animchar = 0;
		    animline++;
		    if(animline > endanimline - 1) 
			animline = offset;
		}
		return 1;
	    }
	    if(!ci_strncmp(str,"printa(",7)) {
		newstr = strip_function(str);
		move(animline,animchar);
		prints("%s",newstr);
		move(endanimline,0);
		refresh(); 
		animchar = animchar++;
		if(animchar > 79) {
		    animchar = 0;
		    animline++;
		    if(animline > endanimline - 1) 
			animline = offset;
		}
		return 1;
	    }
	    if(!ci_strncmp(str,"printb(",7)) {
		newstr = strip_function(str);
		move(animline,animchar);
		prints("%s",newstr);
		move(endanimline,0);
		refresh(); 
		animchar = animchar--;
		if(animchar < 0) {
		    animchar = 79;
		    animline--;
		    if(animline < offset) 
			animline = endanimline - 1;
		}
		return 1;
	    }
	    if(stringlen < 9)
		return 0;
	    if(!ci_strncmp(str,"printclr(",9)) {
		newstr = strip_function(str);
		move(animline,animchar);
		clrtoeol();
		prints("%s",newstr);
		move(endanimline,0);
		refresh(); 
		animline++;
		if(animline > endanimline - 1) 
		    animline = offset;
		animchar = 0;
		return 1;
	    }
	    return 0;
	} /* *str == 'p' */

	if(*str == 'c' || *str == 'C') {
	    if(stringlen < 7)
		return 0;
	    if(!ci_strncmp(str,"clear()",7)) {
		for(i=offset;i<endanimline;i++) {
		    move(i,0);
		    clrtoeol();
		}
		animline = offset;
		animchar = 0;
		refresh(); 
		return 1;
	    }
	    if(stringlen < 10)
		return 0;
	    if(!ci_strncmp(str,"clrtobot()",10)) {
		for(i=animline;i<endanimline;i++) {
		    move(i,0);
		    clrtoeol();
		}
		refresh(); 
		return 1;
	    }
	    if(!ci_strncmp(str,"clrtoeol()",10)) {
		move(animline,animchar);
		clrtoeol();
		refresh(); 
		return 1;
	    }
	    return 0;
	} /* *str == 'c' */

	return 0;
}

int
animate_spell(str,offset,endanimline)
char *str;
int offset;
int endanimline;
{
	int i,direction;
	int keypress,startline,startpos,spelldelay,textlen;
	char dobuf[STRLEN+40];
	char *newstr;
	char textstr[STRLEN+40];
	if(*str == 's' || *str == 'S') {
	    for (i=0;i<5;i++) str++;
	    if(*str == 'a') 
		direction=0;
	    else if(*str == 'b')
		direction=1;
	    else
		return -1;
	    str++;
	    if(*str == '(') str++;
	    else return -1;
	    startline = atoi(str);
	    if((startline + offset) > endanimline)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    startpos = atoi(newstr);
	    if(!(str = index(newstr, ',')))
		return -1;
	    str++;
	    spelldelay = atoi(str);
	    if(!(newstr = index(str, ',')))
		return -1;
	    *(newstr) = '(';
	    str = strip_function(newstr);
	    sprintf(textstr,"%s",str);
	    textlen = strlen(textstr);
	    if((startpos + textlen) > 80)
		return -1;
	}
	else if (*str == 'e' || *str == 'E') {
	    for (i=0;i<5;i++) str++;
	    if(*str == 'a') 
		direction=0;
	    else if(*str == 'b')
		direction=1;
	    else
		return -1;
	    str++;
	    if(*str == '(') str++;
	    else return -1;
	    startline = atoi(str);
	    if((startline + offset) > endanimline)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    startpos = atoi(newstr);
	    if(!(str = index(newstr, ',')))
		return -1;
	    str++;
	    spelldelay = atoi(str);
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    textlen = atoi(newstr);
	    if((startpos + textlen) > 80)
		textlen = 80 - startpos;
	    bzero(&textstr,sizeof(textstr));
	    for (i=0;i<textlen;i++) textstr[i] = ' ';
	}
	else return -1;
	if(direction)
	    sprintf(dobuf, "move(%d,%d)",startline,startpos + textlen - 1);
	else
	    sprintf(dobuf, "move(%d,%d)",startline,startpos);
	do_anim(&dobuf,offset,endanimline);
	for (i=0;i<textlen;i++) {
	    if(direction) {
		sprintf(dobuf,"printb(\"%c\")",textstr[textlen-1-i]);
		do_anim(&dobuf,offset,endanimline);
	    }
	    else {
		sprintf(dobuf,"printa(\"%c\")",textstr[i]);
		do_anim(&dobuf,offset,endanimline);
	    }
	    addm_io(0,1);
	    if((keypress = igetch()) != I_TIMEOUT) {
		addm_io(0,0);
		break;
	    }
	    addm_io(0,0);
	    usleep(spelldelay * 1000);
	}
	if(keypress != I_TIMEOUT)
	    return keypress;
	else
	    return 0;
}

int
animate_random(str,offset,endanimline,randomfile)
char *str;
int offset;
int endanimline;
FILE *randomfile;
{
	int i,j;
	int erasestr,keypress,textlen,printtimes;
	int nlines,npos,linetimes,linex,lastpos,lastline;
	int startpos,endpos,randomdelay,startline,endline;
	char inbuf[STRLEN+40];
	char *newstr;
	char textstr[STRLEN+40];
	unsigned int seed;
	lastpos=0;
	str++;
	str++;
	if(*str == 'n') {
	    /* random screen stuff */
	    char sarray[24][STRLEN+1];
	    int tarray[24][STRLEN+1];
	    for(i=0;i<9;i++) str++;
	    if(*str == '(') str++;
	    else return -1;
	    randomdelay = atoi(str);
	    if(randomdelay > 1000)
		randomdelay = 1000;
	    if(randomdelay < 1)
		randomdelay = 1;
	    if((newstr = index(str, ','))) {
		newstr++;
		printtimes = atoi(newstr);
		if(printtimes)
		    printtimes = 1;
	    }
	    else printtimes = 0;
	    if(animchar) {
		animline++;
		animchar = 0;
	    }
	    if(animline > endanimline - 1) 
		animline = offset;
	    move(animline,animchar);
	    startline = animline;
	    nlines = 0;
	    bzero(&sarray,sizeof(sarray));
	    bzero(&tarray,sizeof(tarray));
	    /* read in random text */
	    while(fgets(inbuf,sizeof(inbuf),randomfile) != NULL) {
		char *tmpptr;
		if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
		if(inbuf[0] == 'e')
		    if(strlen(inbuf) > 11)
			if(!ci_strncmp(inbuf,"end_random()",12)) break;
		if(!startpos) {
		    textlen = strlen(inbuf);
		    if(textlen > 80) {
			textlen = 80;
			inbuf[80] = '\0';
		    }
		    strncpy(sarray[nlines],inbuf,textlen);
		    sarray[nlines][textlen] = '\0';
		    nlines++;
		    animline++;
		    if(animline > endanimline - 1) {
			animline = offset;
			startpos = 1;
		    }
		}
	    }
	    if(nlines) {
		int donelines[24];
		bzero(&donelines,sizeof(donelines));
		/* block out spaces */
		if(!printtimes) {
		    for(i=0;i<nlines;i++) {
			for(j=0;j<strlen(sarray[i]);j++) {
			    if(sarray[i][j] == ' ')
				tarray[i][j] = 1;
			}
		    }
		}
		seed = (int)((getpid() * time(0)) % MAXINT);
		srandom(seed);
		linex = 0;
		linetimes = (nlines / 3) * 2;
		if(!linetimes) linetimes = 1;
		while(YEA) {
		    int temparray[24];
		    int linearray[24];
		    int numdonelines;
		    numdonelines = nlines;
		    for(i=0;i<nlines;i++) {
			donelines[i] = 1;
			for(j=0;j<strlen(sarray[i]);j++) {
			    if(!tarray[i][j]) {
				if(donelines[i]) {
				    numdonelines--;
				    donelines[i] = 0;
				}
			    }
			}
		    }
		    if(numdonelines == nlines)
			break;
		    if((numdonelines + linetimes) > nlines)
			linetimes = nlines - numdonelines;
		    if(!linex) {
			for(i=0;i<nlines;i++) {
			    if(donelines[i])
				temparray[i] = 1;
			    else
				temparray[i] = 0;
			}
			linex = linetimes;
			for(i=0;i<linetimes;i++) {
			    while(temparray[j = (random() % nlines)])
				/* find empty spot */ ;
			    temparray[j] = 1;
			    linearray[i] = j;
			}
		    }
		    lastline = linearray[linex - 1];
		    textlen = strlen(sarray[lastline]);
		    while(tarray[lastline][lastpos = (random() % textlen)])
				/* find empty spot */ ;
		    tarray[lastline][lastpos] = 1;
		    move(lastline + startline, lastpos);
		    prints("%c",sarray[lastline][lastpos]);
		    move(23,0);
		    refresh();
		    linex--;
		    /* detect key press */
		    addm_io(0,1);
		    if((keypress = igetch()) != I_TIMEOUT) {
			addm_io(0,0);
			break;
		    }
		    addm_io(0,0);
		    usleep(randomdelay * 100);
		}
		if(keypress != I_TIMEOUT) {
		    for(i=0;i<nlines;i++) {
			move(startline+i,0);
			prints(sarray[i]);
		    }
		    refresh();
		}
	    }
	    return 0;
	}
	if(*str == 'r')
	    erasestr = 0;
	if(*str == 'l')
	    erasestr = 1;
	for (i=0;i<4;i++) str++;
	if(*str == '(') str++;
	else return -1;
	startline = atoi(str);
	if((startline + offset) > endanimline)
	    startline = endanimline - offset;
	if(startline < 1)
	    startline = 1;
	if(!(newstr = index(str, ',')))
	    return -1;
	newstr++;
	startpos = atoi(newstr);
	if(startpos < 1)
	    startpos = 1;
	if(startpos > 80)
	    startpos = 80;
	if(!(str = index(newstr, ',')))
	    return -1;
	str++;
	endline = atoi(str);
	if((endline + offset) > endanimline)
	    endline = endanimline - offset;
	if(endline < 1)
	    endline = 1;
	if(!(newstr = index(str, ',')))
	    return -1;
	newstr++;
	endpos = atoi(newstr);
	if(endpos > 80)
	    endpos = 80;
	if(!(str = index(newstr, ',')))
	    return -1;
	str++;
	printtimes = atoi(str);
	if(printtimes < 0)
	    return -1;
	if(printtimes == MAXINT)
	    printtimes = MAXINT - 1;
	else if(printtimes)
	    printtimes++;
	if(!(newstr = index(str, ',')))
	    return -1;
	newstr++;
	randomdelay = atoi(newstr);
	if(!(str = index(newstr, ',')))
	    return -1;
	*(str) = '(';
	newstr = strip_function(str);
	sprintf(textstr,"%s",newstr);
	textlen = strlen(textstr);
	if((startpos + textlen) > endpos)
	    return -1;
	else
	    endpos = endpos - textlen + 1;
	if(startline > endline)
	    return -1;
	seed = (int)((getpid() * time(0)) % MAXINT);
	srandom(seed);
	nlines = endline - startline + 1;
	npos = endpos - startpos + 1;
	linetimes = ((nlines / 3) * 2);
	if(!linetimes) linetimes = 1;
	linex = 0;
	while(YEA) {
	    int temparray[24];
	    int linearray[24];
	    if(printtimes == 1) break;
	    if(!linex) {
		for(i=0;i<nlines;i++) temparray[i] = 0;
		linex = linetimes;
		for(i=0;i<linetimes;i++) {
		    while(temparray[j = (random() % nlines)])
			/* find empty spot */ ;
		    temparray[j] = 1;
		    linearray[i] = j;
		}
	    }
	    if(erasestr && lastpos) {
		move(lastline - 1 + offset, lastpos - 1);
		for(i=0;i<textlen;i++)
		    prints(" ");
	    }
	    lastline = linearray[linex - 1] + startline;
	    lastpos = (random() % npos) + startpos;
	    move(lastline - 1 + offset, lastpos - 1);
	    prints(textstr);
	    /* detect key press */
	    addm_io(0,1);
	    if((keypress = igetch()) != I_TIMEOUT) {
		addm_io(0,0);
		break;
	    }
	    addm_io(0,0);
	    usleep(randomdelay * 1000);
	    linex--;
	    printtimes--;
	}
	if(printtimes < 1)
	    return 0;
	if(keypress != I_TIMEOUT)
	    return keypress;
	else
	    return 0;
}

int
animate_scroll(str,offset,endanimline)
char *str;
int offset;
int endanimline;
{
	int i,direction,startwin,endwin;
	int eraselast=0;
	int keypress,startline,startpos,endpos,scrolldelay,textlen;
	char dobuf[STRLEN+40];
	char *newstr;
	char textstr[STRLEN+40];
	char textbuf[STRLEN+40];
	if(*str == 's' || *str == 'S') {
	    startwin = 1;
	    endwin = 80;
	    for (i=0;i<6;i++) str++;
	    if(*str == 'a') 
		direction=0;
	    else if(*str == 'b')
		direction=1;
	    else
		return -1;
	    str++;
	    if(*str == '(') str++;
	    else return -1;
	    startline = atoi(str);
	    if((startline + offset) > endanimline)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    startpos = atoi(newstr);
	    if(!(str = index(newstr, ',')))
		return -1;
	    str++;
	    endpos = atoi(str);
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    scrolldelay = atoi(newstr);
	}
	else if(*str == 'w' || *str == 'W') {
	    for (i=0;i<7;i++) str++;
	    if(*str == 'a')
		direction=0;
	    else if(*str == 'b')
		direction=1;
	    else
		return -1;
	    str++;
	    if(*str == '(') str++;
	    else return -1;
	    startline = atoi(str);
	    if((startline + offset) > endanimline)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    startpos = atoi(newstr);
	    if(!(str = index(newstr, ',')))
		return -1;
	    str++;
	    endpos = atoi(str);
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    startwin = atoi(newstr);
	    if(startwin < 1)
		startwin = 1;
	    if(!(str = index(newstr, ',')))
		return -1;
	    str++;
	    endwin = atoi(str);
	    if(endwin > 80)
		endwin = 80;
	    if(endwin <= startwin)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    scrolldelay = atoi(newstr);
	}
	else if(*str == 'l' || *str == 'L') {
	    startwin = 1;
	    endwin = 80;
	    for (i=0;i<7;i++) str++;
	    if(*str == 'a') {
		direction=0;
		startpos = 0;
		endpos = 81;
	    }
	    else if(*str == 'b') {
		direction=1;
		startpos = 81;
		endpos = 0;
	    }
	    else
		return -1;
	    str++;
	    if(*str == '(') str++;
	    else return -1;
	    startline = atoi(str);
	    if((startline + offset) > endanimline)
		return -1;
	    if(!(newstr = index(str, ',')))
		return -1;
	    newstr++;
	    scrolldelay = atoi(newstr);
	}
	else return -1;
	if(!(str = index(newstr, ',')))
	    return -1;
	*(str) = '(';
	newstr = strip_function(str);
	sprintf(textstr,"%s",newstr);
	textlen = strlen(textstr) + 1;

	/* compute starting and ending point */


	if(direction) {
	    if(startpos > endwin)
		startpos = endwin;
	    if(endpos < startwin) 
		endpos = startwin - textlen;
	    if(endpos >= startpos)
		return -1;
	}
	else {
	    if(startpos < startwin) 
		startpos = startwin - textlen + 1;
	    else
		startpos--;
	    if(endpos > endwin) {
		endpos = endwin;
		eraselast = 1;
	    }
	    if(startpos >= endpos)
		return -1;
	}
	if(direction)
	    if(startpos < startwin)
		sprintf(dobuf, "move(%d,%d)",startline,startwin);
	    else
		sprintf(dobuf, "move(%d,%d)",startline,startpos);
	else {
	    if(startpos < startwin)
		sprintf(dobuf, "move(%d,%d)",startline,startwin);
	    else
		sprintf(dobuf, "move(%d,%d)",startline,startpos);
	}
	do_anim(&dobuf,offset,endanimline);

	if(direction) {
	    for(i=startpos;i>=endpos;i--) {
		int startbuf,buflen;
		if(i <= startwin) {
		    int k;
		    if(i < startwin) 
			startbuf = startwin - i;
		    else 
			startbuf = 0;
		    if((i + textlen) > endwin)
			buflen = textlen - (i + textlen - endwin) + 1;
		    else
			buflen = textlen - 1;
		    bzero(&textbuf,sizeof(textbuf));
		    for(k=startbuf;k<buflen;k++)
			textbuf[k-startbuf] = textstr[k];
		    sprintf(dobuf,"print(\"%s \")",textbuf);
		}
		else if((i + textlen - 1) > endwin) {
		    int k;
		    buflen = textlen - ((i + textlen) - endwin) + 1;
		    bzero(&textbuf,sizeof(textbuf));
		    for(k=0;k<buflen;k++)
			textbuf[k] = textstr[k];
		    sprintf(dobuf,"printb(\"%s\")",textbuf);
		}
		else
		    sprintf(dobuf,"printb(\"%s \")",textstr);
		do_anim(&dobuf,offset,endanimline);
		/* detect key press */
		addm_io(0,1);
		if((keypress = igetch()) != I_TIMEOUT) {
		    addm_io(0,0);
		    break;
		}
		addm_io(0,0);
		usleep(scrolldelay * 1000);
	    }
	}
	else {
	    for(i=startpos;i<endpos;i++) {
		int startbuf,buflen;
		if(i < startwin) {
		    int k;
		    startbuf = startwin - i - 1;
		    if((i + textlen - 1) > endwin)
			buflen = textlen - (i + textlen - endwin);
		    else
			buflen = textlen - 1;
		    bzero(&textbuf,sizeof(textbuf));
		    for(k=startbuf;k<buflen;k++)
			textbuf[k-startbuf] = textstr[k];
		    sprintf(dobuf,"print(\"%s\")",textbuf);
		}
		else if((i + textlen - 1) > endwin) {
		    int k;
		    buflen = textlen - ((i + textlen) - endwin);
		    bzero(&textbuf,sizeof(textbuf));
		    for(k=0;k<buflen;k++)
			textbuf[k] = textstr[k];
		    sprintf(dobuf,"printa(\" %s\")",textbuf);
		}
		else
		    sprintf(dobuf,"printa(\" %s\")",textstr);
		do_anim(&dobuf,offset,endanimline);
		/* detect key press */
		addm_io(0,1);
		if((keypress = igetch()) != I_TIMEOUT) {
		    addm_io(0,0);
		    break;
		}
		addm_io(0,0);
		usleep(scrolldelay * 1000);
	    }
	    if(keypress == I_TIMEOUT && eraselast) {
		sprintf(dobuf,"print(\" \")");
		do_anim(&dobuf,offset,endanimline);
	    }
	}

	if(keypress != I_TIMEOUT)
	    return keypress;
	else
	    return 0;
}

int
animate_plan(planfile)
FILE *planfile;
{
	prints("Plan:\n\n");
	animate_file(planfile,4,23,"Press any key to stop animation...");
	pressreturn(); 
	return 0;
}

int
animate_file(infile,startline,endline,prompt)
FILE *infile;
int startline;
int endline;
char prompt[STRLEN];
{
	int keypress=0;
	int doit=1;
        char buf[80];
	char inbuf[STRLEN+40];
	char dobuf[STRLEN+40];
        bzero(&buf,sizeof(buf));
	if(fgets(buf,sizeof(buf),infile) != NULL) {
	    if(!ci_strncmp(buf,"[anim]",6)) {
		move(endline,0);
		clrtoeol();
		prints(prompt);
		sprintf(inbuf,"clear()");
		do_anim(&inbuf,startline,endline);
	    }
	    else {
		int planpos = startline + 1;
		prints("%s",buf);
	    	while(fgets(buf,sizeof(buf),infile) != NULL) {
		    prints("%s",buf);
		    planpos++;
		    if(planpos > endline) break;
		}
		return;
	    }
	}
	else {
	    return;
	}
	while(fgets(inbuf,sizeof(inbuf),infile) != NULL) {
	    char *tmpptr;
	    if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
	    if(inbuf[0] == '[') {
		if(!ci_strncmp(inbuf,"[endanim]",9)) break;
		if(!ci_strncmp(inbuf,"[noanim]",8) || 
		  !ci_strncmp(inbuf,"[bothanim]",10)) {
		    int bothprint = YEA;
		    int donewithit=YEA;
		    if(!ci_strncmp(inbuf,"[noanim]",8))
			bothprint = NA;
#ifdef MOD_EXTRAS
		    if(bothprint || (usermodcfg.anim[0] == 'N')) 
#else
		    if(bothprint)
#endif
			move(startline,0);
		    while(fgets(inbuf,sizeof(inbuf),infile) != NULL) {
			if((tmpptr = rindex(inbuf,'\n'))) *tmpptr = '\0';
			if(inbuf[0] == '[') {
			    if(!ci_strncmp(inbuf,"[endanim]",9)) break;
			    if(!ci_strncmp(inbuf,"[startanim]",11)) {
				if(fgets(inbuf,sizeof(inbuf),infile) != NULL) {
				    if((tmpptr = rindex(inbuf,'\n')))
					*tmpptr = '\0';
				    donewithit = NA;
				}
				break;
			    }
			}
			else
#ifdef MOD_EXTRAS
			    if(bothprint || (usermodcfg.anim[0] == 'N')) 
#else
			    if(bothprint)
#endif
				prints("%s\n",inbuf);
		    }
		    if(donewithit) 
			break;
		}
	    }
#ifdef MOD_EXTRAS
            if(usermodcfg.anim[0] == 'Y') {
#endif
		if(inbuf[0] == 's') {
		    if(strlen(inbuf) > 6) {
			if(!ci_strncmp(inbuf,"scroll",6)) {
			    doit = animate_scroll(&inbuf,startline,endline);
			    if(doit > 0)
				keypress = doit;
			    if(doit) 
				break;
			}
			if(!ci_strncmp(inbuf,"spell",5)) {
			    doit = animate_spell(&inbuf,startline,endline);
			    if(doit > 0)
				keypress = doit;
			    if(doit) 
				break;
			}
		    }
		}
		if(inbuf[0] == 'w') {
		    if(strlen(inbuf) > 7) {
			if(!ci_strncmp(inbuf,"wscroll",7)) {
			    doit = animate_scroll(&inbuf,startline,endline);
			    if(doit)
				break;
			}
		    }
		}
		if(inbuf[0] == 'l') {
		    if(strlen(inbuf) > 8) {
			if(!ci_strncmp(inbuf,"linescr",7)) {
			    doit = animate_scroll(&inbuf,startline,endline);
			    if(doit > 0)
				keypress = doit;
			    if(doit)
				break;
			}
		    }
		}
		if(inbuf[0] == 'e') {
		    if(strlen(inbuf) > 6) {
			if(!ci_strncmp(inbuf,"erase",5)) {
			    doit = animate_spell(&inbuf,startline,endline);
			    if(doit > 0)
				keypress = doit;
			    if(doit)
				break;
			}
		    }
		}
		if(inbuf[0] == 'r') {
		    if(strlen(inbuf) > 7) {
			if(!ci_strncmp(inbuf,"rprint(",7)) {
			    doit = animate_random(&inbuf,startline,endline,infile);
			    if(doit > 0)
				keypress = doit;
			    if(doit)
				break;
			}
			if(!ci_strncmp(inbuf,"rplace(",7)) {
			    doit = animate_random(&inbuf,startline,endline,infile);
			    if(doit > 0)
				keypress = doit;
			    if(doit)
				break;
			}
			if(strlen(inbuf) > 12) {
			    if(!ci_strncmp(inbuf,"random_text(",12)) {
				doit = animate_random(&inbuf,startline,endline,infile);
				if(doit > 0)
				    keypress = doit;
				if(doit)
				    break;
			    }
			}
		    }
		}

		if(doit) {
		    sprintf(dobuf,"%s",inbuf);
		    if(!do_anim(&dobuf,startline,endline)) {
			sprintf(inbuf,"printclr(\"%s\")",dobuf);
			do_anim(&inbuf,startline,endline);
		    }
		    /* detect key press */
		    addm_io(0,1);
		    if((keypress = igetch()) != I_TIMEOUT) {
			addm_io(0,0);
			break;
		    }
		    addm_io(0,0);
		}
		doit = 1;
#ifdef MOD_EXTRAS
	    }
#endif
	}
	return keypress;
}

#endif
