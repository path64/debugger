arch.o: arch.cc arch.h pstream.h dbg_types.h dbg_except.h dis.h opcodes.h \
  breakpoint.h gen_loc.h file_info.h exp_value.h process.h thread.h \
  target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h dbg_thread_db.h
breakpoint.o: breakpoint.cc breakpoint.h dbg_types.h dbg_except.h \
  pstream.h gen_loc.h file_info.h exp_value.h process.h thread.h target.h \
  dbg_elf.h bstream.h map_range.h cli_param.h cli.h cli_history.h \
  dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h dwf_attr.h \
  dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h junk_stream.h arch.h \
  dbg_thread_db.h
bstream.o: bstream.cc bstream.h dbg_types.h dbg_except.h err_nice.h
cli.o: cli.cc cli.h cli_history.h pstream.h dbg_types.h dbg_except.h \
  dbg_dwarf.h dwf_spec.h dbg_elf.h bstream.h map_range.h expr.h \
  exp_value.h exp_fmt.h type_base.h dwf_attr.h dwf_locs.h file_info.h \
  target.h cli_param.h xml.h readline.h utils.h pcm.h breakpoint.h \
  gen_loc.h version.h process.h thread.h dis.h opcodes.h arch.h
cli_history.o: cli_history.cc utils.h dbg_types.h dbg_except.h \
  cli_history.h pstream.h
cli_param.o: cli_param.cc cli_param.h pstream.h
dbg_dwarf.o: dbg_dwarf.cc process.h dbg_types.h dbg_except.h breakpoint.h \
  pstream.h gen_loc.h file_info.h exp_value.h thread.h target.h dbg_elf.h \
  bstream.h map_range.h cli_param.h cli.h cli_history.h dbg_dwarf.h \
  dwf_spec.h expr.h exp_fmt.h type_base.h dwf_attr.h dwf_locs.h xml.h \
  readline.h pcm.h dis.h opcodes.h dwf_cfa.h dwf_names.h dwf_cunit.h \
  dwf_stab.h dwf_entry.h dwf_abbrv.h type_nspace.h type_array.h \
  type_class.h type_struct.h type_enum.h type_union.h type_pointer.h \
  arch.h utils.h junk_stream.h dbg_stl.h
dbg_elf.o: dbg_elf.cc dbg_elf.h dbg_types.h dbg_except.h bstream.h \
  pstream.h map_range.h utils.h
dbg_except.o: dbg_except.cc dbg_except.h
dbg_proc_service.o: dbg_proc_service.cc dbg_proc_service.h pcm.h \
  dbg_types.h dbg_except.h target.h dbg_elf.h bstream.h pstream.h \
  map_range.h breakpoint.h gen_loc.h file_info.h exp_value.h
dbg_stl.o: dbg_stl.cc dbg_stl.h dbg_types.h dbg_except.h dbg_dwarf.h \
  dwf_spec.h dbg_elf.h bstream.h pstream.h map_range.h expr.h exp_value.h \
  exp_fmt.h type_base.h dwf_attr.h dwf_locs.h process.h breakpoint.h \
  gen_loc.h file_info.h thread.h target.h cli_param.h cli.h cli_history.h \
  xml.h readline.h pcm.h dis.h opcodes.h utils.h arch.h symtab.h \
  dwf_info.h dwf_stab.h dwf_abbrv.h dwf_cunit.h dwf_entry.h
dbg_thread_db.o: dbg_thread_db.cc dbg_thread_db.h dbg_types.h \
  dbg_except.h dbg_proc_service.h
dis.o: dis.cc dis.h dbg_types.h dbg_except.h opcodes.h pstream.h \
  process.h breakpoint.h gen_loc.h file_info.h exp_value.h thread.h \
  target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h
doctext.o: doctext.cc doctext.h
doctool.o: doctool.cc doctext.h
driver.o: driver.cc debugger.h arch.h pstream.h dbg_types.h dbg_except.h \
  dis.h opcodes.h breakpoint.h gen_loc.h file_info.h exp_value.h pcm.h \
  target.h dbg_elf.h bstream.h map_range.h process.h thread.h cli_param.h \
  cli.h cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h symtab.h dwf_info.h dwf_stab.h \
  dwf_abbrv.h dwf_cunit.h dwf_entry.h version.h version_bk.h
