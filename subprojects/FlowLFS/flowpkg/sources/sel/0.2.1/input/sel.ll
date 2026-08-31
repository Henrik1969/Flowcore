; Flowcore target artifact: main
; Flowcore generic structured lowering plan: ordered blocks, calls, branches and returns
target triple = "x86_64-pc-linux-gnu"
@flow_string_41 = private unnamed_addr constant [77 x i8] c"Flowcore sel\0A\0A> alpha\0A  beta\0A  gamma\0A\0AUp/Down: move  Enter: select  q: quit\0A\00"
@flow_string_42 = private unnamed_addr constant [15 x i8] c"cursor: alpha\0A\00"
@flow_string_43 = private unnamed_addr constant [14 x i8] c"cursor: beta\0A\00"
@flow_string_44 = private unnamed_addr constant [15 x i8] c"cursor: gamma\0A\00"
@flow_string_45 = private unnamed_addr constant [16 x i8] c"selected: alpha\00"
@flow_string_46 = private unnamed_addr constant [15 x i8] c"selected: beta\00"
@flow_string_47 = private unnamed_addr constant [16 x i8] c"selected: gamma\00"
@flow_string_48 = private unnamed_addr constant [16 x i8] c"selection: none\00"
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
  %flow_slot_32 = alloca i32, align 4
  %flow_slot_33 = alloca i32, align 4
  %flow_slot_34 = alloca i32, align 4
  %flow_slot_35 = alloca i32, align 4
  %flow_slot_36 = alloca i32, align 4
  %flow_slot_37 = alloca i32, align 4
  %flow_slot_38 = alloca i32, align 4
  %flow_slot_39 = alloca i32, align 4
  %flow_slot_40 = alloca i32, align 4
  %flow_slot_41 = alloca ptr, align 8
  %flow_slot_42 = alloca ptr, align 8
  %flow_slot_43 = alloca ptr, align 8
  %flow_slot_44 = alloca ptr, align 8
  %flow_slot_45 = alloca ptr, align 8
  %flow_slot_46 = alloca ptr, align 8
  %flow_slot_47 = alloca ptr, align 8
  %flow_slot_48 = alloca ptr, align 8
  br label %flow_block_0
flow_block_0:
  store ptr null, ptr %flow_slot_28
  store i32 0, ptr %flow_slot_29
  store i32 0, ptr %flow_slot_30
  store i32 0, ptr %flow_slot_31
  store i32 0, ptr %flow_slot_32
  store i32 0, ptr %flow_slot_33
  store i32 0, ptr %flow_slot_34
  store i32 1, ptr %flow_slot_35
  store i32 2, ptr %flow_slot_36
  store i32 256, ptr %flow_slot_37
  store i32 258, ptr %flow_slot_38
  store i32 259, ptr %flow_slot_39
  store i32 76, ptr %flow_slot_40
  store ptr @flow_string_41, ptr %flow_slot_41
  store ptr @flow_string_42, ptr %flow_slot_42
  store ptr @flow_string_43, ptr %flow_slot_43
  store ptr @flow_string_44, ptr %flow_slot_44
  store ptr @flow_string_45, ptr %flow_slot_45
  store ptr @flow_string_46, ptr %flow_slot_46
  store ptr @flow_string_47, ptr %flow_slot_47
  store ptr @flow_string_48, ptr %flow_slot_48
  %flow_call_0 = call ptr @flow_terminal_open()
  store ptr %flow_call_0, ptr %flow_slot_28
  %flow_call_1 = call i32 @flow_terminal_enter()
  store i32 %flow_call_1, ptr %flow_slot_29
  %flow_load_0 = load ptr, ptr %flow_slot_41
  %flow_load_1 = load i32, ptr %flow_slot_40
  %flow_call_2 = call i32 @flow_terminal_write(ptr %flow_load_0, i32 %flow_load_1)
  store i32 %flow_call_2, ptr %flow_slot_29
  %flow_call_3 = call i32 @flow_terminal_present()
  store i32 %flow_call_3, ptr %flow_slot_29
  br label %flow_loop_condition_0
