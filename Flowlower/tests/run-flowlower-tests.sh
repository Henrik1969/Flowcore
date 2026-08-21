#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
lowerer=${FLOWLOWER_BIN:?FLOWLOWER_BIN is required}
optimizer=${FLOWOPTIMIZE_BIN:-$root/Flowoptimize/build/flowoptimize}
analyst=${FLOWANALYST_BIN:-$root/Flowanalyst/build/flowanalyst}
parallel=${FLOWPARALLEL_BIN:-$root/Flowparallel/build/flowparallel}
bind=${FLOWBIND_BIN:-$root/Flowbind/build/flowbind}
flowmini=${FLOWMINI_BIN:-$root/Flowmini/flowmini_v25_symboltable_projection/cmake-build-debug/flowmini}
fixture="$root/Flowmini/flowmini_v25_symboltable_projection/examples/ast/call_expression_probe.flow"
test -x "$flowmini"
test -x "$parallel"
test -x "$lowerer"

report=$("$flowmini" --dump-frontend-bundle "$fixture" | "$analyst" | "$parallel" | "$optimizer" | "$lowerer")
printf '%s\n' "$report" | grep -q '"format": "flowlower.lowering_report"'
printf '%s\n' "$report" | grep -q '"name": "llvm"'
printf '%s\n' "$report" | grep -q '"format": "llvm-ir"'
printf '%s\n' "$report" | jq -e --arg source "$fixture" '.source.path == $source' >/dev/null

if printf '%s' '{"format":"flowoptimize.optimization_report","version":1,"status":"blocked"}' | "$lowerer" >/dev/null 2>&1; then
    echo 'blocked optimization report unexpectedly lowered' >&2
    exit 1
fi

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
trial="$root/Flowlower/tests/empty_program_main.flow"
"$flowmini" --dump-frontend-bundle "$trial" | "$analyst" | "$parallel" | "$optimizer" | "$lowerer" --emit-llvm "$tmpdir/trial.ll" > "$tmpdir/lowering-report.json"
grep -q '"status": "emitted"' "$tmpdir/lowering-report.json"
test -s "$tmpdir/trial.ll"
clang "$tmpdir/trial.ll" -o "$tmpdir/trial"
"$tmpdir/trial"

policy="$tmpdir/abi.policy"
printf '%s\n' \
    'allow libc.so.6 strlen c pure' \
    'allow libc.so.6 abs c pure' \
    'allow libc.so.6 labs c pure' \
    'allow libc.so.6 puts c io' \
    'allow libc.so.6 getpid c readonly' \
    'allow libc.so.6 clock_gettime c readonly' \
    'allow libc.so.6 getrandom c readonly' \
    'allow libc.so.6 uname c readonly' \
    'allow libc.so.6 openat c filesystem' \
    'allow libc.so.6 read c filesystem' \
    'allow libc.so.6 write c filesystem' \
    'allow libc.so.6 lseek c filesystem' \
    'allow libc.so.6 unlinkat c filesystem' \
    'allow libc.so.6 rmdir c filesystem' \
    'allow libc.so.6 pipe2 c process_ipc' \
    'allow libc.so.6 fork c process_ipc' \
    'allow libc.so.6 waitpid c process_ipc' \
    'allow libc.so.6 socketpair c socket_ipc' \
    'allow libc.so.6 socket c loopback' \
    'allow libc.so.6 bind c loopback' \
    'allow libc.so.6 listen c loopback' \
    'allow libc.so.6 poll c loopback' \
    'allow libc.so.6 accept4 c loopback' \
    'allow libc.so.6 connect c loopback' \
    'allow libc.so.6 unshare c namespace' \
    'allow libc.so.6 sethostname c namespace' \
    'allow libc.so.6 gethostname c namespace' \
    'allow libc.so.6 open c io' \
    'allow libc.so.6 read c io' \
    'allow libc.so.6 write c io' \
    'allow libc.so.6 close c io' > "$policy"
