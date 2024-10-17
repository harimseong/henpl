NAME    =  fsam


CXX     :=  c++
CXXFLAGS:=  -Wall -Wextra -Werror -std=c++17 
LDFLAGS :=   $(shell root-config --cflags --libs) \
	      -I/opt/local/include -L/opt/local/lib -lfmt
DEBUGFLAGS  :=  -g -fsanitize=address
RM      :=  rm -f


SRC      :=  \
	      fsam.cpp \
	      HistManager.cpp \
	      EventHist.cpp \
	      getFsam.cpp

TEMPLATE_SRC:=

OPEN_LIST  :=  $(SRC)\
	      $(TEMPLATE_SRC)

OBJ      :=  $(SRC:%.cpp=%.o)
DEP      :=  $(OBJ:%.o=%.d)

STATE    :=  $(shell ls DEBUG.mode 2> /dev/null)
ifeq ($(STATE), DEBUG.mode)
CXXFLAGS  +=  $(DEBUGFLAGS)
COMPILE_MODE:=  DEBUG.mode
else
COMPILE_MODE:=  RELEASE.mode
endif


.PHONY: all debug clean fclean re open

all: $(COMPILE_MODE)
	$(MAKE) $(NAME)

release: RELEASE.mode
	$(MAKE) all

debug: DEBUG.mode
	$(MAKE) all

RELEASE.mode:
	$(MAKE) fclean
	touch RELEASE.mode

DEBUG.mode:
	$(MAKE) fclean
	touch DEBUG.mode

clean:
	$(RM) $(OBJ)
	$(RM) RELEASE.mode DEBUG.mode

fclean: clean
	$(RM) $(NAME)

re: fclean
	$(MAKE) all

open:
	@nvim -c ':tabdo let $$a=expand("%") | let $$b=substitute($$a, "cpp", "hpp", "g") | let $$c=trim($$b) | vsplit $$c' -p $(OPEN_LIST)

$(NAME): $(OBJ)
	$(CXX) $^ $(CXXFLAGS) -o $@ $(LDFLAGS)

$(OBJ): %.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@ $(LDFLAGS)

ratio: edepRatio.cpp
	$(CXX) $< $(CXXFLAGS) -o $@ $(LDFLAGS)

-include $(DEP)
