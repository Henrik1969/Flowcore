#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace langlab {

struct SourcePos { std::size_t line{1}; std::size_t column{1}; };

struct Rule {
    std::string name;
    std::string pattern_text;
    std::regex pattern;
    std::string action;
};

struct Options {
    std::string rules_path{"config/flowcore.rules"};
    std::string source_label{"<stdin>"};
    std::string origin{"stdin"};
    std::size_t source_id{1};
};

class LexError final : public std::runtime_error {
public:
    LexError(SourcePos pos, const std::string& message)
        : std::runtime_error(message), pos_(pos) {}
    [[nodiscard]] const SourcePos& pos() const noexcept { return pos_; }
private:
    SourcePos pos_;
};

[[nodiscard]] std::string trim(std::string text) {
    auto ws=[](unsigned char c){ return std::isspace(c)!=0; };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [&](unsigned char c){return !ws(c);}));
    text.erase(std::find_if(text.rbegin(), text.rend(), [&](unsigned char c){return !ws(c);}).base(), text.end());
    return text;
}

[[nodiscard]] std::vector<std::string> split_pipe(const std::string& line) {
    // The delimiter is deliberately " | " rather than bare '|'.
    // Regular expressions frequently contain literal or alternation pipes.
    std::vector<std::string> out;
    std::size_t begin = 0;

    while (true) {
        const std::size_t separator = line.find(" | ", begin);
        if (separator == std::string::npos) {
            out.push_back(trim(line.substr(begin)));
            break;
        }

        out.push_back(trim(line.substr(begin, separator - begin)));
        begin = separator + 3;
    }

    return out;
}

[[nodiscard]] std::string json_escape(std::string_view text) {
    std::string out; out.reserve(text.size()+8);
    static constexpr char hex[]="0123456789abcdef";
    for(unsigned char c: text) {
        switch(c) {
            case '"': out+="\\\""; break;
            case '\\': out+="\\\\"; break;
            case '\n': out+="\\n"; break;
            case '\r': out+="\\r"; break;
            case '\t': out+="\\t"; break;
            default:
                if(c<0x20) { out+="\\u00"; out+=hex[(c>>4)&0xf]; out+=hex[c&0xf]; }
                else out += static_cast<char>(c);
        }
    }
    return out;
}

[[nodiscard]] std::string replace_all(std::string text, const std::string& needle, const std::string& repl) {
    std::size_t pos=0;
    while((pos=text.find(needle,pos))!=std::string::npos) {
        text.replace(pos, needle.size(), repl); pos += repl.size();
    }
    return text;
}

class Lexer final {
public:
    Lexer(std::string source, std::vector<Rule> rules, Options options)
        : source_(std::move(source)), rules_(std::move(rules)), options_(std::move(options)) {}

    void run() {
        write_stream_begin(); write_source_begin();
        while(!eof()) {
            auto matched = match_rule();
            if(!matched) throw LexError(pos_, unexpected_message());
            process_match(matched->first, matched->second);
        }
        write_token("EOF","",pos_);
        write_source_end(); write_stream_end();
    }

private:
    [[nodiscard]] bool eof() const noexcept { return index_>=source_.size(); }

    [[nodiscard]] std::string unexpected_message() const {
        if(eof()) return "unexpected end of input";
        std::ostringstream out;
        out<<"no configured lexer rule matched byte 0x"<<std::hex<<static_cast<unsigned int>(static_cast<unsigned char>(source_[index_]));
        return out.str();
    }

    [[nodiscard]] std::optional<std::pair<const Rule*, std::string>> match_rule() const {
        std::string_view remaining{source_.data()+index_, source_.size()-index_};
        const Rule* best=nullptr; std::string lexeme_best;
        for(const Rule& rule: rules_) {
            std::cmatch m;
            const char* begin=remaining.data(); const char* end=remaining.data()+remaining.size();
            if(!std::regex_search(begin,end,m,rule.pattern,std::regex_constants::match_continuous)) continue;
            std::string lexeme=m.str(0);
            if(lexeme.empty()) throw std::runtime_error("rule '"+rule.name+"' matched empty string");
            if(best==nullptr || lexeme.size()>lexeme_best.size()) { best=&rule; lexeme_best=std::move(lexeme); }
        }
        if(!best) return std::nullopt;
        return std::make_pair(best,lexeme_best);
    }

    void process_match(const Rule* rule,const std::string& lexeme) {
        SourcePos start=pos_;
        if(rule->action=="skip") { consume(lexeme); return; }
        if(rule->action=="newline") { consume(lexeme); write_token("NEWLINE","\\n",start); return; }
        if(rule->action=="identifier") { consume(lexeme); write_token(keywords().find(lexeme) != keywords().end() ? "KEYWORD" : "IDENTIFIER",lexeme,start); return; }
        if(rule->action=="annotation") { consume(lexeme); process_annotation(lexeme,start); return; }
        if(rule->action=="emit") { consume(lexeme); write_token(rule->name,lexeme,start); return; }
        throw std::runtime_error("unknown rule action '"+rule->action+"'");
    }

    void consume(const std::string& lexeme) {
        for(std::size_t i=0;i<lexeme.size();++i) {
            char c=lexeme[i];
            if(c=='\r' && i+1<lexeme.size() && lexeme[i+1]=='\n') { ++i; ++pos_.line; pos_.column=1; continue; }
            if(c=='\n') { ++pos_.line; pos_.column=1; continue; }
            ++pos_.column;
        }
        index_ += lexeme.size();
    }