abs_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_abs_main.flow"
"$flowmini" --dump-frontend-bundle "$abs_source" > "$tmpdir/abs.bundle.json"
"$analyst" < "$tmpdir/abs.bundle.json" > "$tmpdir/abs.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/abs.semantic.json" > "$tmpdir/abs.binding.json"
"$parallel" < "$tmpdir/abs.semantic.json" > "$tmpdir/abs.parallel.json"
"$optimizer" < "$tmpdir/abs.parallel.json" > "$tmpdir/abs.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/abs.ll" --binding-report "$tmpdir/abs.binding.json" < "$tmpdir/abs.optimized.json" > "$tmpdir/abs.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/abs.lowering.json"
clang "$tmpdir/abs.ll" -o "$tmpdir/abs"
set +e
"$tmpdir/abs"
abs_rc=$?
set -e
test "$abs_rc" -eq 42

strlen_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_strlen_main.flow"
"$flowmini" --dump-frontend-bundle "$strlen_source" > "$tmpdir/strlen.bundle.json"
"$analyst" < "$tmpdir/strlen.bundle.json" > "$tmpdir/strlen.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/strlen.semantic.json" > "$tmpdir/strlen.binding.json"
"$parallel" < "$tmpdir/strlen.semantic.json" > "$tmpdir/strlen.parallel.json"
"$optimizer" < "$tmpdir/strlen.parallel.json" > "$tmpdir/strlen.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/strlen.ll" --binding-report "$tmpdir/strlen.binding.json" < "$tmpdir/strlen.optimized.json" > "$tmpdir/strlen.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/strlen.lowering.json"
clang "$tmpdir/strlen.ll" -o "$tmpdir/strlen"
set +e
"$tmpdir/strlen"
strlen_rc=$?
set -e
test "$strlen_rc" -eq 8

licbinds_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/test_licbinds.flow"
"$flowmini" --dump-frontend-bundle "$licbinds_source" > "$tmpdir/licbinds.bundle.json"
"$analyst" < "$tmpdir/licbinds.bundle.json" > "$tmpdir/licbinds.semantic.json"
grep -q '"lowering_profile": "test_licbinds_main"' "$tmpdir/licbinds.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/licbinds.semantic.json" > "$tmpdir/licbinds.binding.json"
"$parallel" < "$tmpdir/licbinds.semantic.json" > "$tmpdir/licbinds.parallel.json"
"$optimizer" < "$tmpdir/licbinds.parallel.json" > "$tmpdir/licbinds.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/licbinds.ll" --binding-report "$tmpdir/licbinds.binding.json" < "$tmpdir/licbinds.optimized.json" > "$tmpdir/licbinds.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/licbinds.lowering.json"
clang -g "$tmpdir/licbinds.ll" -o "$tmpdir/licbinds"
"$tmpdir/licbinds" > "$tmpdir/licbinds.output"
printf '%s\n' 'Flowcore libc bindings' | cmp -s - "$tmpdir/licbinds.output"
gdb -q -batch -ex 'set pagination off' -ex 'break main' -ex run -ex bt -ex quit "$tmpdir/licbinds" > "$tmpdir/licbinds.gdb.txt" 2>&1
grep -q 'Breakpoint 1' "$tmpdir/licbinds.gdb.txt"
if grep -q 'ptrace: Operation not permitted' "$tmpdir/licbinds.gdb.txt"; then
    grep -q 'During startup program exited' "$tmpdir/licbinds.gdb.txt"
else
    grep -q 'main' "$tmpdir/licbinds.gdb.txt"
fi

