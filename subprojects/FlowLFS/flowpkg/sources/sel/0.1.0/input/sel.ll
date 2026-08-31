; Flowcore target artifact: main
; Flowcore generic structured lowering plan: ordered blocks, calls, branches and returns
target triple = "x86_64-pc-linux-gnu"
@flow_string_148 = private unnamed_addr constant [76 x i8] c"Flowcore sel\0A\0A> alpha\0A  beta\0A  gamma\0A\0AUp/Down: move  Enter: select  q: quit\00"
@flow_string_149 = private unnamed_addr constant [16 x i8] c"selected: alpha\00"
@flow_string_150 = private unnamed_addr constant [16 x i8] c"selection: none\00"
declare i32 @cbreak()
declare i32 @endwin()
declare ptr @initscr()
declare i32 @keypad(ptr, i32)
declare i32 @noecho()
declare i32 @waddnstr(ptr, ptr, i32)
declare i32 @wgetch(ptr)
declare i32 @wrefresh(ptr)
declare i32 @puts(ptr)
declare i64 @read(i32, ptr, i64)
declare i64 @write(i32, ptr, i64)
declare ptr @memset(ptr, i32, i64)
define i32 @main(i32 %argc, ptr %argv) {
entry:
  %flow_slot_143 = alloca ptr, align 8
  %flow_slot_144 = alloca i32, align 4
  %flow_slot_145 = alloca i32, align 4
  %flow_slot_146 = alloca i32, align 4
  %flow_slot_147 = alloca i32, align 4
  %flow_slot_148 = alloca ptr, align 8
  %flow_slot_149 = alloca ptr, align 8
  %flow_slot_150 = alloca ptr, align 8
  %flow_slot_151 = alloca ptr, align 8
  %flow_slot_152 = alloca i64, align 8
  %flow_slot_153 = alloca i64, align 8
  %flow_slot_154 = alloca i64, align 8
  %flow_slot_155 = alloca i32, align 4
  %flow_slot_156 = alloca i32, align 4
  %flow_slot_157 = alloca i32, align 4
  %flow_slot_158 = alloca ptr, align 8
  %flow_slot_159 = alloca i64, align 8
  %flow_storage_151 = alloca [4096 x i8], align 1
  %flow_storage_ptr_151 = getelementptr [4096 x i8], ptr %flow_storage_151, i64 0, i64 0
  br label %flow_block_0
flow_block_0:
  store ptr null, ptr %flow_slot_143
  store i32 0, ptr %flow_slot_144
  store i32 0, ptr %flow_slot_145
  store i32 1, ptr %flow_slot_146
  store i32 58, ptr %flow_slot_147
  store ptr @flow_string_148, ptr %flow_slot_148
  store ptr @flow_string_149, ptr %flow_slot_149
  store ptr @flow_string_150, ptr %flow_slot_150
  store ptr %flow_storage_ptr_151, ptr %flow_slot_151
  store i64 4096, ptr %flow_slot_152
  store i64 4095, ptr %flow_slot_153
  store i64 0, ptr %flow_slot_154
  store i32 0, ptr %flow_slot_155
  store i32 1, ptr %flow_slot_156
  store i32 %argc, ptr %flow_slot_157
  %flow_call_1 = call ptr @initscr()
  store ptr %flow_call_1, ptr %flow_slot_143
  %flow_call_2 = call i32 @noecho()
  store i32 %flow_call_2, ptr %flow_slot_144
  %flow_call_3 = call i32 @cbreak()
  store i32 %flow_call_3, ptr %flow_slot_144
  %flow_load_0 = load ptr, ptr %flow_slot_143
  %flow_load_1 = load i32, ptr %flow_slot_146
  %flow_call_4 = call i32 @keypad(ptr %flow_load_0, i32 %flow_load_1)
  store i32 %flow_call_4, ptr %flow_slot_144
  %flow_load_2 = load ptr, ptr %flow_slot_143
  %flow_load_3 = load ptr, ptr %flow_slot_148
  %flow_load_4 = load i32, ptr %flow_slot_147
  %flow_call_5 = call i32 @waddnstr(ptr %flow_load_2, ptr %flow_load_3, i32 %flow_load_4)
  store i32 %flow_call_5, ptr %flow_slot_144
  %flow_load_5 = load i32, ptr %flow_slot_157
  %flow_condition_6 = icmp sgt i32 %flow_load_5, 1
  br i1 %flow_condition_6, label %flow_block_1, label %flow_block_2
flow_block_1:
  %flow_arg_address_7 = getelementptr ptr, ptr %argv, i32 1
  %flow_arg_8 = load ptr, ptr %flow_arg_address_7
  store ptr %flow_arg_8, ptr %flow_slot_158
  %flow_load_9 = load ptr, ptr %flow_slot_143
  %flow_load_10 = load ptr, ptr %flow_slot_158
  %flow_load_11 = load i32, ptr %flow_slot_147
  %flow_call_6 = call i32 @waddnstr(ptr %flow_load_9, ptr %flow_load_10, i32 %flow_load_11)
  store i32 %flow_call_6, ptr %flow_slot_144
  br label %flow_join_0
flow_block_2:
  %flow_load_12 = load ptr, ptr %flow_slot_151
  %flow_load_13 = load i32, ptr %flow_slot_155
  %flow_load_14 = load i64, ptr %flow_slot_152
  %flow_call_7 = call ptr @memset(ptr %flow_load_12, i32 %flow_load_13, i64 %flow_load_14)
  store ptr %flow_call_7, ptr %flow_slot_151
  %flow_load_15 = load i32, ptr %flow_slot_155
  %flow_load_16 = load ptr, ptr %flow_slot_151
  %flow_load_17 = load i64, ptr %flow_slot_153
  %flow_call_8 = call i64 @read(i32 %flow_load_15, ptr %flow_load_16, i64 %flow_load_17)
  store i64 %flow_call_8, ptr %flow_slot_154
  %flow_load_18 = load i64, ptr %flow_slot_154
  %flow_condition_19 = icmp slt i64 %flow_load_18, 0
  br i1 %flow_condition_19, label %flow_block_3, label %flow_block_4
flow_block_3:
  %flow_call_9 = call i32 @endwin()
  store i32 %flow_call_9, ptr %flow_slot_144
  ret i32 2
flow_block_4:
  %flow_load_20 = load i64, ptr %flow_slot_154
  %flow_condition_21 = icmp sgt i64 %flow_load_20, 0
  br i1 %flow_condition_21, label %flow_block_5, label %flow_join_2
flow_block_5:
  %flow_load_22 = load i64, ptr %flow_slot_154
  store i64 %flow_load_22, ptr %flow_slot_159
  %flow_load_23 = load i32, ptr %flow_slot_156
  %flow_load_24 = load ptr, ptr %flow_slot_151
  %flow_load_25 = load i64, ptr %flow_slot_159
  %flow_call_10 = call i64 @write(i32 %flow_load_23, ptr %flow_load_24, i64 %flow_load_25)
  store i64 %flow_call_10, ptr %flow_slot_154
  br label %flow_join_2
flow_join_2:
  br label %flow_join_1
flow_join_1:
  br label %flow_join_0
flow_join_0:
  %flow_load_26 = load ptr, ptr %flow_slot_143
  %flow_call_11 = call i32 @wrefresh(ptr %flow_load_26)
  store i32 %flow_call_11, ptr %flow_slot_144
  %flow_load_27 = load ptr, ptr %flow_slot_143
  %flow_call_12 = call i32 @wgetch(ptr %flow_load_27)
  store i32 %flow_call_12, ptr %flow_slot_145
  %flow_call_13 = call i32 @endwin()
  store i32 %flow_call_13, ptr %flow_slot_144
  %flow_load_28 = load i32, ptr %flow_slot_145
  %flow_condition_29 = icmp eq i32 %flow_load_28, 113
  br i1 %flow_condition_29, label %flow_block_6, label %flow_block_7
flow_block_6:
  %flow_load_30 = load ptr, ptr %flow_slot_150
  %flow_call_14 = call i32 @puts(ptr %flow_load_30)
  store i32 %flow_call_14, ptr %flow_slot_144
  ret i32 1
flow_block_7:
  %flow_load_31 = load ptr, ptr %flow_slot_149
  %flow_call_15 = call i32 @puts(ptr %flow_load_31)
  store i32 %flow_call_15, ptr %flow_slot_144
  ret i32 0
flow_join_3:
  br label %flow_exit
flow_exit:
  ret i32 0
}
