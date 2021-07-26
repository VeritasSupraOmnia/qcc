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

//Flags {{{
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
#define prs_was_arg			0x40
//need semi-colon to end the statement
#define prs_need_semi		0x100
//passthrough to C mode
#define prs_c_passth		0x200
//asm conversion
#define prs_asm				0x400
//tells the parser that an error has occurred.
#define prs_fail			0x800

#define pflgsz U2
static pflgsz parse_flags=0;
//for delaying flags by one iteration
static pflgsz parse_next_flags=0;

	//I might make an incremental flag array, separate from
	//the regular flag array, which copies each index up one
	//for a sort of rolling source state control.

int scope=0;
//}}}

//error flags{{{
#define err_no_type_for_varlist 0x01
#define err_type_does_not_exist 0x01

#define eflgsz U2
static eflgsz error_flags=0;
//}}}

//}}}

//for testing
static char * wanted_code=	"int main(int argc,char **argv){\n\treturn 0;\n}";


//Main Data section{{{

//Main data in buffer
//TODO: When the transpiler moves to files, make this start
//as null.
static char * qc_code=		"{#dmain[#dargc#ppbargv]\n\t{:return 0;:}\n}";
int qc_size;

//For adding previous IDs to tracking arrays.
char * plastId;
int slastId;

//For keeping track of function name strings.
char ** function_array;
int function_array_size;
int *function_array_string_sizes;

//For saving output code to.
char * result_code;
int result_size=	0;
//}}}

//data structures{{{

//struct enumeration{{{

typedef struct func_identifier{	//{{{
}fid;							//}}}

//}}}

//}}}

//Low order utilities {{{
int pushToOutput(char* symbol,int size){{{
	//increases the size of the output allocation while
	//copying the same amount of bytes to the allocation
	//basically just a copy but with the expansion of the
	//allocation.
	
	result_code=	realloc(result_code,result_size+size);
	bcopy(symbol,&result_code[result_size],size);
	result_size+=	size;
	return 0;
}}}

static inline int maybeId(char *start){{{
	//returns whether a current symbol might indicate an
	//identifier
	if(((	*start&0x5f	)<=	0x5a	&&	
	   (	*start&0x5f	)>	0x40) 	||
	   ((	*start&0x7f	)<=	0x7a	&&	
	   (	*start&0x7f	)>	0x60))	return 1;
	return 0;
}}}

static inline int isWhitespace(char *start){{{
	//just tells of the next character is whitespace
	if(*start==' '||*start=='\n'||*start=='\t')
		return 1;
	return 0;
}}}

static inline int toSymbol(char *start){{{
	//Returns byte offset to when the next symbol is,
	//ignoring spaces, tabs and newlines and adds the
	//whitespace to the C code.
	int i=0;char c=start[i];
	while (c==' '||c=='\t'||c=='\n')
		pushToOutput(&c,1),i++,c=start[i];
	return i;//returns the amount to increase the parser increment counter.
}}}


static inline int addType(char * start){{{

	//Start is pointer at offset of the new type
	//declaration. Directly adds the type, with a following
	//space, to the output allocation.
	
	//TODO: make a type lookup table 
	//(qcc type string array to c type string array)
	//This way I can expand the amount of types dynamically,
	//as I should do.
	U1 c;U1 pointers=0;int i=0;
	do{
		U1 * temp;//temp is removed after the do-while
		c=start[i];
		switch(start[i]){
			case 'b': temp="char ";					break;
			case 'w': temp="word ";					break;
			case 'd': temp="int ";					break;
			case 'q': temp="long long ";			break;
			case 'B': temp="unsigned char ";		break;
			case 'W': temp="unsigned word ";		break;
			case 'D': temp="unsigned int ";			break;
			case 'Q': temp="unsigned long long ";	break;
			case 'p': pointers++,i++;				continue;//goto do
			default: break;
		}
	i++;
	//parse_flags|=prs_was_op;
	pushToOutput(temp,strlen(temp));
	
	//keep going while there is no endcap
	}while (	c!='b'	&&	c!='B'	&&
				c!='w'	&&	c!='W'	&&
				c!='d'	&&	c!='D'	&&
				c!='q'	&&	c!='Q');

	//add pointers to 
	U1 *temp="*";
	for(U1 j=0;j<pointers;j++)
		pushToOutput(temp,1);
	return i;//out;
}}}

static inline int addId(char *start){{{
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
	pushToOutput(start,i);
	return i;
}}}

static inline int addJump(char *start){{{
	//determines if the next symbol is a jump symbol,
	//adding the translated value to the result and
	//returning the qc_code that was jumped over.

	//check the first character to see if any of the jump
	//symbols are possible
	switch (*start){
		case 'g':
		case 'c':
		case 'b':
		case 'r':
	}
}}}


