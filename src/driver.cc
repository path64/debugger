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

file: driver.cc
created on: Fri Aug 13 11:07:39 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#include <unistd.h>
#include "debugger.h"
#include "cli.h"
#include "pstream.h"
#include "target.h"
#include <sys/stat.h>
#include <ctype.h>
#include "version.h"
#include "version_bk.h"

void usage() {
   std::cerr << "usage: pathdb [option...] [program [core|pid]]\n" ;
   exit (1); 
}

int main (int argc, char **argv) {
    std::string prompt = "pathdb> ";
    std::string filename = "" ;
    std::string corepid = "" ;
    std::string run_args = "";
    std::string commandfile ;
    int cli_flags = 0 ;
    bool quiet = false ;
    bool subverbose = false ;
    bool print_ver = false ;
    bool print_vver = false;

    for (int i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-') {
            std::string flag = argv[i] ;
            if (flag.size() > 2 && flag[1] == '-') {
                flag = flag.substr (1) ;
            }
            if (flag == "-nw") {
                 /* XXX: why isn't this implemented? */
            } else if (flag == "-nx") {
                cli_flags |= CLI_FLAG_NOINIT ;
            } else if (flag == "-nt") {
                cli_flags |= CLI_FLAG_NOTERM ;
            } else if (flag == "-gdb") {                // gdb compatibility
                cli_flags |= CLI_FLAG_GDB ;
            } else if (flag == "-fullname") {
                cli_flags |= CLI_FLAG_EMACS ;
            } else if (flag == "-cd") {
               i++;
               if (i < argc) {
                  const char* s = argv[i];
                  if (chdir(s) != 0) {
                     std::cerr << "pathdb: option '-cd' contains invalid directory\n";
                     exit(1);
                  }
               } else {
                  std::cerr << "pathdb: '-cd' requires an argument\n";
                  exit(1);
               }
            } else if (flag == "-subverbose") {
                subverbose = true ;
            } else if (flag == "-prompt") {
                i++;
                if (i < argc) {
                   prompt = argv[i];
                } else {
                   std::cerr << "pathdb: option '-prompt' requires an argument\n" ;
                   exit(1);
                }
            } else if (flag == "-args") {
                if (++i >= argc) {
                    std::cerr << "pathdb: option '-args' requires an argument\n" ;
                    exit(1);
                }

                filename = argv[i++];

                // first without space
                if (i < argc) {
                   run_args += argv[i++];
                }

                // remaining with leading space
                while (i < argc) {
                    run_args += " ";
                    run_args += argv[i++];
                }
            } else if (flag == "-command" || flag == "-x") {
                unsigned int ch = 0 ;
                while (ch < flag.size() && flag[ch] != '=') {
                    ch++ ;
                }
                if (ch == flag.size()) {
                    if (i == (argc-1)) {
                        std::cerr << "pathdb: option '-command' requires an argument\n" ;
                        exit(1);
                    } else {
                       commandfile = argv[i+1] ;
                       i++ ;
                    }
                } else {
                    commandfile = flag.substr (ch+1) ;
                }
            } else if (flag == "-quiet" || flag == "-q") {
                quiet = true ;
            } else if (flag == "-version" || flag == "-v") {
                print_ver = true;
            } else if (flag == "-vv") {
                print_vver = true;
            } else {
                usage() ;
            }
        } else {
            if (filename == "") {
                filename = argv[i] ;            // first arg is filename
            } else if (corepid == "") {
                corepid = argv[i] ;             // second is core/pid
            } else {
                usage() ;
            }
        }
    }

    if (print_vver) {
        printf ("PathScale Debugger, Version %s\n", VERSION) ;
        printf ("%s\n", COPYNOTE);
        printf ("Changeset: %s (%s)\n", cset_rev, cset_key);
        printf ("Build Host: %s\n", build_host);
        printf ("Build Date: %s\n", build_date);
        exit(0);
    }

    if (print_ver) {
        printf ("PathScale Debugger, Version %s\n", VERSION) ;
        printf ("%s\n", COPYNOTE);
        exit(0);
    }

    if (!quiet) {
        printf ("PathScale Debugger, Version %s\n", VERSION) ;
        printf ("%s\n", COPYNOTE);
    }

    try {
        PStream output (1) ;          // output stream attached to standard output

        DirectoryTable dirlist ;
        //ProcessController pcm (output, &aliases, dirlist, subverbose) ;
        CommandInterpreter cli (output, dirlist, cli_flags, subverbose) ;

        if ( run_args != "" ) {
           cli.set_args (run_args);
        }

        cli.set_opt("prompt", prompt.c_str());

        if (filename != "") {
            if (corepid == "") {                        // no core/pid arg, we have a new program
                cli.pcm->attach (filename, true) ;
            } else {
                // the user passed a core/pid arg, have to work out what it is
                if (isdigit (corepid[0])) {         // possible pid, but might also be core file
                    int pid = atoi (corepid.c_str()) ;          // XXX: make this better
                    cli.pcm->attach (pid, true) ;
                } else {
                    cli.pcm->attach_core (filename, corepid, true) ;
                }
            }
            if (filename.size() > 5 && filename.find("pathdb") == filename.size() - 6) {
                cli.set_opt("prompt", "top-pathdb> ") ;
            }
            if (filename.size() > 7 && filename.find("pathdb64") == filename.size() - 8) {
                cli.set_opt("prompt", "top-pathdb> ") ;
            }
        }
        if (commandfile != "") {        
            cli.import_commands (commandfile) ;
        }
        cli.import_pathdbrc() ;
        cli.run() ;

    // NOTE: For some reason, stdout is closed at this point.  It appears to be related
    // to throwing exceptions but I don't know why.  Possible bug in runtime library
    } catch (Exception e) {
        e.report(std::cerr) ;
        exit (1) ;
    } catch (Exception *e) {
        e->report(std::cout) ;
        exit (1) ;
    } catch (const char *s) {
        std::cerr << s << "\n" ;
        exit (1) ;
    } catch (std::string s) {
        std::cerr << s << "\n" ;
        exit (1) ;
    } catch (...) {
        std::cerr << "unknown exception in driver\n" ;
        exit (1) ;
    }
    exit (0) ;
}

