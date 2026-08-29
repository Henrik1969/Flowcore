#!/usr/bin/env python3
"""Compile the pinned LFS Chapter 8 command blocks into a resumable runner."""

from __future__ import annotations

import argparse
import re
import tarfile
from pathlib import Path

from bs4 import BeautifulSoup


PAGES = """
man-pages iana-etc glibc zlib bzip2 xz lz4 zstd file readline pcre2 m4 bc
flex tcl expect dejagnu ninja pkgconf binutils gmp mpfr mpc attr acl libcap
libxcrypt shadow gawk gcc ncurses sed psmisc gettext bison grep bash libtool
gdbm gperf expat inetutils less perl autoconf automake openssl libelf libffi
sqlite mpdecimal Python flit-core packaging wheel setuptools meson kmod
coreutils diffutils findutils groff grub gzip iproute2 kbd libpipeline make
patch tar texinfo vim markupsafe jinja2 systemd dbus man-db procps-ng
util-linux e2fsprogs
""".split()

PREFIX = {
    "libelf": "elfutils-",
    "flit-core": "flit_core-",
    "Python": "Python-",
    "sqlite": "sqlite-autoconf-",
    "tcl": "tcl8.6.18-src",
    "expect": "expect5",
}

TEST_PATTERN = re.compile(
    r"(?:make[^\n]*(?:check|test)|ninja\s+test|meson\s+test|"
    r"pytest|test_summary|tests?/)", re.I
)


def archive_for(page: str, archives: list[str]) -> str:
    prefix = PREFIX.get(page, page + "-")
    candidates = [a for a in archives if a.startswith(prefix) and ".patch" not in a]
    candidates = [a for a in candidates if "-docs-" not in a and "-html." not in a
                  and "-man-pages-" not in a]
    if len(candidates) != 1:
        raise SystemExit(f"archive mapping for {page!r}: {candidates}")
    return candidates[0]


def shell_name(page: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", page.lower()).strip("_")


def mutate(page: str, command: str) -> str:
    if page == "glibc":
        command = command.replace("/usr/share/zoneinfo/<xxx>",
                                  "/usr/share/zoneinfo/Europe/Copenhagen")
    if page == "groff":
        command = command.replace("PAGE=<paper_size>", "PAGE=A4")
    if page == "glibc" and command.startswith('grep "Timed out"'):
        command += " || true"
    return command


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--book", type=Path, required=True)
    parser.add_argument("--wget-list", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    archives = [Path(x.strip()).name for x in args.wget_list.read_text().splitlines()
                if x.strip()]
    with tarfile.open(args.book, "r:bz2") as book:
        members = {Path(m.name).name: m for m in book.getmembers()
                   if "/chapter08/" in m.name and m.name.endswith(".html")}
        recipes: dict[str, list[str]] = {}
        for page in PAGES:
            member = members.get(page + ".html")
            if member is None:
                raise SystemExit(f"missing book page: {page}")
            stream = book.extractfile(member)
            assert stream is not None
            soup = BeautifulSoup(stream.read(), "html.parser")
            recipes[page] = []
            for pre in soup.select("div.installation pre.userinput"):
                command = pre.get_text().rstrip()
                # Upgrade-only commands do not apply to a new LFS filesystem.
                if page == "glibc" and command in {
                    "rm -f /usr/sbin/nscd", "systemctl disable --now nscd"
                }:
                    continue
                # The book uses this only to refresh an interactive shell.
                if page == "bash" and command == "exec /usr/bin/bash --login":
                    continue
                # This is an explicitly 32-bit-only invocation; our target is x86_64.
                if page == "gmp" and command == "ABI=32 ./configure ...":
                    continue
                recipes[page].append(mutate(page, command))
            if not recipes[page]:
                raise SystemExit(f"no installation commands: {page}")

    lines = ["""#!/usr/bin/env bash

set -euo pipefail
test "$(id -u)" -eq 0
test -r /proc/self/mounts

log_dir=/var/log/flowlfs/ch08
mkdir -p "$log_dir"
exec > >(tee -a "$log_dir/chapter08.log") 2>&1

run_step()
{
  local name=$1 marker="$log_dir/$1.done"
  shift
  if test -e "$marker"; then
    printf 'SKIP completed step: %s\\n' "$name"
    return
  fi
  printf '\\n===== START %s %s =====\\n' "$name" "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')"
  "$@"
  printf 'completed=%s\\n' "$(date --utc +'%Y-%m-%dT%H:%M:%SZ')" > "$marker"
  printf '===== PASS %s =====\\n' "$name"
}

unpack_source()
{
  local archive=$1 top
  cd /sources
  top=$(tar -tf "$archive" | sed -n '1{s#/.*##;p}')
  test -n "$top"
  rm -rf "$top"
  tar -xf "$archive"
  cd "$top"
  FLOWLFS_SOURCE_TOP=$top
}

finish_source()
{
  cd /sources
  rm -rf "$FLOWLFS_SOURCE_TOP"
}
"""]

    for page in PAGES:
        func = shell_name(page)
        archive = archive_for(page, archives)
        lines += [f"\npackage_{func}()\n{{",
                  f"  unpack_source {archive}"]
        for index, command in enumerate(recipes[page], 1):
            if TEST_PATTERN.search(command):
                lines += ["  set +e", command,
                          "  test_status=$?", "  set -e",
                          f"  printf 'TEST-STATUS {page} block-{index}=%s\\n' \"$test_status\""]
            else:
                lines.append(command)
        lines += ["  finish_source", "}",
                  f"run_step {page} package_{func}"]

    lines += ["", "printf '\\nCHAPTER 8 PACKAGES COMPLETE %s\\n' \"$(date --utc +'%Y-%m-%dT%H:%M:%SZ')\"", ""]
    args.output.write_text("\n".join(lines))
    args.output.chmod(0o755)


if __name__ == "__main__":
    main()