flow_loop_condition_0:
  %flow_load_2 = load i32, ptr %flow_slot_32
  %flow_load_3 = load i32, ptr %flow_slot_35
  %flow_condition_4 = icmp slt i32 %flow_load_2, %flow_load_3
  br i1 %flow_condition_4, label %flow_block_1, label %flow_loop_exit_1
flow_block_1:
  %flow_call_4 = call i32 @flow_terminal_read_event()
  store i32 %flow_call_4, ptr %flow_slot_30
  %flow_load_5 = load i32, ptr %flow_slot_30
  %flow_condition_6 = icmp eq i32 %flow_load_5, 113
  br i1 %flow_condition_6, label %flow_block_2, label %flow_block_3
flow_block_2:
  %flow_load_7 = load i32, ptr %flow_slot_35
  store i32 %flow_load_7, ptr %flow_slot_33
  %flow_load_8 = load i32, ptr %flow_slot_35
  store i32 %flow_load_8, ptr %flow_slot_32
  br label %flow_join_2
flow_block_3:
  %flow_load_9 = load i32, ptr %flow_slot_30
  %flow_load_10 = load i32, ptr %flow_slot_37
  %flow_condition_11 = icmp eq i32 %flow_load_9, %flow_load_10
  br i1 %flow_condition_11, label %flow_block_4, label %flow_block_5
flow_block_4:
  %flow_load_12 = load i32, ptr %flow_slot_35
  store i32 %flow_load_12, ptr %flow_slot_32
  br label %flow_join_3
flow_block_5:
  %flow_load_13 = load i32, ptr %flow_slot_30
  %flow_load_14 = load i32, ptr %flow_slot_38
  %flow_condition_15 = icmp eq i32 %flow_load_13, %flow_load_14
  br i1 %flow_condition_15, label %flow_block_6, label %flow_block_8
flow_block_6:
  %flow_load_16 = load i32, ptr %flow_slot_31
  %flow_load_17 = load i32, ptr %flow_slot_34
  %flow_condition_18 = icmp sgt i32 %flow_load_16, %flow_load_17
  br i1 %flow_condition_18, label %flow_block_7, label %flow_join_5
flow_block_7:
  %flow_load_19 = load i32, ptr %flow_slot_31
  %flow_load_20 = load i32, ptr %flow_slot_35
  %flow_arithmetic_21 = sub i32 %flow_load_19, %flow_load_20
  store i32 %flow_arithmetic_21, ptr %flow_slot_31
  br label %flow_join_5
flow_join_5:
  br label %flow_join_4
flow_block_8:
  %flow_load_22 = load i32, ptr %flow_slot_30
  %flow_load_23 = load i32, ptr %flow_slot_39
  %flow_condition_24 = icmp eq i32 %flow_load_22, %flow_load_23
  br i1 %flow_condition_24, label %flow_block_9, label %flow_join_6
flow_block_9:
  %flow_load_25 = load i32, ptr %flow_slot_31
  %flow_load_26 = load i32, ptr %flow_slot_36
  %flow_condition_27 = icmp slt i32 %flow_load_25, %flow_load_26
  br i1 %flow_condition_27, label %flow_block_10, label %flow_join_7
flow_block_10:
  %flow_load_28 = load i32, ptr %flow_slot_31
  %flow_load_29 = load i32, ptr %flow_slot_35
  %flow_arithmetic_30 = add i32 %flow_load_28, %flow_load_29
  store i32 %flow_arithmetic_30, ptr %flow_slot_31
  br label %flow_join_7
flow_join_7:
  br label %flow_join_6
flow_join_6:
  br label %flow_join_4
flow_join_4:
  %flow_load_31 = load i32, ptr %flow_slot_31
  %flow_load_32 = load i32, ptr %flow_slot_34
  %flow_condition_33 = icmp eq i32 %flow_load_31, %flow_load_32
  br i1 %flow_condition_33, label %flow_block_11, label %flow_block_12
