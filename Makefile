CC := clang
CFLAGS := -g -Wall -Werror -Wno-unused-function -Wno-unused-variable

all: main

clean:
	rm -f main

main: main.c 
	$(CC) $(CFLAGS) -o main main.c -lncurses

zip:
	@echo "Generating main.zip file to submit to Gradescope..."
	@zip -q -r main.zip . -x .git/\* .vscode/\* .clang-format .gitignore main
	@echo "Done. Please upload main.zip to Gradescope."
