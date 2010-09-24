/*

 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at 
 * http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/CDDL.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END

 * Copyright (c) 2004-2005 PathScale, Inc.  All rights reserved.
 * Use is subject to license terms.

file: readline.cc
created on: Tue Jan 18 13:08:14 PDT 2005
author: James Strother <jims@pathscale.com>

*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <curses.h>
#include <term.h>

#include "readline.h"
#include "utils.h"

struct termios st;
struct termios pr;


/* One readline per program
 ******************************************************
 */
Readline readl;


/* Signal handlers
 ******************************************************
 */
void t_resize(int sig) {
  if (sig != SIGWINCH) return;

  readl.resize();
}


/* Public functions
 ******************************************************
 */

#define DFLT_NROW 24
#define DFLT_NCOL 80

Readline::Readline() {
   tab_armed = false;
   dumb_term = false;
   expl_dumb = false;
   has_top_line=false;
   last = pos = 0;
   history = NULL;
   comp = NULL;
   active = true;
   loaded = false;
   prmpt = ""; 

   /* catch term resize */
   struct sigaction act;
   memset(&act, 0, sizeof(act));
   act.sa_handler = &t_resize;
   sigaction(SIGWINCH,&act,NULL);
}

void
Readline::setcompl(Completor* _comp) {
   comp = _comp;
}

void
Readline::setprompt(const std::string& s) {
   prmpt = s;
   plen = s.size();
}

void
Readline::sethistory(HList* h) {
   history = h;

   if (history != NULL) {
      hist_pos = h->end();
   }
}

void
Readline::setdumb(bool is) {
   expl_dumb = is;
}

const std::string&
Readline::getline() {
   /* reset */
   active = true;
   pos = last = 0;
   str.erase();

   load_term();

   /* hand out prompt */
   printf("%s", prmpt.c_str());
   fflush(stdout);

   /* dumbies are easy */
   if (dumb_term) {
      unload_term();
      int a = read_slow();
      while (a != '\n') {
         if (a == -1) {
            die();
         }

         str += a;
         a = read_slow();
      }
      return str;
   }

   /* main loop */
   while (active) {
      int a = read_slow();
      if (a == -1) {
         die();
      }

      recv_gen_char(a);
      write_flush();
   }

   unload_term();

   /* give back string */
   return str;
}

int
Readline::getchar(const char* allowed) {
   /* act independent, don't reset line */

   load_term();

   int a = read_slow(); 
   while (strchr(allowed, a) == NULL) {
      act_alert();
      a = read_slow(); 
   }

   unload_term();

   return a;
}

void
Readline::get_dims(int& row, int& col) {
   if (loaded) {
      row = nrow;
      col = ncol;
      return;
   }

   load_term();
   row = nrow;
   col = ncol;
   unload_term();
}

void
Readline::resize() {
  if (!loaded) return;

   /* get new term sizes */
   if ( setupterm(NULL, 1, NULL) != OK ) {
      return;
   }

   int fcol = tigetnum("cols");
   int frow = tigetnum("lines");

   /* optimized track through */
   if (fcol == (int)ncol &&
       frow == (int)nrow)
      return;

   /* clear screen */
   write_clear();

   /* absorb only sane values */
   if (fcol > 0) ncol = fcol;
   if (frow > 0) nrow = frow;

   /* restore state */
   write_sync();
   fflush(stdout);
}


/*  Pick up terminal capabilities
 ******************************************************
 */

