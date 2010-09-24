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

file: cli.h
created on: Fri Aug 13 11:02:26 PDT 2004
author: David Allison <dallison@pathscale.com>

*/

#ifndef cli_h_included
#define cli_h_included

#include "cli_history.h"
#include "dbg_types.h"
#include "dbg_dwarf.h"
#include "file_info.h"
#include <queue>
#include <signal.h>
#include <fstream>
#include "target.h"
#include "cli_param.h"
#include "xml.h"
#include "readline.h"
#include <sstream>

class ProcessController ;
class Command ;
class Format ;
class DebuggerVar ;

class CommandInterpreter ;

// command line interpreter

struct Match {
    Match (std::string c, Command *p):cmd(c), processor(p) {}
    std::string cmd  ;
    Command *processor ;
} ;

typedef std::vector<Match> CommandVec ;

class CommandCompletor : public Completor {
public:
    CommandCompletor (CommandInterpreter *cli, PStream &os) : cli(cli), os(os) {}
    virtual ~CommandCompletor() { }
	
    std::string complete (std::string text, int ch) ;
    void reset() ;
    void set_matches (std::vector<std::string> &m) ;
    void set_leadin(const std::string &t);
    void list_matches ();
    void list_matches_bare ();
    unsigned num_matches () ;
private:
    CommandInterpreter *cli ;
    PStream &os ;
    std::string leadin;
    std::vector<std::string> matches ;
} ;

class Command {
public:
    Command (CommandInterpreter *cli, ProcessController *pcm, const char **commands) ;
    virtual ~Command() { }

    virtual void check (std::string root, CommandVec &result) ;          // check if string matches command exactly
    virtual void check_abbreviation (std::string root, CommandVec &result) ;          // check if string matches command abbreviation
    virtual void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) { }

    virtual void execute (std::string root, std::string tail) = 0 ;          // parse the command
    virtual bool is_dangerous (std::string cmd) { return false ; }

protected:
    CommandInterpreter *cli ;
    ProcessController *pcm ;
    const char **commands;

public:

public:
    static std::vector<std::string> split (std::string s) ;
    static std::string trim (std::string s) ;
    static int extract_number (std::string s, int &ch) ;
    static std::string extract_word(const std::string& s, int &ch) ;
    static void split_shell(CommandInterpreter*,const std::string&, std::vector<std::string>*);
    static std::string extract_shell(CommandInterpreter*,const std::string& s, int &ch);
    static void skip_spaces (std::string s, int &ch) ;
    void get_address_arg (std::string tail, std::vector<Address> &result, bool allow_all, bool skip_preamble, int &end) ;
    void get_address_arg (std::string tail, std::vector<Address> &result, bool allow_all, bool skip_preamble) ;
    static std::string unescape (std::string s) ;


private:
    static bool extract_qtoken(const std::string& s, std::string& tok, int& ch);
    static std::string expand_env (CommandInterpreter* cli, const std::string&);
} ;




