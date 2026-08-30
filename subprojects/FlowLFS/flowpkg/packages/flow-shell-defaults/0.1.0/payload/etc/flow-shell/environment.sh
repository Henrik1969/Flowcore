# FlowLFS shell-neutral environment. POSIX shell compatible and idempotent.

_flow_path_prepend()
{
    [ -d "$1" ] || return 0
    case ":${PATH-}:" in
        *":$1:"*) ;;
        *) PATH="$1${PATH:+:$PATH}" ;;
    esac
}

_flow_path_prepend "$HOME/bin"
_flow_path_prepend "$HOME/.local/bin"

export PATH
export EDITOR="${EDITOR:-vim}"
export VISUAL="${VISUAL:-$EDITOR}"
export PAGER="${PAGER:-less}"
export LESS="${LESS:--FRX}"

unset -f _flow_path_prepend 2>/dev/null || true