dwf_abbrv.o: dwf_abbrv.cc dwf_abbrv.h bstream.h dbg_types.h dbg_except.h \
  dwf_spec.h dwf_names.h dwf_info.h dbg_elf.h pstream.h map_range.h \
  dwf_stab.h dbg_dwarf.h expr.h exp_value.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h dwf_cunit.h dwf_entry.h file_info.h
dwf_cfa.o: dwf_cfa.cc dbg_dwarf.h dwf_spec.h dbg_elf.h dbg_types.h \
  dbg_except.h bstream.h pstream.h map_range.h expr.h exp_value.h \
  exp_fmt.h type_base.h dwf_attr.h dwf_locs.h dwf_info.h dwf_stab.h \
  dwf_abbrv.h dwf_cunit.h dwf_entry.h file_info.h err_nice.h dwf_cfa.h
dwf_cunit.o: dwf_cunit.cc dwf_cunit.h dbg_types.h dbg_except.h dwf_stab.h \
  bstream.h dwf_locs.h dwf_attr.h expr.h dbg_elf.h pstream.h map_range.h \
  dwf_spec.h exp_value.h exp_fmt.h dwf_entry.h dwf_abbrv.h type_base.h \
  file_info.h dwf_names.h dwf_info.h dbg_dwarf.h type_array.h \
  type_pointer.h type_struct.h type_qual.h
dwf_entry.o: dwf_entry.cc dwf_names.h dwf_spec.h dwf_entry.h dwf_abbrv.h \
  bstream.h dbg_types.h dbg_except.h dwf_cunit.h dwf_stab.h dwf_locs.h \
  dwf_attr.h expr.h dbg_elf.h pstream.h map_range.h exp_value.h exp_fmt.h \
  type_base.h file_info.h
dwf_info.o: dwf_info.cc process.h dbg_types.h dbg_except.h breakpoint.h \
  pstream.h gen_loc.h file_info.h exp_value.h thread.h target.h dbg_elf.h \
  bstream.h map_range.h cli_param.h cli.h cli_history.h dbg_dwarf.h \
  dwf_spec.h expr.h exp_fmt.h type_base.h dwf_attr.h dwf_locs.h xml.h \
  readline.h pcm.h dis.h opcodes.h dwf_info.h dwf_stab.h dwf_abbrv.h \
  dwf_cunit.h dwf_entry.h dwf_cfa.h dwf_names.h type_array.h type_class.h \
  type_struct.h type_enum.h type_union.h type_nspace.h type_pointer.h \
  type_qual.h
dwf_locs.o: dwf_locs.cc dbg_dwarf.h dwf_spec.h dbg_elf.h dbg_types.h \
  dbg_except.h bstream.h pstream.h map_range.h expr.h exp_value.h \
  exp_fmt.h type_base.h dwf_attr.h dwf_locs.h dwf_info.h dwf_stab.h \
  dwf_abbrv.h dwf_cunit.h dwf_entry.h file_info.h process.h breakpoint.h \
  gen_loc.h thread.h target.h cli_param.h cli.h cli_history.h xml.h \
  readline.h pcm.h dis.h opcodes.h
dwf_names.o: dwf_names.cc dwf_names.h dwf_spec.h
dwf_spec.o: dwf_spec.cc dwf_spec.h
expr.o: expr.cc expr.h dbg_elf.h dbg_types.h dbg_except.h bstream.h \
  pstream.h map_range.h dwf_spec.h exp_value.h exp_fmt.h type_array.h \
  type_base.h dwf_attr.h type_struct.h symtab.h dbg_dwarf.h dwf_locs.h \
  dwf_info.h dwf_stab.h dwf_abbrv.h dwf_cunit.h dwf_entry.h file_info.h \
  gen_loc.h process.h breakpoint.h thread.h target.h cli_param.h cli.h \
  cli_history.h xml.h readline.h pcm.h dis.h opcodes.h arch.h dbg_stl.h
file_info.o: file_info.cc utils.h dbg_types.h dbg_except.h file_info.h \
  pstream.h
