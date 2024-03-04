# Source directory and build directory
SRCDIR = ./src
BUILDDIR = ./build
LOGDIR = logs

# UTILOBJ
UTIL_OBJ = $(BUILDDIR)/util.o


# Default target
all: $(BUILDDIR) util wd server km drone map main targets obstacles

# create build directory and logs
$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/$(LOGDIR)

# Run project
run:
	./$(BUILDDIR)/main $(ARGS)

clean:
	rm -f $(BUILDDIR)/main
	rm -f $(BUILDDIR)/server
	rm -f $(BUILDDIR)/map
	rm -f $(BUILDDIR)/key_manager
	rm -f $(BUILDDIR)/drone
	rm -f $(BUILDDIR)/watchdog
	rm -f $(BUILDDIR)/obstacles
	rm -f $(BUILDDIR)/targets
	rm -f $(BUILDDIR)/util
	rm -f $(BUILDDIR)/util.o
	rm -f $(BUILDDIR)/$(LOGDIR)/*

	
drone:
	gcc -I include -o $(BUILDDIR)/drone $(UTIL_OBJ) $(SRCDIR)/drone.c -pthread -lm
main:
	gcc -I include -o $(BUILDDIR)/main $(UTIL_OBJ) $(SRCDIR)/main.c
server:
	gcc -I include -o $(BUILDDIR)/server $(UTIL_OBJ) $(SRCDIR)/server.c -pthread
map:
	gcc -I include -o $(BUILDDIR)/map $(UTIL_OBJ) $(SRCDIR)/map.c -lncurses -pthread
km:
	gcc -I include -o $(BUILDDIR)/key_manager $(UTIL_OBJ) $(SRCDIR)/key_manager.c -pthread
targets:
	gcc -I include -o $(BUILDDIR)/targets $(UTIL_OBJ) $(SRCDIR)/targets.c
obstacles:
	gcc -I include -o $(BUILDDIR)/obstacles $(UTIL_OBJ) $(SRCDIR)/obstacles.c
wd:
	gcc -I include -o $(BUILDDIR)/watchdog $(UTIL_OBJ) $(SRCDIR)/watchdog.c
util:
	gcc -I include -o $(BUILDDIR)/util.o  -c $(SRCDIR)/util.c