class ControlCommand : public Command {
public:
    virtual ~ControlCommand() { }
    ControlCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    bool is_dangerous (std::string cmd) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;

class DebuggerCommand : public Command {
public:
    virtual ~DebuggerCommand() { }
    DebuggerCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;


class BreakpointCommand : public Command {
public:
    virtual ~BreakpointCommand() { }
    BreakpointCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
    bool is_dangerous (std::string cmd) ;
private:
    static const char *cmds[] ;
} ;

class CatchSubcommand : public Command {
public:
    CatchSubcommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;

class CatchCommand : public Command {
public:
    CatchCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
    CatchSubcommand *subcommands ;
} ;

class StackCommand : public Command {
public:
    StackCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;

class PrintCommand : public Command {
public:
    PrintCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
    void extract_format (std::string s, int &ch, Format &f) ;
    Format *last_format ;
    Address get_list_address (std::string s) ;
} ;

class QuitCommand : public Command {
public:
    QuitCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
private:
    static const char *cmds[] ;
} ;


class InfoSubcommand : public Command {
public:
    InfoSubcommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;

class InfoCommand : public Command {
public:
    InfoCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
    InfoSubcommand *subcommands ;
} ;


class SetSubcommand : public Command {
public:
    SetSubcommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void check_abbreviation (std::string root, CommandVec &result) ;          // check if string matches command abbreviation
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;


class SetCommand : public Command {
public:
    SetCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
    SetSubcommand *subcommands ;
} ;

class ShowSubcommand : public Command {
public:
    ShowSubcommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void check_abbreviation (std::string root, CommandVec &result) ;          // check if string matches command abbreviation
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
} ;



class ShowCommand : public Command {
public:
    ShowCommand(CommandInterpreter *cli, ProcessController *pcm) ;
    void execute (std::string root, std::string tail) ;
    void  complete (std::string root, std::string tail, int ch, std::vector<std::string> &result) ;
private:
    static const char *cmds[] ;
    ShowSubcommand *subcommands ;
} ;

// complex commands (like while and if) are held in a tree.
enum ComplexCommandCode {
    CMD_REG,            // regular command
    CMD_WHILE,          // while command
    CMD_IF,             // if command
    CMD_SEP             // command separator
} ;

class ComplexCommand {
    public:
    ComplexCommand (ComplexCommandCode c, std::string h): code(c), head(h), left(NULL), right(NULL) {}
    ComplexCommand (ComplexCommandCode c, ComplexCommand *l, ComplexCommand *r): code(c), left(l), right(r) {}
    ~ComplexCommand() { if (left != NULL) delete left ; if (right != NULL) delete right ; }
    ComplexCommandCode code ;
    std::string head ;
    ComplexCommand *left ;
    ComplexCommand *right ;

    void print (PStream &os, int indent=0) ;
    ComplexCommand *clone() ;
} ;


struct DefinedCommand {
    DefinedCommand (std::string nm) : name(nm) {}
    std::string name ;                  // command name
    std::vector<ComplexCommand *> def ;      // command definition
    std::string help ;
    void print (PStream &os, int indent=0) ;
} ;

// command interpreter flags
const int CLI_FLAG_NOINIT = 0x01 ;
const int CLI_FLAG_NOTERM = 0x02 ;              
const int CLI_FLAG_GDB = 0x04 ;                 // gdb compatibility
const int CLI_FLAG_EMACS = 0x08 ;

class CommandInterpreter {
public:
    CommandInterpreter (ProcessController *pcm, PStream &os, DirectoryTable &dirlist, int flags) ;
    void run(Process *proc=NULL, Address endsp = 0) ;
    void quit() ;
    bool confirm (const char *prompt1, const char *prompt2) ;
    PStream &get_os() { return os ; }
    void inject_command (ComplexCommand *command) ;
    void execute_subcommand (const char *name, Command *cmd, std::string subcmd) ;
    void interrupt(int signum) ;

    ProcessController *get_pcm() { return pcm ; }

    void execute_command (std::string cmd, bool is_repeat, int depth=0, bool execute_hook=true) ;

    std::string complete_command (std::string command, int ch) ;

    std::string readline (const char *prompt, bool recordhist) ;


    // Routines to set and access program arguments 
    const std::string& get_args() const {
       return saved_args;
    }
    void set_args (const std::string& args) {
       set_opt("args", args.c_str());
       saved_args = args;
    }

    void set_running(bool v) { program_running = v ; }
    std::string expand_env (const std::string& s);
    void set_env (const std::string&, const std::string&);
    void show_env(const std::string& var);
    EnvMap& get_env() { return saved_env; }
    const char* get_env(const std::string& name) {
       EnvMap::iterator i = saved_env.find(name);
       if (i != saved_env.end()) {
          return i->second.c_str();
       }
       return NULL;
    }
    void load_env();

    void init_dirlist() ;
    void add_dir (std::string dir) ;
    void show_dirlist() ;

    /* access and modification of user options */
    long get_int_opt(CliParamId id) {
       long a = options.get_int(id);

       /* some have magic meaning */
       if (id == PRM_WIDTH || id == PRM_HEIGHT) {
          return a <= 0 ? 1000000L : a;
       }

       /* but most actually don't */
       return a;
    }
    const char* get_str_opt(CliParamId id) {
       return options.get_str(id);
    }
    bool set_opt(const char* nm, const char* val) {
       return options.set(nm, val);
    }
    void show_opts() {
       options.show_values();    
    }
    void show_opt(const char* nm) {
       options.show_value(nm);
    }

