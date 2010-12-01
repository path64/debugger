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

file: cli.cc
created on: Fri Aug 13 11:07:34 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include "cli.h"

#include "utils.h"
#include "pcm.h"
#include <string.h>
#include <string>
#include "expr.h"
#include "pstream.h"
#include "readline.h"
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include "version.h"
#include "process.h"
#include <dirent.h>
#include "arch.h"
#include <algorithm>
#include <string.h>
#include <sys/sysctl.h>

extern char **environ ;

std::string CommandCompletor::complete (std::string text, int ch) {
    return cli->complete_command (text, ch) ;
}

void CommandCompletor::set_leadin(const std::string& s) {
    leadin = s;
}

void CommandCompletor::set_matches(std::vector<std::string> &m) {
    std::vector<std::string>::iterator end;

    // set completor matches
    matches = m;

    // filter out non-unique matches
    std::sort (matches.begin(), matches.end()) ;
    end = std::unique (matches.begin(), matches.end()) ;
    matches.erase (end, matches.end()) ;
}

void CommandCompletor::list_matches_bare() {
    for (unsigned i=0; i < matches.size(); i++) {
       printf("%s%s\n", leadin.c_str(), matches[i].c_str());
    }
}

void CommandCompletor::list_matches () {

    int nmatches = matches.size() ;
    if (nmatches > 100) {
        char buf[256] ;
        snprintf (buf, sizeof(buf), "List all %d matches", nmatches) ;
        if (!cli->confirm (NULL, buf)) {
            return ;
        }
    }
    int width = cli->get_int_opt(PRM_WIDTH) ;
    int maxlen = 0 ;
    for (unsigned int i = 0 ; i < matches.size() ; i++) {
        int len = matches[i].size() + 4 ;
        if (len > maxlen) {
            maxlen = len ;
        }
    }
    int x = 0 ;
    if (maxlen > width) {
        maxlen = width ;
    }

    os.begin_paging();
    try {
        for (unsigned int i = 0 ; i < matches.size() ; i++) {
            std::string s = matches[i] ;
            if ((x + maxlen) >= width) {
                os.print ("\n") ;
                x = 0 ;
            } 
            os.print ("%s", s.c_str()) ; 
            int spaces = maxlen - s.size() ;
            while (spaces-- > 0) {
                os.print (" ") ;
            }
            x += maxlen ;
        }
        os.print ("\n") ;
    } catch (QuitOutput q) {    
        os.print ("Quit\n") ;
    }
    os.end_paging();

}

uint CommandCompletor::num_matches () {
    return matches.size() ;
}

void CommandCompletor::reset() {
    matches.clear() ;
}

Command::Command (CommandInterpreter *cli, ProcessController *p, const char **commands) :cli(cli), pcm(p), commands(commands) {
}

// split a command into parts, mindful of strings in quotes
std::vector<std::string> Command::split (std::string str) {
    std::vector<std::string> vec ;
    unsigned int i = 0 ;
    while (i < str.size()) {
        while (i < str.size() && isspace (str[i])) i++ ;                // skip spaces to start
        if (i >= str.size()) {
            break ;
        }
        std::string s ;
        if (str[i] == '"') {
            while (i < str.size() && str[i] != '"') {
                s += str[i++] ;
            }
            i++ ;
        } else if (str[i] == '\'') {
            while (i < str.size() && str[i] != '\'') {
                s += str[i++] ;
            }
            i++ ;
        } else {
            while (i < str.size() && !isspace(str[i])) {
                s += str[i++] ;
            }
        }
        vec.push_back (s) ;
    }
    return vec ;
}


// trim white space off both ends of a string
std::string Command::trim (std::string s) {
    int begin = 0 ;
    while (begin < (int)s.size() && isspace (s[begin])) begin++ ;
    int end = (int)s.size() - 1 ;
    while (end > begin && isspace (s[end])) end-- ;
    return s.substr (begin, end-begin+1) ;
}

void Command::check (std::string root, CommandVec &result) {
    for (int i = 0 ; commands[i] != NULL ; i++) {
        if (root ==  commands[i]) {
            result.push_back (Match (commands[i], this)) ;
        }
    }
}

// extract a number from a string and write terminal index to ch
int Command::extract_number (std::string s, int &ch) {
    skip_spaces (s, ch) ;
    int n = 0 ;
    if (ch < (int)s.size()) {
        while (ch < (int)s.size() && isdigit (s[ch])) {
            n = n * 10 + s[ch++] - '0' ;
        }
    }
    return n ;
}

void Command::skip_spaces (std::string s, int &ch) {
    while (ch < (int)s.size() && isspace (s[ch])) ch++ ;
}


std::string Command::extract_word (const std::string& s, int &ch) {
    skip_spaces (s, ch) ;
    std::string r ;
    while (ch < (int)s.size() && !isspace (s[ch])) {
        r += s[ch++] ;
    }
    return r ;
}

std::string Command::expand_env (CommandInterpreter* cli, const std::string& s) {
/* This function expands an environmental variable using the
 * variable mapping stored in the CommandInterpreter.  This
 * should be called on a token returned by extract_qtoken.
 */

   std::string r;
   int i;
    
   for (i = 0 ; i < (int)s.size() ; i++) {
       std::string var;

       /* check for escaped dollar, these are leftover
        * from extract_qtoken because we can deal with
        * them more intelligently over here
        */
       if ( s[i] == '\\' &&
            i < (int)s.size() - 1 &&
            s[i+1] == '$' )
       {
          r += "$";
          i += 2;
          continue;
       }
 
       /* simple append case */
       if ( s[i] != '$' ) {
          r += s[i] ;
          continue;
       }
       i++; /* skip $ symbol */

       /* check for end of line */
       if ( i == (int)s.size() ) {
          r += "$";
          break;
       }

       /* Hereafter, must be variable */
       if ( s[i] == '{' ) {
           i++; /* skip first '{' symbol */
           while (i < (int)s.size() && s[i] != '}') {
              var += s[i++] ;
           }

           if ( s[i] != '}' ) {
              throw Exception("unmatched '{' in variable expansion");
           }

           i++; /* skip last '}' symbol */
        } else {
           while ( i < (int)s.size() ) {
              if (isalnum(s[i]) || s[i] == '_') {
                 var += s[i++] ;
              } else {
                 break;
              }
           }
        }

        const char* v = cli->get_env(var);
        if (v != NULL) {
           r += v ;
        }
   }
   return r ;
}

bool Command::extract_qtoken(const std::string& s, std::string& tok, int& ch) {
/* This function extracts a single quoted token, returning true
 * if the token should be macro-expanded and false if should not.
 */

  /* check double quote token */
  if ( s[ch] == '\"' ) {
     ch++; /* skip the first quote */

     while ( ch < (int)s.size() ) {
        /* check for escaped chars */
        if ( s[ch] == '\\' ) {
           if (ch < (int)s.size() - 1 &&
               ( s[ch+1] == '\"' ||
                 s[ch+1] == '\\' ||
                 s[ch+1] == '\''
               )) {
              ch++;
           }
           tok += s[ch++];
           continue;
        }

        /* check for closing quote */
        if ( s[ch] == '\"' ) {
           break;
        }

        /* plain vanilla append */
        tok += s[ch++];
     }

     /* check for mismatch */
     if ( s[ch] != '\"') {
        throw Exception("mismatched quotes in string");
     }

     ch++; /* skip last quote */
     return true;
  }


  /* check single quote token */
  if ( s[ch] == '\'' ) {
     ch++; /* skip the first quote */

     while ( ch < (int)s.size() ) {
        /* there is escape from paradise */

        /* check for closing quote */
        if ( s[ch] == '\'' ) {
           break;
        }

        /* plain vanilla append */
        tok += s[ch++];
     }

     /* check for mismatch */
     if ( s[ch] != '\'') {
        throw Exception("mismatched quotes in string");
     }

     ch++; /* skip last quote */
     return false;
  }


  /* check whitespace token */
  while ( ch < (int)s.size() ) {
     if ( s[ch] == '\"' ||
          s[ch] == '\'' ||
          isspace(s[ch])
        ) break;

     tok += s[ch++];
  }
  return true;
}

std::string Command::extract_shell(CommandInterpreter* cli, const std::string& s, int &ch) {
/* This function extracts a shell-style argument, performing 
 * the desired expansion of environmental variables.
 */

   skip_spaces(s, ch);
   std::string r;

   if (extract_qtoken(s, r, ch)) {
      r = expand_env(cli, r);
   }

   return r; 
}

std::string Command::unescape (std::string s) {
    std::string r ;
    for (unsigned int i = 0 ; i < s.size() ; i++) {
        if (s[i] == '\\') {
            i++ ;
            switch (s[i]) {
            case 'n':
                r += '\n' ;
                break ;
            case 'r':
                r += '\r' ;
                break ;
            case 't':
                r += '\t' ;
                break ;
            case 'a':
                r += '\a' ;
                break ;
            case 'b':
                r += '\b' ;
                break ;
            case 'v':
                r += '\v' ;
                break ;
            default:
                r += s[i] ;
                break ;
            }
        } else {
           r += s[i] ;
        }
    }
    return r ;
}

void Command::check_abbreviation (std::string root, CommandVec &result) {
    for (int i = 0 ; commands[i] != NULL ; i++) {
        if (strncmp (root.c_str(), commands[i], root.size()) == 0) {
            result.push_back (Match (commands[i], this)) ;
        }
    }
}

static void check_junk (std::string s, int end) {
    while (end < (int)s.size() && isspace (s[end])) end++ ;
    if (end != (int)s.size()) {
        throw Exception ("Junk at end of arguments.") ;
    }
}

static Address get_number (ProcessController *pcm, std::string expr, int def, int &end) {
    if (expr.size() > 0) {
        int start = end ;
        if (end != 0) {
            expr = expr.substr(end) ;
        }
        Address res = pcm->evaluate_expression(expr, end, true) ;
        end += start ;
        return res ;
    }
    return def ;
}

static Address get_number (ProcessController *pcm, std::string expr, int def) {
    int end = 0 ;
    Address res =  get_number (pcm, expr, def, end) ;
    check_junk (expr, end) ;
    return res ;
}


static Address get_address (ProcessController *pcm, std::string expr, int &end) {
    if (expr.size() > 0) {
        return pcm->evaluate_expression(expr, end, true) ;
    }
    return 0 ;
}

static Address get_address (ProcessController *pcm, std::string expr) {
    int end = 0;
    Address a = get_address (pcm, expr, end) ;
    check_junk (expr, end) ;
    return a ;
}



void Command::get_address_arg (std::string tail, std::vector<Address> &result, bool allow_all, bool skip_preamble) {
    int end = 0;
    get_address_arg (tail, result, allow_all, skip_preamble, end) ;
    check_junk (tail, end) ;
}


void Command::get_address_arg (std::string tail, std::vector<Address> &result, bool allow_all, bool skip_preamble, int &end) {
    Location current_location = pcm->get_current_location() ;
    Address addr = 0 ;

    if (tail.size() == 0) {                     // no command tail?
        result.push_back (current_location.get_addr()) ;
        end = 0 ;
    } else if (tail[0] == '*') {               // address
        end = 1 ;
        addr = get_number (pcm, tail, 0, end) ;
        if (pcm->is_running() && !pcm->test_address (addr)) {
            throw PendingException ("Bad address") ;
        } else {
            result.push_back (addr) ;
        }
    } else {                            // check for line, filename:line
        std::string::size_type colon ;
        if (tail[0] != '\'' && (colon = tail.find (':')) != std::string::npos && tail[colon+1] != ':') {                // disallow ::
            std::string filename = tail.substr (0, colon) ;
            end = colon + 1 ;
            if (isdigit (tail[end]) || tail[end] == '$') {
                int lineno = get_number (pcm, tail, 0, end) ;
                int start = lineno ;
                do {
                    addr = pcm->lookup_line (filename, lineno) ;
                    lineno += 1; 
                } while (addr == 0 && lineno < (start + 100)) ;
                if (addr == 0) { 
                    throw Exception ("No line %d in file \"%s\".\n", start, filename.c_str()) ;
                } else {
                    result.push_back (addr) ;
                }
            } else {
                std::string func = extract_word (tail, end) ;
                addr = pcm->lookup_function (func, filename, skip_preamble) ;
                if (addr == 0) {
                    throw Exception ("No function %s in file \"%s\".\n", func.c_str(), filename.c_str()) ;
                } else {
                    result.push_back (addr) ;
                }
            }
        } else if (isdigit (tail[0]) || tail[0] == '$') {         // simple line number?
            end = 0 ;
            int lineno = get_number(pcm, tail, 0, end) ;
            int start = lineno ;
            do {
                addr = pcm->lookup_line (lineno) ;
                lineno++ ;
            } while (addr == 0 && lineno < (start + 100)) ;
            if (addr == 0) {
                throw Exception ("No code at line %d.\n", start) ;
            } else {
                result.push_back (addr) ;
            }
        } else if (tail[0] == '+' || tail[0] == '-') {      // relative line number
            end = 1 ;
            int delta = get_number(pcm, tail, 0, end) ;
            int sdelta = delta ;
            do {
                addr = pcm->lookup_line (current_location.get_line()+ delta) ;
                delta += 1 ;
            } while (addr == 0 && delta < (sdelta + 100)) ;
            if (addr == 0) {
                throw Exception ("No code at the specified line") ;
            } else {
                result.push_back (addr) ;
            }
        } else {                                // function name
            std::string func ;
            if (tail[0] == '\'' || tail[0] == '"') {
               char term = tail[0] ;
               end = 1 ;
               while (end < (int)tail.size() && tail[end] != term) {
                    end++ ;
               }
               if (end == (int)tail.size()) {
                   func = tail.substr(1) ;
               } else {
                   func = tail.substr (1, end - 1) ;
                   end++ ;
               }
            } else {
                end = 0 ;
                func = extract_word (tail, end) ;
            }

            // for fortran, the user might type 'b main' when he really wants to stop at "MAIN__".
            if (func == "main" || func == "MAIN") {
                int lang = pcm->get_main_language() ;
                if (lang == DW_LANG_Fortran77 || lang == DW_LANG_Fortran90) {
                    printf ("Note: breakpoint set at the beginning of the FORTRAN program.\n") ;
                    func = "MAIN_" ;
                }
            } 

            std::vector<std::string> matches ;
            pcm->enumerate_functions (func, matches) ;
            std::string funcname ;

            if (matches.size() == 0) {
                funcname = func ;
            } else if (matches.size() > 1) {
                PStream &os = cli->get_os() ;
                os.print ("Choose one of the following functions:\n") ;
                int n = 1 ;

                const char* fmt_str;
                if (matches.size() > 9) {
                   fmt_str = "%2d.  %s\n";
                } else if (matches.size() > 99) {
                   fmt_str = "%3d.  %s\n";
                } else {
                   fmt_str = "%d.  %s\n";
                }

                os.print (fmt_str, 0, "cancel");
                for (unsigned int i = 0 ; i < matches.size() ; i++) {
                    os.print (fmt_str, n++, matches[i].c_str()) ;
                }
                if (allow_all) {
                    os.print ("a.  all\n") ;
                }

                for (;;) {
                    os.print ("> ") ;
                    os.flush() ;
                    std::string r ;
                    std::cin.clear(); 
                    std::getline (std::cin, r) ;
                    if (r.size() == 0) {
                        continue ;
                    }
                    if (allow_all && r[0] == 'a') {
                        n = -1 ;
                    } else {
                        n = atoi(r.c_str()) ;
                        if (n < 0 || n > (int)matches.size()) {
                            os.print ("Please enter a number between 0 and %d.\n", matches.size()) ;
                            continue ;
                        }
                    }
                    break ;
                }
                if (n == -1) {              // all
                    for (unsigned int i = 0 ; i < matches.size() ; i++) {
                        funcname = pcm->realname (matches[i]) ;
                        addr = pcm->lookup_function (funcname, "", skip_preamble) ;
                        if (addr == 0) {
                            throw PendingException ("Function \"%s\" not defined.", funcname.c_str()) ;
                        } else {
                            result.push_back (addr) ;
                        }
                    }
                    return ;
                } else if (n == 0) {
                    return ;
                } else {
                    funcname = matches[n-1] ;
                }
            } else {
                funcname = matches[0] ;
            }

            funcname = pcm->realname (funcname) ;
            addr = pcm->lookup_function (funcname, "", skip_preamble) ;
            if (addr == 0) {
                throw PendingException ("Function \"%s\" not defined.", funcname.c_str()) ;
            } else {
                result.push_back(addr) ;
            }
        }
    }
}



