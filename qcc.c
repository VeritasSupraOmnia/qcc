//#!/bin/tcc -run
//Using tcc just for initial development loop
//Quick C Compiler - A a psuedo-lisp to C transpiler.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
//#include <string.h>
#define kib(a) (a*1024)
#define U2 unsigned short 
#define U8 unsigned long long 

#define prs_do_out_realloc	0x01
#define prs_needs_new_scope	0x02
#define prs_needs_new_args	0x04

//for testing
char * qc_code=		"{#dmain[] 0 ret}";
char * wanted_code=	"int main(){return 0;}";

char* getType(char * start,int* exit_length){
	//start is pointer at offset that you want to find the type of
	//exit_length is pointer to length of created string
	//returns pointer to new allocation with string of wanted typing symbol
	char * out;
	char * temp;
	switch(start[0]){
		case 'd': 
				temp="int ";
				break;
	}
	*exit_length=strlen(temp);
	out=		malloc(*exit_length);
	bcopy(temp,out,*exit_length);
	return out;
}
	
int main(){
	int scope=0;
	U2 parse_flags=0;
	char * result_code;
	result_code=malloc(1);
	int result_size=	0;
	for (int i=0;i<3/*strlen(qc_code)*/;i++){
		char * symbol;int size=0;	
		//For receiving symbol data for this iteration to add to the
		//result_code string.           						
		switch(qc_code[i]){ 
			case '#': symbol=getType(qc_code+i+1,&size);
				  parse_flags|=prs_do_out_realloc;break; 
			case '{': scope++;
				  parse_flags|=prs_needs_new_scope;break;
			case '}': scope--;break;
		}
		if (parse_flags&prs_do_out_realloc){
			result_code=	realloc(result_code,result_size+size);
			bcopy(symbol,&result_code[result_size],size);
			result_size+=	size;
			free(symbol);
			parse_flags^=prs_do_out_realloc;//reset realloc flag
		}

	}
	printf ("Wanted:\t%s\nResult:\t%s\n",wanted_code,result_code);
	return 0;
}
