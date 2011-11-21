make_dir = $(CURDIR)
src_dir = $(CURDIR)/src
gen_dir = $(CURDIR)/generated
build_dir = $(CURDIR)/build
flex_opts = -o$(gen_dir)/lex.yy.cc -+
comp_opts = -o $(build_dir)/micro
debug_opts = -o $(build_dir)/micro -g

default: compiler

all : group compiler

group : 
	@echo "lrprice"

compiler : parser lexer
	@mkdir -p $(build_dir)
	@g++ $(comp_opts) $(src_dir)/compiler_main.cpp $(src_dir)/driver.cpp $(gen_dir)/parser.tab.cc $(gen_dir)/lex.yy.cc

parser : $(src_dir)/parser.yy
	@mkdir -p $(gen_dir)
	@bison $(src_dir)/parser.yy
	@mv location.hh stack.hh position.hh parser.tab.hh parser.tab.cc $(gen_dir)

lexer : $(src_dir)/scanner.ll
	@mkdir -p $(gen_dir)
	@flex $(flex_opts) $(src_dir)/scanner.ll

clean : 
	@rm -rf $(gen_dir) $(build_dir)
	
debug :
	@mkdir -p $(build_dir)
	@g++ $(debug_opts) $(src_dir)/compiler_main.cpp $(src_dir)/driver.cpp $(gen_dir)/parser.tab.cc $(gen_dir)/lex.yy.cc

