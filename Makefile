CC = g++
ifeq ($(shell sw_vers 2>/dev/null | grep Mac | awk '{ print $$2}'),Mac)
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -I./include/ -I/usr/X11/include -DOSX
	LDFLAGS = -framework GLUT -framework OpenGL \
    	-L"/System/Library/Frameworks/OpenGL.framework/Libraries" \
    	-lGL -lGLU -lm -lstdc++
else
	CFLAGS = -g -DGL_GLEXT_PROTOTYPES -Ias3/glut-3.7.6-bin
	LDFLAGS = -lglut -lGLU 
endif
	
RM = /bin/rm -f 
all: main 
main: as3/Main.o 
	$(CC) $(CFLAGS) -o assignment3 as3/Main.o $(LDFLAGS) 
as3/Main.o: as3/Main.cpp 
	$(CC) $(CFLAGS) -c as3/Main.cpp -o as3/Main.o 
clean: 
	$(RM) *.o as3/*.o as0
 

