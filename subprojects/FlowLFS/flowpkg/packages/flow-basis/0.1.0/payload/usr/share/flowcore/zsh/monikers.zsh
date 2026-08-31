# Flowcore additive directory-moniker mechanism for Zsh.
typeset -g FLOW_MONIKERS_FILE="${FLOW_MONIKERS_FILE:-${XDG_CONFIG_HOME:-$HOME/.config}/flowcore/monikers.tsv}"
typeset -gA FLOW_MONIKERS
typeset -ga FLOW_MONIKER_NAMES

_flow_moniker_expand_path()
{
    local target=$1
    if [[ $target == '~' ]]; then target=$HOME
    elif [[ $target == '~/'* ]]; then target="$HOME/${target#\~/}"
    elif [[ $target == '$HOME' ]]; then target=$HOME
    elif [[ $target == '$HOME/'* ]]; then target="$HOME/${target#\$HOME/}"
    elif [[ $target != /* ]]; then
        print -u2 "Moniker paths must begin with ~, \$HOME, or /: $target"; return 1
    fi
    print -r -- "${target:A}"
}

flow-moniker-reload()
{
    local name target extra expanded
    local -i line_number=0
    for name in "${FLOW_MONIKER_NAMES[@]}"; do unhash -d "$name" 2>/dev/null || true; done
    FLOW_MONIKERS=(); FLOW_MONIKER_NAMES=()
    [[ -r $FLOW_MONIKERS_FILE ]] || return 0
    while IFS=$'\t' read -r name target extra || [[ -n $name || -n $target || -n $extra ]]; do
        (( ++line_number )); [[ -z $name || $name == \#* ]] && continue
        if [[ -z $target || -n $extra ]]; then print -u2 "$FLOW_MONIKERS_FILE:$line_number: expected NAME<TAB>PATH"; continue; fi
        if [[ ! $name =~ '^[a-z][a-z0-9_-]*$' ]]; then print -u2 "$FLOW_MONIKERS_FILE:$line_number: invalid moniker: $name"; continue; fi
        if (( ${+FLOW_MONIKERS[$name]} )); then print -u2 "$FLOW_MONIKERS_FILE:$line_number: duplicate moniker: $name"; continue; fi
        expanded=$(_flow_moniker_expand_path "$target") || continue
        FLOW_MONIKERS[$name]=$expanded; FLOW_MONIKER_NAMES+=("$name"); hash -d "$name=$expanded"
    done < "$FLOW_MONIKERS_FILE"
}

flow-moniker-list()
{
    local name
    (( ${#FLOW_MONIKERS} )) || { print 'No monikers have been declared.'; return 0; }
    for name in ${(ok)FLOW_MONIKERS}; do printf '%-18s %s\n' "$name" "${FLOW_MONIKERS[$name]}"; done
}

flow-moniker-show()
{
    (( $# == 1 )) || { print -u2 'Usage: flow-moniker-show NAME'; return 2; }
    (( ${+FLOW_MONIKERS[$1]} )) || { print -u2 "Unknown moniker: $1"; return 1; }
    print -r -- "${FLOW_MONIKERS[$1]}"
}

flow-moniker-add()
{
    local name=${1-} requested_path=${2:-$PWD} target
    (( $# >= 1 && $# <= 2 )) || { print -u2 'Usage: flow-moniker-add NAME [PATH]'; return 2; }
    [[ $name =~ '^[a-z][a-z0-9_-]*$' ]] || { print -u2 'Invalid moniker name.'; return 2; }
    (( ! ${+FLOW_MONIKERS[$name]} )) || { print -u2 "Moniker already exists: $name"; return 1; }
    target=$(_flow_moniker_expand_path "$requested_path") || return
    [[ -d $target ]] || { print -u2 "Directory does not exist: $target"; return 1; }
    mkdir -p -- ${FLOW_MONIKERS_FILE:h}
    printf '%s\t%s\n' "$name" "$target" >> "$FLOW_MONIKERS_FILE" || return
    flow-moniker-reload
}

flow-moniker-remove()
{
    local name=${1-} temporary
    (( $# == 1 )) || { print -u2 'Usage: flow-moniker-remove NAME'; return 2; }
    (( ${+FLOW_MONIKERS[$name]} )) || { print -u2 "Unknown moniker: $name"; return 1; }
    temporary=$(mktemp "${FLOW_MONIKERS_FILE}.XXXXXX") || return
    awk -F '\t' -v moniker="$name" '$1 != moniker { print }' "$FLOW_MONIKERS_FILE" > "$temporary" || { /bin/rm -f -- "$temporary"; return 1; }
    mv -- "$temporary" "$FLOW_MONIKERS_FILE" || return
    flow-moniker-reload
}

flow-moniker-edit()
{
    "${EDITOR:-vim}" "$FLOW_MONIKERS_FILE" || return
    flow-moniker-reload
}

go2()
{
    local target
    case ${1-} in
        '') flow-moniker-list; return ;;
        -l|--list) flow-moniker-list; return ;;
        -r|--reload) flow-moniker-reload; return ;;
        -s|--show) shift; flow-moniker-show "$@"; return ;;
        -h|--help) print 'Usage: go2 [NAME | --list | --show NAME | --reload]'; return ;;
    esac
    (( $# == 1 )) || { print -u2 'Usage: go2 [NAME | --list | --show NAME | --reload]'; return 2; }
    (( ${+FLOW_MONIKERS[$1]} )) || { print -u2 "Unknown moniker: $1"; return 1; }
    target=${FLOW_MONIKERS[$1]}; [[ -d $target ]] || { print -u2 "Moniker target does not exist: $1 -> $target"; return 1; }
    builtin cd -- "$target"
}

flow-moniker-reload

_flow_moniker_values()
{
    local name
    local -a values
    for name in ${(ok)FLOW_MONIKERS}; do values+=("$name:${FLOW_MONIKERS[$name]}"); done
    _describe 'moniker' values
}

_flow_go2()
{
    _arguments \
        '(-l --list)'{-l,--list}'[list declared monikers]' \
        '(-r --reload)'{-r,--reload}'[reload the moniker file]' \
        '(-s --show)'{-s,--show}'[show a moniker target]:moniker:_flow_moniker_values' \
        '(-h --help)'{-h,--help}'[show help]' \
        '1:moniker:_flow_moniker_values'
}

(( $+functions[compdef] )) && compdef _flow_go2 go2

# Convenience commands are additive and collision-safe. go2 remains the
# canonical interface whenever an existing command owns the same name.
for _flow_moniker_name in "${FLOW_MONIKER_NAMES[@]}"; do
    if (( ! $+commands[$_flow_moniker_name] && ! $+functions[$_flow_moniker_name] &&
          ! $+aliases[$_flow_moniker_name] && ! $+builtins[$_flow_moniker_name] &&
          ! $+reswords[$_flow_moniker_name] )); then
        functions[$_flow_moniker_name]="go2 ${(q)_flow_moniker_name}"
    fi
done
unset _flow_moniker_name