void Readline::load_term() {
   const char* tname;

   /* check for term name */
   tname = getenv("TERM");
   if (tname == NULL || !strcmp(tname,"dumb")) {
      expl_dumb = true;
   }

   /* check for explicit dumb */
   if (expl_dumb) {
      dumb_term = true;
      return;
   }

   /* get the default settings */
   if ( tcgetattr(0, &st) ) {
      dumb_term = true;
      return;
   }

   /* preserve default attributes */
   memcpy(&pr,&st,sizeof(struct termios));

   /* set terminal input modes */
   st.c_iflag &=~ ICRNL;   /* no CR->NL translation */
   st.c_iflag &=~ IGNCR;   /* no ignoring CR chars */
   st.c_iflag &=~ INLCR;   /* no NL->CR translation */

   /* set terminal local modes */
   st.c_lflag &=~ ICANON;  /* no canonical mode */
   st.c_lflag &=~ ECHO;    /* no echoing chars */

   /* do input settings now */
   if ( tcsetattr(0, TCSANOW, &st) ) {
      dumb_term = true;
      return;
   }

   /* set terminal output modes */
   st.c_oflag &=~ OCRNL;   /* no CR->NL translation */
   st.c_oflag &=~ ONLRET;  /* no CR filtering away */
#ifdef OXTABS
   st.c_oflag &=~ OXTABS;   /* no tab->spaces convert */
#else
   st.c_oflag &=~ XTABS;   /* no tab->spaces convert */
#endif

   /* do output settings on flush */
   if ( tcsetattr(0, TCSADRAIN, &st) ) {
      dumb_term = true;
      return;
   }

   /* initialise terminal */
   if ( setupterm(NULL, 1, NULL) != OK ) {
      dumb_term = true;
      return;
   }

   ncol = tigetnum("cols");
   if (ncol <= 0) ncol = DFLT_NCOL;

   nrow = tigetnum("lines");
   if (nrow <= 0) nrow = DFLT_NROW;

   /* find modifier strings */
   s_cursor_down = tigetstr("cud1");
   s_cursor_up = tigetstr("cuu1");
   s_cursor_left = tigetstr("cub1");
   s_cursor_right = tigetstr("cuf1");
   s_cursor_home = tigetstr("home");
   s_clear_screen = tigetstr("clear");
   s_delete_char  = tigetstr("dch1"); 
   s_delete_line = tigetstr("dl1");
   s_enter_insert = tigetstr("smir");
   s_exit_insert = tigetstr("rmir");
   s_audible_bell = tigetstr("bel");

#define BAD_CAP(X) (X == (char*)-1 || X == 0)
   if (BAD_CAP(s_cursor_down)  ||
       BAD_CAP(s_cursor_up)    ||
       BAD_CAP(s_cursor_left)  ||
       BAD_CAP(s_cursor_right) ||
       BAD_CAP(s_cursor_home)  ||
       BAD_CAP(s_clear_screen) ||
       BAD_CAP(s_delete_char)  ||
       BAD_CAP(s_delete_char)  ||
       BAD_CAP(s_enter_insert) ||
       BAD_CAP(s_exit_insert)  ||
       BAD_CAP(s_audible_bell)) {
       dumb_term = true;
   }
#undef BAD_CAP

   /* mark object as loaded */
   loaded = true;
}

void Readline::unload_term() {
   /* check for explicit dumb */
   if (expl_dumb || dumb_term) {
      dumb_term = true;
      return;
   }

   /* mark object as unloaded */
   loaded = false;

   /* restore the prev attributes */
   memcpy(&st,&pr,sizeof(struct termios));

   /* do input settings now */
   tcsetattr(0, TCSANOW, &st);
}


/* Low level output operations
 ******************************************************
 */

void Readline::write_insr(int a) {
   unsigned eff_size = last+plen+1;
   std::string b(1,(char)a);

   /* multiline homing */
   if (eff_size >= ncol
       && !await_sync) {
       write_clear();
       await_sync=true;
   }

   /* maintain str */
   str.insert(pos,b);
   pos++; last++;

   /* multililne gets flush */
   if (await_sync) return;

   /* optimized version */
   if (pos == last) {
      putchar(a);
      return;
   }

   /* only for real terms */
   tputs(s_enter_insert, 1, putchar);
   putchar(a);
   tputs(s_exit_insert, 1, putchar);
}

