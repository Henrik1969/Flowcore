# FlowLFS additive login-shell integration.
[ -r /etc/flow-basis/environment.sh ] && . /etc/flow-basis/environment.sh
if [ -n "${BASH_VERSION-}" ]; then
    case $- in *i*) [ -r /etc/flow-basis/bashrc ] && . /etc/flow-basis/bashrc ;; esac
fi
