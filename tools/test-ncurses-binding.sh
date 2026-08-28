#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
bind=${FLOWBIND_BIN:?FLOWBIND_BIN is required}
test -x "$bind"

policy=$(mktemp)
trap 'rm -f "$policy"' EXIT
printf '%s\n' \
    'allow libncursesw.so.6 initscr c terminal' \
    'allow libncursesw.so.6 endwin c terminal' \
    'allow libncursesw.so.6 noecho c terminal' \
    'allow libncursesw.so.6 cbreak c terminal' \
    'allow libncursesw.so.6 waddnstr c terminal' \
    'allow libncursesw.so.6 wrefresh c terminal' \
    'allow libncursesw.so.6 wgetch c terminal' > "$policy"

report=$(jq -n '{
  format:"flowanalyst.semantic_report", version:1, status:"ok",
  binding_requirements:[
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"initscr",effect:"terminal",parameter_types:"",return_type:"c_pointer"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"endwin",effect:"terminal",parameter_types:"",return_type:"c_int"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"noecho",effect:"terminal",parameter_types:"",return_type:"c_int"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"cbreak",effect:"terminal",parameter_types:"",return_type:"c_int"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"waddnstr",effect:"terminal",parameter_types:"c_pointer,c_string,c_int",return_type:"c_int"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"wrefresh",effect:"terminal",parameter_types:"c_pointer",return_type:"c_int"},
    {contract:"std_abi_ncurses",library:"libncursesw.so.6",convention:"c",symbol:"wgetch",effect:"terminal",parameter_types:"c_pointer",return_type:"c_int"}
  ]
}' | "$bind" --policy "$policy")

printf '%s\n' "$report" | jq -e '
  .format == "flowbind.binding_report" and
  .status == "ready" and
  .execution == "not-performed" and
  (.symbols | sort) == (["cbreak","endwin","initscr","noecho","waddnstr","wgetch","wrefresh"] | sort)
' >/dev/null

printf '%s\n' 'Ncurses binding: PASS'