void Readline::write_insr(const char* a) {
   unsigned eff_size = last+plen+strlen(a);
   std::string b(a);

   /* multiline homing */
   if (eff_size >= ncol
       && !await_sync) {
       write_clear();
       await_sync=true;
   }

   /* maintain str */
   str.insert(pos,b);
   pos += b.size();
   last += b.size();

   /* multililne gets flush */
   if (await_sync) return;

   /* optimized version */
   if (pos == last) {
      printf("%s", a);
      return;
   }

   /* only for real terms */
   tputs(s_enter_insert, 1, putchar);
   printf("%s", a);
   tputs(s_exit_insert, 1, putchar);
}


void Readline::write_move(int a) {
   unsigned eff_size = last+plen;

   /* multiline homing */
   if (eff_size >= ncol
       && !await_sync) {
       write_clear();
       await_sync=true;
   }

   /* maintain str */
   pos += a;

   /* multililne gets flush */
   if (await_sync) return;

   /* only for real terms */
   while (a > 0) {
      tputs(s_cursor_right, 1, putchar);
      --a;
   }
   while (a < 0) {
      tputs(s_cursor_left, 1, putchar);
      ++a;
   }
}

void Readline::write_dele() {
   unsigned eff_size = last+plen;

   /* multiline homing */
   if (eff_size >= ncol
       && !await_sync) {
       write_clear();
       await_sync=true;
   }

   /* maintain str */
   str.erase(pos,1);
   last--;

   /* multililne gets flush */
   if (await_sync) return;

   /* only for real terms */
   tputs(s_delete_char, 1, putchar);
}

void Readline::write_bell() {

   /* only for real terms */
   tputs(s_audible_bell, 1, putchar);
}

void Readline::write_clear() {

   unsigned lrow = (last+plen) / ncol;
   unsigned crow = (pos+plen) / ncol;
   unsigned ccol = (pos+plen) % ncol;

   /* move to the bottom row */
   for (unsigned i=0; i<(lrow-crow); i++) {
      tputs(s_cursor_down, 2, putchar);
   }

   /* move up, deleting rows */
   for (unsigned i=0; i <= lrow; i++) {
      tputs(s_delete_line, 2, putchar);

      if ( i != lrow ) {
         tputs(s_cursor_up, 2, putchar);
      }
   }

   /* move to first column */
   for (unsigned i=0; i<ccol; i++) {
      tputs(s_cursor_left, 2, putchar);
   }
}

void Readline::write_wipe() {

   /* easy, just one instruction */
   tputs(s_clear_screen, nrow, putchar);
}

void Readline::write_sync() {

   unsigned lrow = (last+plen) / ncol;
   unsigned crow = (pos+plen) / ncol;
   unsigned ccol = (pos+plen) % ncol;

   /* get the full display string */
   std::string m = prmpt + str;
   std::string r;

   /* refresh all extant lines */
   for (unsigned i=0; i <= lrow; i++) {
      r = m.substr(i*ncol,ncol);

      tputs(s_enter_insert, 1, putchar);
      printf("%s", r.c_str());
      tputs(s_exit_insert, 1, putchar); 

      if ( i != lrow ) {
         tputs(s_cursor_down, 2, putchar);
      }
   }

   /* move back to current column */
   int i = r.size() - ccol;
   while (i > 0) {
      tputs(s_cursor_left, 1, putchar);
      --i;
   }
   while (i < 0) {
      tputs(s_cursor_right, 1, putchar);
      ++i;
   }

   /* move back to current row */
   for (unsigned i=0; i<(lrow-crow); i++) {
      tputs(s_cursor_up, 2, putchar);
   }
}

void Readline::write_flush() {
   if (!await_sync) {
     fflush(stdout);
     return;
   }

   write_sync();
   fflush(stdout);  
}


/* Low level input operations
 ******************************************************
 */

int Readline::read_slow() {
/* use for plain read */
   unsigned char buf;
   int i;

   /* grab a character */
   do {
      errno = 0;
      i = read(0, &buf, 1);
   } while (i==-1 && errno==EINTR);

   /* should always get */
   return i==1? buf : -1;
}

