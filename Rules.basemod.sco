#Makefile base rule, not for microsoft vc
TEXTUS_DIR = $(HOME)/textus
CC=gcc
CXX=g++
LD=g++
CFLAGS  = -g -Wall -c -fPIC
INCLUDE_DIR = -I$(TEXTUS_DIR)/tinyxml -I$(TEXTUS_DIR)/amor/include
LDFLAGS = -shared -static 
LDLIBS =  -lc
CRYPTO_LIB = -lcrypto
MAKE = make

DEBUG_FLAG= -DNNDEBUG 

LIBSOCKET= -lsocket

PYTHON_HOME=/
