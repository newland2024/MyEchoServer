TARGET = mutex

# c编译选项
CFLAGS = -g -O2 -Wall -Werror -pipe -m64
# c++编译选项
CXXFLAGS = -g -O2 -Wall -Werror -pipe -m64 -std=c++11
# 连接选项
LDFLAGS =
# 头文件目录
INCFLAGS = -I../../
# 源文件目录
SRCDIRS = . ../../
# 单独的源文件
ALONE_SOURCES =
# c编译器
CC = gcc
# c++编译器
CXX = g++
# 源文件类型扩展：c为c源文件，cpp为c++源文件
SRCEXTS = .c .cpp

# foreach函数用于遍历源文件目录，针对每个目录再调用addprefix函数，生成各种指定源文件后缀类型的通用匹配模式（类似正则表达式）
# 使用wildcard函数对每个目录下文件，进行通配符扩展，最后得到所有的TARGET依赖的源文件列表，保存到SOURCES中
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
SOURCES += $(ALONE_SOURCES)

# 过滤掉c语言相关的源文件，这个后续用于判断时采用c编译还是c++编译
SRC_CXX = $(filter-out %.c,$(SOURCES))

# 目标文件列表，先调用basename函数取源文件的前缀，然后再调用addprefix函数添加.o的后缀
OBJS = $(addsuffix .o, $(basename $(SOURCES)))

# 定义编译和链接使用的变量
COMPILE.c   = $(CC)  $(CFLAGS)   $(INCFLAGS) -c
COMPILE.cpp = $(CXX) $(CXXFLAGS) $(INCFLAGS) -c
LINK.c      = $(CC)  $(CFLAGS)
LINK.cpp    = $(CXX) $(CXXFLAGS)

.PHONY: all objs clean help debug

# all生成的依赖规则，就是用于生成TARGET
all: $(TARGET)

# objs生成的依赖规则，就是用于生成各个链接使用的目标文件
objs: $(OBJS)

# 下面的是生成目标文件的通用规则
%.o:%.c
	$(COMPILE.c) $< -o $@

%.o:%.cpp
	$(COMPILE.cpp) $< -o $@

# 最终目标文件的依赖规则
$(TARGET): $(OBJS)
ifeq ($(SRC_CXX),)              # c程序
	$(LINK.c)   $(OBJS) -o $@ $(LDFLAGS)
	@echo Type $@ to execute the program.
else                            # c++程序
	$(LINK.cpp) $(OBJS) -o $@ $(LDFLAGS)
	@echo Type $@ to execute the program.
endif

clean:
	rm $(OBJS) $(TARGET)

help:
	@echo '通用makefile用于编译c/c++程序 版本号1.1'
	@echo
	@echo 'Usage: make [TARGET]'
	@echo 'TARGETS:'
	@echo '  all       (等于直接执行make) 编译并连接'
	@echo '  objs      只编译不连接'
	@echo '  clean     清除目标文件和可执行文件'
	@echo '  debug     显示变量，用于调试'
	@echo '  help      显示帮助信息'
	@echo

debug:
	@echo 'TARGET       :' 	$(TARGET)
	@echo 'SRCDIRS      :'	$(SRCDIRS)
	@echo 'SOURCES      :'	$(SOURCES)
	@echo 'SRC_CXX      :'	$(SRC_CXX)
	@echo 'OBJS         :' 	$(OBJS)
	@echo 'COMPILE.c    :' 	$(COMPILE.c)
	@echo 'COMPILE.cpp  :' 	$(COMPILE.cpp)
	@echo 'LINK.c       :' 	$(LINK.c)
	@echo 'LINK.cpp     :' 	$(LINK.cpp)