int Readline::read_fast() {
/* use for sequence timing */
   unsigned char buf;

   /* set timeout */
   st.c_cc[VMIN] = 0;
   st.c_cc[VTIME] = 5;
   tcsetattr(0, TCSANOW, &st);

   /* grab a character */
   int i = read(0, &buf, 1);

   /* remove timeout */
   st.c_cc[VMIN] = 1;
   st.c_cc[VTIME] = 0;
   tcsetattr(0, TCSANOW, &st);

   /* return if caught */
   return i==1? buf : -1;
}


/* Handy string functions
 ******************************************************
 */

bool Readline::in_word(unsigned a) {
   if (a >= last) return true;

   return isalnum(str[a]);
}


/* String op to low-level translation
 ******************************************************
 */

void Readline::act_echo(int a) {
   write_insr(a);
}

void Readline::act_alert() {
   write_bell();
}

void Readline::act_clear_scrn() {
   act_dele_line();
}

void Readline::act_do_nothing() {
}

void Readline::act_take_line() {
   active = false;
   printf("\n");
}

void Readline::act_move_bchar() {
   if (pos == 0) {
      return;
   }

   write_move(-1);
}

void Readline::act_move_fchar() {
   if (pos == last) {
      return;
   }

   write_move(1);
}

void Readline::act_move_bword() { 
   while (pos > 0 && !in_word(pos-1)) {
      write_move(-1);
   }
   while (pos > 0 && in_word(pos-1)) {
      write_move(-1);
   }
}

void Readline::act_move_fword() {
   while (pos < last && !in_word(pos)) {
      write_move(1);
   }
   while (pos < last && in_word(pos)) {
      write_move(1);
   }
}

void Readline::act_move_bline() {
   while (pos > 0) {
      write_move(-1);
   }
}

void Readline::act_move_fline() {
   while (pos < last) {
      write_move(1);
   }
}

void Readline::act_dele_bchar() {
   if (pos == 0) {
      return;
   }

   write_move(-1);
   write_dele();
}

void Readline::act_dele_fchar() {
   if (pos == last) {
      return;
   }

   write_dele();
}

void Readline::act_dele_bword() {
   while (pos > 0 && !in_word(pos-1)) {
      act_dele_bchar();
   }
   while (pos > 0 && in_word(pos-1)) {
      act_dele_bchar();
   }
}

void Readline::act_dele_fword() {
   while (pos < last && !in_word(pos)) {
      act_dele_fchar();
   }
   while (pos < last && in_word(pos)) {
      act_dele_fchar();
   }
}

void Readline::act_dele_bline() {
   while (pos > 0) {
      act_dele_bchar();
   }
}

void Readline::act_dele_fline() {
   while (pos < last) {
      act_dele_fchar();
   }
}

void Readline::act_dele_line() {
   act_move_bline();
   act_dele_fline();
}

void Readline::act_swap_char() {
   /* in first column */
   if (pos == 0) {
      act_alert();
      return;
   }

   /* swap of 1 char string */
   if (pos == 1 && last == 1) {
      act_alert();
      return;
   }

   /* at the end */
   if (pos == last) {
      act_move_bchar();
   }

   /* grab prior */
   char b = str[pos-1];

   /* do the swap */
   act_dele_bchar();
   if (pos != last ) {
      act_move_fchar();
   }
   act_echo(b);
}