DebuggerCommand::DebuggerCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

const char *DebuggerCommand::cmds[] = {
    "directory", "file", "attach", "detach", "process", "processes", "kill", "handle", "thread",
    "history", "source", "alias", "unalias", "define", "document", "cd", "pwd", "env", "setenv",
    "echo", "symbol", "exec", "help", "shell", "make", "if", "while", "target", "search", "complete", NULL
} ;

void DebuggerCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
    if (root == "directory" || root == "file") {
        // extract the last word (or string) from the tail
        int c = ch - 1;
        while (c > 0 && tail[c] != '"' && tail[c] != '\'' && !isspace (tail[c])) {
            c-- ;
        }
        if (c > 0) {
           c++ ;
        }
        std::string filename = c <= 0 ? tail : tail.substr (c) ;
        cli->complete_file (filename, result) ;
    }
}


void DebuggerCommand::execute (std::string root, std::string tail) {
    if (root == "directory") {
       if (tail == "") {
           if (cli->confirm (NULL, "Reinitialize source path to empty")) {
               cli->init_dirlist() ;
               cli->rerun_push(root, tail) ;
           }
       } else {
           std::string dir = trim(tail) ;
           cli->add_dir (dir) ;
           cli->rerun_push(root, dir) ;
       }
       cli->show_dirlist() ;
    } else if (root == "file") {
        if (tail == "") {
            pcm->attach ("", !cli->get_int_opt(PRM_MULTI_PR)) ;
            cli->rerun_push(root, tail) ;
        } else {
            std::string file = trim(tail) ;
            if (pcm->is_running()) {
                bool ok = cli->confirm (NULL, "A program is being debugged already.  Kill it") ;
                if (!ok) {
                    printf ("Program not killed.\n") ;
                    return ;
                }
            }
            if (pcm->file_ok()) {
                std::string p = std::string ("Load new symbol table from \"") + file + "\"" ;
                bool ok = cli->confirm (NULL, p.c_str()) ;
                if (ok) {
                    pcm->attach (file, !cli->get_int_opt(PRM_MULTI_PR)) ;
                    cli->rerun_push(root, file) ;
                } else {
                    printf ("Not confirmed.\n") ;
                }
            } else {
                pcm->attach (file, !cli->get_int_opt(PRM_MULTI_PR));
                cli->rerun_push(root, file) ;
            }
        }
    } else if (root == "attach") {
        if (tail == "") {
            printf ("Argument required (process-id to attach).\n") ;
        } else {
            if (isdigit (tail[0])) {
                int ch = 0 ;
                int pid = extract_number (tail, ch) ;
                pcm->attach (pid, !cli->get_int_opt(PRM_MULTI_PR));
                cli->rerun_push(root, tail) ;
            } else {
                printf ("Illegal process-id: %s\n", tail.c_str()) ;
            }
        }
    } else if (root == "detach") {
        pcm->detach() ;
        cli->rerun_push(root, tail) ;
    } else if (root == "handle") {
        if (tail == "") {
            printf ("Argument required (signal to handle).\n") ;
        } else {
            std::string signame = "" ;
            unsigned int ch = 0 ;
            while (ch < tail.size() && !isspace (tail[ch])) {
                signame += tail[ch++] ;
            }
            while (ch < tail.size() && isspace (tail[ch])) ch++ ;
            std::vector<std::string> actions = split (tail.substr (ch)) ;
            pcm->set_signal_actions (signame, actions) ;
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "symbol") {
        printf ("'symbol' command not implemented\n") ;
    } else if (root == "exec") {
        printf ("'exec' command not implemented\n") ;
    } else if (root == "kill") {
        pcm->kill() ;
        cli->rerun_push(root, tail) ;
    } else if (root == "process") {
        if (tail == "") {
            printf ("Current process is %d\n", pcm->get_current_process()) ;
        } else {
            int ch = 0 ;
            int n = extract_number (tail, ch) ;
            try {
                pcm->select_process (n) ;
                printf ("Current process is now %d.\n", n) ;
                cli->rerun_push(root, tail) ;
            } catch (...) {
                throw ;
            }
        }
    } else if (root == "processes") {
        pcm->list_processes() ;
    } else if (root == "kill") {
        int ch = 0 ;
        extract_number (tail, ch) ;
    } else if (root == "thread") {
        int ch = 0 ;
        int n = extract_number (tail, ch) ;
        pcm->switch_thread (n) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "history") {
        cli->show_history() ;
    } else if (root == "complete") {
        cli->show_complete(tail);
    } else if (root == "source") {
        cli->read_file (tail) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "alias") {
        if (tail == "") {
            cli->show_aliases() ;
        } else {
            int ch = 0 ;
            std::string name = extract_word (tail, ch) ;
            skip_spaces (tail, ch) ;
            if (ch == (int)tail.size()) {
                cli->show_alias (name) ;
            } else {
                cli->add_alias (name, tail.substr(ch)) ;
                cli->rerun_push(root, tail) ;
            }
        }
    } else if (root == "unalias") {
        cli->remove_alias (tail) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "define") {
        cli->define_command (tail) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "document") {
        cli->document_command (tail) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "cd") {
        static std::string prevdir ;
        std::string cddir = trim(tail) ;
        if (cddir == "") {
            char *d = getcwd (NULL, 0) ;
            prevdir = d ;
            free (d) ;

            int e = chdir (cli->get_home_dir().c_str()) ;
            if (e != 0) {
                perror ("cd") ;
                return ;
            }
            cli->rerun_push(root, cddir) ;
        } else if (cddir == "-") {
            char *d = getcwd (NULL, 0) ;

            int e = chdir (prevdir.c_str()) ;
            if (e != 0) {
                perror ("cd") ;
                return ;
            }

            prevdir = d ;
            free (d) ;
            cli->rerun_push(root, cddir) ;
        } else {
            char *d = getcwd (NULL, 0) ;
            prevdir = d ;
            free (d) ;

            int e = chdir (cddir.c_str()) ;
            if (e != 0) {
                perror ("cd") ;
                return ;
            }
            cli->rerun_push(root, cddir) ;
        }
        char *d = getcwd (NULL, 0) ;
        printf ("Working directory %s.\n", d) ;
        free (d) ;
    } else if (root == "pwd") {
        char *d = getcwd (NULL, 0) ;
        printf ("%s\n", d) ;
        free (d) ;
    } else if (root == "echo") {
        printf ("%s", unescape (tail).c_str()) ;
    } else if (root == "env") {
        cli->show_env(tail);
    } else if (root == "setenv") {
        if (tail == "") {
           cli->show_env(tail);
        } else {
           std::string var, val;
           int i = 0;

           var = extract_word(tail,i);
           val = extract_shell(cli,tail,i);
           /* check for trailing junk */ 

           cli->set_env(var, val);
           cli->rerun_push(root,tail);
        }
    } else if (root == "help") {
        cli->help (tail) ;
    } else if (root == "shell") {
        if (tail == "") {
            const char *shell = getenv ("SHELL") ;
            if (shell == NULL) {
                shell = "/bin/sh" ;
            }
            system (shell) ;
        } else {
            system (tail.c_str()) ;
            /* don't record for re-run */
        }
    } else if (root == "make") {
        system (("make " + tail).c_str()) ;
        /* don't record for re-run */
    } else if (root == "while") {
        if (tail == "") {
            printf ("while command requires arguments.\n") ;
        } else {
            ComplexCommand *cmd = cli->while_block (tail) ;
            cli->execute_complex_command (cmd) ;
            delete cmd ;
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "if") {
        if (tail == "") {
            printf ("if command requires arguments.\n") ;
        } else {
            ComplexCommand *cmd = cli->if_block (tail) ;
            cli->execute_complex_command (cmd) ;
            delete cmd ;
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "target") {
        if (tail == "") {
            printf ("Argument required (target name).\n") ;
        } else {
            int ch = 0 ;
            std::string name = extract_word (tail, ch) ;
            skip_spaces (tail, ch) ;
            if (strncmp (name.c_str(), "core", name.size()) == 0) {
                std::string file = extract_word (tail, ch) ;
                if (file == "") {
                    printf ("No core file specified.\n") ;
                } else {
                    pcm->attach_core (file, !cli->get_int_opt(PRM_MULTI_PR));
                    cli->rerun_push(root, tail) ;
                }
            } else if (strncmp (name.c_str(), "child", name.size()) == 0) {
                printf ("Use the \"run\" command to start a child process.\n") ;
            } else if (strncmp (name.c_str(), "exec", name.size()) == 0) {
                std::string file = extract_word (tail, ch) ;
                if (file == "") {
                    printf ("No exec file specified.\n") ;
                } else {
                    pcm->attach (file, !cli->get_int_opt(PRM_MULTI_PR));
                    cli->rerun_push(root, tail) ;
                }
            }  else {
                printf ("Unsupported \"target\" command.  Try \"help target\".\n") ;
            }
        }
    } else if (root == "search") {
        pcm->search (tail) ;
    }
}

// general control commands

ControlCommand::ControlCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

const char *ControlCommand::cmds[] = {
    "run", "rerun", "step", "stepi", "next", "nexti", "continue", "finish", "until", "signal", "jump", NULL
} ;

bool ControlCommand::is_dangerous (std::string cmd) {
    if (cmd == "run" || cmd == "rerun") {
       return true ;
    }
    return false ;
}

void ControlCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
    if (root == "run") {
        // extract the last word (or string) from the tail
        int c = ch - 1;
        while (c > 0 && tail[c] != '"' && tail[c] != '\'' && !isspace (tail[c])) {
            c-- ;
        }
        if (c > 0) {
           c++ ;
        }
        std::string filename = c <= 0 ? tail : tail.substr (c) ;
        cli->complete_file (filename, result) ;
    }
}


void ControlCommand::execute (std::string root, std::string tail) {
    cli->set_running(true) ;
    if (root == "run") {
        if ( tail != "" ) {
           cli->set_args (tail) ;
        }
        cli->rerun_push(root, tail) ;
        cli->rerun_reset();
        bool wait = pcm->run (cli->get_args(), cli->get_env()) ;
        if (wait) {
            pcm->ready_wait() ;
        }
    } else if (root == "rerun") {
        int end = 0 ;
        int n = get_number (pcm, tail, 1, end) ;
        cli->rerun(n) ;
    } else if (root == "continue") {
        if (tail == "") {
            if (pcm->cont()) {
                pcm->ready_wait() ;
            }
        } else {
            int sig = get_number (pcm, tail, 0) ;
            if (pcm->cont(sig)) {
                pcm->ready_wait() ;
            } 
        } 
        cli->rerun_push(root, tail) ;
    } else if (root == "signal") {
        if (tail == "") {
            printf ("Argument required (signal number).\n") ;
        } else {
            int sig = get_number (pcm, tail, 0) ;
            if (pcm->cont(sig)) {
                pcm->ready_wait() ;
            }
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "step") {
        pcm->step (true, false, get_number (pcm, tail, 1)) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "stepi") {
        pcm->step (false, false, get_number (pcm, tail, 1)) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "next") {
        pcm->step (true, true, get_number (pcm, tail, 1)) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "nexti") {
        pcm->step (false, true, get_number (pcm, tail, 1)) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "finish") {
        if (tail != "") {
            printf ("The \"finish\" command does not take any arguments.\n") ;
        } else {
            pcm->finish() ;
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "until") {
        if (tail == "") {
            pcm->until() ;
            cli->rerun_push(root, tail) ;
        } else {
            std::vector<Address> address ;
            get_address_arg (tail, address, false, true) ;
            if (address.size() == 1) {
                pcm->until (address[0]) ;
                cli->rerun_push(root, tail) ;
            }
        }
    } else if (root == "jump") {
        if (tail == "") {
            printf ("Argument required (starting address.\n") ;
        } else {
            std::vector<Address> address ;
            int end = 0 ;
            get_address_arg (tail, address, false, true, end) ;
            check_junk (tail, end) ;
            if (address.size() == 1) {
                Address addr = address[0] ;
                Location current = pcm->get_current_location() ;
                Location dest = pcm->lookup_address (addr) ;
                if (current.get_funcloc() != dest.get_funcloc()) {
                    char buf[256] ;
                    if (current.get_symname() == "") {
                        snprintf (buf, sizeof(buf), "Destination is not in the current function.  Jump anyway") ;
                    } else {
                        snprintf (buf, sizeof(buf), "Destination is not in '%s'.  Jump anyway", current.get_symname().c_str()) ;
                    }
                    if (cli->confirm (NULL, buf)) {
                        if (pcm->jump (addr)) {
                            pcm->ready_wait() ;
                        }
                        cli->rerun_push(root, tail) ;
                    } else {
                        printf ("Not confirmed.\n") ;
                    }
                } else {
                    if (pcm->jump (addr)) {
                        pcm->ready_wait() ;
                    }
                    cli->rerun_push(root, tail) ;
                }
            }
        }
    }

    cli->set_running(false) ;
   
}


//
// break command
//

BreakpointCommand::BreakpointCommand (CommandInterpreter *cli, ProcessController *pcm): Command (cli, pcm, cmds) {
}

const char *BreakpointCommand::cmds[] = {
    "break", "tbreak", "condition", "ignore", "commands", "delete", "disable", "enable", "watch", "rwatch", "awatch", "hbreak", "thbreak", "advance", "clear", "stop", NULL
} ;

bool BreakpointCommand::is_dangerous (std::string cmd) {
    if (cmd == "delete") {
       return true ;
    }
    return false ;
}



void BreakpointCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
    if (root == "watch" || root == "rwatch" || root == "awatch") {
        // extract the last word (or string) from the tail
        int c = ch - 1;
        while (c > 0 && isalnum (tail[c])) {
            c-- ;
        }
        if (c > 0) {
           c++ ;
        }
        std::string varname = c <= 0 ? tail : tail.substr (c) ;
        pcm->complete_symbol (varname, result) ;
    } else if (root == "break" || root == "hbreak" || root == "tbreak" || root == "thbreak" || root == "advance") {
        char quote = '\0' ;
        if (tail[0] == '\'' || tail[0] == '"') {
            quote = tail[0] ;
            tail = tail.substr(1) ;
        }
        pcm->complete_function (tail, result) ; 
    } else if (root == "stop" || root == "stopi") {
        int end = 0 ;
        std::string n = extract_word (tail, end) ;
        skip_spaces (tail, end) ;
        if (n == "in" || n == "at") {
            tail = tail.substr (end) ;
        }
        pcm->complete_function (tail, result) ; 
    }
}


void BreakpointCommand::execute (std::string root, std::string tail) {
    if (root == "break" || root == "hbreak" || root == "tbreak" || root == "thbreak" || root == "advance" || root == "stop" || root == "stopi") {
        // allow dbx-style 'stop in' etc.
        if (root == "stop" || root == "stopi") {
            int ch = 0 ;
            std::string n = extract_word (tail, ch) ;
            skip_spaces (tail, ch) ;
            if (n == "in" || n == "at") {
                tail = tail.substr (ch) ;
            } else {
                printf ("'in' or 'at' expected after 'stop' command\n") ;
                return ;
            }
        }
        std::vector<Address> addresses ;
        int end = 0 ;
        try {
            get_address_arg (tail, addresses, true, true, end) ;
        } catch (Exception &e) {
            if (e.pending_ok()) {
                e.report (std::cout) ;
                if (cli->confirm (NULL, "Make breakpoint pending on future shared library load")) {
                    BreakpointType type = BP_USER ;
                    if (root == "hbreak" || root == "thbreak") {
                        type = BP_HBREAK ;
                    }
                    Breakpoint *bp = pcm->new_breakpoint (type, tail, 0, true) ;
                    cli->set_last_breakpoint (bp->get_num()) ;
                    if (root == "tbreak" || root == "thbreak" || root == "advance") {
                        bp->set_disposition (DISP_DELETE) ;
                        if (root == "advance") {
                            pcm->cont() ;
                            pcm->ready_wait() ;
                        }
                    }
                }
                return ;
            } else {
                throw  ;
            }
        }

        bool conditional = false ;
        std::string condition ;
        bool thread_specific = false ;
        std::vector<int> threads ;

        // look for postfixes 'if' and 'thread'
        skip_spaces (tail, end) ;
        while (end != (int)tail.size()) {
            std::string postfix ;
            while (end < (int)tail.size() && !isspace (tail[end])) {
                 postfix += tail[end++] ;
            }
            if (strncmp (postfix.c_str(), "if", postfix.size()) == 0) {
                conditional = true ;
                skip_spaces (tail, end) ;
                condition = tail.substr (end) ;
                break ;
            } else if (strncmp (postfix.c_str(), "thread", postfix.size()) == 0) {
                thread_specific = true ;
                skip_spaces (tail, end) ;
                while (end < (int)tail.size()) {
                    int n = get_number (pcm, tail, -1, end) ;
                    if (n == -1) {
                        throw Exception ("Illegal thread number") ;
                    }
                    threads.push_back (n) ;
                    if (tail[end] == ',') {
                        end++ ;
                    }  else {
                        break ;
                    }
                    skip_spaces (tail, end) ;
                }
            } else {
                throw Exception ("Junk at end of arguments.") ;
            }
            skip_spaces (tail, end) ;
        }

        for (unsigned int i = 0 ; i < addresses.size() ; i++) {
            Address addr = addresses[i] ;
            if (addr != 0) {
                BreakpointType type = BP_USER ;
                if (root == "hbreak" || root == "thbreak") {
                    type = BP_HBREAK ;
                }
                Breakpoint *bp = pcm->new_breakpoint (type, tail, addr, false) ;
                if (conditional) {
                    bp->set_condition (condition) ;
                }
                if (thread_specific) {
                    try {
                        for (unsigned int j = 0 ; j < threads.size() ; j++) {
                            bp->set_thread (threads[j]) ;
                        }
                    } catch (Exception e) {
                        pcm->delete_breakpoint (bp->get_num()) ;
                        e.report (std::cout) ;
                        continue ;
                    } catch (const char *s) {
                        pcm->delete_breakpoint (bp->get_num()) ;
                        printf ("%s\n", s) ;
                        continue ;
                    } catch (std::string s) {
                        pcm->delete_breakpoint (bp->get_num()) ;
                        printf ("%s\n", s.c_str()) ;
                        continue ;
                    }
                }
                cli->set_last_breakpoint (bp->get_num()) ;
                if (root == "tbreak" || root == "thbreak" || root == "advance") {
                    bp->set_disposition (DISP_DELETE) ;
                    if (root == "advance") {
                        pcm->cont() ;
                        pcm->ready_wait() ;
                    }
                }
                
            }
        }
        cli->rerun_push(root, tail) ;
    } else if (root == "condition") {
        int ch = 0 ;
        int bpnum = extract_number (tail, ch) ;
        std::string condition = trim (tail.substr (ch)) ;
        pcm->set_breakpoint_condition (bpnum, condition) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "ignore") {
        int ch = 0 ;
        int bpnum = extract_number (tail, ch) ;
        int n = extract_number (tail, ch) ;
        pcm->set_breakpoint_ignore_count (bpnum, n) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "commands") {
        int bpnum ;
        if (tail == "") {
            if ((bpnum = cli->get_last_breakpoint()) == -1) {
                printf ("No breakpoint has been set.\n") ;
                return ;
            }
        } else {
            int ch = 0 ;
            bpnum = extract_number (tail, ch) ;
        }
        printf ("Type commands for when breakpoint %d is hit, one per line.\n", bpnum) ;
        printf ("End with a line saying just \"end\".\n") ;
         
        cli->rerun_push(root, tail) ;
        std::vector<ComplexCommand *> cmds ;
        for (;;) {
            std::string line = cli->readline (">", false) ;
            if (line == "end") {
                break ;
            }
            cli->rerun_push(line) ;
            ComplexCommand *cmd = cli->command (line) ;
            cmds.push_back (cmd) ;
        }
        pcm->set_breakpoint_commands (bpnum, cmds) ;
    } else if (root == "delete") {
        if (tail == "" || strncmp (tail.c_str(),"breakpoints", tail.size()) == 0) {               // delete all?
            if (pcm->breakpoint_count() > 0 && cli->confirm (NULL, "Delete all breakpoints")) {
                pcm->delete_breakpoint (0) ;
                cli->rerun_push(root, tail) ;
            }
        } else {
            int end = 0 ;
            for (;;) {
                int n = get_number (pcm, tail, -1, end) ;
                if (n == -1) {
                    printf ("warning: bad breakpoint number at or near '%s'\n", tail.substr(end).c_str()) ;
                    break ;
                } else {
                    pcm->delete_breakpoint (n) ;
                }
                if (end == (int)tail.size()) {       
                    break ;
                }
            }
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "clear") {
        if (tail == "") {
            Location loc = pcm->get_current_location() ;
            pcm->clear_breakpoints (loc.get_addr()) ;
            cli->rerun_push(root, tail) ;
        } else {
            std::vector<Address> addresses ;
            get_address_arg (tail, addresses, true, true) ;

            for (unsigned int i = 0 ; i < addresses.size() ; i++) {
                Address addr = addresses[i] ;
                if (addr != 0) {
                    pcm->clear_breakpoints (addr) ;
                }
            }
            cli->rerun_push(root, tail) ;
        }

    } else if (root == "enable") {
        if (tail == "") {               // enable all?
            pcm->enable_breakpoint (0) ;
            cli->rerun_push(root, tail) ;
        } else {
            int end = 0 ;
            int func = 0 ;
            if (!isdigit (tail[0]) && tail[0] != '$') {
                std::string what = extract_word (tail, end) ;
                if (strncmp ("delete", what.c_str(), what.size()) == 0) {
                    func = 1 ;
                }
                if (strncmp ("display", what.c_str(), what.size()) == 0) {
                    func = 2 ;
                }
                if (strncmp ("once", what.c_str(), what.size()) == 0) {
                    func = 3 ;
                }
                skip_spaces (tail, end) ;
            }

            if (end == (int)tail.size()) {
                if (func == 2) {
                    pcm->enable_display (-1) ;
                } else {
                    pcm->enable_breakpoint(0) ;                    // disable all breakpoints
                }
                return ;
            }

            for (;;) {
                int n = get_number (pcm, tail, -1, end) ;
                if (n == -1) {
                    if (func != 2) {
                        printf ("warning: bad breakpoint number at or near '%s'\n", tail.substr(end).c_str()) ;
                        break ;
                    } else {
                        pcm->enable_display (-1) ;
                        break ;
                    }
                } else {
                    switch (func) {
                    case 0:
                        pcm->enable_breakpoint (n) ;
                        break ;
                    case 1:
                        pcm->set_breakpoint_disposition (n, DISP_DELETE) ;
                        break ;
                    case 2:
                        pcm->enable_display (n) ;
                        break ;
                    case 3:
                        pcm->set_breakpoint_disposition (n, DISP_DISABLE) ;
                        break ;
                    }
                }
                if (end == (int)tail.size()) {       
                    break ;
                }
            }
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "disable") {
        if (tail == "") {               // enable all?
            pcm->disable_breakpoint (0) ;
            cli->rerun_push(root, tail) ;
        } else {
            int end = 0 ;
            int func = 0 ;
            if (!isdigit (tail[0]) && tail[0] != '$') {
                std::string what = extract_word (tail, end) ;
                if (strncmp ("breakpoints", what.c_str(), what.size()) == 0) {
                    func = 1 ;
                }
                if (strncmp ("display", what.c_str(), what.size()) == 0) {
                    func = 2 ;
                }
                skip_spaces (tail, end) ;
            }
            if (end == (int)tail.size()) {
                if (func == 2) {
                    pcm->disable_display (-1) ;
                } else {
                    pcm->disable_breakpoint(0) ;                    // disable all breakpoints
                }
                return ;
            }

            for (;;) {
                int n = get_number (pcm, tail, -1, end) ;
                if (n == -1) {
                    printf ("warning: bad breakpoint number at or near '%s'\n", tail.substr(end).c_str()) ;
                    break ;
                }
                if (func == 2) {
                    pcm->disable_display (n) ;
                } else {
                    pcm->disable_breakpoint (n) ;
                }
                if (end == (int)tail.size()) {       
                    break ;
                }
            }
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "watch" || root == "rwatch" || root == "awatch") {
        Address addr = 0 ;
        int size = 0 ;
        int end = 0 ;
        Breakpoint *bp = NULL ;

        if (tail.size() == 0) {
            printf ("Argument required (expression to compute).\n") ;
            return;
        }

        if (tail[0] == '*') {               // address
            end = 1 ;
            addr = get_number (pcm, tail, 0, end) ;
            size = 4 ;
            if (addr != 0) {
                // XXX: pending?
                bp = pcm->new_watchpoint (BP_CHWATCH, tail, NULL, addr, size, false) ;
                cli->rerun_push(root, tail) ;
            } else {
                printf ("Bad watchpoint address parameter.\n") ;
                return ;
            }
        } else {                            
            Node *expr = pcm->compile_expression (tail, end, true) ;
            if (expr == NULL) {
                printf ("Unable to create watchpoint.\n") ;
                return ;
            }
            DIE *type = expr->get_type() ;
            // get size of thing to watch (XXX: real size?)
            size = type->get_size() ;                              

            int numvars = expr->num_variables() ;
            bool hw_ok = cli->get_int_opt(PRM_USE_HW);
            if (numvars != 1) {         // only 1 variable for a hardware watchpoint
                hw_ok = false ;
            } else {
                // only if there is a debug register available
                int ndebug = pcm->get_arch()->get_available_debug_regs() ;
                if (ndebug == 0) {
                    hw_ok = false ;
                }
            }
 
            // if we can create a hardware watchpoint, get the address to watch
            if (hw_ok) {
                Value v = pcm->evaluate_expression (expr, true) ;                    // address only
                addr = v.integer ;
            }

            Watchpoint *wp = NULL ;
            if (hw_ok) {
                if (addr != 0) {
                    if (root == "watch") {
                        wp = pcm->new_watchpoint (BP_CHWATCH, tail, expr, addr, size, false) ;
                    } else if (root == "rwatch") {
                        wp = pcm->new_watchpoint (BP_RWATCH, tail, expr, addr, size, false) ;
                    } else if (root == "awatch") {
                        wp = pcm->new_watchpoint (BP_RWWATCH, tail, expr, addr, size, false) ;
                    }
                    cli->rerun_push(root, tail) ;
                } else {
                    printf ("Unable to create hardware watchpoint.\n") ;
                    delete expr ;
                    return ;
                }
            } else {
                if (root == "watch") {
                    wp = pcm->new_watchpoint (BP_SWWATCH, tail, expr, 0, size, false) ;
                    cli->rerun_push(root, tail) ;
                } else {
                    printf ("Cannot create software watchpoint with rwatch or awatch commands\n") ;
                    delete expr ;   
                    return ;
                }
            }
            if (expr->is_local()) {
                wp->set_local() ;
                Address ra = pcm->get_return_addr() ;
                CascadeBreakpoint *cascade = dynamic_cast<CascadeBreakpoint*>(pcm->new_breakpoint (BP_CASCADE, "", ra, false)) ;
                cascade->add_child (wp); 
            }
            bp = wp ;
        }

        bool conditional = false ;
        std::string condition ;
        bool thread_specific = false ;
        std::vector<int> threads ;

        
        // look for postfixes 'if' and 'thread'
        skip_spaces (tail, end) ;
        while (end != (int)tail.size()) {
            std::string postfix ;
            while (end < (int)tail.size() && !isspace (tail[end])) {
                 postfix += tail[end++] ;
            }
            if (strncmp (postfix.c_str(), "if", postfix.size()) == 0) {
                conditional = true ;
                skip_spaces (tail, end) ;
                condition = tail.substr (end) ;
                break ;
            } else if (strncmp (postfix.c_str(), "thread", postfix.size()) == 0) {
                thread_specific = true ;
                skip_spaces (tail, end) ;
                while (end < (int)tail.size()) {
                    int n = get_number (pcm, tail, -1, end) ;
                    if (n == -1) {
                        throw Exception ("Illegal thread number") ;
                    }
                    threads.push_back (n) ;
                    if (tail[end] == ',') {
                        end++ ;
                    }  else {
                        break ;
                    }
                    skip_spaces (tail, end) ;
                }
            } else {
                throw Exception ("Junk at end of arguments.") ;
            }
            skip_spaces (tail, end) ;
        }

        if (conditional) {
            bp->set_condition (condition) ;
        }
        if (thread_specific) {
            try {
                for (unsigned int j = 0 ; j < threads.size() ; j++) {
                    bp->set_thread (threads[j]) ;
                }
            } catch (Exception e) {
                pcm->delete_breakpoint (bp->get_num()) ;
                e.report (std::cout) ;
            } catch (const char *s) {
                pcm->delete_breakpoint (bp->get_num()) ;
                printf ("%s\n", s) ;
            } catch (std::string s) {
                pcm->delete_breakpoint (bp->get_num()) ;
                printf ("%s\n", s.c_str()) ;
            }
        }
        cli->set_last_breakpoint (bp->get_num()) ;

    }
}


StackCommand::StackCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

const char *StackCommand::cmds[] = {
    "backtrace", "down", "frame", "return", "select-frame", "up", NULL
} ;

void StackCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}


void StackCommand::execute (std::string root, std::string tail) {
    if (root == "backtrace" || root == "where") {
        pcm->stacktrace(get_number (pcm, tail, -1)) ;
    } else if (root == "down") {
        pcm->down(get_number (pcm, tail, 1)) ;  
        cli->rerun_push(root, tail) ;
    } else if (root == "frame" || root == "select-frame") {
        int n = get_number (pcm, tail, 1000000) ;
        if (n == 1000000) {
            pcm->show_frame() ;
        } else {
            pcm->set_frame(n) ;  
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "return") {              // XXX: confirm and return value
        
        Value v = 0 ;
        if (tail.size() > 0) {
            int end = 0 ;
            v = pcm->evaluate_expression (tail, end, false) ;
        }
        pcm->return_from_func (v) ;
        cli->rerun_push(root, tail) ;
    } else if (root == "up") {
        pcm->up(get_number (pcm, tail, 1)) ;
        cli->rerun_push(root, tail) ;
    }
}

PrintCommand::PrintCommand (CommandInterpreter *cli, ProcessController *pcm): Command (cli, pcm, cmds) {
    last_format = new Format() ;
}

const char *PrintCommand::cmds[] = {
    "print", "printf", "output", "call", "display", "undisplay", "disassemble", "memdump", "list", "x", "ptype", "whatis", NULL
} ;

void PrintCommand::extract_format (std::string s, int &ch, Format &format) {
    skip_spaces (s, ch) ;
    if (s[ch] == '/') {
        ch++ ;
        skip_spaces (s, ch) ;
        // extract count
        if (isdigit (s[ch])) {
            format.count = 0 ;
            while (ch < (int)s.size() && isdigit (s[ch])) {
               format.count = format.count * 10 + s[ch++] - '0' ;
            }
        }
        // extract format code
        switch (s[ch]) {
        case 'o':
        case 'x':
        case 'd':
        case 'u':
        case 't':
        case 'f':
        case 'a':
        case 'i':
        case 'c':
        case 's':
            format.code = s[ch++] ;
        }
        // extract size
        switch (s[ch]) {
        case 'b':
        case 'h':
        case 'w':
        case 'g':
            format.size = s[ch++] ;
        }

        // check for the end of the format specifier (needs to be space)
        if (!isspace (s[ch])) {
           throw Exception ("Undefined output format") ;
        }
    }
}

Address PrintCommand::get_list_address (std::string tail) {
    if (tail == "") {
        return -1 ;
    }
    std::vector<Address> address ;
    get_address_arg (tail, address, false, false) ;
    if (address.size() == 1) {
        return address[0] ;
    }
    return 0 ;
}

void doprintf (PStream &os, std::string format, std::vector<Value> &args) {
    int actual = 0 ;
    for (unsigned int i = 0 ; i < format.size() ; i++) {
        char ch = format[i] ;
        if (ch == '%') {
            std::string conv = "%";
            i++ ;
            bool done = false ;
            while (i < format.size() && !done) {
                ch = format[i] ;
                conv += ch ;
                if (conv.size() > 10) {
                    break ;
                }
                switch (ch) {
                case 'c':
                case 'o':
                case 'x':
                case 's':
                case 'd':
                case 'u':
                case 'h':
                case 'f':
                case 'g':
                    done = true ;
                }
                i++ ;
            }
            i-- ;                       // one too far
            Value &v = args[actual++] ;
            switch (v.type) {
            case VALUE_INTEGER:
                os.print (conv.c_str(), v.integer) ;
                break ;
            case VALUE_BOOL:
                os.print (conv.c_str(), v.integer) ;
                break ;
            case VALUE_REAL:
                os.print (conv.c_str(), v.real) ;
                break ;
            case VALUE_STRING:
                os.print (conv.c_str(), v.str.c_str()) ;
                break ;
            default:
                os.print ("none") ;
                break ;
            }
        } else {
            os.print ("%c", ch) ;
        }
    }
    os.flush() ;
}


void PrintCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
    if (root == "print" || root == "output" || root == "call" || root == "display") {
        // extract the last word (or string) from the tail
        int c = ch - 1;
        while (c > 0 && isalnum (tail[c])) {
            c-- ;
        }
        if (c > 0) {
           c++ ;
        }
        std::string varname = c <= 0 ? tail : tail.substr (c) ;
        pcm->complete_symbol (varname, result) ;
    }
}


void PrintCommand::execute (std::string root, std::string tail) {
    Format fmt ;
    if (root == "print") {
        int ch = 0 ;
        extract_format (tail, ch, fmt) ;
        if (fmt.count != 1) {
            printf ("Item count other than 1 is meaningless\n") ;
            return ;
        }
        cli->begin_paging(); 
        pcm->print_expression (tail.substr (ch), fmt, false, true) ;
        cli->end_paging();
        cli->rerun_push(root, tail) ;
    } else if (root == "output") {
        int ch = 0 ;
        extract_format (tail, ch, fmt) ;
        if (fmt.count != 1) {
            printf ("Item count other than 1 is meaningless\n") ;
            return ;
        }
        cli->begin_paging(); 
        pcm->print_expression (tail.substr (ch), fmt, true, false) ;
        cli->end_paging();
        cli->rerun_push(root, tail) ;
    } else if (root == "call") {
        int ch = 0 ;
        extract_format (tail, ch, fmt) ;
        if (fmt.count != 1) {
            printf ("Item count other than 1 is meaningless\n") ;
            return ;
        }
        cli->begin_paging(); 
        pcm->print_expression (tail.substr (ch), fmt, false, false) ;
        cli->end_paging();
        cli->rerun_push(root, tail) ;
    } else if (root == "display") {
        int ch = 0 ;
        extract_format (tail, ch, fmt) ;
        skip_spaces (tail, ch) ;
        if (ch == (int)tail.size()) {                // NOTE: the format is part of the expression according to the GDB output
            pcm->list_displays() ;
        } else {
            pcm->set_display (tail, ch, fmt) ;
            cli->rerun_push(root, tail) ;
        }
    } else if (root == "disassemble") {
        std::string addr1 ;
        unsigned int i = 0 ;
        while (i < tail.size() && !isspace (tail[i])) {
           addr1 += tail[i++] ;
        }
        while (i < tail.size() && isspace (tail[i])) i++ ;
        if (i == tail.size()) {                 // one parameter?
            Address addr = get_address (pcm, addr1) ;
            cli->begin_paging(); 
            pcm->disassemble (addr) ;
            cli->end_paging();
        } else {
            std::string addr2 ;
            while (i < tail.size() && !isspace (tail[i])) {
               addr2 += tail[i++] ;
            }
            Address start = get_address (pcm, addr1) ;
            Address end = get_address (pcm, addr2) ;
            cli->begin_paging(); 
            pcm->disassemble (start, end) ;
            cli->end_paging();
        }
    } else if (root == "undisplay") {
        if (tail == "") {
            if (cli->confirm (NULL, "Delete all auto-display expressions")) {
                pcm->undisplay (-1) ;
            }
        } else {
            int n = get_number (pcm, tail, 1000000) ;
            pcm->undisplay (n) ;
        }
        cli->rerun_push(root, tail) ;
    } else if (root == "memdump") {
        int end = 0 ;
        Address addr = get_address (pcm, tail, end) ;
        int size = 128 ;
        if (end < (int)tail.size()) {
            size = get_number (pcm, tail, 128, end) ;
        }
        pcm->dump (addr, size) ;
    } else if (root == "list") {
        if (tail == "-") {
            pcm->list_back() ;
        } else if (tail == "") {
            pcm->list() ;
        } else {
            int end = 0 ;
            if (tail[0] == '*') {
                end = 1 ;
                Address addr = get_number (pcm, tail, 0, end) ;
                pcm->list (addr) ;
            } else {
                std::string sloc = tail ;
                std::string eloc = "" ;
                std::string::size_type comma = tail.find (',') ;
                if (comma != std::string::npos) {
                    sloc = tail.substr (0, comma) ;
                    eloc = tail.substr (comma+1) ;
                } 
                std::string::size_type colon ;
                if ((colon = sloc.find (':')) != std::string::npos) {
                    std::string filename = tail.substr (0, colon) ;
                    end = colon + 1 ;
                    if (isdigit (sloc[end]) || sloc[end] == '$') {
                        int slineno = get_number (pcm, sloc, 0, end) ;
                        if (eloc != "") {        
                            end = 0 ;
                            int elineno = get_number (pcm, eloc, 0, end) ;
                            pcm->list (filename, slineno, elineno) ;
                        } else {
                            pcm->list (filename, slineno) ;
                        }
                    } else {                    // function name
                        std::string func = extract_word (sloc, end) ;
                        Address addr = pcm->lookup_function (func, filename, false) ;   
                        if (addr == 0) {
                            printf ("No function %s in file %s\n", func.c_str(), filename.c_str()) ;
                        } else {
                            pcm->list (addr) ;
                        }
                    }
                } else {                        // no filename
                    end = 0 ;
                    if (isdigit (sloc[end]) || sloc[end] == '$') {
                        int slineno = get_number (pcm, sloc, 0, end) ;
                        if (eloc != "") {        
                            end = 0 ;
                            int elineno = get_number (pcm, eloc, 0, end) ;
                            pcm->list ("", slineno, elineno) ;
                        } else {
                            pcm->list ("", slineno) ;
                        }
                    } else {                    // function name
                        std::string func = extract_word (sloc, end) ;
                        func = pcm->realname (func) ;           // look up mangled name
                        Address addr = pcm->lookup_function (func, "", false) ;   
                        if (addr == 0) {
                            printf ("Function \"%s\" not defined\n", func.c_str()) ;
                        } else {
                            pcm->list (addr) ;
                        }
                    }
                }

            }
        }

    } else if (root == "x") {
        fmt = *last_format ;
        int ch = 0 ;
        extract_format (tail, ch, fmt) ;
        fmt.fill = true ;
        Address addr = get_address (pcm, tail.substr(ch)) ;
        pcm->examine (fmt, addr) ;
    } else if (root == "ptype" || root == "whatis") {
        cli->begin_paging();
        pcm->print_type (tail, root=="ptype") ;
        cli->end_paging();
    } else if (root == "printf") {
        if (tail == "") {
            printf ("Argument required (format-control string and values to print).\n") ;
        } else {
            int end = 0 ;
            skip_spaces (tail, end) ;
            if (tail[end] == '"') {
                std::string format ;
                unsigned int nargs = 0 ;
                end++ ;
                while (end < (int)tail.size()) {
                    if (tail[end] == '%') {
                        nargs++ ;
                    }
                    if (tail[end] == '"') {
                        break ;
                    }
                    if (tail[end] == '\\') {
                        end++ ;
                        switch (tail[end]) {
                        case 'n':
                            format += '\n' ;
                            break ;
                        case 'r':
                            format += '\r' ;
                            break ;
                        case 'a':
                            format += '\a' ;
                            break ;
                        case 'b':
                            format += '\b' ;
                            break ;
                        case 'v':
                            format += '\v' ;
                            break ;
                        case 't':
                            format += '\t' ;
                            break ;
                        case '\\':
                            format += '\\' ;
                            break ;
                        default:
                            printf ("Unrecognized escape character: \\%c.\n", tail[end]) ;
                            return ;
                        }
                    } else {
                        format += tail[end] ;
                    }
                    end++ ;
                }
                if (end < (int)tail.size() && tail[end] == '"') {
                    end++ ;
                } else {
                    printf ("Bad format string, non-terminated.\n") ;
                    return ;
                }

                std::vector<Node *> actualnodes ;           // expression to print
                while (end < (int)tail.size()) {
                    skip_spaces (tail, end) ;
                    if (tail[end] != ',') {
                        break ;
                    }
                    end++ ;                     // skip comma
                    int start = end ;
                    actualnodes.push_back (pcm->compile_expression (tail.substr (end), end, true)) ;
                    if (end == start) {                 // prevent infinite loop
                        break ;
                    }
                    end += start ;
                }
                skip_spaces (tail, end) ;
                if (end != (int)tail.size()) {
                    printf ("Invalid argument syntax\n") ;
                    return ;
                }
                if (actualnodes.size() != nargs) {
                    printf ("Wrong number of arguments for specified format-string\n") ;
                    for (unsigned int i = 0 ; i < actualnodes.size() ; i++) {
                        delete actualnodes[i] ;
                    }
                } else {
                    // now evaluate the actual expressions
                    std::vector<Value> actuals ;
                    for (unsigned int i = 0 ; i < actualnodes.size() ; i++) {
                        actuals.push_back (pcm->evaluate_expression (actualnodes[i])) ;
                    }

                    doprintf (cli->get_os(), format, actuals) ;

                    for (unsigned int i = 0 ; i < actualnodes.size() ; i++) {
                        delete actualnodes[i] ;
                    }
                }
            } else {
                printf ("Bad format string, missing '\"'.\n") ;
            }
        }
    }
    *last_format = fmt ;
}

QuitCommand::QuitCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

const char *QuitCommand::cmds[] = {
    "quit", NULL
} ;

void QuitCommand::execute (std::string root, std::string tail) {
    if (pcm->is_running()) {
        bool yes = cli->confirm (NULL, "The program is running.  Exit anyway") ;
        if (!yes) {
            cli->get_os().print ("Not confirmed.\n") ;
            return ;
        }
    }
    cli->quit() ;
}


InfoCommand::InfoCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
    subcommands = new InfoSubcommand (cli, pcm) ;
}

const char *InfoCommand::cmds[] = {
    "info", NULL
} ;

void InfoCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void InfoCommand::execute (std::string root, std::string tail) {
    if (tail == "") {
    } else {
        cli->execute_subcommand ("info", subcommands, tail) ;
    }
}


InfoSubcommand::InfoSubcommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

const char *InfoSubcommand::cmds[] = {
    "address", "all-registers", "args", "program", "catch", "display", "frame", "functions", "line", "breakpoints", "watchpoints", 
    "locals", "proc", "registers", "scope", "sharedlibrary", "sources", "source", "stack",
    "symbol", "signals", "threads", "types", "variables", "warranty", "copying", "all-breakpoints", NULL
} ;

void InfoSubcommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void InfoSubcommand::execute (std::string root, std::string tail) {
    if (root == "set") {
        cli->show_opts() ;
    } else if (root == "warranty" || root == "copying") {
        printf ("%s\n", COPYNOTE);
        printf ("%s\n", WARRANTY);
    } else {
        pcm->info (root, tail) ;
    }
}

SetSubcommand::SetSubcommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

// the validation of the set sub commands is done during execution, so here we
// just accept it

void SetSubcommand::check_abbreviation (std::string root, CommandVec &result) {
    result.push_back (Match (root, this)) ;
}


const char *SetSubcommand::cmds[] = {
    NULL
} ;

void SetSubcommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

static void split_back(std::string& alpha, std::string& beta) {
    std::string::size_type t;

    beta = "";

    t = alpha.find_last_of(" \t");
    if (t == std::string::npos ||
        t == alpha.size()-1)
       return;

    beta = alpha.substr(t+1);
    alpha = alpha.substr(0,t);

    t = alpha.find_last_not_of(" \t");
    if (t == std::string::npos ||
        t == alpha.size()-1)
       return;

    alpha = alpha.substr(0,t+1);
}

void SetSubcommand::execute (std::string root, std::string tail) {
    std::string myrec, myroot, mytail;

    /* XXX: this function is just grotesque, at some
       point I'll get of this whole root/tail mess */

    /* undo useless root-tail split */
    myrec = root;
    if ( tail != "" ) {
       myrec += " " + tail;
    }

    /* check for variable tag */
    if (Utils::is_completion(root.c_str(), "variable")) {
        int end = 0;
        pcm->evaluate_expression (tail, end) ;
        cli->rerun_push("set", myrec);
        return;
    }

    /* check for arguments */
    if (root == "args") {
        cli->set_args (tail);
        cli->rerun_push("set", myrec);
        return;
    }

    /* check for environmental variable */
    if (root == "environment") {
        std::string var, val;
        int i = 0;

        var = extract_word(tail, i);
        val = extract_shell(cli, tail, i);
       
        cli->set_env (var, val);
        cli->rerun_push("set environment", tail);
        return;
    }

    /* undo crappy processing */
    if ( tail != "" ) {
        myroot = root + " " + tail;
        split_back(myroot, mytail);
    } else {
        myroot = root;
    }

    /* check for history filename */
    if (myroot == "history filename") {
        cli->set_opt(myroot.c_str(), mytail.c_str());
        cli->load_history();
    }

    /* check for history size */
    if (myroot == "history size") {
        cli->set_opt(myroot.c_str(), mytail.c_str());
        cli->shrink_history();
    }

    /* try as parameter, then as symbol */
    if (cli->set_opt(myroot.c_str(), mytail.c_str())) {
       int end = 0 ;
       pcm->evaluate_expression (root+tail, end) ;
    }
    cli->rerun_push("set", myrec) ;
}

void CommandInterpreter::load_history() {
   const char* fname;
   fname = get_str_opt(PRM_HSTFILE);
   history.set_file(fname);
}

void CommandInterpreter::save_history() {
   unsigned int should;
   should = get_int_opt(PRM_HSTSAVE);

   if (should) {
      history.save_file();
   }
}

void CommandInterpreter::shrink_history() {
   unsigned hsize;
   hsize = get_int_opt(PRM_HSTSIZE);
   history.set_size(hsize);
}

SetCommand::SetCommand(CommandInterpreter *cli, ProcessController *pcm)
 : Command (cli, pcm, cmds) {
    subcommands = new SetSubcommand (cli, pcm) ;
}

const char *SetCommand::cmds[] = {
    "set", NULL
} ;

void SetCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void SetCommand::execute (std::string root, std::string tail) {
    if (tail == "") {
    } else {
        cli->execute_subcommand ("set", subcommands, tail) ;
    }
}


CatchSubcommand::CatchSubcommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}


const char *CatchSubcommand::cmds[] = {
    "throw", "catch", "signal", "fork", "vfork", "exec", "thread_start", 
    "thread_exit", "thread_join", "start", "exit", "load", "unload", "stop", NULL
} ;

void CatchSubcommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void CatchSubcommand::execute (std::string root, std::string tail) {
    if (root == "throw") {
        pcm->new_catchpoint (CATCH_THROW, "") ;
        cli->rerun_push(root, tail) ;
    } else if (root == "catch") {
        pcm->new_catchpoint (CATCH_CATCH, "") ;
        cli->rerun_push (root, tail) ;
    } else if (root == "signal") {
        printf ("Catch of signal not yet implemented\n") ;
    } else if (root == "fork") {
        pcm->new_catchpoint (CATCH_FORK, "") ;
        cli->rerun_push (root, tail) ;
    } else if (root == "vfork") {
        pcm->new_catchpoint (CATCH_VFORK, "") ;
        cli->rerun_push (root, tail) ;
    } else if (root == "exec") {
        pcm->new_catchpoint (CATCH_EXEC, "") ;
        cli->rerun_push (root, tail) ;
    } else if (root == "thread_start") {
        printf ("Catch of thread_start not yet implemented\n") ;
    } else if (root == "thread_exit") {
        printf ("Catch of thread_exit not yet implemented\n") ;
    } else if (root == "thread_join") {
        printf ("Catch of thread_join not yet implemented\n") ;
    } else if (root == "start") {
        printf ("Catch of start not yet implemented\n") ;
    } else if (root == "exit") {
        printf ("Catch of exit not yet implemented\n") ;
    } else if (root == "load") {
        printf ("Catch of load not yet implemented\n") ;
    } else if (root == "unload") {
        printf ("Catch of unload not yet implemented\n") ;
    } else if (root == "stop") {
        printf ("Catch of stop not yet implemented\n") ;
    }
}


CatchCommand::CatchCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
    subcommands = new CatchSubcommand (cli, pcm) ;
}

const char *CatchCommand::cmds[] = {
    "catch", NULL
} ;

void CatchCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void CatchCommand::execute (std::string root, std::string tail) {
    if (tail == "") {
        printf ("Catch requires an event name.\n") ;
    } else {
        cli->execute_subcommand ("catch", subcommands, tail) ;
    }
}


ShowSubcommand::ShowSubcommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
}

// the validation of the show sub commands is done during execution, so here we
// just accept it

void ShowSubcommand::check_abbreviation (std::string root, CommandVec &result) {
    result.push_back (Match (root, this)) ;
}


const char *ShowSubcommand::cmds[] = {
    NULL
} ;

void ShowSubcommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void ShowSubcommand::execute (std::string root, std::string tail) {
    if (root == "") {
        cli->show_opts() ;
    } else {
        if (strncmp (root.c_str(), "version", root.size()) == 0) {
            // XXX: get proper stuff to put here
            printf ("PathScale Debugger, Version %s\n", VERSION);
            printf("%s", COPYNOTE);
        } else if (strncmp (root.c_str(), "user", root.size()) == 0) {
            int ch = 0 ;
            std::string name = extract_word (tail, ch) ;
            cli->show_defined_command("") ;
        } else if (strncmp (root.c_str(), "environment", root.size()) == 0) {
            cli->show_env(tail);
        } else {
            std::string complete = root;
            if (tail != "") {
               complete += " " + tail;
            }
            cli->show_opt(complete.c_str()) ;
        }
    }
}


ShowCommand::ShowCommand(CommandInterpreter *cli, ProcessController *pcm) : Command (cli, pcm, cmds) {
    subcommands = new ShowSubcommand (cli, pcm) ;
}

const char *ShowCommand::cmds[] = {
    "show", NULL
} ;

void ShowCommand::complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) {
}