kernel_getpid_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_getpid_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_getpid_source" > "$tmpdir/kernel-getpid.bundle.json"
"$analyst" < "$tmpdir/kernel-getpid.bundle.json" > "$tmpdir/kernel-getpid.semantic.json"
grep -q '"lowering_profile": "abi_kernel_getpid_main"' "$tmpdir/kernel-getpid.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-getpid.semantic.json" > "$tmpdir/kernel-getpid.binding.json"
"$parallel" < "$tmpdir/kernel-getpid.semantic.json" > "$tmpdir/kernel-getpid.parallel.json"
"$optimizer" < "$tmpdir/kernel-getpid.parallel.json" > "$tmpdir/kernel-getpid.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-getpid.ll" --binding-report "$tmpdir/kernel-getpid.binding.json" < "$tmpdir/kernel-getpid.optimized.json" > "$tmpdir/kernel-getpid.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-getpid.lowering.json"
grep -q 'call i32 @getpid' "$tmpdir/kernel-getpid.ll"
clang "$tmpdir/kernel-getpid.ll" -o "$tmpdir/kernel-getpid"
"$tmpdir/kernel-getpid"

kernel_clock_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_clock_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_clock_source" > "$tmpdir/kernel-clock.bundle.json"
"$analyst" < "$tmpdir/kernel-clock.bundle.json" > "$tmpdir/kernel-clock.semantic.json"
grep -q '"lowering_profile": "abi_kernel_clock_main"' "$tmpdir/kernel-clock.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-clock.semantic.json" > "$tmpdir/kernel-clock.binding.json"
"$parallel" < "$tmpdir/kernel-clock.semantic.json" > "$tmpdir/kernel-clock.parallel.json"
"$optimizer" < "$tmpdir/kernel-clock.parallel.json" > "$tmpdir/kernel-clock.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-clock.ll" --binding-report "$tmpdir/kernel-clock.binding.json" < "$tmpdir/kernel-clock.optimized.json" > "$tmpdir/kernel-clock.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-clock.lowering.json"
grep -q 'call i32 @clock_gettime' "$tmpdir/kernel-clock.ll"
clang "$tmpdir/kernel-clock.ll" -o "$tmpdir/kernel-clock"
"$tmpdir/kernel-clock"

kernel_random_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_random_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_random_source" > "$tmpdir/kernel-random.bundle.json"
"$analyst" < "$tmpdir/kernel-random.bundle.json" > "$tmpdir/kernel-random.semantic.json"
grep -q '"lowering_profile": "abi_kernel_random_main"' "$tmpdir/kernel-random.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-random.semantic.json" > "$tmpdir/kernel-random.binding.json"
"$parallel" < "$tmpdir/kernel-random.semantic.json" > "$tmpdir/kernel-random.parallel.json"
"$optimizer" < "$tmpdir/kernel-random.parallel.json" > "$tmpdir/kernel-random.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-random.ll" --binding-report "$tmpdir/kernel-random.binding.json" < "$tmpdir/kernel-random.optimized.json" > "$tmpdir/kernel-random.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-random.lowering.json"
grep -q 'call i64 @getrandom' "$tmpdir/kernel-random.ll"
clang "$tmpdir/kernel-random.ll" -o "$tmpdir/kernel-random"
"$tmpdir/kernel-random"

kernel_uname_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_uname_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_uname_source" > "$tmpdir/kernel-uname.bundle.json"
"$analyst" < "$tmpdir/kernel-uname.bundle.json" > "$tmpdir/kernel-uname.semantic.json"
grep -q '"lowering_profile": "abi_kernel_uname_main"' "$tmpdir/kernel-uname.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-uname.semantic.json" > "$tmpdir/kernel-uname.binding.json"
"$parallel" < "$tmpdir/kernel-uname.semantic.json" > "$tmpdir/kernel-uname.parallel.json"
"$optimizer" < "$tmpdir/kernel-uname.parallel.json" > "$tmpdir/kernel-uname.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-uname.ll" --binding-report "$tmpdir/kernel-uname.binding.json" < "$tmpdir/kernel-uname.optimized.json" > "$tmpdir/kernel-uname.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-uname.lowering.json"
grep -q 'call i32 @uname' "$tmpdir/kernel-uname.ll"
clang "$tmpdir/kernel-uname.ll" -o "$tmpdir/kernel-uname"
"$tmpdir/kernel-uname"