funclookup.o: funclookup.cc funclookup.h symtab.h dbg_dwarf.h dwf_spec.h \
  dbg_elf.h dbg_types.h dbg_except.h bstream.h pstream.h map_range.h \
  expr.h exp_value.h exp_fmt.h type_base.h dwf_attr.h dwf_locs.h \
  dwf_info.h dwf_stab.h dwf_abbrv.h dwf_cunit.h dwf_entry.h file_info.h \
  gen_loc.h
gen_loc.o: gen_loc.cc gen_loc.h file_info.h pstream.h dbg_types.h \
  dbg_except.h symtab.h dbg_dwarf.h dwf_spec.h dbg_elf.h bstream.h \
  map_range.h expr.h exp_value.h exp_fmt.h type_base.h dwf_attr.h \
  dwf_locs.h dwf_info.h dwf_stab.h dwf_abbrv.h dwf_cunit.h dwf_entry.h
junk_stream.o: junk_stream.cc utils.h dbg_types.h dbg_except.h \
  junk_stream.h pstream.h process.h breakpoint.h gen_loc.h file_info.h \
  exp_value.h thread.h target.h dbg_elf.h bstream.h map_range.h \
  cli_param.h cli.h cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h \
  type_base.h dwf_attr.h dwf_locs.h xml.h readline.h pcm.h dis.h \
  opcodes.h
new_type_base.o: new_type_base.cc new_type_base.h
opcodes.o: opcodes.cc opcodes.h
pathcore.o: pathcore.cc dbg_elf.h dbg_types.h dbg_except.h bstream.h \
  pstream.h map_range.h
pcm.o: pcm.cc pcm.h dbg_types.h dbg_except.h target.h dbg_elf.h bstream.h \
  pstream.h map_range.h breakpoint.h gen_loc.h file_info.h exp_value.h \
  version_bk.h symtab.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h \
  type_base.h dwf_attr.h dwf_locs.h dwf_info.h dwf_stab.h dwf_abbrv.h \
  dwf_cunit.h dwf_entry.h process.h thread.h cli_param.h cli.h \
  cli_history.h xml.h readline.h dis.h opcodes.h arch.h
process.o: process.cc err_nice.h process.h dbg_types.h dbg_except.h \
  breakpoint.h pstream.h gen_loc.h file_info.h exp_value.h thread.h \
  target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h arch.h \
  type_struct.h utils.h symtab.h dwf_info.h dwf_stab.h dwf_abbrv.h \
  dwf_cunit.h dwf_entry.h dbg_thread_db.h dwf_cfa.h junk_stream.h \
  dbg_proc_service.h
pstream.o: pstream.cc pstream.h readline.h dbg_except.h
readline.o: readline.cc readline.h utils.h dbg_types.h dbg_except.h
symtab.o: symtab.cc symtab.h dbg_dwarf.h dwf_spec.h dbg_elf.h dbg_types.h \
  dbg_except.h bstream.h pstream.h map_range.h expr.h exp_value.h \
  exp_fmt.h type_base.h dwf_attr.h dwf_locs.h dwf_info.h dwf_stab.h \
  dwf_abbrv.h dwf_cunit.h dwf_entry.h file_info.h gen_loc.h process.h \
  breakpoint.h thread.h target.h cli_param.h cli.h cli_history.h xml.h \
  readline.h pcm.h dis.h opcodes.h arch.h
target.o: target.cc target.h dbg_types.h dbg_except.h dbg_elf.h bstream.h \
  pstream.h map_range.h dbg_thread_db.h arch.h dis.h opcodes.h \
  breakpoint.h gen_loc.h file_info.h exp_value.h
thread.o: thread.cc thread.h dbg_types.h dbg_except.h pstream.h arch.h \
  dis.h opcodes.h breakpoint.h gen_loc.h file_info.h exp_value.h \
  process.h target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h
trial.o: trial.cc bstream.h dbg_types.h dbg_except.h dbg_elf.h pstream.h \
  map_range.h dwf_abbrv.h dwf_spec.h dwf_cunit.h dwf_stab.h dwf_locs.h \
  dwf_attr.h expr.h exp_value.h exp_fmt.h dwf_entry.h type_base.h \
  file_info.h dwf_info.h dbg_dwarf.h
