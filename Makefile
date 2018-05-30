#General Makefile
                             
target := upgrade 
   
object := $(patsubst %.c,%.o,$(wildcard *.c))  
PATH_PREFIX = ../../toolchain/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf

CROSS_COMPILE ?= $(PATH_PREFIX)/bin/arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)g++ 
EXTRA_FLAGS += -g -Wall -std=c++11


#CFLAGS += -I$(PATH_PREFIX)/include #-I$(PATH_PREFIX)/arm-linux-gnueabihf/include  
#CFLAGS += -I/home/jason/work/source/wifi_nvr/platform/archive/104.10.03/src/srcs/kernel/include  
LDFLAGS += -L$(PATH_PREFIX)/lib  
LDFLAGS += 
							    
%.o : %c
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)  
		    
all : $(target)  
$(target): $(object)  
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)  
clean:  
	rm -f $(target) $(object)  
