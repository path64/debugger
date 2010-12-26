#!/usr/bin/python

import sys
import os

infile = os.path.abspath(sys.argv[1])
o = open(infile)
lines = o.readlines()
o.close()

outfile = sys.stdout;

# append the x87 opcodes

def column(mnemonic):
    for i in range(0, 8):
        lines.append(mnemonic + " " + "*st(0),*st(" + str(i) + ')')


def column_r(mnemonic):
    for i in range(0, 8):
        lines.append(mnemonic + " " + "*st(" + str(i) + "),*st(0)")


def column_1(mnemonic):
    for i in range(0, 8):
        lines.append(mnemonic + " " + "*st(" + str(i) + ')')


def table_a_10_D8():
    for col in ["fadd", "fmul", "fcom", "fcomp",
                "fsub", "fsubr", "fdiv", "fdivr"]:
        column(col)


def table_a_10_D9():
    for col in ["fld", "fxch"]:
        column(col)

    lines.append("fnop")
    for i in range(0, 7):
        lines.append("xxx")

    column("xxx")

    for op in ["fchs", "fabs", "xxx", "xxx", "ftst", "fxam", "xxx", "xxx"]:
       lines.append(op)

    for op in ["fld1", "fldl2t", "fldl2e", "fldpi", "fldlg2",
               "fldln2", "fldz",  "xxx"]:
       lines.append(op)

    for op in ["f2xm1", "fyl2x", "fptan", "fpatan",
               "fxtract", "fprem1", "fdecstp",  "fincstp"]:
       lines.append(op)

    for op in ["fprem", "fyl2xp1", "fsqrt", "fsincos",
               "frndint", "fscale", "fsin",  "fcos"]:
       lines.append(op)


def table_a_10_DA():
    for col in ["fcmovb", "fcmove", "fcmovbe", "fcmovu", "xxx"]:
        column(col)

    lines.append("xxx")
    lines.append("fucompp")
    for i in range(0, 6):
        lines.append("xxx")

    for col in ["xxx","xxx"]:
        column(col)


def table_a_10_DB():
    for col in ["fcmovnb", "fcmovne", "fcmovnbe", "fcmovnu"]:
        column(col)

    lines.append("xxx")
    lines.append("xxx")
    lines.append("fclex")
    lines.append("finit")
    for i in range(0, 4):
        lines.append("xxx")

    for col in ["fucomi", "fcomi", "xxx"]:
        column(col)


def table_a_10_DC():
    for col in ["fadd", "fmul", "xxx", "xxx",
                "fsubr", "fsub", "fdivr", "fdiv"]:
        column_r(col)


def table_a_10_DD():
    for col in ["ffree", "xxx", "fst", "fstp",
                "fucom", "fucomp", "xxx", "xxx"]:
        if col == "fcom":
            column_r(col)
        else:
            column_1(col)


def table_a_10_DE():
    for col in ["faddp", "fmulp", "xxx"]:
        column_r(col)

    lines.append("xxx")
    lines.append("fcompp")
    for i in range(0, 6):
        lines.append("xxx")

    for col in ["fsubrp", "fsubp", "fdivrp", "fdivp"]:
        column_r(col)


def table_a_10_DF():
    for col in ["xxx", "xxx", "xxx", "xxx"]:
        column(col)

    lines.append("fstsw %ax")
    for i in range(0, 7):
        lines.append("xxx")

    for col in ["fucomip", "fcomip", "xxx"]:
        column(col)
    

lines.append("^x87")
for i in range(0xD8, 0xE0):
    exec("table_a_10_%02X()" % i)

lines.append("^---")


# Handle the regular (i.e. non-x87) opcodes.

def parse_operands(operands):
    f = operands.split(",")
    if len(f) == 0:
        return []

    return f


def parse_optable(name, lines, i):
    print >> outfile
    print >> outfile
    print >> outfile, ("Instruction " + name + "[] = {")
    row = 0
    while i < len(lines):
        line = lines[i].strip()
        i += 1
        if len(line) == 0:
           continue
        if line[0] == '#':
            continue
        if line == "^---":
            break

        fields = line.split(' ')
        op = fields[0]

        if (row % 8) == 0:
            print >> outfile, ("// row " + str(row / 8))

        row += 1

        if op == "xxx":                             # undefined
            print >> outfile, ("{\"(bad)\", NULL, NULL, NULL}, ")
        elif op[0] == '~':                          # group
            print >> outfile, ("{ \"" + op + "\", NULL, NULL, NULL}, ")
        else:
            outstr = "{ \"" + op + "\""
            if len(fields) > 1:
                ops = parse_operands (fields[1])
                for op in ops:
                    outstr = outstr + ", \"" + op + "\""
                if len(ops) != 3:
                    n = 3 - len(ops)
                    while n > 0:
                        outstr = outstr + ", NULL"
                        n -= 1
            else:
                outstr = outstr + ", NULL, NULL, NULL"
            print >> outfile, (outstr + "},")
    print >> outfile, ("{NULL, NULL, NULL}} ;")
    return i


def parse_groups(lines, i):
    print >> outfile
    print >> outfile
    print >> outfile, ("Group groups[] = {")
    groupno = -1
    opcode = 0
    comma = False
    while i < len(lines):
        line = lines[i].strip()
        i += 1
        if len(line) == 0:
            continue
        if line[0] == '#':
            continue
        if line == "^---":
            break

        if line[0] == '~':
            if groupno != -1:                     # close previous?
                print >> outfile, ("}},")
            f = line[1:].split()
            groupno = int(f[0])
            print >> outfile, ("{" + str(groupno) + ", 0x" + f[1] + ", {")
            comma = False
        else:
            if comma:
                outstr = ", "
            else:
                outstr = ""
            fields = line.split()
            op = fields[0]
            if op == "xxx":                      # undefined
                print >> outfile, (outstr + "{\"(bad)\",NULL,NULL,NULL}")
            else:
                outstr = outstr + "{\"" + op + "\""
                if len(fields) > 1:
                    ops = parse_operands(fields[1])
                    for op in ops:
                        outstr = outstr + ", \"" + op + "\""
                    if len(ops) != 3:
                        n = 3 - len(ops)
                        while n > 0:
                            outstr = outstr + ", NULL"
                            n -= 1
                    print >> outfile, (outstr + "}")
                else:
                    print >> outfile, (outstr + ", NULL,NULL,NULL}")
            comma = True 
    print >> outfile, ("}}, { -1, 0, { ")
    comma = False
    for j in range(0, 8):
        outstr = ""
        if comma: outstr = outstr + ","
        print >> outfile, (outstr + "{NULL, NULL, NULL, NULL}")
        comma = True
    print >> outfile, ("}}} ;")
    return i


print >> outfile, ("#include \"opcodes.h\"")
print >> outfile, ("#include <stdio.h>")
print >> outfile

i = 0
while i < len(lines):
    line = lines[i].strip()
    i += 1
    if len(line) == 0:
       continue
    if line[0] == '#':
       continue
    if line[0] == '^':                          # section
        sectionname = line[1:]
        if(sectionname in ["one_byte", "two_byte", "two_byte_F3",
                           "two_byte_66", "two_byte_F2", "x87", "x87_alt"]):
            i = parse_optable(sectionname, lines, i)
        elif sectionname == "groups":
            i = parse_groups(lines, i)
        else:
            raise "unknown section " + sectionname

outfile.close() 