int printSyntaxErrorLocation(char *start,int local_loc){{{
	//TODO: make this print the area around the current
	//location, if there is room, so that people can
	//understand where the syntax error is.
	
	//This OUGHT to be accompanied by a message describing
	//the error, why it happened, what might fix it, as well
	//as exactly where it is and in which file. This isn't
	//related to the todo but I thought I should include it
	//to remind myself.
}}}

static inline int addArgs(char * start){{{
	//add argument declarations
	int i=0;

	//start the argument list with the needful
	{char *temp="(";pushToOutput(temp,1);}

	//for doing comma based type repitition for multiple
	//variable identifiers as well as error checking.
	char* lastArgTypeLoc=(char*)0;
	char* lastIdLoc=(char*)0;

	U1 argstate=0;
	#define argstate_inlist		0x01
	#define argstate_done_ID	0x02

	//do argument parsing
	while (start[i]!=']'){

		//clear whitespace
		if (isWhitespace(start+i))
				i+=toSymbol(start+i);

		//if just finished with an ID 
		if (argstate&argstate_done_ID){
			char *temp=",";pushToOutput(temp,1);}

		//check for varlists
		if (start[i]==','){{{
			//push comma right away
			pushToOutput(start+i,1);
			i++;

			//clear whatever whitespace there is
			if (isWhitespace(start+i))
				i+=toSymbol(start+i);


			//Checking that a previous type and ID were
			//declared within this var list and the comma
			//isn't in the beginning of a new list.
			if(argstate&argstate_done_ID==0){
				//return the local location and set the
				//parse fail flag for error reporting later
				//on.
				parse_flags|=prs_fail,
				error_flags|=err_no_type_for_varlist;
				return i;}
			
			//add The last type to the output
			addType(lastArgTypeLoc);
				
			//add the next ID in the varlist
			i+=addId(start+i);

			continue;
		}}}

		//check for new type
		if(start[i]=='#'){{{
				
			i++;
			lastArgTypeLoc=start+i;
			i+=addType(lastArgTypeLoc);
			//clear argstate so that 
			argstate^=argstate;
			continue;
		}}}


		//do id parsing
		if (maybeId(start+i)){{{
			i+=addId(start+i);
			argstate|=argstate_done_ID;
			continue;
		}}}

		i++;
	}

	//end the argument
	{char * temp=")";pushToOutput(temp,1);}

	//reset the 
	

	return i;

	//I don't need these defines elsewhere in the program
	#undef argstate_inlist
	#undef argstate_done_ID
}}}

//}}}

//High order utilities {{{

static inline int addPassthrough(char *start){{{
	//tells the transpiler to pass this string directly to
	//the C output until it reaches the character pair :}.
	//This function is invoked when the pair {: is parsed,
	//so the whole of the passthrough is enclosed with:
	//  qc code  {: passthrough C code :} qc code
	//
	//Obviously the brackets and colon do not show up in the
	//resulting code

	int i=0;
	while (	//keep iterating while not at the end
			!(start[i]==':' && start[i+1]=='}')){
		i++;
	}

	//push unadulterated code direcly
	pushToOutput(start,i);

	//skip the passthrough escape sequence
	i+=2;
	return i;
}}}

int pushFuncName(char* symbol,int size){{{

}}}
//}}}

int main(){{{
	result_code=malloc(1);char * symbol=malloc(1);
	for (int i=0;i<strlen(qc_code);i++){
start:

		//If there's a new arglist and a new scope, that
		//means that this curly bracket is needed in the C
		//output.
		if(parse_flags&prs_was_arg&&parse_flags&prs_new_scope){
			char *c="{";pushToOutput(c,1);i++;parse_flags^=prs_was_arg;}

		//For receiving symbol data for this iteration to
		//add to the result_code string.
		char *temp=&qc_code[i];

		//for dealing with whitespace
		if (isWhitespace(temp))
			i+=toSymbol(temp),parse_flags|=prs_ws_sep_sym;

		if (parse_flags & prs_new_id && maybeId(temp)){
			//If a previous declaration says to expect a new
			//identifier in the qc source
			int size=addId(temp);
			i+=size;

			

			parse_flags|=prs_was_id;
			parse_flags^=prs_new_id;
			if (isWhitespace(temp))
				i+=toSymbol(temp),parse_flags|=prs_ws_sep_sym;
		}

		//check for special characters
		temp=&qc_code[i];
		switch(*temp){ 
			case '#':
						i+=addType(temp+1);
						parse_flags|=prs_new_id;
						continue;
			case '{': 	
						if(*(temp+1)==':'){
							i+=addPassthrough(temp+2)+1;
							continue;
						}
				 		parse_flags|=prs_new_scope;
						scope++;
						continue;
			case '}':
						scope--;char *brkt="}";
						pushToOutput(brkt,1);
						continue;
			case '[':
						i+=addArgs(temp+1);
						parse_flags|=prs_was_arg;
						continue;
			default:	
						
		}
	}
	printf ("From:\n%s\n\nWanted:\n%s\n\nResult:\n%s\n",
			qc_code,wanted_code,result_code);
	return 0;
}}}
