NAME = "mario"

MAC_OPT = "-I/opt/X11/include"

all:
	@echo "Compiling..."
	g++ -o $(NAME) $(NAME).cpp -L/usr/X11R6/lib -lX11 -lstdc++ 
	@echo "Running..."
	./$(NAME) 