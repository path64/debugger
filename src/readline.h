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

file: readline.h
created on: Tue Jan 18 13:08:14 PDT 2005
author: James Strother <jims@pathscale.com>

*/

#ifndef _READLINE_H_
#define _READLINE_H_

#include <string>
#include <list>

class CommandInterpreter ;

class Completor {
public:
   virtual ~Completor() { }
   virtual std::string complete (std::string text, int ch) = 0;
   virtual void list_matches() = 0;
   virtual unsigned num_matches() = 0;
};

class Readline {
public:
   typedef std::list<std::string> HList;

   Readline();

   void setcompl(Completor* comp);
   void setprompt(const std::string&);
   void sethistory(HList * history);
   void setdumb(bool is_dumb);
   void setcli(CommandInterpreter *c);

   const std::string& getline();
   int getchar(const char* allowed);
   void get_dims(int& row, int& col);

   void load_term();
   void unload_term();

protected:
   friend void t_resize(int);
   void resize();

private:
   /* low level ops */
   void write_insr(int);
   void write_insr(const char*);
   void write_move(int);
   void write_dele();
   void write_bell();
   void write_clear();
   void write_wipe();
   void write_sync();
   void write_flush();

   /* terminal readers */
   int read_slow();
   int read_fast();

   /* string helpers */
   bool in_word(unsigned pos);
   
   /* string operators */
   void act_alert();
   void act_echo(int ch);
   void act_clear_scrn();
   void act_do_nothing();
   void act_take_line();
   void act_move_bchar();
   void act_move_fchar();
   void act_move_bword();
   void act_move_fword();
   void act_move_bline();
   void act_move_fline();
   void act_dele_bchar();
   void act_dele_fchar();
   void act_dele_bword();
   void act_dele_fword();
   void act_dele_bline();
   void act_dele_fline();
   void act_dele_line();
   void act_swap_char();
   void act_swap_word();
   void act_uppc_word();
   void act_lowc_word();
   void act_hist_prev();
   void act_hist_next();
   void act_hist_first();
   void act_hist_last();
   void act_hist_word();
   void act_compl_insr();
   void act_compl_show();
   void act_tilde_exp();
   void act_refresh_all();

   /* char receivers */
   void recv_escb_char();
   void recv_esco_char();
   void recv_escc_char();
   void recv_gen_char(int);

   /* exit function */
   void die();

   /* str members */
   std::string str;
   unsigned pos;
   unsigned last;

   /* state members */
   bool await_sync;
   bool tab_armed;
   bool dumb_term;
   bool expl_dumb;
   bool loaded;
   bool active;

   /* prompt members */
   std::string prmpt;
   unsigned plen;

   /* history members */
   HList * history;
   HList::iterator hist_pos;
   bool has_top_line;
   std::string top_line;

   /* completion members */
   Completor* comp;

   /* term members */
   unsigned ncol;
   unsigned nrow;

   /* list of capabilities */
   char* s_cursor_down;
   char* s_cursor_up;
   char* s_cursor_left;
   char* s_cursor_right;
   char* s_cursor_home;
   char* s_clear_screen;
   char* s_delete_char;
   char* s_delete_line;
   char* s_enter_insert;
   char* s_exit_insert; 
   char* s_audible_bell;
   CommandInterpreter *cli;
};

/* give handle for global */
extern Readline readl;

#endif
