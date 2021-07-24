//#!/bin/tcc -run
//Using tcc just for initial development loop
//Quick C Compiler - A a psuedo-lisp to C transpiler.
//buildflags {{{

#define EMIT_SPACES false
#define EMIT_SPACES_INDENT_SIZE 4
//}}}

//includes {{{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <string.h>
//}}}

//custom type names {{{
#define kib(a) (a*1024)
#define U2 unsigned short 
#define U4 unsigned int 
#define U1 unsigned char 
#define U8 unsigned long long 
//}}}

//parsing flags {{{
//
//tell the parser to increase scope after the next iteration
//because a new curly bracket is needed
#define prs_new_scope		0x01
//tell the parser that a new argument or indentifier is
//needed and should do those transformation operations.
#define prs_new_args		0x02
#define prs_new_id			0x04
//tell the parser that the current state has occurred after
//whitespace
#define prs_ws_sep_sym		0x08
//tell the parser that the current state is directly after a
//constant or an identifier or an operation (like addition
//or declaration)
#define prs_was_id			0x10
#define prs_was_const		0x20
#define prs_was_op			0x40
//need semi-colon to end the statement
#define prs_need_semi		0x80
//passthrough to C mode
#define prs_c_passth		0x100
//asm conversion
#define prc_c_asm			0x200

#define pflgsz U2
static pflgsz parse_flags=0;
//for delaying flags by one iteration
static pflgsz parse_next_flags=0;

	//I might make an incremental flag array, separate from
	//the regular flag array, which copies each index up one
	//for a sort of rolling source state control.

int scope=0;
//}}}


//for testing
char * qc_code=		"{#dmain[] 0 ret}";
char * wanted_code=	"int main(){return 0;}";

//needed for full functioning
char * result_code;
int result_size=	0;


int pushToOutput(char* symbol,int size){//{{{
	//increases the size of the output allocation while
	//copying the same amount of bytes to the allocation
	//basically just a copy but with the expansion of the
	//allocation.
	
	result_code=	realloc(result_code,result_size+size);
	bcopy(symbol,&result_code[result_size],size);
	result_size+=	size;
	return 0;
}//}}}


int addType(char * start){//{{{

	//Start is pointer at offset of the new type
	//declaration. Directly adds the type, with a following
	//space, to the output allocation.

	char * temp;int i=0;
	switch(start[0]){
		case 'd': 
				temp="int "; i++;
				break;
		default: break;
	}
	parse_flags|=prs_new_id;
	//parse_flags|=prs_was_op;
	pushToOutput(temp,strlen(temp));
	return i;//out;
}//}}}

static inline int addId(char *start){//{{{
	//pushes an identifier to the output buffer which ends
	//in whitespace or any other character that isn't
	//aphabetical.
	
	int i=0; 

	//while alphabetical, increment
	while(	((start[i]&0x5f)<=0x5a	&&	
			(start[i]&0x5f)>0x40) 	||
			((start[i]&0x7f)<=0x7a	&&	
			(start[i]&0x7f)>0x60))	i++;

	//remind the parser that it is no longer in "ID" state
	//but was, just.
	parse_flags|=prs_was_id;
	parse_flags^=prs_new_id;
	pushToOutput(start,i);
	return i;
}//}}}


static inline int toSymbol(char *start){//{{{
	//Returns byte offset to when the next symbol is,
	//ignoring spaces, tabs and newlines.
	int i=0;char c=start[i];
	while (c==' '||c=='\t'||c=='\n')
			i++,c=start[i];
	return i;//returns the amount to increase the parser increment counter.
}//}}}


int main(){
	result_code=malloc(1);char * symbol=malloc(1);
	for (int i=0;i<strlen(qc_code);i++){
start:
		int size=0;	

		//For receiving symbol data for this iteration to
		//add to the result_code string.
		char temp=qc_code[i];

		//for dealing with whitespace
		if (temp==' '||temp=='\t')
			i+=toSymbol(qc_code+i),
			parse_flags|=prs_ws_sep_sym;//

		if (parse_flags & prs_new_id){
			//If a previous declaration says to expect a new
			//identifier in the qc source
			i+=addId(&qc_code[i]);
		}else if(
				(parse_flags & prs_was_id)		&&
				(parse_flags & prs_new_scope))	{

		}else{//normal
			switch(qc_code[i]){ 
				case '#':
							i+=addType(qc_code+i+1);
							break; 
				case '{': 
							scope++;
					 		parse_flags|=prs_new_scope;
							break;
				case '}':
							char *brkt="}";
							scope--;
							break;

			}
		}
	}
	printf ("Wanted:\t%s\nResult:\t%s\n",wanted_code,result_code);
	return 0;
}
