# FlowLFS global login-shell integration.
[ -r /etc/flow-shell/environment.sh ] && . /etc/flow-shell/environment.sh

if [ -n "${BASH_VERSION-}" ]; then
    case $- in
        *i*) [ -r /etc/flow-shell/bashrc ] && . /etc/flow-shell/bashrc ;;
    esac
fi