kernel_openat_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_openat_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_openat_source" > "$tmpdir/kernel-openat.bundle.json"
"$analyst" < "$tmpdir/kernel-openat.bundle.json" > "$tmpdir/kernel-openat.semantic.json"
grep -q '"lowering_profile": "abi_kernel_openat_main"' "$tmpdir/kernel-openat.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-openat.semantic.json" > "$tmpdir/kernel-openat.binding.json"
"$parallel" < "$tmpdir/kernel-openat.semantic.json" > "$tmpdir/kernel-openat.parallel.json"
"$optimizer" < "$tmpdir/kernel-openat.parallel.json" > "$tmpdir/kernel-openat.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-openat.ll" --binding-report "$tmpdir/kernel-openat.binding.json" < "$tmpdir/kernel-openat.optimized.json" > "$tmpdir/kernel-openat.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-openat.lowering.json"
grep -q 'call i32 @openat' "$tmpdir/kernel-openat.ll"
clang "$tmpdir/kernel-openat.ll" -o "$tmpdir/kernel-openat"
"$tmpdir/kernel-openat"

kernel_read_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_read_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_read_source" > "$tmpdir/kernel-read.bundle.json"
"$analyst" < "$tmpdir/kernel-read.bundle.json" > "$tmpdir/kernel-read.semantic.json"
grep -q '"lowering_profile": "abi_kernel_read_main"' "$tmpdir/kernel-read.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-read.semantic.json" > "$tmpdir/kernel-read.binding.json"
"$parallel" < "$tmpdir/kernel-read.semantic.json" > "$tmpdir/kernel-read.parallel.json"
"$optimizer" < "$tmpdir/kernel-read.parallel.json" > "$tmpdir/kernel-read.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-read.ll" --binding-report "$tmpdir/kernel-read.binding.json" < "$tmpdir/kernel-read.optimized.json" > "$tmpdir/kernel-read.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-read.lowering.json"
grep -q 'call i64 @read' "$tmpdir/kernel-read.ll"
clang "$tmpdir/kernel-read.ll" -o "$tmpdir/kernel-read"
"$tmpdir/kernel-read"

kernel_write_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_write_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_write_source" > "$tmpdir/kernel-write.bundle.json"
"$analyst" < "$tmpdir/kernel-write.bundle.json" > "$tmpdir/kernel-write.semantic.json"
grep -q '"lowering_profile": "abi_kernel_write_main"' "$tmpdir/kernel-write.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-write.semantic.json" > "$tmpdir/kernel-write.binding.json"
"$parallel" < "$tmpdir/kernel-write.semantic.json" > "$tmpdir/kernel-write.parallel.json"
"$optimizer" < "$tmpdir/kernel-write.parallel.json" > "$tmpdir/kernel-write.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-write.ll" --binding-report "$tmpdir/kernel-write.binding.json" < "$tmpdir/kernel-write.optimized.json" > "$tmpdir/kernel-write.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-write.lowering.json"
grep -q 'call i64 @write' "$tmpdir/kernel-write.ll"
clang "$tmpdir/kernel-write.ll" -o "$tmpdir/kernel-write"
"$tmpdir/kernel-write"