void ShowCommand::execute (std::string root, std::string tail) {
    cli->execute_subcommand ("show", subcommands, tail) ;
}



// --------------------------------------------------------------------------------------
//              main command interpreter
// --------------------------------------------------------------------------------------

CommandInterpreter* current_cli = NULL;

void globl_sighandler(int sig) {
    if (current_cli != NULL) {
        current_cli->interrupt(sig) ;
    }
}

CommandInterpreter::CommandInterpreter (ProcessController *pcm, PStream &os,
   DirectoryTable &dirlist, int flags)
  : pcm(pcm), os(os), program_running(false),
    instream(NULL), dirlist(dirlist), options(os), history(os),
    flags(flags), debugger_var_num(0), last_breakpoint_num(-1) {

    pcm->set_cli (this) ;
    load_env();
    rerun_mark = -1;

    commands.push_back (new ControlCommand (this, pcm)) ;
    commands.push_back (new BreakpointCommand (this, pcm)) ;
    commands.push_back (new StackCommand (this, pcm)) ;
    commands.push_back (new PrintCommand (this, pcm)) ;
    commands.push_back (new QuitCommand (this, pcm)) ;
    commands.push_back (new InfoCommand (this, pcm)) ;
    commands.push_back (new SetCommand (this, pcm)) ;
    commands.push_back (new ShowCommand (this, pcm)) ;
    commands.push_back (new DebuggerCommand (this, pcm)) ;
    commands.push_back (new CatchCommand (this, pcm)) ;

    open_help_file() ;

    completor = new CommandCompletor(this, os) ;

    // a few builtin aliases
    add_alias ("b", "break") ;
    add_alias ("s", "step") ;
    add_alias ("n", "next") ;
    add_alias ("c", "continue") ;
    add_alias ("si", "stepi") ;
    add_alias ("ni", "nexti") ;
    add_alias ("bt", "backtrace") ;
    add_alias ("where", "backtrace") ;
    add_alias ("p", "print") ;
    add_alias ("i", "info") ;
    add_alias ("d", "delete") ;
    add_alias ("f", "frame") ;
    add_alias ("r", "run") ;
    add_alias ("exit", "quit") ;
    add_alias ("core", "target core") ;


    /* dimensions found runtime */
    int rows, cols;
    readl.get_dims(rows, cols);
    options.set(PRM_WIDTH,cols);
    options.set(PRM_HEIGHT,rows); 

    /* override defaults for gdb */
    if (flags & CLI_FLAG_GDB) {
       options.set(PRM_P_PRETTY,0);
       options.set(PRM_P_ARRAY,0);
       options.set(PRM_P_7BIT,0);
    }

    /* push sizes to output stream */
    init_pstream();

    /* override defaults for emacs */
    if (flags & CLI_FLAG_EMACS) {
       options.set(PRM_ANNOTE,1);
    }

    /* setup directory list */
    init_dirlist() ;
    char *tmp = getcwd(NULL, 0);
    dirlist.push_back (std::string(tmp)) ;
    free(tmp);

    /* load history file */
    unsigned hsize;
    hsize = get_int_opt(PRM_HSTSIZE);
    history.set_size(hsize);

    const char* hfile;
    hfile = get_str_opt(PRM_HSTFILE);
    history.set_file(hfile);
}

