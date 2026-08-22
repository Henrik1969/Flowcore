#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace flowlower::structured {

struct Json;
using Array = std::vector<Json>;
using Object = std::map<std::string, Json>;
struct Json : std::variant<std::nullptr_t, bool, double, std::string, Array, Object> {
    using variant::variant;
};

class Parser {
public:
    explicit Parser(std::string text) : text_(std::move(text)) {}
    Json parse() { skip(); auto result = value(); skip(); if (at() != '\0') fail("trailing input"); return result; }
private:
    std::string text_;
    std::size_t position_ = 0;
    char at() const { return position_ < text_.size() ? text_[position_] : '\0'; }
    void skip() { while (std::isspace(static_cast<unsigned char>(at()))) ++position_; }
    [[noreturn]] void fail(const std::string& message) const { throw std::runtime_error("JSON: " + message); }
    void expect(char expected) { if (at() != expected) fail(std::string("expected '") + expected + "'"); ++position_; }
    void literal(std::string_view expected) { if (text_.compare(position_, expected.size(), expected) != 0) fail("invalid literal"); position_ += expected.size(); }
    Json value() {
        skip();
        switch (at()) {
            case '{': return object(); case '[': return array(); case '"': return string();
            case 't': literal("true"); return true; case 'f': literal("false"); return false;
            case 'n': literal("null"); return nullptr; default: return number();
        }
    }
    Json object() {
        Object result; expect('{'); skip(); if (at() == '}') { ++position_; return result; }
        for (;;) {
            skip(); if (at() != '"') fail("object key must be a string");
            auto key = std::get<std::string>(string()); skip(); expect(':');
            if (!result.emplace(std::move(key), value()).second) fail("duplicate object key");
            skip(); if (at() == '}') { ++position_; return result; } expect(',');
        }
    }
    Json array() {
        Array result; expect('['); skip(); if (at() == ']') { ++position_; return result; }
        for (;;) { result.push_back(value()); skip(); if (at() == ']') { ++position_; return result; } expect(','); }
    }
    Json string() {
        expect('"'); std::string result;
        while (at() != '"') {
            if (at() == '\0') fail("unterminated string");
            if (at() == '\\') {
                ++position_;
                switch (at()) {
                    case '"': case '\\': case '/': result += at(); ++position_; break;
                    case 'b': result += '\b'; ++position_; break; case 'f': result += '\f'; ++position_; break;
                    case 'n': result += '\n'; ++position_; break; case 'r': result += '\r'; ++position_; break;
                    case 't': result += '\t'; ++position_; break; default: fail("unsupported string escape");
                }
            } else { result += at(); ++position_; }
        }
        ++position_; return result;
    }
    Json number() {
        const auto begin = position_; if (at() == '-') ++position_;
        while (std::isdigit(static_cast<unsigned char>(at()))) ++position_;
        if (at() == '.') { ++position_; while (std::isdigit(static_cast<unsigned char>(at()))) ++position_; }
        if (at() == 'e' || at() == 'E') { ++position_; if (at() == '+' || at() == '-') ++position_; while (std::isdigit(static_cast<unsigned char>(at()))) ++position_; }
        if (begin == position_) fail("expected value");
        return std::stod(text_.substr(begin, position_ - begin));
    }
};

inline const Json* field(const Json& value, std::string_view name) {
    const auto* object = std::get_if<Object>(&value); if (!object) return nullptr;
    const auto found = object->find(std::string{name}); return found == object->end() ? nullptr : &found->second;
}
inline std::string text(const Json* value) { return value && std::holds_alternative<std::string>(*value) ? std::get<std::string>(*value) : std::string{}; }
inline int integer(const Json* value, const char* name, int fallback = -1) {
    if (!value) return fallback;
    if (!std::holds_alternative<double>(*value)) throw std::runtime_error(std::string("JSON field '") + name + "' must be numeric");
    return static_cast<int>(std::get<double>(*value));
}
inline const Array& array(const Json* value, const char* name) {
    if (!value || !std::holds_alternative<Array>(*value)) throw std::runtime_error(std::string("JSON field '") + name + "' must be an array");
    return std::get<Array>(*value);
}

