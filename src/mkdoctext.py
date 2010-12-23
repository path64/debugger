#!/usr/bin/python

import sys
import xml.dom.minidom

#  Useful definitions
###########################################################
prog_name = "mkdoctext.py"
objt_name = sys.argv[1]

#  Lookup table for entries
###########################################################
body_hash = {}
ident_hash = {}
hash_id = 0
tab = "   "

#  Helper Functions
###########################################################

# Die verbose death
def die (x):
  print (prog_name + " error: " + x)
  sys.exit(1)

# Write to output file
def do_write(s,indent=1):
   m = tab*indent + s
   sys.stdout.write(m)

# Escape lines for output
def do_lines(s):
   # Protect following
   if s == None:
      return 
   if len(s) == 0:
      return

   # Start of string
   spos = 0
   retval = ""

   # Flatly replace some
   s = s.replace("\\","\\\\")
   s = s.replace("\"","\\\"")
   s = s.replace("\'","\\\'")

   # Running newline replace
   x = s.find('\n',spos)
   while x != -1:
      retval = "\""
      retval += s[spos:x]
      retval += "\\n\"\n"
      do_write(retval) 
      spos = x + 1
      x = s.find('\n',spos)

   # Setup the last bit
   if spos != len(s):
      retval = "\""
      retval += s[spos:]
      retval += "\\n\"\n"
      do_write(retval)

# Tabify a text block
def tabify(s):
   # Setup initial
   p = 0
   r = ""

   # Traverse string
   t = s.find("\n")
   while t != -1:
      r = r+tab
      r = r+s[p:t+1]
      p = t + 1
      t = s.find("\n",p)

   # Clip on hanging gabits
   if p != len(s):
      r = r + s[p:]

   return r

# Trimspace from text block
def trimspace(s):
   # All whitespace falls
   if s.isspace():
      return "" 

   # Setup initial
   p = 0
   r = ""

   # Clip leading space
   for i in range(0,len(s)) :
      if not s[i].isspace() :
         break 
   s = s[i:]
 
   # Travserse string
   t = s.find("\n")
   while t != -1:
      m = s[p:t]
      r = r+m.strip()+"\n"
      p = t + 1
      t = s.find("\n",p)

   # Clip on hanging gabits
   if p != len(s) and \
      not s[p:].isspace():
      r = r+s[p:].strip()+"\n"
   return r    


# Textify a list object
def list_textify(lNode,indent=0):
   # Concat of output
   s = ""
   n = 1

   # Traverse list items
   for x in lNode.childNodes:
      if x.nodeType == x.COMMENT_NODE:
         continue
      if x.nodeType == x.ELEMENT_NODE:
         if x.nodeName != "item":
            die("lists can only have items") 
         for y in x.childNodes:
            t = str(n)+ ". " + textify(y)
            s = s + tabify(t)
            n = n + 1
         continue
      if x.nodeType == x.TEXT_NODE:
         if not x.nodeValue.isspace():
            die("invalid text in list")

   # Return formated text
   return s


# Textify an object block
def textify(aNode,indent=0):

   # Some just pass right through
   if aNode.nodeType == x.TEXT_NODE:
      return trimspace(aNode.nodeValue)
   if aNode.nodeType == x.COMMENT_NODE:
      return ""

   # Elements are handled case-by-case
   if aNode.nodeType == x.ELEMENT_NODE:
      if aNode.nodeName == "list":
         return list_textify(aNode,indent)
      die("invalid tag "+aNode.nodeName)   

   # Oh crap, just bail out 
   die("unable to parse "+aNode.nodeName)

# Accumlate text nodes
def accum_nodes(node):
   text = ""
   for x in node.childNodes:
      text = text + textify(x)
   return text

# Parse a topic element
def parse_topic(ticNode):
   global hash, hash_id

   # Get subject of topic element
   name = ticNode.attributes['name'].value

   # Textify all children
   text_body = accum_nodes(ticNode)

   # Add name to hash
   body_hash[name] = text_body
   ident_hash[hash_id] = name
   hash_id = hash_id + 1