// read and process the .pathdbrc file
void CommandInterpreter::import_pathdbrc() {
    std::string pathdbrc = std::string (home) + "/.pathdbrc" ;
    import_commands (pathdbrc) ;
}

void CommandInterpreter::init_pstream() {
  // set the dimensions for the output
  os.set_width (get_int_opt(PRM_WIDTH));
  os.set_height (get_int_opt(PRM_HEIGHT));
  if (!get_int_opt(PRM_PAGINATE)) {
     os.set_height(0);
  }
  os.reset();
}

void CommandInterpreter::interrupt(int signum) {
  switch (signum) {
  case SIGHUP:
  case SIGQUIT:
  case SIGTERM:
     /* close processes */
     quit();
     break;
  case SIGINT:
     /* forward to process */
     if (program_running) {
        pcm->interrupt();
     }
     break;
  default:
     break;
  }
}

void CommandInterpreter::run(Process *proc, Address endsp) {
    std::string lastcommand ;
  
    // Add signal handlers 
    current_cli = this;
    {
       struct sigaction sigact;
       sigset_t sigmask;

       sigemptyset(&sigmask);
       sigaddset(&sigmask, SIGHUP);
       sigaddset(&sigmask, SIGQUIT);
       sigaddset(&sigmask, SIGTERM);
       sigaddset(&sigmask, SIGINT);

       memset(&sigact, 0, sizeof(sigact));
       sigact.sa_handler = &globl_sighandler;
       sigact.sa_mask = sigmask;

       sigaction(SIGHUP, &sigact, NULL);
       sigaction(SIGQUIT, &sigact, NULL);
       sigaction(SIGTERM, &sigact, NULL);
       sigaction(SIGINT, &sigact, NULL);
    }
 
    while (instream == NULL || !instream->eof()) {
        // determine if this instance of the cli needs to terminate
        if (proc != NULL) {
            Address sp = proc->get_reg("sp") ;
            if (sp >= endsp) {
                 break ;
            }
        }
        std::string command ;
        bool repeat_command = false ;           // command was a repeat
        if (!injected_commands.empty()) {
            ComplexCommand *ccmd = injected_commands.front() ;
            injected_commands.pop() ;
            bool quit = false ;
            try {
                execute_complex_command (ccmd) ;
            } catch (Exception e) {
                os.flush() ;
                e.report (std::cout) ;
            } catch (const char *s) {
                os.flush() ;
                std::cerr << s << "\n" ;
            } catch (std::string s) {
                os.flush() ;
                std::cerr << s << "\n" ;
            } catch (QuitOutput q) {
                os.print ("Quit\n") ;   
                quit = true ;
            } catch (bool b) {
            } catch (...) {
                os.flush() ;
                std::cerr << "unknown exception in injected command processing\n" ;
            }
            delete ccmd ;
            // if we've been asked to quit, flush the injected commands queue
            if (quit) {         
                while (!injected_commands.empty()) {
                    injected_commands.pop() ;
                }
            }
            continue ;
        } else {
            try {
                completor->reset() ;
                std::string prompt = get_str_opt(PRM_PROMPT);
                command = readline (prompt.c_str(), true) ;
                if (instream != NULL && instream->eof()) {
                    break ;
                }
                if (instream != NULL) {
                    printf ("%s\n", command.c_str()) ;
                }
                if (command == "") {
                    command = lastcommand ;
                    repeat_command = true ;
                }
                lastcommand = command ;
            } catch (const char *s) {
                printf ("%s\n", s) ;
                continue ;
            }
        }
        try {
            init_pstream();
            execute_command (command, repeat_command) ;
        } catch (Exception e) {
            os.flush() ;
            e.report (std::cout) ;
        } catch (const char *s) {
            os.flush() ;
            std::cerr << s << "\n" ;
        } catch (std::string s) {
            os.flush() ;
            std::cerr << s << "\n" ;
        } catch (QuitOutput q) {
            os.print ("Quit\n") ;
        } catch (bool b) {
        } catch (...) {
            os.flush() ;
            std::cerr << "unknown exception in cli\n" ;
        }
    }
}