void Readline::act_swap_word() {
   unsigned f_strt, f_last;
   unsigned s_strt, s_last;
   unsigned cpos = pos;

/* This looks overbuilt, it's not. Emacs
 * actually does some pretty interesting
 * manuevers to get word boundaries.
 */ 

   /* scan back, first doesn't count */
   if (cpos > 0 && in_word(cpos)) {
      --cpos;
   }

   /* scan back, thru trailing space */
   while (cpos > 0 && !in_word(cpos)) {
      --cpos;
   }

   /* scan back, thru word characters */
   while (cpos > 0 && in_word(cpos)) {
      --cpos;
   }

   /* scan forw, thru any whitespace */
   while (cpos < last && !in_word(cpos)) {
      ++cpos;
   }
   f_strt = cpos;

   /* scan forw, thru word characters */
   while (cpos < last && in_word(cpos)) {
      ++cpos;
   }
   f_last = cpos;

   /* scan forw, thru any whitespace */
   while (cpos < last && !in_word(cpos)) {
      ++cpos;
   }
   s_strt = cpos;

   /* scan forw, thru word characters */
   while (cpos < last && in_word(cpos)) {
      ++cpos;
   }
   s_last = cpos;

   /* only move for zero-length words */
   if (f_strt == f_last || s_strt == s_last) {
      write_move(f_strt-pos);
      return;
   }

   /* extract the first word */
   std::string f_word;
   f_word = str.substr(f_strt,f_last-f_strt);

   /* extract the second word */
   std::string s_word;
   s_word = str.substr(s_strt,s_last-s_strt);

   /* find position difference */
   int dif = s_word.size()-f_word.size();
   s_strt += dif; s_last += dif;

   /* make change in string data */
   str.replace(f_strt,f_last-f_strt,s_word);
   str.replace(s_strt,s_last-s_strt,f_word);

   /* blow away displayed version */
   write_clear();

   /* show the new version */
   write_sync();

   /* move to end of second */ 
   write_move(s_last-pos-dif);
}

void Readline::act_uppc_word() {
   while (pos < last && !in_word(pos)) {
      act_move_fchar();
   }
   while (pos < last && in_word(pos)) {
      char b = str[pos];
      act_dele_fchar();
      act_echo(toupper(b));
   }
}

void Readline::act_lowc_word() {
   while (pos < last && !in_word(pos)) {
      act_move_fchar();
   }
   while (pos < last && in_word(pos)) {
      char b = str[pos];
      act_dele_fchar();
      act_echo(tolower(b));
   }
}

void Readline::act_hist_prev() {
   /* no history available */
   if (history == NULL) {
      act_alert();
      return;
   }

   /* check for top of list */
   if (hist_pos == history->begin()) {
      act_alert();
      return;
   }

   /* save the current */
   if (!has_top_line) {
       top_line = str;
       has_top_line=true;
   }

   /* move up one line */
   hist_pos--;

   /* replace the display */
   write_clear();
   str = *hist_pos;
   pos = str.size();
   last = str.size();
   write_sync();
}

void Readline::act_hist_next() {
   /* no history available */
   if (history == NULL) {
      act_alert();
      return;
   }

   /* move down one line */
   if (hist_pos != history->end()) {
      hist_pos++;
   }

   /* check for list bottom */
   if (hist_pos == history->end()) {
      /* check for current */
      if (!has_top_line) {
         act_alert();
         return;
      }

      write_clear();
      str = top_line;
      pos = str.size();
      last = str.size();
      write_sync();
      has_top_line=false;
      return;
   }

   /* replace the display */
   write_clear();
   str = *hist_pos;
   pos = str.size();
   last = str.size();
   write_sync();
}

void Readline::act_hist_first() {
   /* no history available */
   if (history == NULL) {
      act_alert();
      return;
   }

   /* save the current */
   if (!has_top_line) {
       top_line = str;
       has_top_line=true;
   }

   /* set the history position */
   hist_pos = history->begin();
   if (hist_pos == history->end()) {
      act_alert();
      return;
   }

   /* replace the display */
   write_clear();
   str = *hist_pos;
   pos = str.size();
   last = str.size();
   write_sync();
}

void Readline::act_hist_last() {
   /* no history available */
   if (history == NULL) {
      act_alert();
      return;
   }

   /* set the history position */
   hist_pos = history->end();

   /* check for saved line */
   if (!has_top_line) {
      act_alert();
      return;
   }

   /* replace the display */
   write_clear();
   str = top_line;
   pos = str.size();
   last = str.size();
   write_sync();
   has_top_line=false;
}