struct Provider {
    std::string contract, library, convention, symbol, effect, parameters, result;
    auto tie() const { return std::tie(contract, library, convention, symbol, effect, parameters, result); }
    bool operator<(const Provider& other) const { return tie() < other.tie(); }
};
struct Operation {
    int id = -1, statement = -1, block = -1, result_symbol = -1, then_block = -1, else_block = -1, body_block = -1;
    std::string kind;
    const Json* operand = nullptr;
    std::optional<Provider> provider;
};

inline std::string llvm_type(std::string_view carrier) {
    if (carrier == "c_int") return "i32";
    if (carrier == "c_long" || carrier == "c_ulong" || carrier == "c_size_t") return "i64";
    if (carrier == "c_string" || carrier == "c_pointer") return "ptr";
    return {};
}
inline bool c_symbol(std::string_view symbol) {
    if (symbol.empty() || (!std::isalpha(static_cast<unsigned char>(symbol.front())) && symbol.front() != '_')) return false;
    return std::all_of(symbol.begin() + 1, symbol.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; });
}
inline std::vector<std::string> carriers(std::string_view joined) {
    std::vector<std::string> result;
    for (std::size_t start = 0; start < joined.size();) {
        const auto end = joined.find(',', start); result.emplace_back(joined.substr(start, end == std::string_view::npos ? joined.size() - start : end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return result;
}
inline std::string slot(int symbol) { return "%flow_slot_" + std::to_string(symbol); }

class Emitter {
public:
    Emitter(const Json& root, const Json& binding) : root_(root), binding_(binding) { load(); }
    bool applicable() const { return (has_branch_ || has_declared_carrier_) && !invalid_control_ && !unsupported_ && !operations_.empty(); }
    bool requires_structured_control() const { return has_nonroot_block_ && !unsupported_; }
    std::string emit() {
        authorize();
        std::ostringstream out;
        out << "; Flowcore generic structured lowering plan: ordered blocks, calls, branches and returns\n"
               "target triple = \"x86_64-pc-linux-gnu\"\n";
        emit_globals(out); emit_declarations(out);
        out << (uses_args_ ? "define i32 @main(i32 %argc, ptr %argv) {\n" : "define i32 @main() {\n") << "entry:\n";
        emit_allocations(out);
        out << "  br label %flow_block_0\n";
        emit_block(0, out, "flow_exit");
        out << "flow_exit:\n  ret i32 0\n}\n";
        return out.str();
    }
private:
    const Json& root_; const Json& binding_;
    std::vector<Operation> operations_;
    std::map<int, std::vector<const Operation*>> blocks_;
    std::map<int, std::string> symbol_types_;
    std::map<int, const Json*> definitions_;
    std::map<std::string, std::string> carrier_representations_;
    std::set<Provider> providers_, authorized_;
    bool has_branch_ = false, has_declared_carrier_ = false, has_nonroot_block_ = false, invalid_control_ = false, unsupported_ = false, uses_args_ = false;
    int temporary_ = 0, label_ = 0;

    static Provider provider(const Json& value) {
        return {text(field(value,"contract")), text(field(value,"library")), text(field(value,"convention")),
                text(field(value,"symbol")), text(field(value,"effect")), text(field(value,"parameter_types")), text(field(value,"return_type"))};
    }
    std::string llvm_type(std::string_view carrier) const {
        const auto builtin = flowlower::structured::llvm_type(carrier);
        if (!builtin.empty()) return builtin;
        const auto found = carrier_representations_.find(std::string{carrier});
        if (found != carrier_representations_.end() && (found->second == "void*" || found->second == "const void*")) return "ptr";
        return {};
    }
    void load() {
        if (text(field(root_, "format")) != "flowoptimize.optimization_report" || integer(field(root_, "version"), "version") != 1) return;
        for (const auto& item : array(field(root_, "abi_type_contracts"), "abi_type_contracts")) {
            const auto name = text(field(item, "name"));
            const auto representation = text(field(item, "repr"));
            if (!name.empty() && !representation.empty()) carrier_representations_[name] = representation;
        }
        const auto* plan = field(root_, "lowering_plan");
        if (!plan || text(field(*plan,"format")) != "flowcore.lowering_plan" || integer(field(*plan,"version"),"lowering_plan.version") != 1) return;
        for (const auto& item : array(field(*plan,"operations"), "lowering_plan.operations")) {
            Operation op; op.id=integer(field(item,"id"),"id"); op.statement=integer(field(item,"statement_id"),"statement_id");
            op.block=integer(field(item,"block_id"),"block_id"); op.kind=text(field(item,"kind"));
            op.result_symbol=integer(field(item,"result_symbol_id"),"result_symbol_id");
            op.then_block=integer(field(item,"then_block_id"),"then_block_id"); op.else_block=integer(field(item,"else_block_id"),"else_block_id");
            op.body_block=integer(field(item,"body_block_id"),"body_block_id");
            const auto& operands=array(field(item,"operands"),"operation.operands"); if (!operands.empty()) op.operand=&operands.front();
            if (const auto* facts=field(item,"provider")) {
                op.provider=provider(*facts); providers_.insert(*op.provider);
                if (flowlower::structured::llvm_type(op.provider->result).empty() && !llvm_type(op.provider->result).empty()) has_declared_carrier_=true;
                for (const auto& carrier : carriers(op.provider->parameters))
                    if (flowlower::structured::llvm_type(carrier).empty() && !llvm_type(carrier).empty()) has_declared_carrier_=true;
                if (op.result_symbol>=0) symbol_types_[op.result_symbol]=op.provider->result;
            }
            if (op.kind=="value_definition" && op.result_symbol>=0 && op.operand) { definitions_[op.result_symbol]=op.operand; symbol_types_[op.result_symbol]=text(field(*op.operand,"type")); }
            if (op.kind=="branch") {
                has_branch_=true;
                if (op.operand) {
                    const auto* left=field(*op.operand,"left");
                    const auto left_type=left?text(field(*left,"type")):std::string{};
                    if (llvm_type(left_type)=="ptr") unsupported_=true;
                }
            }
            if (op.block!=0) has_nonroot_block_=true;
            if (op.kind!="call" && op.kind!="external_call" && op.kind!="value_definition" && op.kind!="branch" && op.kind!="return_value" && op.kind!="loop" && op.kind!="assignment") unsupported_=true;
            operations_.push_back(std::move(op));
        }
        for (auto& op:operations_) if (op.kind!="call") blocks_[op.block].push_back(&op);
        std::map<int,int> block_start;
        for (const auto& [block,ops]:blocks_) for (const auto* op:ops)
            if (op->kind!="loop" && (!block_start.count(block) || op->statement<block_start[block])) block_start[block]=op->statement;
        for (auto& [block, ops]:blocks_) std::stable_sort(ops.begin(),ops.end(),[&](auto* a,auto* b){
            const int a_statement=a->kind=="loop"&&block_start.count(a->body_block)?block_start[a->body_block]:a->statement;
            const int b_statement=b->kind=="loop"&&block_start.count(b->body_block)?block_start[b->body_block]:b->statement;
            return a_statement < b_statement || (a_statement==b_statement && a->id<b->id);
        });
        std::set<int> reachable{0};
        bool changed=true;
        while(changed) {
            changed=false;
            const auto snapshot=reachable;
            for(int block:snapshot) for(const auto* op:blocks_[block]) {
                const int children[]={op->then_block,op->else_block,op->body_block};
                for(int child:children) if(child>=0 && reachable.insert(child).second) changed=true;
            }
        }
        for(const auto& [block,ops]:blocks_) if(!ops.empty()&&!reachable.count(block)) invalid_control_=true;
        if (text(field(binding_,"format"))=="flowbind.binding_report" && text(field(binding_,"status"))=="ready")
            for (const auto& item:array(field(binding_,"capabilities"),"binding.capabilities")) if (text(field(item,"status"))=="authorized") authorized_.insert(provider(item));
        for (const auto& [symbol, definition]:definitions_) {
            const auto kind=text(field(*definition,"kind")); const auto intrinsic=text(field(*definition,"intrinsic"));
            if (intrinsic=="list_length" || intrinsic=="list_index") uses_args_=true;
        }
    }
    void authorize() const {
        if (authorized_.empty()) throw std::runtime_error("generic structured plan requires a ready typed binding report");
        for (const auto& required:providers_) if (!authorized_.count(required)) throw std::runtime_error("generic structured operation is not exactly authorized: "+required.symbol);
    }
    static std::string escaped_string(std::string_view value) {
        static constexpr char hex[]="0123456789ABCDEF"; std::string result;
        for (unsigned char c:value) {
            if (c>=32 && c<=126 && c!='"' && c!='\\') result.push_back(static_cast<char>(c));
            else { result.push_back('\\'); result.push_back(hex[c>>4]); result.push_back(hex[c&15]); }
        }
        return result+"\\00";
    }
    void emit_globals(std::ostringstream& out) const {
        for (const auto& [symbol, value]:definitions_) if (text(field(*value,"kind"))=="string_literal") {
            const auto literal=text(field(*value,"value")); out << "@flow_string_"<<symbol<<" = private unnamed_addr constant ["<<literal.size()+1<<" x i8] c\""<<escaped_string(literal)<<"\"\n";
        }
    }
    void emit_declarations(std::ostringstream& out) const {
        for (const auto& p:providers_) {
            if (!c_symbol(p.symbol) || llvm_type(p.result).empty()) throw std::runtime_error("unsupported structured provider ABI");
            out << "declare "<<llvm_type(p.result)<<" @"<<p.symbol<<"("; const auto params=carriers(p.parameters);
            for (std::size_t i=0;i<params.size();++i) { if(i) out<<", "; const auto type=llvm_type(params[i]); if(type.empty()) throw std::runtime_error("unsupported structured parameter carrier"); out<<type; }
            out << ")\n";
        }
    }
    void emit_allocations(std::ostringstream& out) const {
        for (const auto& [symbol,type]:symbol_types_) { const auto llvm=llvm_type(type); if(!llvm.empty()) out<<"  "<<slot(symbol)<<" = alloca "<<llvm<<", align "<<(llvm=="i32"?4:8)<<"\n"; }
        for (const auto& [symbol,value]:definitions_) if(text(field(*value,"kind"))=="writable_storage") {
            const auto* storage=field(*value,"storage"); const int bytes=integer(storage?field(*storage,"bytes"):nullptr,"storage.bytes");
            if(bytes<=0) throw std::runtime_error("invalid compatibility writable storage size");
            out<<"  %flow_storage_"<<symbol<<" = alloca ["<<bytes<<" x i8], align 1\n"
               <<"  %flow_storage_ptr_"<<symbol<<" = getelementptr ["<<bytes<<" x i8], ptr %flow_storage_"<<symbol<<", i64 0, i64 0\n";
        }
    }
    std::string load_symbol(int symbol,std::ostringstream& out) {
        const auto found=symbol_types_.find(symbol); if(found==symbol_types_.end()) return {};
        const auto name="%flow_load_"+std::to_string(temporary_++); out<<"  "<<name<<" = load "<<llvm_type(found->second)<<", ptr "<<slot(symbol)<<"\n"; return name;
    }
    std::pair<std::string,std::string> expression(const Json& value,std::ostringstream& out,std::string expected={}) {
        const auto kind=text(field(value,"kind")); auto type=text(field(value,"type")); if(!expected.empty()) type=expected;
        if(kind=="integer_literal") {
            const auto literal=text(field(value,"value"));
            if (llvm_type(type)=="ptr" && literal=="0") return {"ptr","null"};
            return {llvm_type(type),literal};
        }
        if(kind=="identifier") {
            const int symbol=integer(field(value,"symbol_id"),"symbol_id"); const auto native_type=llvm_type(symbol_types_[symbol]); auto loaded=load_symbol(symbol,out);
            const auto wanted=expected.empty()?native_type:llvm_type(expected); if(wanted==native_type) return {native_type,loaded};
            const auto converted="%flow_promote_"+std::to_string(temporary_++);
            if(native_type=="i32"&&wanted=="i64") out<<"  "<<converted<<" = sext i32 "<<loaded<<" to i64\n";
            else if(native_type=="i64"&&wanted=="i32") out<<"  "<<converted<<" = trunc i64 "<<loaded<<" to i32\n"; else return {};
            return {wanted,converted};
        }
        if(kind=="call" && text(field(value,"intrinsic"))=="list_length") return {"i32","%argc"};
        if(kind=="index" && text(field(value,"intrinsic"))=="list_index") {
            const auto [index_type,index_value]=expression(*field(value,"index"),out,"c_int");
            const auto address="%flow_arg_address_"+std::to_string(temporary_++), loaded="%flow_arg_"+std::to_string(temporary_++);
            out<<"  "<<address<<" = getelementptr ptr, ptr %argv, i32 "<<index_value<<"\n  "<<loaded<<" = load ptr, ptr "<<address<<"\n"; return {"ptr",loaded};
        }
        if(kind=="conversion") {
            const auto* operand=field(value,"operand"); if(!operand) return {};
            auto [from_type,from]=expression(*operand,out); const auto to=llvm_type(text(field(value,"type")));
            if(from_type==to) return {to,from};
            const auto result="%flow_convert_"+std::to_string(temporary_++);
            if(from_type=="i64"&&to=="i32") out<<"  "<<result<<" = trunc i64 "<<from<<" to i32\n";
            else if(from_type=="i32"&&to=="i64") out<<"  "<<result<<" = sext i32 "<<from<<" to i64\n"; else return {};
            return {to,result};
        }
        if(kind=="unary") {
            auto [operand_type,operand]=expression(*field(value,"operand"),out,type); const auto op=text(field(value,"operator"));
            if(op=="+") return {operand_type,operand};
            if(op!="-") return {};
            const auto result="%flow_unary_"+std::to_string(temporary_++); out<<"  "<<result<<" = sub "<<operand_type<<" 0, "<<operand<<"\n"; return {operand_type,result};
        }
        if(kind=="binary") {
            auto [left_type,left]=expression(*field(value,"left"),out); auto [right_type,right]=expression(*field(value,"right"),out,text(field(*field(value,"left"),"type")));
            const auto op=text(field(value,"operator")); std::string instruction;
            if(op=="==")instruction="eq"; else if(op=="!=")instruction="ne"; else if(op=="<")instruction="slt"; else if(op=="<=")instruction="sle"; else if(op==">")instruction="sgt"; else if(op==">=")instruction="sge";
            if(!instruction.empty()&&!left.empty()&&!right.empty()&&left_type==right_type) {
                const auto result="%flow_condition_"+std::to_string(temporary_++); out<<"  "<<result<<" = icmp "<<instruction<<" "<<left_type<<" "<<left<<", "<<right<<"\n"; return {"i1",result};
            }
            if(op=="+")instruction="add"; else if(op=="-")instruction="sub"; else if(op=="*")instruction="mul"; else if(op=="/")instruction="sdiv"; else return {};
            if(left.empty()||right.empty()||left_type!=right_type) return {};
            const auto result="%flow_arithmetic_"+std::to_string(temporary_++); out<<"  "<<result<<" = "<<instruction<<" "<<left_type<<" "<<left<<", "<<right<<"\n"; return {left_type,result};
        }
        return {};
    }
    bool emit_block(int block,std::ostringstream& out,const std::string& continuation) {
        out<<"flow_block_"<<block<<":\n"; bool terminated=false;
        for(const auto* op:blocks_[block]) {
            if(terminated) break;
            if(op->kind=="value_definition") {
                const auto kind=text(field(*op->operand,"kind"));
                if(kind=="writable_storage") out<<"  store ptr %flow_storage_ptr_"<<op->result_symbol<<", ptr "<<slot(op->result_symbol)<<"\n";
                else if(kind=="string_literal") out<<"  store ptr @flow_string_"<<op->result_symbol<<", ptr "<<slot(op->result_symbol)<<"\n";
                else { auto [type,value]=expression(*op->operand,out); if(value.empty()) throw std::runtime_error("unsupported structured value definition"); out<<"  store "<<type<<" "<<value<<", ptr "<<slot(op->result_symbol)<<"\n"; }
            } else if(op->kind=="external_call") {
                const auto& p=*op->provider; const auto params=carriers(p.parameters); const auto& operands=array(field(find_json_operation(op->id),"operands"),"operation.operands");
                if(params.size()!=operands.size()) throw std::runtime_error("structured call operand count mismatch");
                std::vector<std::pair<std::string,std::string>> args; for(std::size_t i=0;i<params.size();++i) args.push_back(expression(operands[i],out,params[i]));
                const auto result="%flow_call_"+std::to_string(op->id); out<<"  "<<result<<" = call "<<llvm_type(p.result)<<" @"<<p.symbol<<"(";
                for(std::size_t i=0;i<args.size();++i){if(i)out<<", ";out<<args[i].first<<" "<<args[i].second;} out<<")\n";
                if(op->result_symbol>=0) out<<"  store "<<llvm_type(p.result)<<" "<<result<<", ptr "<<slot(op->result_symbol)<<"\n";
            } else if(op->kind=="branch") {
                auto [type,condition]=expression(*op->operand,out); if(type!="i1"||condition.empty()) throw std::runtime_error("unsupported structured branch condition");
                const auto join="flow_join_"+std::to_string(label_++); const auto then_label="flow_block_"+std::to_string(op->then_block); const auto else_label=op->else_block>=0?"flow_block_"+std::to_string(op->else_block):join;
                out<<"  br i1 "<<condition<<", label %"<<then_label<<", label %"<<else_label<<"\n";
                emit_block(op->then_block,out,join); if(op->else_block>=0) emit_block(op->else_block,out,join); out<<join<<":\n";
            } else if(op->kind=="loop") {
                const auto condition_label="flow_loop_condition_"+std::to_string(label_++), exit_label="flow_loop_exit_"+std::to_string(label_++);
                out<<"  br label %"<<condition_label<<"\n"<<condition_label<<":\n";
                auto [type,condition]=expression(*op->operand,out); if(type!="i1"||condition.empty()) throw std::runtime_error("unsupported structured loop condition");
                out<<"  br i1 "<<condition<<", label %flow_block_"<<op->body_block<<", label %"<<exit_label<<"\n";
                emit_block(op->body_block,out,condition_label); out<<exit_label<<":\n";
            } else if(op->kind=="assignment") {
                auto [type,value]=expression(*op->operand,out); const auto target=symbol_types_.find(op->result_symbol);
                if(target==symbol_types_.end()||type!=llvm_type(target->second)||value.empty()) throw std::runtime_error("unsupported structured assignment");
                out<<"  store "<<type<<" "<<value<<", ptr "<<slot(op->result_symbol)<<"\n";
            } else if(op->kind=="return_value") {
                auto [type,value]=expression(*op->operand,out,"c_int"); if(type!="i32"||value.empty()) throw std::runtime_error("unsupported structured return"); out<<"  ret i32 "<<value<<"\n"; terminated=true;
            }
        }
        if(!terminated) out<<"  br label %"<<continuation<<"\n";
        return terminated;
    }
    const Json& find_json_operation(int id) const {
        const auto& ops=array(field(*field(root_,"lowering_plan"),"operations"),"operations");
        for(const auto& item:ops) if(integer(field(item,"id"),"id")==id) return item;
        throw std::runtime_error("operation identity lost");
    }
};

inline std::optional<std::string> emit(std::string_view report,std::string_view binding) {
    const auto root=Parser{std::string(report)}.parse(); const auto auth=Parser{std::string(binding)}.parse(); Emitter emitter(root,auth);
    if(!emitter.applicable()) {
        if (emitter.requires_structured_control()) throw std::runtime_error("structured plan lost its controlling branch operation");
        return std::nullopt;
    }
    return emitter.emit();
}

} // namespace flowlower::structured
