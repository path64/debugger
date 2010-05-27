# Put together a toolset
######################################################
LEX = flex -p 
YACC = bison -d
RM = rm -rf

CC = gcc -std=c99
CFLAGS = -O0 -m32 -g3
CCOMPILE = $(CC) $(CFLAGS)
CLINK = $(CC) -static $(CFLAGS)

CXX = pathCC
CXXFLAGS = -O0 -m32 -g
CXXCOMPILE = $(CXX) $(CXXFLAGS)
CXXLINK = $(CXX) -static $(CXXFLAGS)

# Setup standard handles
######################################################

all : c99_expr f90_expr

clean : c99-clean f90-clean
	$(RM) *.o


# Build common parser object files
######################################################
COMOBJS := expr_node.o expr_arena.o expr_low.o

$(COMOBJS) : %.o : %.c
	$(CCOMPILE) -o $@ -c $<


# Build C99 parser object files
######################################################
C99OBJS := yyc99.tab.o lex.yyc99.o expr_c99.o \
           expr_c99_lower.o 

yyc99.tab.c : iso_c99.y expr_node.h
	$(YACC) -b yyc99 -p yyc99 iso_c99.y

lex.yyc99.c : iso_c99.y iso_c99.l expr_node.h
	$(LEX) -Pyyc99 iso_c99.l

$(C99OBJS) : %.o : %.c expr_node.h
	$(CCOMPILE) -o $@ -c $<

c99_expr : $(C99OBJS) $(COMOBJS)
	$(CLINK) -o c99_expr $(C99OBJS) $(COMOBJS)

c99-clean : FORCE
	$(RM) lex.yyc99.*
	$(RM) yyc99.tab.*
	$(RM) c99_expr


# Build F90 parser object files
######################################################
F90OBJS := yyf90.tab.o lex.yyf90.o expr_f90.o

yyf90.tab.c : iso_f90.y expr_node.h
	$(YACC) -b yyf90 -p yyf90 iso_f90.y

lex.yyf90.c : iso_f90.y iso_f90.l expr_node.h
	$(LEX) -Pyyf90 iso_f90.l

$(F90OBJS) : %.o : %.c expr_node.h
	$(CCOMPILE) -o $@ -c $<

f90_expr : $(F90OBJS) $(COMOBJS)
	$(CLINK) -o f90_expr $(F90OBJS) $(COMOBJS)

f90-clean : FORCE
	$(RM) lex.yyf90.*
	$(RM) yyf90.tab.*
	$(RM) f90_expr


FORCE :