void Readline::act_hist_word() {
   unsigned f_start, f_stop;

   /* no history available */
   if (history == NULL) {
      act_alert();
      return;
   }

   /* check for beginning */
   if (hist_pos == history->begin()) {
      act_alert();
      return;
   }

   /* get previous string */
   HList::iterator j = hist_pos;
   std::string a = *--j;
   if (a.size() == 0) {
      return;
   }

   /* scan through trailing spaces */
   unsigned c = a.size()-1;
   while (c > 0 && isspace(a[c])) {
       --c;
   }
   f_stop = c;

   /* scan through word characters */
   while (c > 0 && isalnum(a[c])) {
       --c;
   }

   /* scan through leading space */
   while (c < a.size() && isspace(a[c])) {
       ++c;
   }
   f_start = c;

   /* bail for zero-length substitutes */
   if (f_stop == f_start) {
      return;
   }

   /* show the selected substring */
   a = a.substr(f_start, f_stop-f_start+1);
   write_insr(a.c_str());
}

void Readline::act_compl_insr() {
   /* no completion available */
   if (comp == NULL) {
      act_alert();
      return;
   }

   /* query the completor */
   std::string a;
   a = comp->complete(str,pos);

   /* do the actual insert */ 
   write_insr(a.c_str());

   /* add space for perfect */
   if (comp->num_matches() == 1) {
     write_insr(' ');
   }
}

void Readline::act_compl_show() {
   /* no completion available */
   if (comp == NULL) {
      act_alert();
      return;
   }

   /* check for no completions */
   if (comp->num_matches() == 0) {
      act_alert();
      return;
   }

   /* drop term */
   unload_term();
   putchar('\n');

   /* query completor */
   comp->list_matches();

   /* restore term */
   load_term();
   write_sync();
}

void Readline::act_tilde_exp() {
   /* blow away old */
   write_clear();

   /* query for new */
   Utils::expand_path(str);

   /* post new display */
   pos = str.size();
   last = str.size();
   write_sync();
}

void Readline::act_refresh_all() {
   write_wipe();
   write_sync();
}

/* Keycode to string op translation
 ******************************************************
 */

void Readline::recv_escb_char() {
/* this handles 'Esc-[' sequences */
   int A;

   A = read_fast();

   /* look for arrow keys */
   switch (A) {
   case 'A': act_hist_prev();   return;  /* UP    */
   case 'B': act_hist_next();   return;  /* DOWN  */
   case 'C': act_move_fchar();  return;  /* RIGHT */
   case 'D': act_move_bchar();  return;  /* LEFT  */
   }

   /* look for page dn key */
   if (A == 0x36) {
      A = read_fast();

      if (A == 0x7e) {
         act_hist_next();
         return;
      } 
      act_alert();
   }

   /* look for page up key */
   else if (A == 0x35) {
      A = read_fast();

      if (A == 0x7e) {
         act_hist_prev();
         return;
      }
      act_alert();
   } 

   /* look for delete key */
   else if (A == 0x33) {
      A = read_fast();

      if (A == 0x7e) {
         act_dele_fchar();
         return;
      }
      act_alert();
   }

   /* look for Ctrl-arrow key */
   else if (A == 0x31) {
      if (read_fast() != 0x3b ||
          read_fast() != 0x35) {
         act_alert();
         return; 
      }

      A = read_fast();
      if (A == 0x43) {
         act_move_fword();
      } else if (A == 0x44) {
         act_move_bword();
      } else {
         act_alert();
      }
   }

   /* fall back on beep */
   else {
      act_alert();
   }
}

void Readline::recv_esco_char() {
/* this handles 'Esc-O' sequences */
   int A;

   A = read_fast();
   switch (A) {
   case 0x46: act_move_fline();  return;  /* END  */
   case 0x48: act_move_bline();  return;  /* HOME */
   }

   act_alert();
}

