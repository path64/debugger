#include "opcodes.h"
#include <stdio.h>



Instruction one_byte[] = {
// row 0
{ "add", "Eb", "Gb", NULL},
{ "add", "Ev", "Gv", NULL},
{ "add", "Gb", "Ev", NULL},
{ "add", "Gv", "Ev", NULL},
{ "add", "%al", "Ib", NULL},
{ "add", "%Xax", "Iv", NULL},
{ "push", "%es", NULL, NULL},
{ "pop", "%es", NULL, NULL},
// row 1
{ "adc", "Eb", "Gb", NULL},
{ "adc", "Ev", "Gv", NULL},
{ "adc", "Gb", "Ev", NULL},
{ "adc", "Gv", "Ev", NULL},
{ "adc", "%al", "Ib", NULL},
{ "adc", "%Xax", "Iv", NULL},
{ "push", "%ss", NULL, NULL},
{ "pop", "%ss", NULL, NULL},
// row 2
{ "and", "Eb", "Gb", NULL},
{ "and", "Ev", "Gv", NULL},
{ "and", "Gb", "Ev", NULL},
{ "and", "Gv", "Ev", NULL},
{ "and", "%al", "Ib", NULL},
{ "and", "%Xax", "Iv", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "daa", NULL, NULL, NULL},
// row 3
{ "xor", "Eb", "Gb", NULL},
{ "xor", "Ev", "Gv", NULL},
{ "xor", "Gb", "Ev", NULL},
{ "xor", "Gv", "Ev", NULL},
{ "xor", "%al", "Ib", NULL},
{ "xor", "%Xax", "Iv", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "aaa", NULL, NULL, NULL},
// row 4
{ "inc", "%Xax", NULL, NULL},
{ "inc", "%Xcx", NULL, NULL},
{ "inc", "%Xdx", NULL, NULL},
{ "inc", "%Xbx", NULL, NULL},
{ "inc", "%Xsp", NULL, NULL},
{ "inc", "%Xbp", NULL, NULL},
{ "inc", "%Xsi", NULL, NULL},
{ "inc", "%Xdi", NULL, NULL},
// row 5
{ "push", "%rax/%r8", NULL, NULL},
{ "push", "%rcx/%r9", NULL, NULL},
{ "push", "%rdx/%r10", NULL, NULL},
{ "push", "%rbx/%r11", NULL, NULL},
{ "push", "%rsp/%r12", NULL, NULL},
{ "push", "%rbp/%r13", NULL, NULL},
{ "push", "%rsi/%r14", NULL, NULL},
{ "push", "%rdi/%r15", NULL, NULL},
// row 6
{ "pusha", NULL, NULL, NULL},
{ "popa", NULL, NULL, NULL},
{ "bound", "Gv", "Ma", NULL},
{ "movsx", "Gv", "Ed", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 7
{ "jo", "Jb", NULL, NULL},
{ "jno", "Jb", NULL, NULL},
{ "jb", "Jb", NULL, NULL},
{ "jnb", "Jb", NULL, NULL},
{ "jz", "Jb", NULL, NULL},
{ "jnz", "Jb", NULL, NULL},
{ "jbe", "Jb", NULL, NULL},
{ "jnbe", "Jb", NULL, NULL},
// row 8
{ "~1", NULL, NULL, NULL}, 
{ "~1", NULL, NULL, NULL}, 
{ "~1", NULL, NULL, NULL}, 
{ "~1", NULL, NULL, NULL}, 
{ "test", "Eb", "Gb", NULL},
{ "test", "Ev", "Gv", NULL},
{ "xchg", "Eb", "Gb", NULL},
{ "xchg", "Ev", "Gv", NULL},
// row 9
{ "nop", NULL, NULL, NULL},
{ "xchg", "%Xcx", NULL, NULL},
{ "xchg", "%Xdx", NULL, NULL},
{ "xchg", "%Xbx", NULL, NULL},
{ "xchg", "%Xsp", NULL, NULL},
{ "xchg", "%Xbp", NULL, NULL},
{ "xchg", "%Xsi", NULL, NULL},
{ "xchg", "%Xdi", NULL, NULL},
// row 10
{ "mov", "%al", "Ob", NULL},
{ "mov", "%Xax", "Ov", NULL},
{ "mov", "Ob", "%al", NULL},
{ "mov", "Ov", "%Xax", NULL},
{ "movs", "Yb", "Xb", NULL},
{ "movs", "Yv", "Xv", NULL},
{ "cmps", "Yb", "Xb", NULL},
{ "cmps", "Xv", "Yv", NULL},
// row 11
{ "mov", "%al/%r8b", "Ib", NULL},
{ "mov", "%cl/%r9b", "Ib", NULL},
{ "mov", "%dl/%r10b", "Ib", NULL},
{ "mov", "%bl/%r11b", "Ib", NULL},
{ "mov", "%ah/%r12b", "Ib", NULL},
{ "mov", "%ch/%r13b", "Ib", NULL},
{ "mov", "%dh/%r14b", "Ib", NULL},
{ "mov", "%bh/%r15b", "Ib", NULL},
// row 12
{ "~2", NULL, NULL, NULL}, 
{ "~2", NULL, NULL, NULL}, 
{ "ret", "Iw", NULL, NULL},
{ "ret", NULL, NULL, NULL},
{ "les", "Gv", "Mp", NULL},
{ "lds", "Gv", "Mp", NULL},
{ "~11", NULL, NULL, NULL}, 
{ "~11", NULL, NULL, NULL}, 
// row 13
{ "~2", NULL, NULL, NULL}, 
{ "~2", NULL, NULL, NULL}, 
{ "~2", NULL, NULL, NULL}, 
{ "~2", NULL, NULL, NULL}, 
{ "aam", "Ib", NULL, NULL},
{ "aad", "Ib", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "xlat", NULL, NULL, NULL},
// row 14
{ "loopne", "Jb", NULL, NULL},
{ "loope", "Jb", NULL, NULL},
{ "loop", "Jb", NULL, NULL},
{ "jcxz", "Jb", NULL, NULL},
{ "in", "%al", "Ib", NULL},
{ "in", "%Xax", "Ib", NULL},
{ "out", "Ib", "%al", NULL},
{ "out", "Ib", "%Xax", NULL},
// row 15
{ "lock", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "repne", NULL, NULL, NULL},
{ "rep", NULL, NULL, NULL},
{ "hlt", NULL, NULL, NULL},
{ "cmc", NULL, NULL, NULL},
{ "~3", NULL, NULL, NULL}, 
{ "~3", NULL, NULL, NULL}, 
// row 16
{ "or", "Eb", "Gb", NULL},
{ "or", "Ev", "Gv", NULL},
{ "or", "Gb", "Eb", NULL},
{ "or", "Gv", "Ev", NULL},
{ "or", "%al", "Ib", NULL},
{ "or", "%Xax", "Iz", NULL},
{ "push", "%cs", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 17
{ "sbb", "Eb", "Gb", NULL},
{ "sbb", "Ev", "Gv", NULL},
{ "sbb", "Gb", "Eb", NULL},
{ "sbb", "Gv", "Ev", NULL},
{ "sbb", "%al", "Ib", NULL},
{ "sbb", "%Xax", "Iz", NULL},
{ "push", "%ds", NULL, NULL},
{ "pop", "%ds", NULL, NULL},
// row 18
{ "sub", "Eb", "Gb", NULL},
{ "sub", "Ev", "Gv", NULL},
{ "sub", "Gb", "Eb", NULL},
{ "sub", "Gv", "Ev", NULL},
{ "sub", "%al", "Ib", NULL},
{ "sub", "%Xax", "Iz", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "das", NULL, NULL, NULL},
// row 19
{ "cmp", "Eb", "Gb", NULL},
{ "cmp", "Ev", "Gv", NULL},
{ "cmp", "Gb", "Eb", NULL},
{ "cmp", "Gv", "Ev", NULL},
{ "cmp", "%al", "Ib", NULL},
{ "cmp", "%Xax", "Iz", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "aas", NULL, NULL, NULL},
// row 20
{ "dec", "%Xax", NULL, NULL},
{ "dec", "%Xcx", NULL, NULL},
{ "dec", "%Xdx", NULL, NULL},
{ "dec", "%Xbx", NULL, NULL},
{ "dec", "%Xsp", NULL, NULL},
{ "dec", "%Xbp", NULL, NULL},
{ "dec", "%Xsi", NULL, NULL},
{ "dec", "%Xdi", NULL, NULL},
// row 21
{ "pop", "%rax/%r8", NULL, NULL},
{ "pop", "%rcx/%r9", NULL, NULL},
{ "pop", "%rdx/%r10", NULL, NULL},
{ "pop", "%rbx/%r11", NULL, NULL},
{ "pop", "%rsp/%r12", NULL, NULL},
{ "pop", "%rbp/%r13", NULL, NULL},
{ "pop", "%rsi/%r14", NULL, NULL},
{ "pop", "%rdi/%r15", NULL, NULL},
// row 22
{ "push", "Iz", NULL, NULL},
{ "imul", "Gv", "Ev", "Iz"},
{ "push", "Ib", NULL, NULL},
{ "imul", "Gv", "Ev", "Ib"},
{ "ins", "Yb", "%dx", NULL},
{ "ins", "Yz", "%dx", NULL},
{ "outs", "%dx", "Xb", NULL},
{ "outs", "%dx", "Xz", NULL},
// row 23
{ "js", "Jb", NULL, NULL},
{ "jss", "Jb", NULL, NULL},
{ "jp", "Jb", NULL, NULL},
{ "jnp", "Jb", NULL, NULL},
{ "jl", "Jb", NULL, NULL},
{ "jnl", "Jb", NULL, NULL},
{ "jle", "Jb", NULL, NULL},
{ "jnle", "Jb", NULL, NULL},
// row 24
{ "mov", "Eb", "Gb", NULL},
{ "mov", "Ev", "Gv", NULL},
{ "mov", "Gb", "Eb", NULL},
{ "mov", "Gv", "Ev", NULL},
{ "mov", "Ew", "Sw", NULL},
{ "lea", "Gv", "M", NULL},
{ "mov", "Sw", "Ew", NULL},
{ "pop", "Ev", NULL, NULL},
// row 25
{ "cbw", NULL, NULL, NULL},
{ "cwd", NULL, NULL, NULL},
{ "call", "Ap", NULL, NULL},
{ "fwait", NULL, NULL, NULL},
{ "pushf", "Fv", NULL, NULL},
{ "popf", "Fv", NULL, NULL},
{ "sahf", NULL, NULL, NULL},
{ "lahf", NULL, NULL, NULL},
// row 26
{ "test", "%al", "Ib", NULL},
{ "test", "%Xax", "Iz", NULL},
{ "stos", "Yb", "%al", NULL},
{ "stos", "Yv", "%Xax", NULL},
{ "lods", "%al", "Xb", NULL},
{ "lods", "%Xax", "Xv", NULL},
{ "scas", "%al", "Yb", NULL},
{ "scas", "%Xax", "Yv", NULL},
// row 27
{ "mov", "%Xax/%r8", "Iv", NULL},
{ "mov", "%Xcx/%r9", "Iv", NULL},
{ "mov", "%Xdx/%r10", "Iv", NULL},
{ "mov", "%Xbx/%r11", "Iv", NULL},
{ "mov", "%Xsp/%r12", "Iv", NULL},
{ "mov", "%Xbp/%r13", "Iv", NULL},
{ "mov", "%Xsi/%r14", "Iv", NULL},
{ "mov", "%Xdi/%r15", "Iv", NULL},
// row 28
{ "enter", "Iw", "Ib", NULL},
{ "leave", NULL, NULL, NULL},
{ "retf", "Iw", NULL, NULL},
{ "retf", NULL, NULL, NULL},
{ "int3", NULL, NULL, NULL},
{ "int", "Ib", NULL, NULL},
{ "into", NULL, NULL, NULL},
{ "iret", NULL, NULL, NULL},
// row 29
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 30
{ "call", "Jz", NULL, NULL},
{ "jmp", "Jz", NULL, NULL},
{ "jmp", "Ap", NULL, NULL},
{ "jmp", "Jb", NULL, NULL},
{ "in", "%al", "%dx", NULL},
{ "in", "%Xax", "%dx", NULL},
{ "out", "%dx", "%al", NULL},
{ "out", "%dx", "%Xax", NULL},
// row 31
{ "clc", NULL, NULL, NULL},
{ "stc", NULL, NULL, NULL},
{ "cli", NULL, NULL, NULL},
{ "sti", NULL, NULL, NULL},
{ "cld", NULL, NULL, NULL},
{ "std", NULL, NULL, NULL},
{ "~4", NULL, NULL, NULL}, 
{ "~5", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;


Group groups[] = {
{1, 0x80, {
{"add", "Eb", "Ib", NULL}
, {"or", "Eb", "Ib", NULL}
, {"adc", "Eb", "Ib", NULL}
, {"sbb", "Eb", "Ib", NULL}
, {"and", "Eb", "Ib", NULL}
, {"sub", "Eb", "Ib", NULL}
, {"xor", "Eb", "Ib", NULL}
, {"cmp", "Eb", "Ib", NULL}
}},
{1, 0x81, {
{"add", "Ev", "Iz", NULL}
, {"or", "Ev", "Iz", NULL}
, {"adc", "Ev", "Iz", NULL}
, {"sbb", "Ev", "Iz", NULL}
, {"and", "Ev", "Iz", NULL}
, {"sub", "Ev", "Iz", NULL}
, {"xor", "Ev", "Iz", NULL}
, {"cmp", "Ev", "Iz", NULL}
}},
{1, 0x82, {
{"add", "Eb", "Ib", NULL}
, {"or", "Eb", "Ib", NULL}
, {"adc", "Eb", "Ib", NULL}
, {"sbb", "Eb", "Ib", NULL}
, {"and", "Eb", "Ib", NULL}
, {"sub", "Eb", "Ib", NULL}
, {"xor", "Eb", "Ib", NULL}
, {"cmp", "Eb", "Ib", NULL}
}},
{1, 0x83, {
{"add", "Ev", "Ib", NULL}
, {"or", "Ev", "Ib", NULL}
, {"adc", "Ev", "Ib", NULL}
, {"sbb", "Ev", "Ib", NULL}
, {"and", "Ev", "Ib", NULL}
, {"sub", "Ev", "Ib", NULL}
, {"xor", "Ev", "Ib", NULL}
, {"cmp", "Ev", "Ib", NULL}
}},
{2, 0xc0, {
{"rol", "Eb", "Ib", NULL}
, {"ror", "Eb", "Ib", NULL}
, {"rcl", "Eb", "Ib", NULL}
, {"rcr", "Eb", "Ib", NULL}
, {"shl", "Eb", "Ib", NULL}
, {"shr", "Eb", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Eb", "Ib", NULL}
}},
{2, 0xc1, {
{"rol", "Ev", "Ib", NULL}
, {"ror", "Ev", "Ib", NULL}
, {"rcl", "Ev", "Ib", NULL}
, {"rcr", "Ev", "Ib", NULL}
, {"shl", "Ev", "Ib", NULL}
, {"shr", "Ev", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Ev", "Ib", NULL}
}},
{2, 0xd0, {
{"rol", "Eb", "1", NULL}
, {"ror", "Eb", "1", NULL}
, {"rcl", "Eb", "1", NULL}
, {"rcr", "Eb", "1", NULL}
, {"shl", "Eb", "1", NULL}
, {"shr", "Eb", "1", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Eb", "1", NULL}
}},
{2, 0xd1, {
{"rol", "Ev", "1", NULL}
, {"ror", "Ev", "1", NULL}
, {"rcl", "Ev", "1", NULL}
, {"rcr", "Ev", "1", NULL}
, {"shl", "Ev", "1", NULL}
, {"shr", "Ev", "1", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Ev", "1", NULL}
}},
{2, 0xd2, {
{"rol", "Eb", "%cl", NULL}
, {"ror", "Eb", "%cl", NULL}
, {"rcl", "Eb", "%cl", NULL}
, {"rcr", "Eb", "%cl", NULL}
, {"shl", "Eb", "%cl", NULL}
, {"shr", "Eb", "%cl", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Eb", "%cl", NULL}
}},
{2, 0xd3, {
{"rol", "Ev", "%cl", NULL}
, {"ror", "Ev", "%cl", NULL}
, {"rcl", "Ev", "%cl", NULL}
, {"rcr", "Ev", "%cl", NULL}
, {"shl", "Ev", "%cl", NULL}
, {"shr", "Ev", "%cl", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"sar", "Ev", "%cl", NULL}
}},
{3, 0xf6, {
{"test", "Eb", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"not", "Eb", NULL, NULL}
, {"neg", "Eb", NULL, NULL}
, {"mul", "Eb", NULL, NULL}
, {"imul", "Eb", NULL, NULL}
, {"div", "Eb", NULL, NULL}
, {"idiv", "Eb", NULL, NULL}
}},
{3, 0xf7, {
{"test", "Ev", "Iz", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"not", "Ev", NULL, NULL}
, {"neg", "Ev", NULL, NULL}
, {"mul", "Ev", NULL, NULL}
, {"imul", "Ev", NULL, NULL}
, {"div", "Ev", NULL, NULL}
, {"idiv", "Ev", NULL, NULL}
}},
{4, 0xfe, {
{"inc", "Eb", NULL, NULL}
, {"dec", "Eb", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{5, 0xff, {
{"inc", "Ev", NULL, NULL}
, {"dec", "Ev", NULL, NULL}
, {"call", "Ev", NULL, NULL}
, {"call", "Ep", NULL, NULL}
, {"jmp", "Ev", NULL, NULL}
, {"jmp", "Ep", NULL, NULL}
, {"push", "Ev", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{6, 0x0f00, {
{"sldt", "Mw", "Rv", NULL}
, {"str", "Mw", "Rv", NULL}
, {"lldt", "Ew", NULL, NULL}
, {"ltr", "Ew", NULL, NULL}
, {"verr", "Ew", NULL, NULL}
, {"verw", "Ew", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{7, 0x0f01, {
{"sgdt", "Ms", NULL, NULL}
, {"sidt", "Ms", NULL, NULL}
, {"lgdt", "Ms", NULL, NULL}
, {"lidt", "Ms", NULL, NULL}
, {"smsw", "Mw", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
, {"lmsw", "Ew", NULL, NULL}
, {"invlpg", "Mb", NULL, NULL}
}},
{8, 0x0fba, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"bt", "Ev", "Ib", NULL}
, {"bts", "Ev", "Ib", NULL}
, {"btr", "Ev", "Ib", NULL}
, {"btc", "Ev", "Ib", NULL}
}},
{9, 0x0fc7, {
{"(bad)",NULL,NULL,NULL}
, {"cmpxchg8b", "Mq", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{10, 0x0fb9, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{11, 0xc6, {
{"mov", "Eb", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{11, 0xc7, {
{"mov", "Ev", "Iz", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{12, 0x0f71, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrlw", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psraw", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psllw", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{12, 0x660f71, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrlw", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psraw", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psllw", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{13, 0x0f72, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrld", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrad", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"pslld", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{13, 0x660f72, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrld", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrad", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"pslld", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{14, 0x0f73, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrlq", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psllq", "PRq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
}},
{14, 0x660f73, {
{"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psrlq", "VRdq", "Ib", NULL}
, {"psrldq", "VRdq", "Ib", NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"psllq", "VRdq", "Ib", NULL}
, {"pslldq", "VRdq", "Ib", NULL}
}},
{15, 0x0fae, {
{"fxsave", "M", NULL, NULL}
, {"fsrstor", "M", NULL, NULL}
, {"ldmxcsr", "Md", NULL, NULL}
, {"stmxcsr", "Md", NULL, NULL}
, {"(bad)",NULL,NULL,NULL}
, {"lfence", NULL,NULL,NULL}
, {"mfence", NULL,NULL,NULL}
, {"sfence", NULL,NULL,NULL}
}},
{16, 0x0f18, {
{"prefetch", "%nta", NULL, NULL}
, {"prefetch", "%t0", NULL, NULL}
, {"prefetch", "%t1", NULL, NULL}
, {"prefetch", "%t2", NULL, NULL}
, {"nop", NULL,NULL,NULL}
, {"nop", NULL,NULL,NULL}
, {"nop", NULL,NULL,NULL}
, {"nop", NULL,NULL,NULL}
}},
{17, 0x0f0d, {
{"prefetch", NULL,NULL,NULL}
, {"prefetch", NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"prefetch", NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
, {"(bad)",NULL,NULL,NULL}
}}, { -1, 0, { 
{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
,{NULL, NULL, NULL, NULL}
}}} ;


Instruction two_byte[] = {
// row 0
{ "~6", NULL, NULL, NULL}, 
{ "~7", NULL, NULL, NULL}, 
{ "lar", "Gv", "Ew", NULL},
{ "lsl", "Gv", "Ew", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "syscall", NULL, NULL, NULL},
{ "clts", NULL, NULL, NULL},
{ "sysret", NULL, NULL, NULL},
// row 1
{ "movups", "Vps", "Wps", NULL},
{ "movups", "Wps", "Vps", NULL},
{ "movlps", "Vps", "Mq", NULL},
{ "movlps", "Mq", "Vps", NULL},
{ "unpcklps", "Vps", "Wq", NULL},
{ "unpckhps", "Vps", "Wq", NULL},
{ "movhps", "Vps", "Mq", NULL},
{ "movhps", "Mq", "Vps", NULL},
// row 2
{ "mov", "Rd", "Cd", NULL},
{ "mov", "Rd", "Dd", NULL},
{ "mov", "Cd", "Rd", NULL},
{ "mov", "Dd", "Rd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 3
{ "wrmsr", NULL, NULL, NULL},
{ "rdtsc", NULL, NULL, NULL},
{ "rdmsr", NULL, NULL, NULL},
{ "rdpmc", NULL, NULL, NULL},
{ "sysenter", NULL, NULL, NULL},
{ "sysexit", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 4
{ "cmovo", "Gv", "Ev", NULL},
{ "cmovno", "Gv", "Ev", NULL},
{ "cmovb", "Gv", "Ev", NULL},
{ "cmovnb", "Gv", "Ev", NULL},
{ "cmovz", "Gv", "Ev", NULL},
{ "cmovnz", "Gv", "Ev", NULL},
{ "cmovbe", "Gv", "Ev", NULL},
{ "cmovnbe", "Gv", "Ev", NULL},
// row 5
{ "movmskps", "Gd", "VRps", NULL},
{ "sqrtps", "Vps", "Wps", NULL},
{ "rsqrtps", "Vps", "Wps", NULL},
{ "rcpps", "Vps", "Wps", NULL},
{ "andps", "Vps", "Wps", NULL},
{ "andnps", "Vps", "Wps", NULL},
{ "orps", "Vps", "Wps", NULL},
{ "xorps", "Vps", "Wps", NULL},
// row 6
{ "punpcklbw", "Pq", "Qd", NULL},
{ "punpcklwd", "Pq", "Qd", NULL},
{ "punpckldq", "Pq", "Qd", NULL},
{ "packsswb", "Pq", "Qd", NULL},
{ "pcmpgtb", "Pq", "Qd", NULL},
{ "pcmpgtw", "Pq", "Qd", NULL},
{ "pcmpgtd", "Pq", "Qd", NULL},
{ "packuswb", "Pq", "Qd", NULL},
// row 7
{ "pshufw", "Pq", "Qd", "Ib"},
{ "~12", NULL, NULL, NULL}, 
{ "~13", NULL, NULL, NULL}, 
{ "~14", NULL, NULL, NULL}, 
{ "pcmpeqb", "Pq", "Qd", NULL},
{ "pcmpeqw", "Pq", "Qd", NULL},
{ "pcmpeqd", "Pq", "Qd", NULL},
{ "emms", NULL, NULL, NULL},
// row 8
{ "jo", "Jz", NULL, NULL},
{ "jno", "Jz", NULL, NULL},
{ "jb", "Jz", NULL, NULL},
{ "jnb", "Jz", NULL, NULL},
{ "jz", "Jz", NULL, NULL},
{ "jnz", "Jz", NULL, NULL},
{ "jbe", "Jz", NULL, NULL},
{ "jnbe", "Jz", NULL, NULL},
// row 9
{ "seto", "Eb", NULL, NULL},
{ "setno", "Eb", NULL, NULL},
{ "setb", "Eb", NULL, NULL},
{ "setnb", "Eb", NULL, NULL},
{ "setz", "Eb", NULL, NULL},
{ "setnz", "En", NULL, NULL},
{ "setbe", "Ez", NULL, NULL},
{ "setnbe", "Ez", NULL, NULL},
// row 10
{ "push", "%fs", NULL, NULL},
{ "pop", "%fs", NULL, NULL},
{ "cpuid", NULL, NULL, NULL},
{ "bt", "Ev", "Gv", NULL},
{ "shld", "Ev", "Gv", "Ib"},
{ "shld", "Ev", "Gv", "%cl"},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 11
{ "cmpxchg", "Eb", "Gb", NULL},
{ "cmpxchg", "Ev", "Gv", NULL},
{ "lss", "Gz", "Mp", NULL},
{ "btr", "Ev", "Gv", NULL},
{ "lfs", "Gz", "Mp", NULL},
{ "lgs", "Gz", "Mp", NULL},
{ "movzx", "Gv", "Eb", NULL},
{ "movzx", "Gv", "Ew", NULL},
// row 12
{ "xadd", "Eb", "Gb", NULL},
{ "xadd", "Ev", "Gv", NULL},
{ "cmpps", "Vps", "Wps", "Ib"},
{ "movnti", "Md", "Gd", NULL},
{ "pinsrw", "Pq", "Ew", "Ib"},
{ "pextrw", "Gd", "PRq", "Ib"},
{ "shufps", "Vps", "Wps", "Ib"},
{ "~9", NULL, NULL, NULL}, 
// row 13
{"(bad)", NULL, NULL, NULL}, 
{ "psrlw", "Pq", "Qd", NULL},
{ "psrld", "Pq", "Qd", NULL},
{ "psrlq", "Pq", "Qd", NULL},
{ "paddq", "Pq", "Qd", NULL},
{ "pmullw", "Pq", "Qd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "pmovmskb", "Pq", "PRd", NULL},
// row 14
{ "pavgb", "Pq", "Qd", NULL},
{ "psraw", "Pq", "Qd", NULL},
{ "psrad", "Pq", "Qd", NULL},
{ "pavgw", "Pq", "Qd", NULL},
{ "pmulhuw", "Pq", "Qd", NULL},
{ "pmulhw", "Pq", "Qd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "movntq", "Mq", "Pq", NULL},
// row 15
{"(bad)", NULL, NULL, NULL}, 
{ "psllw", "Pq", "Qd", NULL},
{ "pslld", "Pq", "Qd", NULL},
{ "psllq", "Pq", "Qd", NULL},
{ "pmuludq", "Pq", "Qd", NULL},
{ "pmaddwd", "Pq", "Qd", NULL},
{ "psadbw", "Pq", "Qd", NULL},
{ "maskmovq", "Pq", "PRd", NULL},
// row 16
{ "invd", NULL, NULL, NULL},
{ "wbinvd", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "ud2", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "~17", NULL, NULL, NULL}, 
{ "femms", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 17
{ "~16", NULL, NULL, NULL}, 
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
{ "nop", NULL, NULL, NULL},
// row 18
{ "movaps", "Vps", "Wps", NULL},
{ "movaps", "Wps", "Vps", NULL},
{ "cvtpi2ps", "Vps", "Qd", NULL},
{ "movntps", "Mdq", "Vps", NULL},
{ "cvttps2pi", "Pq", "Wps", NULL},
{ "cvtps2pi", "Pq", "Wps", NULL},
{ "ucomiss", "Vss", "Wss", NULL},
{ "comiss", "Vps", "Wps", NULL},
// row 19
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 20
{ "cmovs", "Gv", "Ev", NULL},
{ "cmovns", "Gv", "Ev", NULL},
{ "cmovp", "Gv", "Ev", NULL},
{ "cmovnp", "Gv", "Ev", NULL},
{ "cmovl", "Gv", "Ev", NULL},
{ "cmovnl", "Gv", "Ev", NULL},
{ "cmovle", "Gv", "Ev", NULL},
{ "cmovnle", "Gv", "Ev", NULL},
// row 21
{ "addps", "Vps", "Wps", NULL},
{ "mulps", "Vps", "Wps", NULL},
{ "cvtps2pd", "Vps", "Wps", NULL},
{ "cvtdq2ps", "Vps", "Wps", NULL},
{ "subps", "Vps", "Wps", NULL},
{ "minps", "Vps", "Wps", NULL},
{ "divps", "Vps", "Wps", NULL},
{ "maxps", "Vps", "Wps", NULL},
// row 22
{ "punpckhbw", "Pq", "Qd", NULL},
{ "punpckhwd", "Pq", "Qd", NULL},
{ "punpckhdq", "Pq", "Qd", NULL},
{ "packssdw", "Pq", "Qd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movd", "Pq", "Ed", NULL},
{ "movq", "Pq", "Qd", NULL},
// row 23
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movd", "Ed", "Pd", NULL},
{ "movq", "Qd", "Pq", NULL},
// row 24
{ "js", "Jz", NULL, NULL},
{ "jns", "Jz", NULL, NULL},
{ "jp", "Jz", NULL, NULL},
{ "jnp", "Jz", NULL, NULL},
{ "jl", "Jz", NULL, NULL},
{ "jnl", "Jz", NULL, NULL},
{ "jle", "Jz", NULL, NULL},
{ "jnle", "Jz", NULL, NULL},
// row 25
{ "sets", "Eb", NULL, NULL},
{ "setns", "Eb", NULL, NULL},
{ "setp", "Eb", NULL, NULL},
{ "setnp", "Eb", NULL, NULL},
{ "setl", "Eb", NULL, NULL},
{ "setnl", "Eb", NULL, NULL},
{ "setle", "Eb", NULL, NULL},
{ "setnle", "Eb", NULL, NULL},
// row 26
{ "push", "%gs", NULL, NULL},
{ "pop", "%gs", NULL, NULL},
{ "rsm", NULL, NULL, NULL},
{ "bts", "Ev", "Gv", NULL},
{ "shrd", "Ev", "Gv", "Ib"},
{ "shrd", "Ev", "Gv", "%cl"},
{ "~15", NULL, NULL, NULL}, 
{ "imul", "Gv", "Ev", NULL},
// row 27
{"(bad)", NULL, NULL, NULL}, 
{ "~10", NULL, NULL, NULL}, 
{ "~8", NULL, NULL, NULL}, 
{ "btc", "Ev", "Gv", NULL},
{ "bsf", "Gv", "Ev", NULL},
{ "bsr", "Gv", "Ev", NULL},
{ "movsx", "Gv", "Eb", NULL},
{ "movsx", "Gv", "Ew", NULL},
// row 28
{ "bswap", "%rax", "%r8", NULL},
{ "bswap", "%rcx", "%r9", NULL},
{ "bswap", "%rdx", "%r10", NULL},
{ "bswap", "%rbx", "%r11", NULL},
{ "bswap", "%rsp", "%r12", NULL},
{ "bswap", "%rbp", "%r13", NULL},
{ "bswap", "%rsi", "%r14", NULL},
{ "bswap", "%rdi", "%r15", NULL},
// row 29
{ "psubusb", "Pq", "Qd", NULL},
{ "psubusw", "Pq", "Qd", NULL},
{ "pminub", "Pq", "Qd", NULL},
{ "pand", "Pq", "Qd", NULL},
{ "paddusb", "Pq", "Qd", NULL},
{ "paddusw", "Pq", "Qd", NULL},
{ "pmaxub", "Pq", "Qd", NULL},
{ "pandn", "Pq", "Qd", NULL},
// row 30
{ "psubsb", "Pq", "Qd", NULL},
{ "psubsw", "Pq", "Qd", NULL},
{ "pminsw", "Pq", "Qd", NULL},
{ "por", "Pq", "Qd", NULL},
{ "paddsb", "Pq", "Qd", NULL},
{ "paddsw", "Pq", "Qd", NULL},
{ "pmaxsw", "Pq", "Qd", NULL},
{ "pxor", "Pq", "Qd", NULL},
// row 31
{ "psubb", "Pq", "Qd", NULL},
{ "psubw", "Pq", "Qd", NULL},
{ "psubd", "Pq", "Qd", NULL},
{ "psubq", "Pq", "Qd", NULL},
{ "paddb", "Pq", "Qd", NULL},
{ "paddw", "Pq", "Qd", NULL},
{ "paddd", "Pq", "Qd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;


Instruction two_byte_F3[] = {
// row 0
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 1
{ "movss", "Vss", "Wss", NULL},
{ "movss", "Wpd", "Vpd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 2
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 3
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 4
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 5
{"(bad)", NULL, NULL, NULL}, 
{ "sqrtss", "Vss", "Wss", NULL},
{ "rsqrtss", "Vss", "Wss", NULL},
{ "rcpss", "Vss", "Wss", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 6
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 7
{ "pshufhw", "Vq", "Wq", "Ib"},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 8
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 9
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 10
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 11
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 12
{ "xadd", "Eb", "Gb", NULL},
{ "xadd", "Ev", "Gv", NULL},
{ "cmpss", "Vss", "Wss", "Ib"},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "~9", NULL, NULL, NULL}, 
// row 13
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movq2dq", "Vdq", "PRq", NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 14
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "cvtdq2pd", "Vpd", "Wq", NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 15
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 16
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 17
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 18
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "cvtsi2ss", "Vss", "Ed", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "cvttss2si", "Gd", "Wss", NULL},
{ "cvtss2si", "Gd", "Wss", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 19
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 20
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 21
{ "addss", "Vss", "Wss", NULL},
{ "mulss", "Vss", "Wss", NULL},
{ "cvtss2sd", "Vsd", "Wsd", NULL},
{ "cvttps2dq", "Vdq", "Wps", NULL},
{ "subss", "Vss", "Wss", NULL},
{ "minss", "Vss", "Wss", NULL},
{ "divss", "Vss", "Wss", NULL},
{ "maxss", "Vss", "Wss", NULL},
// row 22
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movdqu", "Vdq", "Wdq", NULL},
// row 23
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movq", "Vq", "Wq", NULL},
{ "movdqu", "Wdq", "Vdq", NULL},
// row 24
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 25
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 26
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 27
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 28
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 29
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 30
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 31
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;


Instruction two_byte_66[] = {
// row 0
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 1
{ "movupd", "Vpd", "Wpd", NULL},
{ "movupd", "Wpd", "Vpd", NULL},
{ "movlpd", "Vsd", "Mq", NULL},
{ "movlpd", "Mq", "Vsd", NULL},
{ "unpcklpd", "Vpd", "Wq", NULL},
{ "unpckhpd", "Vpd", "Wq", NULL},
{ "movhpd", "Vsd", "Mq", NULL},
{ "movhpd", "Mq", "Vsd", NULL},
// row 2
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 3
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 4
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 5
{ "movmskpd", "Gd", "VRpd", NULL},
{ "sqrtpd", "Vpd", "Wpd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "andpd", "Vpd", "Wpd", NULL},
{ "andnpd", "Vpd", "Wpd", NULL},
{ "orpd", "Vpd", "Wpd", NULL},
{ "xorpd", "Vpd", "Wpd", NULL},
// row 6
{ "punpcklbw", "Vdq", "Wq", NULL},
{ "punpcklwd", "Vdq", "Wq", NULL},
{ "punpckldq", "Vdq", "Wq", NULL},
{ "packsswb", "Vdq", "Wq", NULL},
{ "pcmpgtb", "Vdq", "Wq", NULL},
{ "pcmpgtw", "Vdq", "Wq", NULL},
{ "pcmpgtd", "Vdq", "Wq", NULL},
{ "packuswb", "Vdq", "Wq", NULL},
// row 7
{ "pshufd", "Vdq", "Wdq", "Ib"},
{ "~12", NULL, NULL, NULL}, 
{ "~13", NULL, NULL, NULL}, 
{ "~14", NULL, NULL, NULL}, 
{ "pcmpeqb", "Vdq", "Wdq", NULL},
{ "pcmpeqw", "Vdq", "Wdq", NULL},
{ "pcmpeqd", "Vdq", "Wdq", NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 8
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 9
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 10
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 11
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 12
{ "xadd", "Eb", "Gb", NULL},
{ "xadd", "Ev", "Gv", NULL},
{ "cmppd", "Vpd", "Wpd", "Ib"},
{"(bad)", NULL, NULL, NULL}, 
{ "pinsrw", "Vdq", "Ew", "Ib"},
{ "pextrw", "Gd", "VRdq", "Ib"},
{ "shufpd", "Vpd", "Wpd", "Ib"},
{ "~9", NULL, NULL, NULL}, 
// row 13
{"(bad)", NULL, NULL, NULL}, 
{ "psrlw", "Pdq", "Wdq", NULL},
{ "psrld", "Pdq", "Wdq", NULL},
{ "psrlq", "Pdq", "Wdq", NULL},
{ "paddq", "Pdq", "Wdq", NULL},
{ "pmullw", "Pdq", "Wdq", NULL},
{ "movq", "Wq", "Vq", NULL},
{ "pmovmskb", "Gd", "VRdq", NULL},
// row 14
{ "pavgb", "Vdq", "Wdq", NULL},
{ "psraw", "Vdq", "Wdq", NULL},
{ "psrad", "Vdq", "Wdq", NULL},
{ "pavgw", "Vdq", "Wdq", NULL},
{ "pmulhuw", "Vdq", "Wdq", NULL},
{ "pmulhw", "Vdq", "Wdq", NULL},
{ "cvtpd2dq", "Vq", "Wpd", NULL},
{ "movntq", "Mdq", "Vdq", NULL},
// row 15
{"(bad)", NULL, NULL, NULL}, 
{ "psllw", "Vdq", "Wdq", NULL},
{ "pslld", "Vdq", "Wdq", NULL},
{ "psllq", "Vdq", "Wdq", NULL},
{ "pmuludq", "Vdq", "Wdq", NULL},
{ "pmaddwd", "Vdq", "Wdq", NULL},
{ "psadbw", "Vdq", "Wdq", NULL},
{ "maskmovq", "Vdq", "VRdq", NULL},
// row 16
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 17
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 18
{ "movapd", "Vpd", "Wpd", NULL},
{ "movapd", "Wpd", "Vpd", NULL},
{ "cvtpi2pd", "Vpd", "Qd", NULL},
{ "movntpd", "Mdq", "Vpd", NULL},
{ "cvttpd2pi", "Pq", "Wpd", NULL},
{ "cvtpd2pi", "Pq", "Wpd", NULL},
{ "ucomisd", "Vsd", "Wsd", NULL},
{ "comisd", "Vpd", "Wsd", NULL},
// row 19
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 20
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 21
{ "addpd", "Vpd", "Wpd", NULL},
{ "mulpd", "Vpd", "Wpd", NULL},
{ "cvtpd2ps", "Vpd", "Wpd", NULL},
{ "cvtps2dq", "Vpd", "Wpd", NULL},
{ "subpd", "Vpd", "Wpd", NULL},
{ "minpd", "Vpd", "Wpd", NULL},
{ "divpd", "Vpd", "Wpd", NULL},
{ "maxpd", "Vpd", "Wpd", NULL},
// row 22
{ "punpckhbw", "Vdq", "Wq", NULL},
{ "punpckhwd", "Vdq", "Wq", NULL},
{ "punpckhdq", "Vdq", "Wq", NULL},
{ "packssdw", "Vdq", "Wdq", NULL},
{ "punpcklqdq", "Vdq", "Wq", NULL},
{ "punpckhqdq", "Vdq", "Wq", NULL},
{ "movd", "Vdq", "Eq", NULL},
{ "movq", "Vdq", "Wdq", NULL},
// row 23
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movd", "Ed", "Vd", NULL},
{ "movdqa", "Wdq", "Vdq", NULL},
// row 24
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 25
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 26
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 27
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 28
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 29
{"(bad)", NULL, NULL, NULL}, 
{ "psubusb", "Pdq", "Wdq", NULL},
{ "psubusw", "Pdq", "Wdq", NULL},
{ "pminub", "Pdq", "Wdq", NULL},
{ "pand", "Pdq", "Wdq", NULL},
{ "paddusb", "Pdq", "Wdq", NULL},
{ "paddusw", "Pdq", "Wdq", NULL},
{ "pmaxub", "Pdq", "Wdq", NULL},
// row 30
{ "pandn", "Pdq", "Wdq", NULL},
{ "psubsb", "Pdq", "Wdq", NULL},
{ "psubsw", "Pdq", "Wdq", NULL},
{ "pminsw", "Pdq", "Wdq", NULL},
{ "por", "Pdq", "Wdq", NULL},
{ "paddsb", "Pdq", "Wdq", NULL},
{ "paddsw", "Pdq", "Wdq", NULL},
{ "pmaxsw", "Pdq", "Wdq", NULL},
// row 31
{ "pxor", "Pdq", "Wdq", NULL},
{ "psubb", "Pdq", "Wdq", NULL},
{ "psubw", "Pdq", "Wdq", NULL},
{ "psubd", "Pdq", "Wdq", NULL},
{ "psubq", "Pdq", "Wdq", NULL},
{ "paddb", "Pdq", "Wdq", NULL},
{ "paddw", "Pdq", "Wdq", NULL},
{ "paddd", "Pdq", "Wdq", NULL},
// row 32
{"(bad)", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;


Instruction two_byte_F2[] = {
// row 0
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 1
{ "movsd", "Vdq", "Wsd", NULL},
{ "movsd", "Wpd", "Vpd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 2
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 3
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 4
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 5
{"(bad)", NULL, NULL, NULL}, 
{ "sqrtsd", "Vsd", "Wsd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 6
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 7
{ "pshuflw", "Vq", "Wq", "Ib"},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 8
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 9
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 10
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 11
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 12
{ "xadd", "Eb", "Gb", NULL},
{ "xadd", "Ev", "Gv", NULL},
{ "cmpsd", "Vsd", "Wsd", "Ib"},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "~9", NULL, NULL, NULL}, 
// row 13
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "movdq2q", "Pq", "VRq", NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 14
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "cvtpd2dq", "Vq", "Wpd", NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 15
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 16
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 17
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 18
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "cvtsi2sd", "Vsd", "Ed", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "cvttsd2si", "Gd", "Wsd", NULL},
{ "cvtsd2si", "Gd", "Wsd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 19
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 20
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 21
{ "addsd", "Vsd", "Wsd", NULL},
{ "mulsd", "Vsd", "Wsd", NULL},
{ "cvtsd2ss", "Vss", "Wsd", NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "subsd", "Vsd", "Wsd", NULL},
{ "minsd", "Vsd", "Wsd", NULL},
{ "divsd", "Vsd", "Wsd", NULL},
{ "maxsd", "Vsd", "Wsd", NULL},
// row 22
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 23
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 24
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 25
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 26
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 27
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 28
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 29
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 30
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 31
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;


Instruction x87_alt[] = {
// row 0
{ "fadd", "$w", NULL, NULL},
{ "fmul", "$w", NULL, NULL},
{ "fcom", "$w", NULL, NULL},
{ "fcomp", "$w", NULL, NULL},
{ "fsub", "$w", NULL, NULL},
{ "fsubr", "$w", NULL, NULL},
{ "fdiv", "$w", NULL, NULL},
{ "fdivr", "$w", NULL, NULL},
// row 1
{ "fld", "$w", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fst", "$w", NULL, NULL},
{ "fstp", "$w", NULL, NULL},
{ "fldenv", "$e", NULL, NULL},
{ "fldcw", "$s", NULL, NULL},
{ "fstenv", "$e", NULL, NULL},
{ "fstcw", "$s", NULL, NULL},
// row 2
{ "fiadd", "$w", NULL, NULL},
{ "fimul", "$w", NULL, NULL},
{ "ficom", "$w", NULL, NULL},
{ "ficomp", "$w", NULL, NULL},
{ "fisub", "$w", NULL, NULL},
{ "fisubr", "$w", NULL, NULL},
{ "fidiv", "$w", NULL, NULL},
{ "fidivr", "$w", NULL, NULL},
// row 3
{ "fild", "$w", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fist", "$w", NULL, NULL},
{ "fistp", "$w", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fld", "$r", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fstp", "$r", NULL, NULL},
// row 4
{ "fadd", "$z", NULL, NULL},
{ "fmul", "$z", NULL, NULL},
{ "fcom", "$z", NULL, NULL},
{ "fcomp", "$z", NULL, NULL},
{ "fsub", "$z", NULL, NULL},
{ "fsubr", "$z", NULL, NULL},
{ "fdiv", "$z", NULL, NULL},
{ "fdivr", "$z", NULL, NULL},
// row 5
{ "fld", "$z", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fst", "$z", NULL, NULL},
{ "fstp", "$z", NULL, NULL},
{ "frstor", "$y", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fsave", "$y", NULL, NULL},
{ "fstsw", "$s", NULL, NULL},
// row 6
{ "fiadd", "$s", NULL, NULL},
{ "fimul", "$s", NULL, NULL},
{ "ficom", "$s", NULL, NULL},
{ "ficomp", "$s", NULL, NULL},
{ "fisub", "$s", NULL, NULL},
{ "fisubr", "$s", NULL, NULL},
{ "fidiv", "$s", NULL, NULL},
{ "fidivr", "$s", NULL, NULL},
// row 7
{ "fild", "$s", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{ "fist", "$s", NULL, NULL},
{ "fistp", "$s", NULL, NULL},
{ "fbld", "$r", NULL, NULL},
{ "fild", "$z", NULL, NULL},
{ "fbstp", "$r", NULL, NULL},
{ "fistp", "$z", NULL, NULL},
{NULL, NULL, NULL, NULL}} ;


Instruction x87[] = {
// row 0
{ "fadd", "*st(0)", "*st(0)", NULL},
{ "fadd", "*st(0)", "*st(1)", NULL},
{ "fadd", "*st(0)", "*st(2)", NULL},
{ "fadd", "*st(0)", "*st(3)", NULL},
{ "fadd", "*st(0)", "*st(4)", NULL},
{ "fadd", "*st(0)", "*st(5)", NULL},
{ "fadd", "*st(0)", "*st(6)", NULL},
{ "fadd", "*st(0)", "*st(7)", NULL},
// row 1
{ "fmul", "*st(0)", "*st(0)", NULL},
{ "fmul", "*st(0)", "*st(1)", NULL},
{ "fmul", "*st(0)", "*st(2)", NULL},
{ "fmul", "*st(0)", "*st(3)", NULL},
{ "fmul", "*st(0)", "*st(4)", NULL},
{ "fmul", "*st(0)", "*st(5)", NULL},
{ "fmul", "*st(0)", "*st(6)", NULL},
{ "fmul", "*st(0)", "*st(7)", NULL},
// row 2
{ "fcom", "*st(0)", "*st(0)", NULL},
{ "fcom", "*st(0)", "*st(1)", NULL},
{ "fcom", "*st(0)", "*st(2)", NULL},
{ "fcom", "*st(0)", "*st(3)", NULL},
{ "fcom", "*st(0)", "*st(4)", NULL},
{ "fcom", "*st(0)", "*st(5)", NULL},
{ "fcom", "*st(0)", "*st(6)", NULL},
{ "fcom", "*st(0)", "*st(7)", NULL},
// row 3
{ "fcomp", "*st(0)", "*st(0)", NULL},
{ "fcomp", "*st(0)", "*st(1)", NULL},
{ "fcomp", "*st(0)", "*st(2)", NULL},
{ "fcomp", "*st(0)", "*st(3)", NULL},
{ "fcomp", "*st(0)", "*st(4)", NULL},
{ "fcomp", "*st(0)", "*st(5)", NULL},
{ "fcomp", "*st(0)", "*st(6)", NULL},
{ "fcomp", "*st(0)", "*st(7)", NULL},
// row 4
{ "fsub", "*st(0)", "*st(0)", NULL},
{ "fsub", "*st(0)", "*st(1)", NULL},
{ "fsub", "*st(0)", "*st(2)", NULL},
{ "fsub", "*st(0)", "*st(3)", NULL},
{ "fsub", "*st(0)", "*st(4)", NULL},
{ "fsub", "*st(0)", "*st(5)", NULL},
{ "fsub", "*st(0)", "*st(6)", NULL},
{ "fsub", "*st(0)", "*st(7)", NULL},
// row 5
{ "fsubr", "*st(0)", "*st(0)", NULL},
{ "fsubr", "*st(0)", "*st(1)", NULL},
{ "fsubr", "*st(0)", "*st(2)", NULL},
{ "fsubr", "*st(0)", "*st(3)", NULL},
{ "fsubr", "*st(0)", "*st(4)", NULL},
{ "fsubr", "*st(0)", "*st(5)", NULL},
{ "fsubr", "*st(0)", "*st(6)", NULL},
{ "fsubr", "*st(0)", "*st(7)", NULL},
// row 6
{ "fdiv", "*st(0)", "*st(0)", NULL},
{ "fdiv", "*st(0)", "*st(1)", NULL},
{ "fdiv", "*st(0)", "*st(2)", NULL},
{ "fdiv", "*st(0)", "*st(3)", NULL},
{ "fdiv", "*st(0)", "*st(4)", NULL},
{ "fdiv", "*st(0)", "*st(5)", NULL},
{ "fdiv", "*st(0)", "*st(6)", NULL},
{ "fdiv", "*st(0)", "*st(7)", NULL},
// row 7
{ "fdivr", "*st(0)", "*st(0)", NULL},
{ "fdivr", "*st(0)", "*st(1)", NULL},
{ "fdivr", "*st(0)", "*st(2)", NULL},
{ "fdivr", "*st(0)", "*st(3)", NULL},
{ "fdivr", "*st(0)", "*st(4)", NULL},
{ "fdivr", "*st(0)", "*st(5)", NULL},
{ "fdivr", "*st(0)", "*st(6)", NULL},
{ "fdivr", "*st(0)", "*st(7)", NULL},
// row 8
{ "fld", "*st(0)", "*st(0)", NULL},
{ "fld", "*st(0)", "*st(1)", NULL},
{ "fld", "*st(0)", "*st(2)", NULL},
{ "fld", "*st(0)", "*st(3)", NULL},
{ "fld", "*st(0)", "*st(4)", NULL},
{ "fld", "*st(0)", "*st(5)", NULL},
{ "fld", "*st(0)", "*st(6)", NULL},
{ "fld", "*st(0)", "*st(7)", NULL},
// row 9
{ "fxch", "*st(0)", "*st(0)", NULL},
{ "fxch", "*st(0)", "*st(1)", NULL},
{ "fxch", "*st(0)", "*st(2)", NULL},
{ "fxch", "*st(0)", "*st(3)", NULL},
{ "fxch", "*st(0)", "*st(4)", NULL},
{ "fxch", "*st(0)", "*st(5)", NULL},
{ "fxch", "*st(0)", "*st(6)", NULL},
{ "fxch", "*st(0)", "*st(7)", NULL},
// row 10
{ "fnop", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 11
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 12
{ "fchs", NULL, NULL, NULL},
{ "fabs", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "ftst", NULL, NULL, NULL},
{ "fxam", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 13
{ "fld1", NULL, NULL, NULL},
{ "fldl2t", NULL, NULL, NULL},
{ "fldl2e", NULL, NULL, NULL},
{ "fldpi", NULL, NULL, NULL},
{ "fldlg2", NULL, NULL, NULL},
{ "fldln2", NULL, NULL, NULL},
{ "fldz", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
// row 14
{ "f2xm1", NULL, NULL, NULL},
{ "fyl2x", NULL, NULL, NULL},
{ "fptan", NULL, NULL, NULL},
{ "fpatan", NULL, NULL, NULL},
{ "fxtract", NULL, NULL, NULL},
{ "fprem1", NULL, NULL, NULL},
{ "fdecstp", NULL, NULL, NULL},
{ "fincstp", NULL, NULL, NULL},
// row 15
{ "fprem", NULL, NULL, NULL},
{ "fyl2xp1", NULL, NULL, NULL},
{ "fsqrt", NULL, NULL, NULL},
{ "fsincos", NULL, NULL, NULL},
{ "frndint", NULL, NULL, NULL},
{ "fscale", NULL, NULL, NULL},
{ "fsin", NULL, NULL, NULL},
{ "fcos", NULL, NULL, NULL},
// row 16
{ "fcmovb", "*st(0)", "*st(0)", NULL},
{ "fcmovb", "*st(0)", "*st(1)", NULL},
{ "fcmovb", "*st(0)", "*st(2)", NULL},
{ "fcmovb", "*st(0)", "*st(3)", NULL},
{ "fcmovb", "*st(0)", "*st(4)", NULL},
{ "fcmovb", "*st(0)", "*st(5)", NULL},
{ "fcmovb", "*st(0)", "*st(6)", NULL},
{ "fcmovb", "*st(0)", "*st(7)", NULL},
// row 17
{ "fcmove", "*st(0)", "*st(0)", NULL},
{ "fcmove", "*st(0)", "*st(1)", NULL},
{ "fcmove", "*st(0)", "*st(2)", NULL},
{ "fcmove", "*st(0)", "*st(3)", NULL},
{ "fcmove", "*st(0)", "*st(4)", NULL},
{ "fcmove", "*st(0)", "*st(5)", NULL},
{ "fcmove", "*st(0)", "*st(6)", NULL},
{ "fcmove", "*st(0)", "*st(7)", NULL},
// row 18
{ "fcmovbe", "*st(0)", "*st(0)", NULL},
{ "fcmovbe", "*st(0)", "*st(1)", NULL},
{ "fcmovbe", "*st(0)", "*st(2)", NULL},
{ "fcmovbe", "*st(0)", "*st(3)", NULL},
{ "fcmovbe", "*st(0)", "*st(4)", NULL},
{ "fcmovbe", "*st(0)", "*st(5)", NULL},
{ "fcmovbe", "*st(0)", "*st(6)", NULL},
{ "fcmovbe", "*st(0)", "*st(7)", NULL},
// row 19
{ "fcmovu", "*st(0)", "*st(0)", NULL},
{ "fcmovu", "*st(0)", "*st(1)", NULL},
{ "fcmovu", "*st(0)", "*st(2)", NULL},
{ "fcmovu", "*st(0)", "*st(3)", NULL},
{ "fcmovu", "*st(0)", "*st(4)", NULL},
{ "fcmovu", "*st(0)", "*st(5)", NULL},
{ "fcmovu", "*st(0)", "*st(6)", NULL},
{ "fcmovu", "*st(0)", "*st(7)", NULL},
// row 20
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 21
{"(bad)", NULL, NULL, NULL}, 
{ "fucompp", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 22
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 23
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 24
{ "fcmovnb", "*st(0)", "*st(0)", NULL},
{ "fcmovnb", "*st(0)", "*st(1)", NULL},
{ "fcmovnb", "*st(0)", "*st(2)", NULL},
{ "fcmovnb", "*st(0)", "*st(3)", NULL},
{ "fcmovnb", "*st(0)", "*st(4)", NULL},
{ "fcmovnb", "*st(0)", "*st(5)", NULL},
{ "fcmovnb", "*st(0)", "*st(6)", NULL},
{ "fcmovnb", "*st(0)", "*st(7)", NULL},
// row 25
{ "fcmovne", "*st(0)", "*st(0)", NULL},
{ "fcmovne", "*st(0)", "*st(1)", NULL},
{ "fcmovne", "*st(0)", "*st(2)", NULL},
{ "fcmovne", "*st(0)", "*st(3)", NULL},
{ "fcmovne", "*st(0)", "*st(4)", NULL},
{ "fcmovne", "*st(0)", "*st(5)", NULL},
{ "fcmovne", "*st(0)", "*st(6)", NULL},
{ "fcmovne", "*st(0)", "*st(7)", NULL},
// row 26
{ "fcmovnbe", "*st(0)", "*st(0)", NULL},
{ "fcmovnbe", "*st(0)", "*st(1)", NULL},
{ "fcmovnbe", "*st(0)", "*st(2)", NULL},
{ "fcmovnbe", "*st(0)", "*st(3)", NULL},
{ "fcmovnbe", "*st(0)", "*st(4)", NULL},
{ "fcmovnbe", "*st(0)", "*st(5)", NULL},
{ "fcmovnbe", "*st(0)", "*st(6)", NULL},
{ "fcmovnbe", "*st(0)", "*st(7)", NULL},
// row 27
{ "fcmovnu", "*st(0)", "*st(0)", NULL},
{ "fcmovnu", "*st(0)", "*st(1)", NULL},
{ "fcmovnu", "*st(0)", "*st(2)", NULL},
{ "fcmovnu", "*st(0)", "*st(3)", NULL},
{ "fcmovnu", "*st(0)", "*st(4)", NULL},
{ "fcmovnu", "*st(0)", "*st(5)", NULL},
{ "fcmovnu", "*st(0)", "*st(6)", NULL},
{ "fcmovnu", "*st(0)", "*st(7)", NULL},
// row 28
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{ "fclex", NULL, NULL, NULL},
{ "finit", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 29
{ "fucomi", "*st(0)", "*st(0)", NULL},
{ "fucomi", "*st(0)", "*st(1)", NULL},
{ "fucomi", "*st(0)", "*st(2)", NULL},
{ "fucomi", "*st(0)", "*st(3)", NULL},
{ "fucomi", "*st(0)", "*st(4)", NULL},
{ "fucomi", "*st(0)", "*st(5)", NULL},
{ "fucomi", "*st(0)", "*st(6)", NULL},
{ "fucomi", "*st(0)", "*st(7)", NULL},
// row 30
{ "fcomi", "*st(0)", "*st(0)", NULL},
{ "fcomi", "*st(0)", "*st(1)", NULL},
{ "fcomi", "*st(0)", "*st(2)", NULL},
{ "fcomi", "*st(0)", "*st(3)", NULL},
{ "fcomi", "*st(0)", "*st(4)", NULL},
{ "fcomi", "*st(0)", "*st(5)", NULL},
{ "fcomi", "*st(0)", "*st(6)", NULL},
{ "fcomi", "*st(0)", "*st(7)", NULL},
// row 31
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 32
{ "fadd", "*st(0)", "*st(0)", NULL},
{ "fadd", "*st(1)", "*st(0)", NULL},
{ "fadd", "*st(2)", "*st(0)", NULL},
{ "fadd", "*st(3)", "*st(0)", NULL},
{ "fadd", "*st(4)", "*st(0)", NULL},
{ "fadd", "*st(5)", "*st(0)", NULL},
{ "fadd", "*st(6)", "*st(0)", NULL},
{ "fadd", "*st(7)", "*st(0)", NULL},
// row 33
{ "fmul", "*st(0)", "*st(0)", NULL},
{ "fmul", "*st(1)", "*st(0)", NULL},
{ "fmul", "*st(2)", "*st(0)", NULL},
{ "fmul", "*st(3)", "*st(0)", NULL},
{ "fmul", "*st(4)", "*st(0)", NULL},
{ "fmul", "*st(5)", "*st(0)", NULL},
{ "fmul", "*st(6)", "*st(0)", NULL},
{ "fmul", "*st(7)", "*st(0)", NULL},
// row 34
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 35
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 36
{ "fsubr", "*st(0)", "*st(0)", NULL},
{ "fsubr", "*st(1)", "*st(0)", NULL},
{ "fsubr", "*st(2)", "*st(0)", NULL},
{ "fsubr", "*st(3)", "*st(0)", NULL},
{ "fsubr", "*st(4)", "*st(0)", NULL},
{ "fsubr", "*st(5)", "*st(0)", NULL},
{ "fsubr", "*st(6)", "*st(0)", NULL},
{ "fsubr", "*st(7)", "*st(0)", NULL},
// row 37
{ "fsub", "*st(0)", "*st(0)", NULL},
{ "fsub", "*st(1)", "*st(0)", NULL},
{ "fsub", "*st(2)", "*st(0)", NULL},
{ "fsub", "*st(3)", "*st(0)", NULL},
{ "fsub", "*st(4)", "*st(0)", NULL},
{ "fsub", "*st(5)", "*st(0)", NULL},
{ "fsub", "*st(6)", "*st(0)", NULL},
{ "fsub", "*st(7)", "*st(0)", NULL},
// row 38
{ "fdivr", "*st(0)", "*st(0)", NULL},
{ "fdivr", "*st(1)", "*st(0)", NULL},
{ "fdivr", "*st(2)", "*st(0)", NULL},
{ "fdivr", "*st(3)", "*st(0)", NULL},
{ "fdivr", "*st(4)", "*st(0)", NULL},
{ "fdivr", "*st(5)", "*st(0)", NULL},
{ "fdivr", "*st(6)", "*st(0)", NULL},
{ "fdivr", "*st(7)", "*st(0)", NULL},
// row 39
{ "fdiv", "*st(0)", "*st(0)", NULL},
{ "fdiv", "*st(1)", "*st(0)", NULL},
{ "fdiv", "*st(2)", "*st(0)", NULL},
{ "fdiv", "*st(3)", "*st(0)", NULL},
{ "fdiv", "*st(4)", "*st(0)", NULL},
{ "fdiv", "*st(5)", "*st(0)", NULL},
{ "fdiv", "*st(6)", "*st(0)", NULL},
{ "fdiv", "*st(7)", "*st(0)", NULL},
// row 40
{ "ffree", "*st(0)", NULL, NULL},
{ "ffree", "*st(1)", NULL, NULL},
{ "ffree", "*st(2)", NULL, NULL},
{ "ffree", "*st(3)", NULL, NULL},
{ "ffree", "*st(4)", NULL, NULL},
{ "ffree", "*st(5)", NULL, NULL},
{ "ffree", "*st(6)", NULL, NULL},
{ "ffree", "*st(7)", NULL, NULL},
// row 41
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 42
{ "fst", "*st(0)", NULL, NULL},
{ "fst", "*st(1)", NULL, NULL},
{ "fst", "*st(2)", NULL, NULL},
{ "fst", "*st(3)", NULL, NULL},
{ "fst", "*st(4)", NULL, NULL},
{ "fst", "*st(5)", NULL, NULL},
{ "fst", "*st(6)", NULL, NULL},
{ "fst", "*st(7)", NULL, NULL},
// row 43
{ "fstp", "*st(0)", NULL, NULL},
{ "fstp", "*st(1)", NULL, NULL},
{ "fstp", "*st(2)", NULL, NULL},
{ "fstp", "*st(3)", NULL, NULL},
{ "fstp", "*st(4)", NULL, NULL},
{ "fstp", "*st(5)", NULL, NULL},
{ "fstp", "*st(6)", NULL, NULL},
{ "fstp", "*st(7)", NULL, NULL},
// row 44
{ "fucom", "*st(0)", NULL, NULL},
{ "fucom", "*st(1)", NULL, NULL},
{ "fucom", "*st(2)", NULL, NULL},
{ "fucom", "*st(3)", NULL, NULL},
{ "fucom", "*st(4)", NULL, NULL},
{ "fucom", "*st(5)", NULL, NULL},
{ "fucom", "*st(6)", NULL, NULL},
{ "fucom", "*st(7)", NULL, NULL},
// row 45
{ "fucomp", "*st(0)", NULL, NULL},
{ "fucomp", "*st(1)", NULL, NULL},
{ "fucomp", "*st(2)", NULL, NULL},
{ "fucomp", "*st(3)", NULL, NULL},
{ "fucomp", "*st(4)", NULL, NULL},
{ "fucomp", "*st(5)", NULL, NULL},
{ "fucomp", "*st(6)", NULL, NULL},
{ "fucomp", "*st(7)", NULL, NULL},
// row 46
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 47
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 48
{ "faddp", "*st(0)", "*st(0)", NULL},
{ "faddp", "*st(1)", "*st(0)", NULL},
{ "faddp", "*st(2)", "*st(0)", NULL},
{ "faddp", "*st(3)", "*st(0)", NULL},
{ "faddp", "*st(4)", "*st(0)", NULL},
{ "faddp", "*st(5)", "*st(0)", NULL},
{ "faddp", "*st(6)", "*st(0)", NULL},
{ "faddp", "*st(7)", "*st(0)", NULL},
// row 49
{ "fmulp", "*st(0)", "*st(0)", NULL},
{ "fmulp", "*st(1)", "*st(0)", NULL},
{ "fmulp", "*st(2)", "*st(0)", NULL},
{ "fmulp", "*st(3)", "*st(0)", NULL},
{ "fmulp", "*st(4)", "*st(0)", NULL},
{ "fmulp", "*st(5)", "*st(0)", NULL},
{ "fmulp", "*st(6)", "*st(0)", NULL},
{ "fmulp", "*st(7)", "*st(0)", NULL},
// row 50
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 51
{"(bad)", NULL, NULL, NULL}, 
{ "fcompp", NULL, NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 52
{ "fsubrp", "*st(0)", "*st(0)", NULL},
{ "fsubrp", "*st(1)", "*st(0)", NULL},
{ "fsubrp", "*st(2)", "*st(0)", NULL},
{ "fsubrp", "*st(3)", "*st(0)", NULL},
{ "fsubrp", "*st(4)", "*st(0)", NULL},
{ "fsubrp", "*st(5)", "*st(0)", NULL},
{ "fsubrp", "*st(6)", "*st(0)", NULL},
{ "fsubrp", "*st(7)", "*st(0)", NULL},
// row 53
{ "fsubp", "*st(0)", "*st(0)", NULL},
{ "fsubp", "*st(1)", "*st(0)", NULL},
{ "fsubp", "*st(2)", "*st(0)", NULL},
{ "fsubp", "*st(3)", "*st(0)", NULL},
{ "fsubp", "*st(4)", "*st(0)", NULL},
{ "fsubp", "*st(5)", "*st(0)", NULL},
{ "fsubp", "*st(6)", "*st(0)", NULL},
{ "fsubp", "*st(7)", "*st(0)", NULL},
// row 54
{ "fdivrp", "*st(0)", "*st(0)", NULL},
{ "fdivrp", "*st(1)", "*st(0)", NULL},
{ "fdivrp", "*st(2)", "*st(0)", NULL},
{ "fdivrp", "*st(3)", "*st(0)", NULL},
{ "fdivrp", "*st(4)", "*st(0)", NULL},
{ "fdivrp", "*st(5)", "*st(0)", NULL},
{ "fdivrp", "*st(6)", "*st(0)", NULL},
{ "fdivrp", "*st(7)", "*st(0)", NULL},
// row 55
{ "fdivp", "*st(0)", "*st(0)", NULL},
{ "fdivp", "*st(1)", "*st(0)", NULL},
{ "fdivp", "*st(2)", "*st(0)", NULL},
{ "fdivp", "*st(3)", "*st(0)", NULL},
{ "fdivp", "*st(4)", "*st(0)", NULL},
{ "fdivp", "*st(5)", "*st(0)", NULL},
{ "fdivp", "*st(6)", "*st(0)", NULL},
{ "fdivp", "*st(7)", "*st(0)", NULL},
// row 56
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 57
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 58
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 59
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 60
{ "fstsw", "%ax", NULL, NULL},
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
// row 61
{ "fucomip", "*st(0)", "*st(0)", NULL},
{ "fucomip", "*st(0)", "*st(1)", NULL},
{ "fucomip", "*st(0)", "*st(2)", NULL},
{ "fucomip", "*st(0)", "*st(3)", NULL},
{ "fucomip", "*st(0)", "*st(4)", NULL},
{ "fucomip", "*st(0)", "*st(5)", NULL},
{ "fucomip", "*st(0)", "*st(6)", NULL},
{ "fucomip", "*st(0)", "*st(7)", NULL},
// row 62
{ "fcomip", "*st(0)", "*st(0)", NULL},
{ "fcomip", "*st(0)", "*st(1)", NULL},
{ "fcomip", "*st(0)", "*st(2)", NULL},
{ "fcomip", "*st(0)", "*st(3)", NULL},
{ "fcomip", "*st(0)", "*st(4)", NULL},
{ "fcomip", "*st(0)", "*st(5)", NULL},
{ "fcomip", "*st(0)", "*st(6)", NULL},
{ "fcomip", "*st(0)", "*st(7)", NULL},
// row 63
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{"(bad)", NULL, NULL, NULL}, 
{NULL, NULL, NULL, NULL}} ;