type_array.o: type_array.cc utils.h dbg_types.h dbg_except.h process.h \
  breakpoint.h pstream.h gen_loc.h file_info.h exp_value.h thread.h \
  target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h \
  type_array.h dwf_cunit.h dwf_stab.h dwf_entry.h dwf_abbrv.h
type_base.o: type_base.cc process.h dbg_types.h dbg_except.h breakpoint.h \
  pstream.h gen_loc.h file_info.h exp_value.h thread.h target.h dbg_elf.h \
  bstream.h map_range.h cli_param.h cli.h cli_history.h dbg_dwarf.h \
  dwf_spec.h expr.h exp_fmt.h type_base.h dwf_attr.h dwf_locs.h xml.h \
  readline.h pcm.h dis.h opcodes.h arch.h utils.h dbg_stl.h dwf_names.h \
  dwf_cunit.h dwf_stab.h dwf_entry.h dwf_abbrv.h
type_class.o: type_class.cc type_class.h type_base.h dwf_attr.h expr.h \
  dbg_elf.h dbg_types.h dbg_except.h bstream.h pstream.h map_range.h \
  dwf_spec.h exp_value.h exp_fmt.h type_struct.h
type_enum.o: type_enum.cc utils.h dbg_types.h dbg_except.h type_enum.h \
  type_base.h dwf_attr.h expr.h dbg_elf.h bstream.h pstream.h map_range.h \
  dwf_spec.h exp_value.h exp_fmt.h process.h breakpoint.h gen_loc.h \
  file_info.h thread.h target.h cli_param.h cli.h cli_history.h \
  dbg_dwarf.h dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h \
  junk_stream.h
type_nspace.o: type_nspace.cc type_nspace.h type_base.h dwf_attr.h expr.h \
  dbg_elf.h dbg_types.h dbg_except.h bstream.h pstream.h map_range.h \
  dwf_spec.h exp_value.h exp_fmt.h
type_pointer.o: type_pointer.cc utils.h dbg_types.h dbg_except.h \
  process.h breakpoint.h pstream.h gen_loc.h file_info.h exp_value.h \
  thread.h target.h dbg_elf.h bstream.h map_range.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h \
  dwf_attr.h dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h \
  dwf_cunit.h dwf_stab.h dwf_entry.h dwf_abbrv.h junk_stream.h \
  type_pointer.h
type_qual.o: type_qual.cc dwf_attr.h expr.h dbg_elf.h dbg_types.h \
  dbg_except.h bstream.h pstream.h map_range.h dwf_spec.h exp_value.h \
  exp_fmt.h type_qual.h type_base.h
type_struct.o: type_struct.cc dwf_cunit.h dbg_types.h dbg_except.h \
  dwf_stab.h bstream.h dwf_locs.h dwf_attr.h expr.h dbg_elf.h pstream.h \
  map_range.h dwf_spec.h exp_value.h exp_fmt.h dwf_entry.h dwf_abbrv.h \
  type_base.h file_info.h type_struct.h arch.h dis.h opcodes.h \
  breakpoint.h gen_loc.h utils.h process.h thread.h target.h cli_param.h \
  cli.h cli_history.h dbg_dwarf.h xml.h readline.h pcm.h dbg_stl.h
type_union.o: type_union.cc type_union.h type_struct.h type_base.h \
  dwf_attr.h expr.h dbg_elf.h dbg_types.h dbg_except.h bstream.h \
  pstream.h map_range.h dwf_spec.h exp_value.h exp_fmt.h process.h \
  breakpoint.h gen_loc.h file_info.h thread.h target.h cli_param.h cli.h \
  cli_history.h dbg_dwarf.h dwf_locs.h xml.h readline.h pcm.h dis.h \
  opcodes.h
utils.o: utils.cc utils.h dbg_types.h dbg_except.h pstream.h process.h \
  breakpoint.h gen_loc.h file_info.h exp_value.h thread.h target.h \
  dbg_elf.h bstream.h map_range.h cli_param.h cli.h cli_history.h \
  dbg_dwarf.h dwf_spec.h expr.h exp_fmt.h type_base.h dwf_attr.h \
  dwf_locs.h xml.h readline.h pcm.h dis.h opcodes.h
version_bk.o: version_bk.cc
xml.o: xml.cc xml.h dbg_except.h