void CommandInterpreter::execute_command (std::string cmd, bool is_repeat, int depth, bool execute_hook) {
    if (depth > 10) {
        os.print ("Max user call depth exceeded -- command aborted\n") ;
        return ;
    }

    unsigned int ch = 0 ;
    while (ch < cmd.size() && isspace (cmd[ch])) ch++ ;
    if (ch >= cmd.size()) {
        return ;
    }
    if (cmd[ch] == '#') {               // comment
        return ;
    }
    std::string root ;

    while (ch < cmd.size() && (isalnum (cmd[ch]) || cmd[ch] == '-' || cmd[ch] == '$') ) {
       root += cmd[ch++] ;
    }

    CommandVec matched ;

    // have pcm store the current location of current process
    pcm->push_location();

    // check for alias
    // XXX: more flexibility?  arguments?  replace whole command?
    AliasMap::iterator alias = aliases.find (root) ;
    if (alias != aliases.end()) {
        execute_command (alias->second + cmd.substr (ch), false, depth+1) ;
        return ;
    }

    // check for defined command
    DefinedCommandMap::iterator defcmd = defined_commands.find (root) ;
    if (defcmd != defined_commands.end()) {
        std::vector<std::string> args = Command::split (cmd.substr(ch)) ;
        invoke_defined_command (defcmd->second, args, depth, execute_hook) ;
        return ;
    }

    // look for the root in all the command processors
    for (unsigned int i = 0 ; i < commands.size() ; i++) {
        commands[i]->check (root, matched) ;
    }


    if (matched.size() == 0) {
        // no match for exact command, try abbreviation
        for (unsigned int i = 0 ; i < commands.size() ; i++) {
            commands[i]->check_abbreviation (root, matched) ;
        }
    }

    // check that the command is recognized and unique
    if (matched.size() == 0) {
        if (get_int_opt(PRM_AS_SHELL)) {
            system (cmd.c_str()) ;
        } else {
            std::cerr << "Undefined command: \"" << root << "\".  Try \"help\".\n" ;
        }
        return ;
    }

    if (matched.size() > 1) {
        std::cerr << "Ambiguous command \"" << root << "\": " ;
        bool comma = false ;
        for (unsigned int i = 0 ; i < matched.size() ; i++) {
            if (comma) std::cerr << ", " ;
            comma = true ;
            std::cerr << matched[i].cmd ;
        }
        std::cerr << ".\n" ;
        return ;
    }

    // some commands are dangerous to repeat, check for them
    if (is_repeat) {
        for (unsigned int i = 0 ; i < commands.size() ; i++) {
            if (commands[i]->is_dangerous (matched[0].cmd)) {
                return ;
            }
        }
    }

    if (execute_hook) {
        // look for a hook for the command and execute it before if found
        defcmd = defined_commands.find ("hook-" + matched[0].cmd) ;
        if (defcmd != defined_commands.end()) {
            std::vector<std::string> dummyargs ;
            invoke_defined_command(defcmd->second, dummyargs, depth, false) ;
        }
    }

    // the command is unique, extract the tail and pass it to the command interpreter
    while (ch < cmd.size() && isspace (cmd[ch])) ch++ ;
    std::string tail = cmd.substr (ch) ;
    matched[0].processor->execute (matched[0].cmd, tail) ;
}