flow_block_11:
  %flow_load_34 = load ptr, ptr %flow_slot_42
  %flow_call_5 = call i32 @flow_terminal_write(ptr %flow_load_34, i32 14)
  store i32 %flow_call_5, ptr %flow_slot_29
  br label %flow_join_8
flow_block_12:
  %flow_load_35 = load i32, ptr %flow_slot_31
  %flow_load_36 = load i32, ptr %flow_slot_35
  %flow_condition_37 = icmp eq i32 %flow_load_35, %flow_load_36
  br i1 %flow_condition_37, label %flow_block_13, label %flow_block_14
flow_block_13:
  %flow_load_38 = load ptr, ptr %flow_slot_43
  %flow_call_6 = call i32 @flow_terminal_write(ptr %flow_load_38, i32 13)
  store i32 %flow_call_6, ptr %flow_slot_29
  br label %flow_join_9
flow_block_14:
  %flow_load_39 = load ptr, ptr %flow_slot_44
  %flow_call_7 = call i32 @flow_terminal_write(ptr %flow_load_39, i32 14)
  store i32 %flow_call_7, ptr %flow_slot_29
  br label %flow_join_9
flow_join_9:
  br label %flow_join_8
flow_join_8:
  %flow_call_8 = call i32 @flow_terminal_present()
  store i32 %flow_call_8, ptr %flow_slot_29
  br label %flow_join_3
flow_join_3:
  br label %flow_join_2
flow_join_2:
  br label %flow_loop_condition_0
flow_loop_exit_1:
  %flow_call_9 = call i32 @flow_terminal_close()
  store i32 %flow_call_9, ptr %flow_slot_29
  %flow_load_40 = load i32, ptr %flow_slot_33
  %flow_load_41 = load i32, ptr %flow_slot_35
  %flow_condition_42 = icmp eq i32 %flow_load_40, %flow_load_41
  br i1 %flow_condition_42, label %flow_block_15, label %flow_block_16
flow_block_15:
  %flow_load_43 = load ptr, ptr %flow_slot_48
  %flow_call_10 = call i32 @puts(ptr %flow_load_43)
  store i32 %flow_call_10, ptr %flow_slot_29
  ret i32 1
flow_block_16:
  %flow_load_44 = load i32, ptr %flow_slot_31
  %flow_load_45 = load i32, ptr %flow_slot_34
  %flow_condition_46 = icmp eq i32 %flow_load_44, %flow_load_45
  br i1 %flow_condition_46, label %flow_block_17, label %flow_block_18
flow_block_17:
  %flow_load_47 = load ptr, ptr %flow_slot_45
  %flow_call_11 = call i32 @puts(ptr %flow_load_47)
  store i32 %flow_call_11, ptr %flow_slot_29
  br label %flow_join_11
flow_block_18:
  %flow_load_48 = load i32, ptr %flow_slot_31
  %flow_load_49 = load i32, ptr %flow_slot_35
  %flow_condition_50 = icmp eq i32 %flow_load_48, %flow_load_49
  br i1 %flow_condition_50, label %flow_block_19, label %flow_block_20
flow_block_19:
  %flow_load_51 = load ptr, ptr %flow_slot_46
  %flow_call_12 = call i32 @puts(ptr %flow_load_51)
  store i32 %flow_call_12, ptr %flow_slot_29
  br label %flow_join_12
flow_block_20:
  %flow_load_52 = load ptr, ptr %flow_slot_47
  %flow_call_13 = call i32 @puts(ptr %flow_load_52)
  store i32 %flow_call_13, ptr %flow_slot_29
  br label %flow_join_12
flow_join_12:
  br label %flow_join_11
flow_join_11:
  ret i32 0
flow_join_10:
  br label %flow_exit
flow_exit:
  ret i32 0
}