    void show_history() ; 
    void show_complete(const std::string& s);
    void read_file (std::string file) ;

    // alias management
    void add_alias (std::string name, std::string def) ;
    void show_aliases() ;
    void show_alias (std::string name) ;
    void remove_alias (std::string name) ;

    // command definition
    void define_command (std::string name) ;
    void document_command (std::string name) ;
    void invoke_defined_command (DefinedCommand *cmd, std::vector<std::string> &args, int depth, bool execute_hook) ;
    void show_defined_command (std::string name) ;

    void import_pathdbrc() ;
    void import_commands (std::string file) ;
    int get_flags() { return flags ; }
    std::string get_home_dir() { return home ; }

    // complex commands
    ComplexCommand *while_block(std::string head, int indent = 0) ;
    ComplexCommand *if_block(std::string head, int indent = 0) ;
    ComplexCommand *command (std::string line, int indent = 0) ;

    void execute_complex_command (ComplexCommand *cmd, int depth=0) ;
    void execute_complex_command (ComplexCommand *cmd, int depth, std::vector<std::string> &args, bool execute_hook=true) ;

    void stop_hook() ;

    void complete_file (std::string s, std::vector<std::string> &result) ;

    // emacs mode flag
    bool isemacs() ;

    /* should be private */
    void load_history();
    void save_history();
    void shrink_history();

private:
    ProcessController *pcm ;
    PStream &os ;
    std::vector<Command*> commands ;
    void (*oldint)(int);
    bool program_running ;
    std::istream *instream ;            // current input stream (NULL means keyboard)

    std::queue<ComplexCommand *> injected_commands ;

    std::string saved_args;
    std::map<std::string,std::string> saved_env;

    DirectoryTable &dirlist ;

    /* user options */
    CliParam options;

    /* command history */
    CliHistory history;

    void init_pstream();

    // aliases
    typedef std::map<std::string, std::string> AliasMap ;
    AliasMap aliases ;

    // defined commands
    typedef std::map<std::string, DefinedCommand *> DefinedCommandMap ;
    DefinedCommandMap defined_commands ;

    int flags ;

    // debugger variables
    typedef std::map<std::string, DebuggerVar*> DebuggerVarMap ;

    DebuggerVarMap debugger_variables ;
    int debugger_var_num ;
    DIE *debugger_var_type ;
    std::string home ;
public:
    DebuggerVar *find_debugger_variable (std::string name) ;
    DebuggerVar *add_debugger_variable (std::string name, Value &val, DIE *type) ;
    int new_debugger_var_num() { return ++debugger_var_num ; }

    void set_last_breakpoint (int n) { last_breakpoint_num = n ; }
    int get_last_breakpoint() { return last_breakpoint_num ;}
private:
    // help
    XML::Element *help_root ;
    void open_help_file() ;
    void command_help (XML::Element *command, std::string tail, int level=0) ;
    int last_breakpoint_num ;           // last breakpoint set
public:
    void help (std::string text) ;
private:

    CommandCompletor *completor ;

// rerun command support
    std::vector<std::string> rerun_hist;
    int rerun_mark;

public:
    void begin_paging() { os.begin_paging(); }
    void end_paging() { os.end_paging(); }

    void rerun_push(const std::string& root, const std::string tail) {
       if ( tail != "" ) {
          rerun_hist.push_back(root + " " + tail);
       } else {
          rerun_hist.push_back(root);
       }
    }
    void rerun_mark_spot() {
       rerun_mark = rerun_hist.size() - 1;
    }
    void rerun_push(std::string s) {
       rerun_hist.push_back(s);
    }
    void rerun_reset() { 
       if ( rerun_mark > 0 ) {
          rerun_pop(rerun_hist.size() - rerun_mark - 1);
       }
       rerun_mark = rerun_hist.size()  - 1;
    }
    void rerun_pop(int i) { 
       for (; i > 0 && !rerun_hist.empty(); i--) {
          rerun_hist.pop_back();
       }
    }

    void rerun(int delta) ;
} ;

#endif