kernel_lseek_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_lseek_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_lseek_source" > "$tmpdir/kernel-lseek.bundle.json"
"$analyst" < "$tmpdir/kernel-lseek.bundle.json" > "$tmpdir/kernel-lseek.semantic.json"
grep -q '"lowering_profile": "abi_kernel_lseek_main"' "$tmpdir/kernel-lseek.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-lseek.semantic.json" > "$tmpdir/kernel-lseek.binding.json"
"$parallel" < "$tmpdir/kernel-lseek.semantic.json" > "$tmpdir/kernel-lseek.parallel.json"
"$optimizer" < "$tmpdir/kernel-lseek.parallel.json" > "$tmpdir/kernel-lseek.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-lseek.ll" --binding-report "$tmpdir/kernel-lseek.binding.json" < "$tmpdir/kernel-lseek.optimized.json" > "$tmpdir/kernel-lseek.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-lseek.lowering.json"
grep -q 'call i64 @lseek' "$tmpdir/kernel-lseek.ll"
clang "$tmpdir/kernel-lseek.ll" -o "$tmpdir/kernel-lseek"
"$tmpdir/kernel-lseek"

kernel_unlinkat_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_unlinkat_main.flow"
"$flowmini" --dump-frontend-bundle "$kernel_unlinkat_source" > "$tmpdir/kernel-unlinkat.bundle.json"
"$analyst" < "$tmpdir/kernel-unlinkat.bundle.json" > "$tmpdir/kernel-unlinkat.semantic.json"
grep -q '"lowering_profile": "abi_kernel_unlinkat_main"' "$tmpdir/kernel-unlinkat.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/kernel-unlinkat.semantic.json" > "$tmpdir/kernel-unlinkat.binding.json"
"$parallel" < "$tmpdir/kernel-unlinkat.semantic.json" > "$tmpdir/kernel-unlinkat.parallel.json"
"$optimizer" < "$tmpdir/kernel-unlinkat.parallel.json" > "$tmpdir/kernel-unlinkat.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/kernel-unlinkat.ll" --binding-report "$tmpdir/kernel-unlinkat.binding.json" < "$tmpdir/kernel-unlinkat.optimized.json" > "$tmpdir/kernel-unlinkat.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/kernel-unlinkat.lowering.json"
grep -q 'call i32 @unlinkat' "$tmpdir/kernel-unlinkat.ll"
clang "$tmpdir/kernel-unlinkat.ll" -o "$tmpdir/kernel-unlinkat"
"$tmpdir/kernel-unlinkat"

for kernel_name in rmdir pipe2 fork waitpid socketpair socket bind listen poll accept4 connect unshare sethostname gethostname; do
    kernel_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/pass/abi_kernel_${kernel_name}_main.flow"
    "$flowmini" --dump-frontend-bundle "$kernel_source" > "$tmpdir/kernel-${kernel_name}.bundle.json"
    "$analyst" < "$tmpdir/kernel-${kernel_name}.bundle.json" > "$tmpdir/kernel-${kernel_name}.semantic.json"
    grep -q "\"lowering_profile\": \"abi_kernel_${kernel_name}_main\"" "$tmpdir/kernel-${kernel_name}.semantic.json"
    "$bind" --policy "$policy" < "$tmpdir/kernel-${kernel_name}.semantic.json" > "$tmpdir/kernel-${kernel_name}.binding.json"
    "$parallel" < "$tmpdir/kernel-${kernel_name}.semantic.json" > "$tmpdir/kernel-${kernel_name}.parallel.json"
    "$optimizer" < "$tmpdir/kernel-${kernel_name}.parallel.json" > "$tmpdir/kernel-${kernel_name}.optimized.json"
    "$lowerer" --emit-llvm "$tmpdir/kernel-${kernel_name}.ll" --binding-report "$tmpdir/kernel-${kernel_name}.binding.json" < "$tmpdir/kernel-${kernel_name}.optimized.json" > "$tmpdir/kernel-${kernel_name}.lowering.json"
    grep -q '"status": "emitted"' "$tmpdir/kernel-${kernel_name}.lowering.json"
    clang "$tmpdir/kernel-${kernel_name}.ll" -o "$tmpdir/kernel-${kernel_name}"
    "$tmpdir/kernel-${kernel_name}"
done