    void process_annotation(const std::string& lexeme,const SourcePos& start) {
        static const std::regex directive{R"(^//[ \t]*!!!emit[ \t]+(stream|stdout|stderr)[ \t]*(.*)$)"};
        std::smatch m;
        if(!std::regex_match(lexeme,m,directive)) throw LexError(start,"malformed !!!emit annotation");
        std::string channel=m.str(1); std::string message=expand_macros(m.str(2),start);
        if(channel=="stderr") {
            std::cerr<<"notice["<<options_.source_label<<':'<<start.line<<':'<<start.column<<"]: "<<message<<'\n';
            return;
        }
        std::cout<<"{\"type\":\"GHOST_ANNOTATION\",\"ghost\":true,\"channel\":\"stream\",\"message\":\""
                 <<json_escape(message)<<"\",\"source_id\":"<<options_.source_id
                 <<",\"line\":"<<start.line<<",\"column\":"<<start.column<<"}\n";
    }

    [[nodiscard]] std::string expand_macros(std::string msg,const SourcePos& start) const {
        msg=replace_all(msg,"@file",options_.source_label);
        msg=replace_all(msg,"@sourceid",std::to_string(options_.source_id));
        msg=replace_all(msg,"@linenr",std::to_string(start.line));
        msg=replace_all(msg,"@line",std::to_string(start.line));
        msg=replace_all(msg,"@column",std::to_string(start.column));
        return msg;
    }

    void write_token(const std::string& type,const std::string& lexeme,const SourcePos& start) const {
        std::cout<<"{\"type\":\""<<json_escape(type)<<"\",\"lexeme\":\""<<json_escape(lexeme)
                 <<"\",\"line\":"<<start.line<<",\"column\":"<<start.column<<"}\n";
    }

    void write_stream_begin() const {
        std::cout<<"{\"type\":\"GHOST_STREAM_BEGIN\",\"ghost\":true,\"schema\":\"langlab.tokenstream.jsonl\",\"version\":\"0.1\"}\n";
    }
    void write_source_begin() const {
        std::cout<<"{\"type\":\"GHOST_SOURCE_BEGIN\",\"ghost\":true,\"source_id\":"<<options_.source_id
                 <<",\"file\":\""<<json_escape(options_.source_label)<<"\",\"origin\":\""<<json_escape(options_.origin)<<"\"}\n";
    }
    void write_source_end() const {
        std::cout<<"{\"type\":\"GHOST_SOURCE_END\",\"ghost\":true,\"source_id\":"<<options_.source_id
                 <<",\"file\":\""<<json_escape(options_.source_label)<<"\"}\n";
    }
    void write_stream_end() const { std::cout<<"{\"type\":\"GHOST_STREAM_END\",\"ghost\":true}\n"; }

    [[nodiscard]] static const std::unordered_set<std::string>& keywords() {
        static const std::unordered_set<std::string> k{
            "type","alias","newtype","interface","port_interface","is_type_of",
            "module","import","use","export","as","extern","foreign","with",
            "if","elseif","else","guard","switch","default","match",
            "loop","while","until","do","for","forall","in",
            "break","continue","return","true","false","none","void",
            "ref","mutable","move","try","catch","transaction","commit","rollback",
            "node","flow","property","state","input","output","data","event","ctrl","error",
            "stream","on","emit","analysis","require","forbid","warn"
        }; return k;
    }

    std::string source_;
    std::vector<Rule> rules_;
    Options options_;
    std::size_t index_{0};
    SourcePos pos_{};
};

[[nodiscard]] std::vector<Rule> load_rules(const std::string& path) {
    std::ifstream input{path};
    if(!input) throw std::runtime_error("unable to open rules file: "+path);
    std::vector<Rule> rules; std::string line; std::size_t n=0;
    while(std::getline(input,line)) {
        ++n; std::string cleaned=trim(line);
        if(cleaned.empty() || cleaned.rfind("//", 0) == 0) continue;
        auto fields=split_pipe(cleaned);
        if(fields.size()!=3) throw std::runtime_error("invalid rules entry at "+path+':'+std::to_string(n));
        rules.push_back(Rule{fields[0],fields[1],std::regex{fields[1],std::regex::ECMAScript},fields[2]});
    }
    if(rules.empty()) throw std::runtime_error("rules file contains no lexer rules");
    return rules;
}

[[nodiscard]] Options parse_options(int argc,char** argv) {
    Options o;
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        auto value=[&](){ if(i+1>=argc) throw std::runtime_error("missing value after "+arg); return std::string(argv[++i]); };
        if(arg=="--rules") o.rules_path=value();
        else if(arg=="--source") o.source_label=value();
        else if(arg=="--origin") o.origin=value();
        else if(arg=="--source-id") o.source_id=std::stoull(value());
        else if(arg=="--help") {
            std::cout<<"usage: tablelex [--rules file] [--source label] [--origin kind] [--source-id id] < source.flow\n";
            std::exit(EXIT_SUCCESS);
        } else throw std::runtime_error("unknown option: "+arg);
    }
    return o;
}

} // namespace langlab

int main(int argc,char** argv) {
    try {
        auto options=langlab::parse_options(argc,argv);
        auto rules=langlab::load_rules(options.rules_path);
        std::string source{std::istreambuf_iterator<char>{std::cin},std::istreambuf_iterator<char>{}};
        langlab::Lexer lexer{std::move(source),std::move(rules),std::move(options)};
        lexer.run();
        return EXIT_SUCCESS;
    } catch(const langlab::LexError& e) {
        std::cerr<<"lex error at "<<e.pos().line<<':'<<e.pos().column<<": "<<e.what()<<'\n';
        return EXIT_FAILURE;
    } catch(const std::exception& e) {
        std::cerr<<"fatal lexer error: "<<e.what()<<'\n';
        return EXIT_FAILURE;
    }
}