// if the subcmd begins with a '-' character then we allow 1 space in the root.  This is used
// for multi-word parameters

void CommandInterpreter::execute_subcommand (const char *name, Command *cmd, std::string subcmd) {
    unsigned int ch = 0 ;
    while (ch < subcmd.size() && isspace (subcmd[ch])) ch++ ;

    std::string root ;

    int spaces = 0 ;
    if (ch < subcmd.size() && subcmd[ch] == '-') {              // special case, allow one space
        spaces = 1 ;
        ch++ ;
    }
    // the command root is alphabetic only (no numbers) [but allow 'spaces' spaces]
    while (ch < subcmd.size() && ((subcmd[ch] == ' ' && spaces-- > 0) ||
           isalpha (subcmd[ch]) || subcmd[ch] == '_' ||
           subcmd[ch] == '-' || subcmd[ch] == '$') ) {
       root += subcmd[ch++] ;
    }

    CommandVec matched ;

    // look for the root in command
    cmd->check (root, matched) ;


    if (matched.size() == 0) {
        cmd->check_abbreviation (root, matched) ;
    }

    // check that the command is recognized and unique
    if (matched.size() == 0) {
        std::cerr << "Undefined " << name << " command: \"" << root << "\".  Try \"help " << name << "\".\n" ;
        return ;
    }

    if (matched.size() > 1) {
        std::cerr << "Ambiguous " << name << " command \"" << root << "\": " ;
        bool comma = false ;
        for (unsigned int i = 0 ; i < matched.size() ; i++) {
            if (comma) std::cerr << ", " ;
            comma = true ;
            std::cerr << matched[i].cmd ;
        }
        std::cerr << ".\n" ;
        return ;
    }

    // the command is unique, extract the tail and pass it to the command interpreter
    while (ch < subcmd.size() && isspace (subcmd[ch])) ch++ ;
    std::string tail = subcmd.substr (ch) ;
    matched[0].processor->execute (matched[0].cmd, tail) ;
}

// XXX: ^C handling
bool CommandInterpreter::confirm (const char *prompt1, const char *prompt2) {
    // User can switch off confirmation
    if ( !get_int_opt(PRM_CONFIRM) ) {
        return true;
    }

    if ( prompt1 != NULL ) {
        os.print ("%s.\n", prompt1);
    }

    if ( !(get_flags() & CLI_FLAG_GDB) ) {
       // Default takes first key press
       os.print ("%s? (y or n) ", prompt2);
       os.flush();

       int c = readl.getchar("nNyY");
       os.print("\n");
       return toupper(c) == 'Y';
    } else {
       // Need GDB-safe version for testsuite
       for (int retry = 0; retry < 5; retry++) {
          const int buflen = 1024;
          char buf[buflen];
          int ans;

          os.print ("%s? (y or n) ", prompt2);
          os.flush() ;

          if ( !fgets(buf,buflen,stdin) ) 
             goto try_again; 

          ans = toupper(buf[0]);
          if (ans == 'Y' || ans == 'N') {
             return ans == 'Y';
          }

try_again:
          os.print ("Please answer y or n.\n");
      }

      // Failed after 5 retries
      return false ;
   }
}

void CommandInterpreter::inject_command (ComplexCommand *command) {
    injected_commands.push (command->clone()) ;
}

void CommandInterpreter::load_env () {
    char** eptr = environ;
    char* c;

    c = *eptr;
    while (c != NULL) {
        size_t i = strcspn(c,"=");
        std::string s(c);

        saved_env[s.substr(0,i)] = s.substr(i+1);

        eptr++; c = *eptr;
    }
}


void CommandInterpreter::set_env (const std::string& var, const std::string& val) {
    saved_env[var] = val;
}

void CommandInterpreter::show_env(const std::string& var) {
    std::map<std::string,std::string>::iterator i;

    if ( var == "" ) {
       /* no variable show all of them */
       for (i=saved_env.begin(); i!=saved_env.end(); i++) {
          os.print("%s = %s\n", (i->first).c_str(), (i->second).c_str());
       }
       return;
    }

    /* search for the variables */
    i = saved_env.find(var);
    if (i == saved_env.end()) {
       os.print("Environmental variable \"%s\" not defined\n", var.c_str());
       return;
    }

    /* print out the value */
    os.print("%s = %s\n", (i->first).c_str(), (i->second).c_str());
}

void CommandInterpreter::quit() {
    save_history();
    pcm->detach_all() ;
    exit(0) ;
}

void CommandInterpreter::init_dirlist() {
    dirlist.clear() ;
    dirlist.push_back("$cwd") ;
    dirlist.push_back("$cdir") ;
}

void CommandInterpreter::add_dir(std::string dir) {
    if (dir[0] != '/') {
        char *pwd = getcwd (NULL, 0) ;
        dir = std::string(pwd) + "/" + dir ;
        free (pwd) ;
    }
    dirlist.push_back (dir) ;
    struct stat st ;
    if (stat (dir.c_str(), &st) != 0) {
        os.print ("Warning: %s: No such file or directory.\n", dir.c_str()) ;
    } else if (!S_ISDIR (st.st_mode)) {
        os.print ("warning: %s is not a directory.\n", dir.c_str()) ;
    }
}

void CommandInterpreter::show_dirlist() {
    os.print ("Source directories searched: ") ;
    bool colon = false ;
    for (int i = (int)dirlist.size() -1 ; i >= 0 ; i--) {
       if (colon) {
           os.print (":") ;
      }
       colon = true ;
       os.print ("%s", dirlist[i].c_str()) ;
    }
    os.print ("\n") ;
}


void CommandInterpreter::import_commands (std::string filename) {
    std::ifstream file (filename.c_str()) ;
    if (!file) {
    } else {
        std::istream *oldstream = instream ;
        instream = &file ;
        run() ;
        instream = oldstream ;
        file.close() ;
    }
}

bool CommandInterpreter::isemacs() {
    AnnoteType type = (AnnoteType)get_int_opt(PRM_ANNOTE);
    return (type == ANNOTATE_FULLN);
}

void CommandInterpreter::show_history() {
    history.show(0);
}

void CommandInterpreter::show_complete(const std::string& s) {
    completor->complete(s, s.size());
    completor->list_matches_bare();
}

void CommandInterpreter::read_file (std::string filename) {
    struct stat st ;
    if (stat (filename.c_str(), &st) != 0) {
        os.print ("Cannot open file %s\n", filename.c_str()) ;
        return ;
    }
    if (!S_ISREG(st.st_mode)) {
        os.print ("File %s is not a regular file\n", filename.c_str()) ;
        return ;
    }
    import_commands (filename) ;
}

void CommandInterpreter::add_alias (std::string name, std::string def) {
    AliasMap::iterator i = aliases.find (name) ;
    if (i != aliases.end()) {
        i->second = def ;
    } else {
        aliases[name] = def ;
    }
}

void CommandInterpreter::show_aliases() {
    for (AliasMap::iterator i = aliases.begin() ; i != aliases.end() ; i++) {
        os.print ("%-20s %s\n", i->first.c_str(), i->second.c_str()) ;
    }
}

void CommandInterpreter::show_alias(std::string name) {
    AliasMap::iterator i = aliases.find (name) ;
    if (i != aliases.end()) {
        os.print ("%-20s %s\n", i->first.c_str(), i->second.c_str()) ;
    }
}

void CommandInterpreter::remove_alias (std::string name) {
    AliasMap::iterator i = aliases.find (name) ;
    if (i != aliases.end()) {
        aliases.erase (i) ;
    }
}


// defined commands

void CommandInterpreter::define_command (std::string name) {
    DefinedCommand *cmd = NULL ;

    // check if it matches a builtin command
    CommandVec matched ;
    
    for (unsigned int i = 0 ; i < commands.size() ; i++) {
        commands[i]->check (name, matched) ;
    }
    if (matched.size() != 0) {
        std::string prompt = "Really redefine built-in command \"" + name + "\"" ;
        bool ok = confirm (NULL, prompt.c_str()) ;
        if (!ok) {
            os.print ("Command \"%s\" not redefined.\n", name.c_str()) ;
            return ;
        }
    }

    // check for hook- to give warning
    if (name.size() > 5 && strncmp(name.c_str(), "hook-", 5) == 0) {
        std::string cmdname = name.substr(5) ;
        if (cmdname != "stop") {                // stop is a pseudo command
            matched.clear() ;
            for (unsigned int i = 0 ; i < commands.size() ; i++) {
                commands[i]->check (cmdname, matched) ;
            }
            if (matched.size() == 0) {
                printf ("warning: Your new '%s' command does not hook any existing command.\n", name.c_str()) ;
                if (!confirm (NULL, "Proceed")) {
                    printf ("Not confirmed.\n") ;
                    return ;
                }
            }
        }
    }

    DefinedCommandMap::iterator i = defined_commands.find (name) ;
    if (i != defined_commands.end()) {
        std::string prompt = "Redefine command \"" + name + "\"" ;
        bool ok = confirm (NULL, prompt.c_str()) ;
        if (!ok) {
            os.print ("Command \"%s\" not redefined.\n", name.c_str()) ;
            return ;
        }
        cmd = i->second ;
        cmd->def.clear() ;
    } else {
        cmd = new DefinedCommand (name) ;
        defined_commands[name] = cmd ;
    }
    printf ("Type commands for definition of \"%s\".\n", name.c_str()) ;
    printf ("End with a line saying just \"end\".\n") ;
    for (;;) {
        std::string line = readline (">", false) ;
        if (line == "end") {
            break ;
        }
        ComplexCommand *ccmd = command (line) ; 
        rerun_push(line) ;
        cmd->def.push_back (ccmd) ;
    }
}

