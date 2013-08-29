#!/bin/awk -f
function fatal(string)
{
	printf string > "/dev/stderr"
	exit EXIT_FAILURE
}
function randint(n) {
	return int(n * rand())
}
function GetRandomWord()
{
	"awk 'NR == " randint(MaxWords) "' /usr/share/dict/words" | getline  # TODO: It is a mess!! Optimize it.
	return sprintf("%."MaxLength"s",$0);
}
function GetRandomType(		num)
{
	num = randint(100);
	return TypePercentage["reg"] >= num ? "reg" :
	       TypePercentage["dir"] >= num ? "dir" : "???";
}

function InitRandomTypes()
{
	if(TypePercentage["dir"]+TypePercentage["reg"] != 100) {
		fatal("Wrong type percentage, exiting.");
	}
	TypePercentage["dir"] += TypePercentage["reg"];
}
function GetMaxWords()
{
	"wc -l " DictFile	| getline
	MaxWords = $1
	if( nfiles > MaxWords)
		nfiles = MaxWords;
}
function PrintRandomList(path, num,		name,type,i)
{
	while(i<num) {
		name = GetRandomWord();
		type = GetRandomType();
		
		printf "%s/%s\n",path, name
		
		switch(type) {
			case "dir":
				n = randint(num-i);
				i += n;			
				PrintRandomList(path "/" name, n);
				break;
			case "reg":
		}
		i++;
	}

	
}
function praseCmd(	i)
{
	while(ARGV[++i]) {

		switch(ARGV[i]) {
		case "nfiles":
			nfiles = ARGV[++i]
			break;
		case "MaxLength":
			MaxLength = ARGV[++i]
			break;
		case "DictFile":
			DictFile = ARGV[++i]
			break;
		case "tp.dir":
			TypePercentage["dir"]=ARGV[++i]
			break;
		case "tp.reg":
			TypePercentage["reg"]=ARGV[++i]
			break;
		
		}
	}
}
BEGIN {
	nfiles = 40;
	MaxLength = 14;
	TypePercentage["dir"] = 50;
	TypePercentage["reg"] = 50;
	DictFile="/usr/share/dict/words"
	
	praseCmd();

	srand(systime());
	
	InitRandomTypes();
	GetMaxWords();
	
	PrintRandomList("", nfiles);
}
