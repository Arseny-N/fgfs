#!/bin/awk -f
BEGIN {
	mode=ARGV[1];

	for(i=0; i < ARGC; i++) 
		ARGV[i] = ARGV[i+1]	

	FS="/"
	print "#!/bin/sh"
	print "DIR=$1"

}
NR == 1 && mode == "create" { prevNF=NF; prev=$0}
NR > 1  && mode == "create"{ 	
	if(prevNF < NF) {
		printf "install -d $DIR" prev 
		printf "ls $DIR" prev " > /dev/null\n"
		printf "[ $? -ne 0 ] && failed=$failed $DIR" prev "\n\n"
	} else {
		printf "touch $DIR"prev"\n"
		printf "ls $DIR" prev " > /dev/null\n"
		printf "[ $? -ne 0 ] && failed=$failed $DIR" prev "\n\n"

	}

	prevNF=NF; prev=$0
}
NR == 1 && mode == "remove" { prevNF=NF; prev=$0}
NR > 1  && mode == "remove"{ 	
	if(prevNF < NF) 
		print "rmdir $DIR" prev 
	else
		print "rm $DIR"prev
	
	prevNF=NF; prev=$0
}

mode == "test" {
	print "[ -e $DIR" $0 " ] || vanished=\"$vanished $DIR\""$0

}
END {
	if(mode == "create")
		print "if [ x\"$failed\" != x ]; then echo 'Files witch creation failed:'; for file in $failed; do echo -e '\\t'$file; done else echo 'All files were created succesfully.'; fi"
	if(mode == "test")
		print "if [ x\"$vanished\" != x ]; then echo 'Files vanished:'; for file in $vanished; do echo -e '\\t'$file; done else echo 'All files are on their places.'; fi" 
	
}