void CommandInterpreter::document_command (std::string name) {
    DefinedCommand *cmd = NULL ;

    // check if it matches a builtin command
    CommandVec matched ;
    
    for (unsigned int i = 0 ; i < commands.size() ; i++) {
        commands[i]->check (name, matched) ;
    }
    if (matched.size() != 0) {
        printf ("Command \"%s\" is built-in.\n", name.c_str()) ;
        return ;
    }

    DefinedCommandMap::iterator i = defined_commands.find (name) ;
    if (i != defined_commands.end()) {
        cmd = i->second ;
        cmd->help = "" ;
        printf ("Type documentation for \"%s\".\n", name.c_str()) ;
        printf ("End with a line saying just \"end\".\n") ;
        for (;;) {
            std::string line = readline (">", false) ;
            if (line == "end") {
                break ;
            }
            cmd->help += line + "\n" ;
        }

    } else {
        printf ("Undefined command \"%s\".  Try \"help\".\n", name.c_str()) ;
    }
}

// execute hook-stop command if it is present
void CommandInterpreter::stop_hook() {
    DefinedCommandMap::iterator i = defined_commands.find ("hook-stop") ;
    if (i != defined_commands.end()) {
        std::vector<std::string> args ;
        invoke_defined_command (i->second, args, 0, false) ;
    }
}

// a define command has been invoked, replace the args and inject its definition into the
// command stream
void CommandInterpreter::invoke_defined_command (DefinedCommand *cmd, std::vector<std::string> &args, int depth, bool execute_hook) {
    for (unsigned int i = 0 ; i < cmd->def.size() ; i++) {
        ComplexCommand *ccmd = cmd->def[i] ;
        execute_complex_command (ccmd, depth, args, execute_hook) ;
    }
}

void CommandInterpreter::show_defined_command (std::string name) {
    if (name == "") {
        for (DefinedCommandMap::iterator i = defined_commands.begin() ; i != defined_commands.end() ; i++) {
            i->second->print (os, 0) ;
        }
    } else {
        DefinedCommandMap::iterator i = defined_commands.find (name) ;
        if (i != defined_commands.end()) {
            i->second->print (os, 0) ;
        } else {
            printf ("Undefined command: \"%s\".  Try \"help\".\n", name.c_str()) ;
        }
    }
}

void DefinedCommand::print (PStream &os, int indent) {
    for (int i = 0 ; i < indent ; i++)  os.print (" ") ;
    os.print ("User command %s:\n", name.c_str()) ;
    for (unsigned int i = 0 ; i < def.size() ; i++) {
        def[i]->print (os, indent+2) ;
    }
}


// debugger variables

DebuggerVar *CommandInterpreter::find_debugger_variable (std::string n) {
    if (n == "$0") {
       n = "$" ;
    }
    if (n == "$") {             // most recent value
        char buf[32] ;
        snprintf (buf, sizeof(buf), "$%d", debugger_var_num) ;
        return find_debugger_variable (buf) ;
    }
    if (n[0] == '$' && n[1] == '$') {
        if (n.size() == 2) {                    // just $$
            char buf[32] ;
            snprintf (buf, sizeof(buf), "$%d", debugger_var_num-1) ;
            return find_debugger_variable (buf) ;
        } else {
            int num = 0 ;
            unsigned int i = 2 ;
            while (i < n.size() && isdigit (n[i])) {
                num = num * 10 + n[i++] - '0' ;
            }
            char buf[32] ;
            snprintf (buf, sizeof(buf), "$%d", debugger_var_num-num) ;
            return find_debugger_variable (buf) ;
        }
    }
    DebuggerVarMap::iterator i = debugger_variables.find (n) ;
    if (i == debugger_variables.end()) {
        return NULL ;
    }
    return i->second ;
}
                                                                                                                          
DebuggerVar *CommandInterpreter::add_debugger_variable (std::string n, Value &val, DIE *type) {
    DebuggerVar *var =  new DebuggerVar (val, type) ;
    debugger_variables[n] = var ;
    return var ;
}

//
// help system
//

void CommandInterpreter::open_help_file() {
    char link[256] ;
#if defined (__linux__)
    int e = readlink ("/proc/self/exe", link, sizeof(link)-1) ;
#elif defined (__FreeBSD__)
    int exe_path_mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, getpid() } ;
    size_t exe_path_size = sizeof (link) ;
    int e = sysctl (exe_path_mib, 4, link, &exe_path_size, NULL, 0) ;

    if (e != -1)
        e = strlen (link) ;
#endif
    if (e == -1) {
       throw Exception ("Unable to open help system file - unable to determine directory"); 
    }
    // remove the final executable name from the path
    if (e > 0) {
        // first iteration converts length to last index
        do {
            e-- ;
        } while (e != 0 && link[e] != '/') ;
    }
    // null terminate the string
    link[e] = 0 ;

    std::string helpfile = std::string(link) + "/../etc/pathdb-help.xml" ;
    std::ifstream in (helpfile.c_str()) ;
    if (!in) {
       throw Exception ("Unable to open help system file"); 
    }
    help_root = XML::parseStream (in) ;
    in.close() ;
}


static void print_body (PStream &os, XML::Element *element) {
    // formatting characters
    const int LIST = 0x80 ;            // start list
    const int ENDLIST = 0x81 ;                // end list
    const int LE = 0x82 ;                     // list element

    XML::Element *textelement = element->find ("XML.text") ;
    if (textelement != NULL) {
        std::string text = Command::trim (textelement->getBody()) ;
        // compress all multiple whitespace chars into 1 and replace all non-space whitespace by space
        std::string newtext ;
        bool ws = false ;
        bool sol = true ;
        for (unsigned int i = 0 ; i < text.size() ; i++) {
            if (i > 0 && text[i-1] == '\n') {
                sol = true ;
            } else {
                sol = false ;
            }
            if (isspace (text[i])) {
                if (!ws) {
                    ws = true ;
                    newtext += ' ' ;
                }
            } else if (sol && text[i] == '.' && !isspace(text[i+1])) {
                std::string tag ;
                i++ ;
                while (i < text.size() && text[i] != ';' && text[i] != '\n') {
                    tag += text[i++] ;
                }
                // printf ("tag = %s\n", tag.c_str()) ;
                if (tag == "p") {                       // new paragraph
                    newtext += "\n\n" ;
                } else if (tag == "pre") {
                     i++ ;
                     while (i < text.size()) {
                         if (text[i] == '.' && text[i+1] == 'e' && text[i+2] == 'p' &&
                              text[i+3] == 'r' && text[i+4] == 'e') {
                              i += 4 ;
                              break ;
                         }
                         newtext += text[i] ;
                         i++ ;
                     }
                } else if (tag == "list") {
                    newtext += (char)LIST ;
                } else if (tag == "elist") {
                    newtext += (char)ENDLIST ;
                } else if (tag == "le") {
                    newtext += (char)LE ;
                }
            } else if (text[i] == '.' && isspace (text[i+1])) {
                newtext += ".  " ;
                ws = true ;
                i++ ;
            } else {
                ws = false ;
                newtext += text[i] ;
            }
        }
        // now print the text to the stream, wrapping at word boundaries
        unsigned int width = os.get_width() ;
        unsigned int x = 0 ;                     // current postion
        int indent = 0 ;                        // current indentation level
        int index = 0 ;                         // currnet list index
        for (unsigned int i = 0 ; i < newtext.size() ; i++) {
            std::string word ;
            if (newtext[i] == '&') {                        // replace &lt; &amp; and &gt;
                ws = false ;
                std::string code ;
                unsigned int j = i ;
                while (j < newtext.size() && j < (i + 5) && newtext[j] != ';') {
                    code += newtext[j++] ;
                }
                if (code == "&lt") {
                    word = "<" ;
                    i = j ;
                } else if (code == "&gt") { 
                    word = ">" ;
                    i = j ;
                } else if (code == "&amp") { 
                    word = "&" ;
                    i = j ;
                } else {
                    word = "&" ;
                }
            } else if (isspace (newtext[i])) {                  // white space
                if (newtext[i] == '\n') {
                    os.print ("\n") ;
                    for (int j = 0 ; j < indent ; j++) {
                        os.print (" ") ;
                    }
                    x = indent ;
                } else {                        // XXX: tabs?
                    if (x > (width - 1)) {
                        os.print ("\n") ;
                        for (int j = 0 ; j < indent ; j++) {
                            os.print (" ") ;
                        }
                        x = indent ;
                    } else {
                        os.print ("%c", newtext[i]) ;
                        x++ ;
                    }
                }
                continue ;
            } else if ((int)newtext[i] & 0x80) {                // formatting command
                //printf ("formatting command: %x\n", newtext[i]) ;
                switch ((int)newtext[i] & 0xff) {
                case LIST:
                    index = 1 ;
                    indent += 2 ;
                    os.print ("\n") ;
                    for (int j = 0 ; j < indent ; j++) {
                        os.print (" ") ;
                    }
                    x = indent ;
                    break ;
                case ENDLIST:
                    os.print ("\n") ;
                    indent -= 2 ;
                    for (int j = 0 ; j < indent ; j++) {
                        os.print (" ") ;
                    }
                    x = indent ;
                    break ;
                case LE:
                    os.print ("\n") ;
                    for (int j = 0 ; j < indent ; j++) {
                        os.print (" ") ;
                    }
                    os.print ("%d. ", index++) ;
                    x = indent + 4 ;           // XXX: proper width?
                    break ;
                }
                continue ;
            } else {
                while (i < newtext.size() && !isspace(newtext[i]) && newtext[i] != '&') {
                    word += newtext[i++] ;
                }
                i-- ;                   // back to the space
            }   
            if ((x + word.size()) >= width) {
                os.print ("\n") ;       
                for (int j = 0 ; j < indent ; j++) {
                    os.print (" ") ;
                }
                x = indent ;
            }
            os.print ("%s", word.c_str()) ;
            x += word.size() ;
        }
        os.print ("\n") ;
    }
}

static void find_command_or_topic (XML::Element *root, std::string name, std::vector<XML::Element *> &result) {
    std::vector<XML::Element*> &children = root->getChildren() ;
    // first try exact match
    for (unsigned int i = 0 ; i < children.size() ; i++) {
        XML::Element *child = children[i] ;
        if (child->name == "command" || child->name == "topic") {
            std::string childname = child->getAttribute ("name") ;
            if (childname == name) {
                result.push_back (child) ;
            }
        }
    }
    
    // if no exact match, try substring match
    if (result.size() == 0) {
        for (unsigned int i = 0 ; i < children.size() ; i++) {
            XML::Element *child = children[i] ;
            if (child->name == "command" || child->name == "topic") {
                std::string childname = child->getAttribute ("name") ;
                if (strncmp (childname.c_str(), name.c_str(), name.size()) == 0) {
                    result.push_back (child) ;
                }
            }
        }
    }
}

void CommandInterpreter::command_help (XML::Element *command, std::string tail, int level) {
    if (level > 10) {
        os.print ("Max command help level exceeded\n") ;
        return ;
    }
    if (tail == "") {           // end of command help
        os.print ("Command syntax: %s %s\n", command->getAttribute("name").c_str(), command->getAttribute ("args").c_str()) ;
        XML::Element *purpose = command->find ("purpose") ;
        print_body (os, purpose) ;
        os.print ("\n") ;

        XML::Element *helptext = command->find ("help") ;
        print_body (os, helptext) ;
        os.print ("\n") ;

        std::vector<XML::Element*> &children = command->getChildren() ;
        bool header_output = false ;
        for (unsigned int i = 0 ; i < children.size() ; i++) {
            XML::Element *subcommand = children[i] ;
            if (subcommand->name == "command") {
                if (!header_output) {
                    os.print ("Sub commands available are:\n") ;
                    header_output = true ;
                }
                os.print ("%s -- ", subcommand->getAttribute ("name").c_str()) ;
                purpose = subcommand->find ("purpose"); 
                if (purpose != NULL) {
                    print_body (os, purpose) ;
                }
            }
        }

        XML::Element *seealso = command->find ("seealso") ;
        if (seealso != NULL) {
            os.print ("\nSee the following commands for more information:\n\n") ;
            std::string t = seealso->getAttribute ("commands") ;
            for (unsigned int i = 0 ; i < t.size() ; i++) {
                std::string w ;
                while (i < t.size() && t[i] != ',') {
                    w += t[i++] ;
                }
                std::vector<XML::Element*> results ;
                find_command_or_topic (help_root, w, results) ;
                if (results.size() == 0) {
                    continue ;
                }
                if (results[0]->name == "topic") {
                    std::string summary = results[0]->getAttribute ("summary") ;
                    os.print ("%-15s %s\n", w.c_str(), summary.c_str()) ;
                } else {
                    purpose = results[0]->find ("purpose") ;
                    if (purpose != NULL) {
                        os.print ("%-15s ", w.c_str()) ;
                        print_body (os, purpose) ;
                    }
                }
            }
        }

    } else {
        os.print ("Help for %s\n", tail.c_str()) ;
        unsigned int ch = 0 ;
        std::string root ;
        while (ch < tail.size() && isspace (tail[ch])) ch++ ;
        while (ch < tail.size() && !isspace (tail[ch])) {
            root += tail[ch++] ;
        }
        while (ch < tail.size() && isspace (tail[ch])) ch++ ;
        tail = tail.substr (ch) ;

        AliasMap::iterator alias = aliases.find (root) ;
        if (alias != aliases.end()) {
            os.print ("%s is an alias for %s\n", root.c_str(), alias->second.c_str()) ;
            command_help (help_root, alias->second + " " + tail, level+1) ;
            return ;
        }

        DefinedCommandMap::iterator defcmd = defined_commands.find (root) ;
        if (defcmd != defined_commands.end()) {
            DefinedCommand *cmd = defcmd->second ;
            if (cmd->help != "") {
                os.print ("%s", cmd->help.c_str()) ;
            } else {
                os.print ("%s is a user-defined command:\n", root.c_str()) ;
                for (unsigned int i = 0 ; i < cmd->def.size() ; i++) {
                    cmd->def[i]->print (os, 2) ;
                }
            }
            return ;
        }

        std::vector<XML::Element*> results ;
        find_command_or_topic (command, root, results) ;
        if (results.size() == 0) {
            os.print ("Sorry, no help available for %s.  Try 'help Commands'\n", root.c_str()) ;
            return ;
        }

        for (unsigned int i = 0 ; i < results.size() ; i++) {
            XML::Element *subcommand = results[i] ;
            if (subcommand->name == "command") {                        // help on a specific command?
                command_help (subcommand, tail, level) ;
            } else if (subcommand->name == "topic") {
                std::string topic = subcommand->getAttribute ("name") ;
                if (topic == "Commands") {                              // commands is special, list all commands
                    print_body (os, subcommand) ;
                    std::vector<XML::Element*> &children = help_root->getChildren() ;
                    for (unsigned int i = 0 ; i < children.size() ; i++) {
                        XML::Element *child = children[i] ;
                        if (child->name == "command") {
                            XML::Element *purpose = child->find ("purpose") ;
                            if (purpose != NULL) {
                                os.print ("%-15s ", child->getAttribute("name").c_str()) ;
                                print_body (os, purpose) ;
                            }
                        }
                    }
                } else {
                    print_body (os, subcommand) ;                          // help on a topic
                }
            }
        }
    }
}


