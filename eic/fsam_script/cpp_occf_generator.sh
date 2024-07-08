#!/bin/bash
echo "info) recommend to set ClassNames as UpperCamelCase."
echo "info) Either awk or gsed(gnu sed) is required."

if [ "$1" = "-f" ]
then
	FILENAME=$2
	cat $FILENAME 2> /dev/null 1> /dev/null
	if [ $? != "0" ]
	then
		echo "file doesn't exist."
		exit 1
	fi
	CLASSES=$(cat $FILENAME)
else
	echo "usage) bash  $0  -f  filename"
	exit 1
fi
SRC_LIST=""

AWK=0
which awk > /dev/null
if [ $? == "0" ]
then
AWK=1
fi
which gsed > /dev/null
if [ $? != "0" ]
then
SED=sed
else
SED=gsed
fi

for CLASS in $CLASSES
do
	SRC_LIST="\\
				$CLASS.cpp$SRC_LIST"
	ls $CLASS.cpp 2> /dev/null && ls $CLASS.hpp 2> /dev/null
	if [ $? == "0" ]
	then
		echo "$CLASS: duplicated file name found."
		continue
	else
		echo "$CLASS: generated"
	fi
	touch $CLASS.hpp
	touch $CLASS.cpp
if [ $AWK != "1" ]
then
	HEADER_GUARD="$(echo $CLASS | $SED 's/[A-Z]/&/g' | $SED 's/[a-z]/\U&/g' | $SED 's/$/_HPP/')"
	LOWCAMELCASE_CLASS=$(echo $CLASS | $SED 's/[A-Z]/\L&/')
else
	HEADER_GUARD="$(echo $CLASS | awk '{print toupper($0)}' | $SED 's/$/_HPP/')"
	LOWCAMELCASE_CLASS=$(echo $CLASS | awk '{print tolower($0)}')
fi

##### file contents
HEADER_CONTENT="#ifndef $HEADER_GUARD
#define $HEADER_GUARD

class	$CLASS
{
public:
// constructors & destructor
	$CLASS();
	~$CLASS();
	$CLASS(const $CLASS& $LOWCAMELCASE_CLASS);

// operators
	$CLASS\t&operator=(const $CLASS& $LOWCAMELCASE_CLASS);

// member functions
};

#endif // $HEADER_GUARD"
SOURCE_CONTENT="#include \"$CLASS.hpp\"

// constructors & destructor
$CLASS::$CLASS()
{
}

$CLASS::~$CLASS()
{
}

$CLASS::$CLASS(const $CLASS& $LOWCAMELCASE_CLASS)
{
	(void)$LOWCAMELCASE_CLASS;
}

// operators
$CLASS&
$CLASS::operator=(const $CLASS& $LOWCAMELCASE_CLASS)
{
	(void)$LOWCAMELCASE_CLASS;
	return *this;
}"
#####

	printf '%b\n' "$HEADER_CONTENT" > $CLASS.hpp
	printf '%b\n' "$SOURCE_CONTENT" > $CLASS.cpp

done

MAKEFILE_CONTENT="NAME		=	$FILENAME


CXX			:=	c++
CXXFLAGS	:=	-Wall -Wextra -Werror -std=c++98
DEBUGFLAGS	:=	-g -fsanitize=address
RM			:=	rm -f


SRC			:=	$SRC_LIST

TEMPLATE_SRC:=

OPEN_LIST	:=	\$(SRC)\\
				\$(TEMPLATE_SRC)

SRC			+=	main.cpp
OBJ			:=	\$(SRC:%.cpp=%.o)
DEP			:=	\$(OBJ:%.o=%.d)

STATE		:=	\$(shell ls DEBUG.mode 2> /dev/null)
ifeq (\$(STATE), DEBUG.mode)
CXXFLAGS	+=	\$(DEBUGFLAGS)
COMPILE_MODE:=	DEBUG.mode
else
COMPILE_MODE:=	RELEASE.mode
endif


.PHONY: all debug clean fclean re open

all: \$(COMPILE_MODE)
	\$(MAKE) \$(NAME)

release: RELEASE.mode
	\$(MAKE) all

debug: DEBUG.mode
	\$(MAKE) all

RELEASE.mode:
	\$(MAKE) fclean
	touch RELEASE.mode

DEBUG.mode:
	\$(MAKE) fclean
	touch DEBUG.mode

clean:
	\$(RM) \$(OBJ)
	\$(RM) RELEASE.mode DEBUG.mode

fclean: clean
	\$(RM) \$(NAME)

re: fclean
	\$(MAKE) all

open:
	@nvim -c ':tabdo let \$\$a=expand(\"%\") | let \$\$b=substitute(\$\$a, \"cpp\", \"hpp\", \"g\") | let \$\$c=trim(\$\$b) | vsplit \$\$c' -p \$(OPEN_LIST)

\$(NAME): \$(OBJ)
	\$(CXX) \$(CXXFLAGS) -o \$@ \$^

\$(OBJ): %.o: %.cpp
	\$(CXX) \$(CXXFLAGS) -c -o \$@ \$<

-include \$(DEP)"

VIMSPECTOR="{
  \"configurations\": {
  \"C - Launch\": {
    \"adapter\": \"CodeLLDB\",
    \"configuration\": {
      \"name\": \"Cpp: Launch current file\",
      \"type\": \"lldb\",
      \"request\": \"launch\",
      \"externalConsole\": false,
      \"logging\": {
        \"engineLogging\": true
      },
      \"stopOnEntry\": true,
      \"stopAtEntry\": true,
      \"debugOptions\": [],
      \"MIMode\": \"lldb\",
      \"cwd\" : \"${cwd}\",
      \"args\" : [],
      \"program\": \"$FILENAME\"
    }
  }
  }
}"

read -p "Do you want to delete $FILENAME? (y/n)" RESPONSE
if [ $RESPONSE ]
then
	if [ $RESPONSE = "y" ]
	then
		rm $FILENAME
	fi
fi

read -p "Do you want to generate Makefile? (y/n)" RESPONSE
if [ "$RESPONSE" = "y" ]
then
	MAKEFILE_EXIST=$(ls Makefile 2> /dev/null 1> /dev/null; echo $?)
	if [ $MAKEFILE_EXIST = "0" ]
	then
		read -p "There exists Makefile already. Do you really want to generate Makefile? (y/n)" RESPONSE
	fi
	if [ "$RESPONSE" = "y" ] || [ $MAKEFILE_EXIST != "0" ]
	then

		printf '%b\n' "$MAKEFILE_CONTENT" > Makefile_temp
		cat Makefile_temp | sed 's/ $//' > Makefile
		rm Makefile_temp

	fi
fi

read -p "Do you want to generate .vimspector.json? (y/n)" RESPONSE
if [ "$RESPONSE" = "" ] || [ $RESPONSE != "y" ]
then
	exit 0
fi

touch .vimspector.json
printf '%b\n' "$VIMSPECTOR" > .vimspector.json
