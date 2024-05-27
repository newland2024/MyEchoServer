#include "cmdline.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <map>

namespace CmdLine {
class Opt;  // 前置声明
static Usage usage_ = nullptr;
static std::map<std::string, Opt> opts_;

enum OptType {
  INT64_T = 1,
  BOOL = 2,
  STRING = 3,
};

class Opt {
 public:
  Opt() = default;
  Opt(bool* value, std::string name, bool default_value, bool required) {
    init(BOOL, value, name, required);
    *(bool*)value_ = default_value;
  }
  Opt(int64_t* value, std::string name, int64_t default_value, bool required) {
    init(INT64_T, value, name, required);
    *(int64_t*)value_ = default_value;
  }
  Opt(std::string* value, std::string name, std::string default_value, bool required) {
    init(STRING, value, name, required);
    *(std::string*)value_ = default_value;
  }
  bool IsBoolOpt() { return type_ == BOOL; }
  void SetBoolValue(bool value) {
    value_is_set_ = true;
    *(bool*)value_ = value;
  }
  void SetValue(std::string value) {
    if (type_ == STRING) *(std::string*)value_ = value;
    if (type_ == INT64_T) *(int64_t*)value_ = atoll(value.c_str());
    value_is_set_ = true;
  }
  bool CheckRequired() {
    if (not required_) return true;
    if (required_ && value_is_set_) return true;
    printf("required option %s not set argument\n", name_.c_str());
    return false;
  }

 private:
  void init(OptType type, void* value, std::string name, bool required) {
    type_ = type;
    name_ = name;
    value_ = (void*)value;
    required_ = required;
    if (required_) value_is_set_ = false;
  }

 private:
  OptType type_;  // 选项类型
  std::string name_;  // 选项名称
  void* value_;  // 选项值
  bool value_is_set_{true};  // 选项值是否被设置
  bool required_{false};  // 选项是否必设
};

static bool isValidName(std::string name) {
  if (name == "") return false;
  if (name[0] == '-') {
    printf("option %s begins with -\n", name.c_str());
    return false;
  }
  if (name.find("=") != name.npos) {
    printf("option %s contains =\n", name.c_str());
    return false;
  }
  return true;
}

static int ParseOpt(int argc, char* argv[], int& parse_index) {
  char* opt = argv[parse_index];
  int opt_len = strlen(opt);
  if (opt_len <= 1) {  // 选项的长度必须>=2
    printf("option's len must greater than or equal to 2\n");
    return -1;
  }
  if (opt[0] != '-') {  // 选项必须以'-'开头
    printf("option must begins with '-', %s is invalid option\n", opt);
    return -1;
  }
  opt++;  // 过滤第一个'-'
  opt_len--;
  if (*opt == '-') {
    opt++;  // 过滤第二个'-'
    opt_len--;
  }
  // 过滤完有效的'-'之后还要再check一下后面的内容和长度
  if (opt_len == 0 || *opt == '-' || *opt == '=') {
    printf("bad opt syntax:%s\n", argv[opt_len]);
    return -1;
  }
  // 执行到这里说明是一个选项，接下来判断这个选项是否已经指定了参数
  bool has_argument = false;
  std::string argument = "";
  for (int i = 1; i < opt_len; i++) {
    if (opt[i] == '=') {
      has_argument = true;
      argument = std::string(opt + i + 1);  // 取等号之后的内容赋值为argument
      opt[i] = 0;  // 这样opt指向的字符串就是'='之前的内容。
      break;
    }
  }
  std::string opt_name = std::string(opt);
  if (opt_name == "help" || opt_name == "h") {  // 有help选项，则直接调用_usage函数，并退出
    if (usage_) usage_();
    exit(0);
  }
  auto iter = opts_.find(opt_name);
  if (iter == opts_.end()) {  // 选项不存在
    printf("option provided but not defined: -%s\n", opt_name.c_str());
    return -1;
  }
  if (iter->second.IsBoolOpt()) {  // 不需要参数的bool类型选项
    iter->second.SetBoolValue(true);
    parse_index++;  // parseIndex跳到下一个选项
  } else {  // 需要参数的选项，参数可能在下一个命令行参数中
    if (has_argument) {
      parse_index++;
    } else {
      if (parse_index + 1 < argc) {  // 选项的值在下一个命令行参数
        has_argument = true;
        argument = std::string(argv[parse_index + 1]);
        parse_index += 2;  // parseIndex跳到下一个选项
      }
    }
    if (not has_argument) {
      printf("option needs an argument: -%s\n", opt_name.c_str());
      return -1;
    }
    iter->second.SetValue(argument);
  }
  return 0;
}

static bool checkRequired() {
  auto iter = opts_.begin();
  while (iter != opts_.end()) {
    if (!iter->second.CheckRequired()) return false;
    iter++;
  }
  return true;
}

static void setOptCheck(const std::string& name) {
  if (opts_.find(name) != opts_.end()) {
    printf("%s opt already set\n", name.c_str());
    exit(-1);
  }
  if (not isValidName(name)) {
    printf("%s is invalid name\n", name.c_str());
    exit(-2);
  }
}

void BoolOpt(bool* value, std::string name) {
  setOptCheck(name);
  opts_[name] = Opt(value, name, false, false);
}

void Int64Opt(int64_t* value, std::string name, int64_t default_value) {
  setOptCheck(name);
  opts_[name] = Opt(value, name, default_value, false);
}

void StrOpt(std::string* value, std::string name, std::string default_value) {
  setOptCheck(name);
  opts_[name] = Opt(value, name, default_value, false);
}

void Int64OptRequired(int64_t* value, std::string name) {
  setOptCheck(name);
  opts_[name] = Opt(value, name, 0, true);
}

void StrOptRequired(std::string* value, std::string name) {
  setOptCheck(name);
  opts_[name] = Opt(value, name, "", true);
}

void SetUsage(Usage usage) { usage_ = usage; }

void Parse(int argc, char* argv[]) {
  if (nullptr == usage_) {
    printf("usage function not set\n");
    exit(-1);
  }
  int parse_index = 1;  // 这里跳过命令名不解析，所以parseIndex从1开始
  while (parse_index < argc) {
    if (ParseOpt(argc, argv, parse_index)) {
      exit(-2);
    }
  }
  if (not checkRequired()) {  // 校验必设选项，非必设的则设置默认值
    usage_();
    exit(-1);
  }
}
}  // namespace CmdLine