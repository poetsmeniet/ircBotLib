NAME = ircBot

CODE_FILES = main.c generic_tcp_client_template/tcp_client.c irclib.c read_config/config.c generic_unix_tools/generic_unix_tools.c ascii_hashtable/asciiHashMap.c

DOC_FILES = docs
DEBUG = YES
ifeq ($(DEBUG),YES)
	D = -g
else
	D =
endif

.PHONY: all clean docs

all: $(CODE_FILES)
	gcc -lm -Wno-parentheses -Wextra -fsanitize=undefined -Wall -g -o $(NAME) $(CODE_FILES)

#docs: Doxyfile
#	doxygen Doxyfile

#Doxyfile:
#	doxygen -g

clean:
	rm -rf $(NAME) $(DOC_FILES) *.o
