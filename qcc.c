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
//do reallocation of output buffer
#define prs_do_realloc		0x01
//tell the parser to increase scope after the next iteration
//because a new curly bracket is needed
#define prs_new_scope		0x02
//tell the parser that a new argument or indentifier is
//needed and should do those transformation operations.
#define prs_new_args		0x04
#define prs_new_id			0x08
//tell the parser that the current state has occurred after
//whitespace
#define prs_ws_sep_sym		0x10
//tell the parser that the current state is directly after a
//constant or an identifier or an operation (like addition
//or declaration)
#define prs_was_id			0x20
#define prs_was_const		0x40
#define prs_was_op			0x80
//passthrough to C mode
#define prs_c_passth		0x100
//asm conversion
#define prc_c_asm			0x200
static U2 parse_flags=0;
//for delaying flags by one iteration
static U2 parse_next_flags=0;

	//I might make an incremental flag array, separate from
	//the regular flag array, which copies each index up one
	//for a sort of rolling source state control.

int scope=0;
//}}}

//for testing
char * qc_code=		"{#dmain[] 0 ret}";
char * wanted_code=	"int main(){return 0;}";


char* getType(char * start,int* exit_length){//{{{

	//Start is pointer at offset that you want to find. The
	//type of exit_length is pointer to length of created
	//string. Returns pointer to new allocation with string
	//of wanted typing symbol

	char * out;
	char * temp;
	switch(start[0]){
		case 'd': 
				temp="int ";
				parse_flags|=prs_new_id;
				break;
	}
	*exit_length=strlen(temp);
	out=		malloc(*exit_length);
	bcopy(temp,out,*exit_length);
	parse_flags|=prs_do_realloc;
	return temp;//out;
}//}}}

static inline char* getId(char *start, int* exit_length){//{{{
	//gets an identifier which ends in whitespace or any
	//character that isn't aphabetical.
	
	int i=0; 
	while(	(start[i]&0x5f-1)<0x59 ||	//while alphabetical, increment
			(start[i]&0x7f-1)<0x79	)	i++;

	//remind the parser to do a 
	parse_flags|=prs_do_realloc|prs_was_id;
	return start;
}//}}}


static inline int toSymbol(char *start){//{{{
	//Returns byte offset to when the next symbol is,
	//ignoring spaces, tabs and newlines.
	int i=0;char c=start[i];
	while (c==' '||c=='\t'||c=='\n')
			i++,c=start[i];
	return i;
}//}}}


int main(){
	char * result_code;
	result_code=malloc(1);
	int result_size=	0;
	for (int i=0;i<strlen(qc_code);i++){
		char * symbol;int size=0;	

		//For receiving symbol data for this iteration to
		//add to the result_code string.

			char temp=qc_code[i];
			if (temp==' '||temp=='\t')
				i+=toSymbol(qc_code+i),
				parse_flags|=prs_ws_sep_sym;
		}

		if (parse_flags & prs_new_id){
			//if a previous declaration tells you to 

			//removing any whitespace between the
			//current location and the new symbol
			
		}else{//normal
			switch(qc_code[i]){ 
				case '#':
							symbol=getType(qc_code+i+1,&size);
							break; 
				case '{': 
							scope++;
					 		parse_flags|=prs_new_scope;
							break;
				case '}': 
							scope--;
							break;

				//for handling whitespace
				case ' ':
				case '\t':
				case '\n':	break;
			}
		}
		if (parse_flags&prs_do_realloc){
			result_code=	realloc(result_code,result_size+size);
			bcopy(symbol,&result_code[result_size],size);
			result_size+=	size;
			free(symbol);

			//reset realloc flag
			parse_flags^=prs_do_realloc;
		}

	}
	printf ("Wanted:\t%s\nResult:\t%s\n",wanted_code,result_code);
	return 0;
}
