#!/bin/awk -f
BEGIN { 
	"stty -a" | getline
	c = (ARGV[1]) ? (ARGV[1]) : "="
	while(i++ < strtonum($7))
		printf c
	printf "\n"
}
