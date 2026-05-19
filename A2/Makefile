#/*
 #* Student Name: Kevin Usuanlele / Scott Weaver
 #* Student ID:   3103649 / 3144661
 #* Date:         May 18, 2026
 #* File:         Makefile
 #* Description:  Makefile for A2 - User Input Validation
 #*/

# Variables
CC := gcc-13          # C Compiler
CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2 -g # Compiler flags
LDFLAGS :=

# Valgrind
VALGRIND ?= valgrind
VGFLAGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=all --error-exitcode=1

.PHONY: all clean checkfcfs checkrr valgrind vg

all: sched 

sched: sched.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sched.o: sched.c sched.h
	$(CC) $(CFLAGS) -c $<

checkfcfs: sched
	./sched --policy=FCFS --in=W1.txt
	./sched --policy=FCFS --in=W2.txt
	./sched --policy=FCFS --in=W3.txt

checkrr: sched
	./sched --policy=RR --quantum=2 --in=W1.txt
	./sched --policy=RR --quantum=2 --in=W2.txt
	./sched --policy=RR --quantum=2 --in=W3.txt

valgrind: valgrindfcfs valgrindsrr

valgrindfcfs: sched
	$(VALGRIND) $(VGFLAGS) ./sched --policy=FCFS --in=W1.txt
	$(VALGRIND) $(VGFLAGS) ./sched --policy=FCFS --in=W2.txt
	$(VALGRIND) $(VGFLAGS) ./sched --policy=FCFS --in=W3.txt
valgrindsrr: sched
	$(VALGRIND) $(VGFLAGS) ./sched --policy=RR --quantum=2 --in=W1.txt
	$(VALGRIND) $(VGFLAGS) ./sched --policy=RR --quantum=2 --in=W2.txt
	$(VALGRIND) $(VGFLAGS) ./sched --policy=RR --quantum=2 --in=W3.txt

vg: valgrind

clean:
	rm -f sched sched.o