void Readline::recv_escc_char() {
   int A;

   /* single key press */
   A = read_fast();
   switch (A) {
   case '[':  recv_escb_char();  return;  /* Meta-[ */
   case 'O':  recv_esco_char();  return;  /* Meta-O */
   }

   /* check for sequence */
   if (A == -1) {
      A = read_slow();
   }

   /* single key metas */
   switch (A) {
   case 'b':  act_move_bword();  return;  /* Meta-B */
   case 'd':  act_dele_fword();  return;  /* Meta-D */
   case 'f':  act_move_fword();  return;  /* Meta-F */
   case 'l':  act_lowc_word();   return;  /* Meta-L */
   case 't':  act_swap_word();   return;  /* Meta-T */
   case 'u':  act_uppc_word();   return;  /* Meta-U */
   case '<':  act_hist_first();  return;  /* Meta-< */
   case '>':  act_hist_last();   return;  /* Meta-> */
   case '.':  act_hist_word();   return;  /* Meta-. */
   case '?':  act_compl_show();  return;  /* Meta-? */
   case '=':  act_compl_show();  return;  /* Meta-= */
   case '&':  act_tilde_exp();   return;  /* Meta-& */
   case '~':  act_tilde_exp();   return;  /* Meta-~ */
   case 0x7f: act_dele_bword();  return;  /* Meta-BS */
   }

   act_alert();
}


void Readline::recv_gen_char(int A) {
   /* Note: Several special keys map to control
    * sequences that are treated generically.
    *   ENTER is Ctrl-M
    *   TABSET is Ctrl-I
    *   BACKSPACE is Ctrl-?
    */

   /* special case TAB */
   if (A == 0x09) {
      if (tab_armed) {
         act_compl_show();   
      } else {
         act_compl_insr();
      }
      tab_armed = true;
      return;
   }

   /* special case Ctrl-D */
   if (A == 0x04 && str == "") {
      die();
   }

   /* reset states */
   await_sync = false;
   tab_armed = false;

   /* select out special */
   switch (A) {
   case 0x01:  act_move_bline();  return;  /* Ctrl-A */
   case 0x02:  act_move_bchar();  return;  /* Ctrl-B */
   case 0x04:  act_dele_fchar();  return;  /* Ctrl-D */
   case 0x05:  act_move_fline();  return;  /* Ctrl-E */
   case 0x06:  act_move_fchar();  return;  /* Ctrl-F */
   case 0x07:  act_do_nothing();  return;  /* Ctrl-G */
   case 0x08:  act_dele_bchar();  return;  /* Ctrl-H */
   case 0x0a:  act_take_line();   return;  /* Ctrl-J */
   case 0x0b:  act_dele_fline();  return;  /* Ctrl-K */
   case 0x0c:  act_refresh_all(); return;  /* Ctrl-L */
   case 0x0d:  act_take_line();   return;  /* Ctrl-M */
   case 0x0e:  act_hist_next();   return;  /* Ctrl-N */
   case 0x0f:  act_take_line();   return;  /* Ctrl-O */
   case 0x10:  act_hist_prev();   return;  /* Ctrl-P */
   case 0x14:  act_swap_char();   return;  /* Ctrl-T */
   case 0x15:  act_dele_bline();  return;  /* Ctrl-U */
   case 0x17:  act_dele_bword();  return;  /* Ctrl-W */
   case 0x7f:  act_dele_bchar();  return;  /* Ctrl-? */
   case 0x1b:  recv_escc_char();  return;  /* ESCAPE */
   }

   /* check if printable */
   if (isprint(A)) {
      act_echo(A);
   } else {
      act_alert();
   }
}

void Readline::die() {
   unload_term();
   putchar('\n'); 
   exit(0);
}

#if 0
std::string xi ("bobby");
std::string zed ("joey");

class TempCompl : public Completor {
  const std::string& complete(std::string text, int chr) {
     return xi;
  }
  void list_matches() {
     printf("%s\n", xi.c_str());
     printf("%s\n", zed.c_str());
     fflush(stdout);
  }
  unsigned num_matches() {
     return 1;
  }
};

int main() {
   TempCompl comp;
   std::list<std::string> alpha;

   alpha.push_back("hello");
   alpha.push_back("there");
   alpha.push_back("bobby");

   readl.setcompl(&comp);
   readl.setprompt("pathdb> ");
   readl.sethistory(&alpha);

   /* enter the read loop */
   //for (;;) {
      std::string a = r.getline();
      printf("%s\n", a.c_str());
   // }

}
#endif
