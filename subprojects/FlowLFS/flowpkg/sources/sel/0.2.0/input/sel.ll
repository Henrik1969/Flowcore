; Flowcore target artifact: main
; Flowcore generic structured lowering plan: ordered blocks, calls, branches and returns
target triple = "x86_64-pc-linux-gnu"
@flow_string_32 = private unnamed_addr constant [77 x i8] c"Flowcore sel\0A\0A> alpha\0A  beta\0A  gamma\0A\0AUp/Down: move  Enter: select  q: quit\0A\00"
@flow_string_33 = private unnamed_addr constant [16 x i8] c"selected: alpha\00"
@flow_string_34 = private unnamed_addr constant [16 x i8] c"selection: none\00"
declare i32 @puts(ptr)
declare i32 @flow_terminal_close()
declare i32 @flow_terminal_enter()
declare ptr @flow_terminal_open()
declare i32 @flow_terminal_present()
declare i32 @flow_terminal_read_event()
declare i32 @flow_terminal_write(ptr, i32)
define i32 @main() {
entry:
  %flow_slot_28 = alloca ptr, align 8
  %flow_slot_29 = alloca i32, align 4
  %flow_slot_30 = alloca i32, align 4
  %flow_slot_31 = alloca i32, align 4
  %flow_slot_32 = alloca ptr, align 8
  %flow_slot_33 = alloca ptr, align 8
  %flow_slot_34 = alloca ptr, align 8
  br label %flow_block_0
flow_block_0:
  store ptr null, ptr %flow_slot_28
  store i32 0, ptr %flow_slot_29
  store i32 0, ptr %flow_slot_30
  store i32 76, ptr %flow_slot_31
  store ptr @flow_string_32, ptr %flow_slot_32
  store ptr @flow_string_33, ptr %flow_slot_33
  store ptr @flow_string_34, ptr %flow_slot_34
  %flow_call_0 = call ptr @flow_terminal_open()
  store ptr %flow_call_0, ptr %flow_slot_28
  %flow_call_1 = call i32 @flow_terminal_enter()
  store i32 %flow_call_1, ptr %flow_slot_29
  %flow_load_0 = load ptr, ptr %flow_slot_32
  %flow_load_1 = load i32, ptr %flow_slot_31
  %flow_call_2 = call i32 @flow_terminal_write(ptr %flow_load_0, i32 %flow_load_1)
  store i32 %flow_call_2, ptr %flow_slot_29
  %flow_call_3 = call i32 @flow_terminal_present()
  store i32 %flow_call_3, ptr %flow_slot_29
  %flow_call_4 = call i32 @flow_terminal_read_event()
  store i32 %flow_call_4, ptr %flow_slot_30
  %flow_call_5 = call i32 @flow_terminal_close()
  store i32 %flow_call_5, ptr %flow_slot_29
  %flow_load_2 = load i32, ptr %flow_slot_30
  %flow_condition_3 = icmp eq i32 %flow_load_2, 113
  br i1 %flow_condition_3, label %flow_block_1, label %flow_block_2
flow_block_1:
  %flow_load_4 = load ptr, ptr %flow_slot_34
  %flow_call_6 = call i32 @puts(ptr %flow_load_4)
  store i32 %flow_call_6, ptr %flow_slot_29
  ret i32 1
flow_block_2:
  %flow_load_5 = load ptr, ptr %flow_slot_33
  %flow_call_7 = call i32 @puts(ptr %flow_load_5)
  store i32 %flow_call_7, ptr %flow_slot_29
  ret i32 0
flow_join_0:
  br label %flow_exit
flow_exit:
  ret i32 0
}