flowcat_source="$root/Flowmini/flowmini_v25_symboltable_projection/examples/apps/flowcat/flowcat.flow"
"$flowmini" --dump-frontend-bundle "$flowcat_source" > "$tmpdir/flowcat.bundle.json"
"$analyst" < "$tmpdir/flowcat.bundle.json" > "$tmpdir/flowcat.semantic.json"
grep -q '"lowering_profile": "flowcat_file_main"' "$tmpdir/flowcat.semantic.json"
"$bind" --policy "$policy" < "$tmpdir/flowcat.semantic.json" > "$tmpdir/flowcat.binding.json"
"$parallel" < "$tmpdir/flowcat.semantic.json" > "$tmpdir/flowcat.parallel.json"
"$optimizer" < "$tmpdir/flowcat.parallel.json" > "$tmpdir/flowcat.optimized.json"
"$lowerer" --emit-llvm "$tmpdir/flowcat.ll" --binding-report "$tmpdir/flowcat.binding.json" < "$tmpdir/flowcat.optimized.json" > "$tmpdir/flowcat.lowering.json"
grep -q '"status": "emitted"' "$tmpdir/flowcat.lowering.json"
clang "$tmpdir/flowcat.ll" -o "$tmpdir/flowcat"
printf '%s\n' alpha > "$tmpdir/alpha.txt"
printf '%s\n' beta > "$tmpdir/beta.txt"
"$tmpdir/flowcat" "$tmpdir/alpha.txt" "$tmpdir/beta.txt" > "$tmpdir/flowcat.output"
printf '%s\n' alpha beta | cmp -s - "$tmpdir/flowcat.output"
target_report='{"format": "flowoptimize.optimization_report", "version": 1, "status": "ready", "targets": [{"name":"cli","main_count":1},{"name":"daemon","main_count":1}]}'
printf '%s\n' "$target_report" | "$lowerer" --target cli > "$tmpdir/target-cli.json"
jq -e '.status == "ready" and .target.name == "cli"' "$tmpdir/target-cli.json" >/dev/null
set +e
printf '%s\n' "$target_report" | "$lowerer" > "$tmpdir/target-missing.json"
target_rc=$?
set -e
test "$target_rc" -eq 2
jq -e '.status == "blocked" and (.reason | contains("explicit --target"))' "$tmpdir/target-missing.json" >/dev/null
target_artifact_report='{"format": "flowoptimize.optimization_report", "version": 1, "status": "ready", "lowering_profile": "empty_program_main", "targets": [{"name":"cli","main_count":1},{"name":"daemon","main_count":1}]}'
printf '%s\n' "$target_artifact_report" | "$lowerer" --target cli --emit-llvm "$tmpdir/cli.ll" > "$tmpdir/cli-lowering.json"
printf '%s\n' "$target_artifact_report" | "$lowerer" --target daemon --emit-llvm "$tmpdir/daemon.ll" > "$tmpdir/daemon-lowering.json"
jq -e '.status == "ready" and .target.name == "cli" and .artifact.target_specific == true and .artifact.status == "emitted"' "$tmpdir/cli-lowering.json" >/dev/null
jq -e '.status == "ready" and .target.name == "daemon" and .artifact.target_specific == true and .artifact.status == "emitted"' "$tmpdir/daemon-lowering.json" >/dev/null
grep -q '; Flowcore target artifact: cli' "$tmpdir/cli.ll"
grep -q '; Flowcore target artifact: daemon' "$tmpdir/daemon.ll"
if cmp -s "$tmpdir/cli.ll" "$tmpdir/daemon.ll"; then
    echo 'target artifacts unexpectedly identical' >&2
    exit 1
fi
clang "$tmpdir/cli.ll" -o "$tmpdir/cli"
clang "$tmpdir/daemon.ll" -o "$tmpdir/daemon"
"$tmpdir/cli"
"$tmpdir/daemon"
echo 'Flowlower tests: PASS'