# Parse a command element
def parse_command(comNode,parent=""):
   global hash, hash_id

   # Setup initial
   text_purpose = ""
   text_reefer = ""
   text_misc = ""
   text_help = ""

   # Get subject of command element
   name = comNode.attributes['name'].value

   # Get the fully-qualified name
   if parent == "": fullname = name
   else: fullname = parent + " " + name

   # Textify all children
   for x in comNode.childNodes:
      if x.nodeType == x.COMMENT_NODE:
         continue
      if x.nodeType == x.TEXT_NODE:
         if not x.nodeValue.isspace():
            die("invalid text in command")
         continue
      if x.nodeType != x.ELEMENT_NODE:
         die("invalid element in command "+name)
      if x.nodeName == "command":
         p = parse_command(x, fullname)
         if text_misc == "": text_misc = p
         else: text_misc = text_misc+"\n"+p
         continue
      if x.nodeName == "purpose":
         text_purpose = accum_nodes(x)
         continue
      if x.nodeName == "see":
         text_reefer = accum_nodes(x)
         continue
      if x.nodeName == "help":
         text_help = accum_nodes(x)
         continue

   # Check for purpose, everybody has a purpose!
   if text_purpose == "":
      die("command "+fullname+" has no purpose")

   # Layout the command title area
   text_purpose = text_purpose.strip()
   text_body = "Command "+name+": "+ text_purpose

   # Layout the help for this command
   if text_help != "":
      text_body = text_body+"\n"+text_help

   # Layout the help for subcommands
   if text_misc != "":
      text_body = text_body+"\nSubcommands:" 
      text_body = text_body+"\n"+text_misc

   # Layout any pointers you might have
   if text_reefer != "":
      text_body = text_body+"\nSee Also:"
      text_body = text_body+"\n"+text_reefer
 
   # Add name to hash
   body_hash[fullname] = text_body
   ident_hash[hash_id] = fullname
   hash_id = hash_id + 1

   # Return summary for anyone listening
   return fullname+": "+text_purpose


# Parse an element of pathdb-help
def parse_block(elemNode):
   name = elemNode.nodeName
   if name == 'topic':
      parse_topic(elemNode)
      return
   if name == 'command':
      parse_command(elemNode)
      return
   die("invalid element " + name)


# Parse the pathdb-help block
def parse_root(rootNode):
   if rootNode.nodeName != "pathdb-help":
      die("expected node 'pathdb-help'")
   for x in rootNode.childNodes:
      if x.nodeType == x.TEXT_NODE:
         if not x.nodeValue.isspace():
            die("invalid text: "+x.nodeValue)
         continue
      if x.nodeType == x.ELEMENT_NODE:
         parse_block(x)
         continue
      if x.nodeType == x.COMMENT_NODE:
         continue
      die("bad element type: "+x.nodeName) 


#  Main loop of program
###########################################################

# Do big 'ole parse of the XML tree
docNode = xml.dom.minidom.parse(objt_name)

# Printout out necessary includes
do_write("/* This file automatically generated by xml2src.py        */\n",0)
do_write("/* Do not edit this file directly, changes will be lost   */\n",0)
do_write("#include <stdlib.h>\n",0)
do_write("#include \"doctext.h\"\n\n\n",0)

# Even generated-files like comments
do_write("/* Text for each of the help entries                      */\n",0)
do_write("/**********************************************************/\n",0)

# Main tree traversal
for x in docNode.childNodes:
   if x.nodeType == x.TEXT_NODE:
      die("invalid top-level text block")
   if x.nodeType == x.ELEMENT_NODE:
      parse_root(x)
      continue
   if x.nodeType == x.COMMENT_NODE:
      continue

# Generate topic list from constructed tree
do_write("\nconst char* PATHDB_HELP_ENTRIES[] = {\n",0)
for x in range(0, hash_id):
   name = ident_hash[x]
   do_write('/* Begin entry: '+name+' */\n')
   do_lines(body_hash[name])
   do_write(',/* End entry: '+name+' */\n\n')
do_write("/* Cap the list with NULL */\n")
do_write("NULL\n")
do_write("};\n\n\n\n",0)

# Comments all around, it's on the house
do_write("/* Lookup table for help entries (allows entry aliasing)  */\n",0)
do_write("/**********************************************************/\n\n",0)


# Printout the hash table
do_write("pathdb_help_entry_t PATHDB_HELP_TABLE[] = {\n",0)
for x in ident_hash:
   do_write("{\""+ident_hash[x]+"\", "+str(x)+"},\n")
do_write("{NULL, 0}\n};\n\n")