void CommandInterpreter::help (std::string text) {
    if (text == "") {
        std::vector<XML::Element*> &children = help_root->getChildren() ;
        for (unsigned int i = 0 ; i < children.size() ; i++) {
            XML::Element *child = children[i] ;
            if (child->name == "mainhelp") {
                print_body (os,child) ;
                break ;
            }
        }
        os.print ("\n") ;
    } else {
        command_help (help_root, text) ;
    }
}

void ComplexCommand::print (PStream &os, int indent) {
    for (int i = 0 ; i < indent ; i++)  os.print (" ") ;
    switch (code) {
    case CMD_REG:
        os.print ("%s\n", head.c_str()) ;
        break ;
    case CMD_WHILE:
        os.print ("while %s\n", head.c_str()) ;
        left->print (os, indent+2) ;
        for (int i = 0 ; i < indent ; i++)  os.print (" ") ;
        os.print ("end\n") ;
        break ;
    case CMD_IF:
        os.print ("if %s\n", head.c_str()) ;
        left->print (os, indent+2) ;
        if (right != NULL) {
            for (int i = 0 ; i < indent ; i++)  os.print (" ") ;
            os.print ("else\n") ;
            right->print (os, indent+2) ;
        }
        for (int i = 0 ; i < indent ; i++)  os.print (" ") ;
        os.print ("end\n") ;
        break ;
    case CMD_SEP:
        if (left != NULL) {
            left->print (os, indent) ;
        }
        if (right != NULL) {
            right->print (os, indent) ;
        }
        break ;
    }
}

ComplexCommand *ComplexCommand::clone () {
    ComplexCommand *cmd = new ComplexCommand (code, head) ;
    if (left != NULL) {
        cmd->left = left->clone() ;
    }
    if (right != NULL) {
        cmd->right = right->clone() ;
    }
    return cmd ;
}

ComplexCommand *CommandInterpreter::command(std::string line, int indent) {
    int ch = 0 ;
    std::string root = Command::extract_word (line, ch) ;
    Command::skip_spaces (line, ch) ;
    if (root == "if") {
        return if_block (line.substr (ch), indent+1);
    } else if (root == "while") {
        return while_block (line.substr(ch), indent+1) ;
    } else {
        return new ComplexCommand (CMD_REG, line) ;
    }
}

ComplexCommand *CommandInterpreter::while_block(std::string head, int indent) {
    ComplexCommand *cmd = new ComplexCommand (CMD_WHILE, head) ;
    ComplexCommand *block = NULL ;
    std::string prompt ;
    for (int i = 0 ; i < indent ; i++) prompt += ' ' ;
    prompt += ">" ;
    for (;;) {
        std::string line = readline (prompt.c_str(), get_flags() & CLI_FLAG_NOTERM) ;
        if (line == "end") {
            break ;
        }
        rerun_push(line) ;
        ComplexCommand *c = command (line, indent) ;
        if (block == NULL) {
            block = c ;
        } else {
            block = new ComplexCommand (CMD_SEP, block, c) ;
        }
    }
    cmd->left = block ;
    return cmd ;
}

ComplexCommand *CommandInterpreter::if_block(std::string head, int indent) {
    ComplexCommand *cmd = new ComplexCommand (CMD_IF, head) ;
    ComplexCommand *ifblock = NULL ;
    ComplexCommand *elseblock = NULL ;
    ComplexCommand **block = &ifblock ;
    std::string prompt ;
    for (int i = 0 ; i < indent ; i++) prompt += ' ' ;
    prompt += ">" ;
    for (;;) {
        std::string line = readline (prompt.c_str(), false) ;
        if (line == "end") {
            break ;
        }
        rerun_push(line) ;
        if (line == "else") {
            if (block == &elseblock) {
                printf ("Multiple 'else' clauses for if command") ;
                break ;
            } else {
                block = &elseblock ;
            }
            continue ;
        }
        ComplexCommand *c = command (line, indent) ;
        if (*block == NULL) {
            *block = c ;
        } else {
            *block = new ComplexCommand (CMD_SEP, *block, c) ;
        }
    }
    cmd->left = ifblock ;
    cmd->right = elseblock ;
    return cmd ;
}

void CommandInterpreter::execute_complex_command (ComplexCommand *cmd, int depth, std::vector<std::string> &args, bool execute_hook) {
    if (cmd == NULL) {
        return ;
    }
    // replace the args in the head
    std::string &line = cmd->head ;
    std::string newline ;
    unsigned int ch = 0 ;
    while (ch < line.size()) {
       if (line[ch] == '\\') {
           newline += line[ch++] ;
       } else if (line[ch] == '$') {                // argument?
           std::string argname ;
           ch++ ;
           while (ch < line.size() && !isspace(line[ch])) {
               argname += line[ch++] ;
           }
           if (argname.size() == 4) {              // argn
               if (strncmp (argname.c_str(), "arg", 3) == 0 && isdigit(argname[3])) {
                   int n = argname[3] - '0' ;
                   if (n >= (int)args.size()) {
                       os.print ("Missing argument %d in user function.\n", n) ;            // user function?  eh?
                       return ;
                   } 
                   newline += args[n] ;
               } else {
                   newline += argname ;
               }
           } else {
               newline += argname ;
           }
       } else {
           newline += line[ch++] ;
       }
    }

    int end = 0 ;
    switch (cmd->code) {
    case CMD_REG: 
        execute_command (newline, false, depth+1, execute_hook) ;
        break ;
    case CMD_WHILE: 
        while (pcm->evaluate_expression (newline, end, true) != 0) {
            execute_complex_command (cmd->left, depth, args, execute_hook) ;
        }
        break ;
    case CMD_IF:
        if (pcm->evaluate_expression (newline, end, true) != 0) {
            execute_complex_command (cmd->left, depth, args, execute_hook) ;
        } else {
            execute_complex_command (cmd->right, depth, args, execute_hook) ;
        }
        break ;
    case CMD_SEP:
        execute_complex_command (cmd->left, depth, args, execute_hook) ;
        execute_complex_command (cmd->right, depth, args, execute_hook) ;
        break ;
    }
}

void CommandInterpreter::execute_complex_command (ComplexCommand *cmd, int depth) {
    if (cmd == NULL) {
        return ;
    }
    int end = 0 ;
    switch (cmd->code) {
    case CMD_REG: {
        execute_command (cmd->head, false, depth) ;
        break ;
        }
    case CMD_WHILE: 
        while (pcm->evaluate_expression (cmd->head, end, true) != 0) {
            execute_complex_command (cmd->left) ;
        }
        break ;
    case CMD_IF:
        if (pcm->evaluate_expression (cmd->head, end, true) != 0) {
            execute_complex_command (cmd->left) ;
        } else {
            execute_complex_command (cmd->right) ;
        }
        break ;
    case CMD_SEP:
        execute_complex_command (cmd->left) ;
        execute_complex_command (cmd->right) ;
        break ;
    }
}


std::string CommandInterpreter::complete_command (std::string command, int ch) {
    CommandVec matches ;
    int pos = 0 ;
    while (pos < ch && isspace (command[pos])) pos++ ;
    std::string root ;
    while (pos < ch && !isspace (command[pos])) {
        root += command[pos++] ;
    }

    if (command == "") {
        return "" ;
    }

    // if it matches an alias, return the complete command, but only if the alias
    // is an abbreviation for the whole command.  An alias may have been defined
    // that changes the command completely.  In this case, don't expand it, but instead
    // replace the command.  This is also the case when we are not completing the alias.
   
    AliasMap::iterator alias = aliases.find (root) ;
    if (alias != aliases.end()) {
        // if we are completing the alias, return it
        if (pos == ch && strncmp (root.c_str(), alias->second.c_str(), root.size()) == 0) {
            return alias->second.substr (root.size()) + ' ';
        } else {
            // adjust ch to the new position
            int diff = alias->second.size() - root.size() ;
            ch += diff ;
            command = alias->second + command.substr (pos) ;            // replace root
            pos = 0 ;
            root = "" ;
            while (pos < ch && !isspace (command[pos])) {               // reset root
                root += command[pos++] ;
            }
        }
    }

    // look for the root in all the command processors
    for (unsigned int i = 0 ; i < commands.size() ; i++) {
        commands[i]->check (root, matches) ;
    }


    if (matches.size() == 0) {
        for (unsigned int i = 0 ; i < commands.size() ; i++) {
            commands[i]->check_abbreviation (root, matches) ;
        }
    }

    if (matches.size() == 0) {
        return "" ;
    }

    if (matches.size() == 1) {
        root = matches[0].cmd;
    }

    if (pos == ch) {            // first word?
        std::vector<std::string> m ;
        for (unsigned int i = 0 ; i < matches.size() ; i++) {
           m.push_back (matches[i].cmd) ;
        }
        completor->set_matches (m) ;
        completor->set_leadin("");

        if (matches.size() > 1) {
            return "" ;
        } else {
            return matches[0].cmd.substr (pos);
        }
    }

    // if we get here, the position is beyond the first word.  Ask the appropriate
    // command processor to complete the rest
    while (pos < (int)command.size() && pos < ch && isspace (command[pos])) pos++ ;
    Command *processor = matches[0].processor ;

    std::vector<std::string> matches2 ;

    processor->complete (root, command.substr(pos), ch - pos, matches2) ;

    // if no matches are found just return empty
    // this is needed to protected code which follows
    if (matches2.size() == 0)
    {
      completor->set_matches (matches2) ;
      completor->set_leadin("");
      return "";
    }

    // matches2 contains the set of names of all the files that match the prefix.  The
    // prefix has been removed.  We now have to find the common initial sequence.
    uint i = 0 ;
    bool done = false ;
    std::string prefix ;
    while (!done) {
        char c = '\0' ;
        bool same = true ;

        for (uint m = 0 ; m < matches2.size() ; m++) {
            if (i >= matches2[m].size()) {
                done = true ;
                same = false ;
                break ;
            }
            if (c == '\0') {
                c = matches2[m][i] ;
            } else if (matches2[m][i] != c) {
                same = false ;
                break ;
            }
        }
        if (same) {
            prefix += c ;
            i++ ;
        } else {
            done = true ;
        }
    }

    // fully-qualify the remaining completions
    for (uint i = 0 ; i < matches2.size() ; i++)
    { 
        matches2[i].insert(0, command.substr(pos)) ;
    }

    // hand over list of matches
    completor->set_matches (matches2) ;
    completor->set_leadin(command.substr(0,pos));

    return prefix;
}

// write the completions for the filename specified to the result
void CommandInterpreter::complete_file (std::string s, std::vector<std::string> &result) {
    int ch = (int)s.size() - 1 ;
    while (ch > 0 && s[ch] != '/') ch-- ;
    
    std::string filename = ch <= 0 ? s : s.substr (ch+1) ;
    std::string dirname = ch <= 0 ? "." : s.substr (0, ch) ;

    if (dirname[0] == '~') {
        bool found = false ;
        std::string username = dirname.substr(1) ;
        struct passwd *user = getpwent() ;
        while (user != NULL) {
            if (user->pw_name == username) {
                found = true ;
                dirname = user->pw_dir ;
                break ;
            }
            user = getpwent() ;
        }
        if (!found) {
            return ;
        }
    }

    DIR *dir = opendir (dirname.c_str()) ;
    if (dir == NULL) {
        return ;
    }

    struct dirent *entry ;
    do {
       entry = readdir (dir) ;
       if (entry != NULL && strncmp (filename.c_str(), entry->d_name, filename.size()) == 0) {
           result.push_back (std::string(entry->d_name).substr (filename.size())) ;
       }
    } while (entry != NULL) ;
    closedir (dir) ;

}

// rerun command

void CommandInterpreter::rerun(int delta) {
    if ( delta <= 0 ) {
       throw Exception("Must specify a positive number");
    }
    if ( delta >= (int)rerun_hist.size() ) {
       throw Exception("Number of back steps is too large");
    }

    if (confirm (NULL, "Rerun the program")) {
        rerun_pop(delta);
        rerun_mark_spot();

        bool oldconfirm = get_int_opt(PRM_CONFIRM) ;
        set_opt("confirm", "off") ;
        pcm->delete_breakpoint (0) ;                    // delete all breakpoints
        pcm->reset_bp_num();
        std::istream *savedstream = instream ;          // save current input stream
        std::stringstream newcommands ;
        for (int i = 0 ; i < (int)rerun_hist.size() - delta ; i++) {
            newcommands << rerun_hist[i] << "\n" ;
        }
        instream = &newcommands ;                       // redirect input to rerun commands
        run() ;                                         // replay the commands
        instream = savedstream ;                        // restore input stream
        set_opt("confirm", oldconfirm?"on":"off") ;

        rerun_reset();
    } else {
        printf ("Program not rerun\n") ;
    }
}


std::string CommandInterpreter::readline (const char *prompt, bool recordhist) {
   /* no history for redirect */
   if (instream != NULL) {
      std::string s ;
      std::getline (*instream, s) ;
      return s ;
   }

   std::string str;
   if (recordhist) {
      readl.setprompt(prompt);
      readl.sethistory(history.get_list());
      readl.setcompl(completor);
      readl.setdumb(flags & CLI_FLAG_NOTERM);
      str = readl.getline();
      history.push_back(str);
   } else {
      readl.setprompt(prompt);
      readl.sethistory(NULL);
      readl.setcompl(NULL);
      readl.setdumb(flags & CLI_FLAG_NOTERM);
      str = readl.getline();
   }
   return str;
}

