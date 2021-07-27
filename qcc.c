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
//Flags that matter during the 

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

//mode flags{{{

//Flags determining runtime mode which might be parsed from
//the arguments or mode altering symbols within the text.
#define mflgsz U1
static mflgsz mode_flags=0;

//For telling the compiler to trim these things during
//qc parsingg so that they don't need to be compiled again.
//This is important what with C compilers so often being
//malignant piles of shit when it comes to compile speed.
#define mode_trim_comments			0x1
#define mode_trim_whitespace		0x2
//prints code to standard out
#define mode_print_code				0x4
//non-default file output flag
#define mode_defined_output			0x8
//compiles to 
#define mode_do_compile				0x10

//}}}

//error flags{{{
#define err_no_type_for_varlist 0x01
#define err_type_does_not_exist 0x01

#define eflgsz U2
static eflgsz error_flags=0;
//}}}

//}}}

//Main Data section{{{

//Main data in buffer
//TODO: When the transpiler moves to files, make this start
//as null.
static char * qc_code;/*=		"{#dmain[#dargc#ppbargv]\n"
							"\t;this is a line comment\n"
							"\t{:return 0;:}\n"
							"\t;*this is a block comment that\n"
							"\tkeeps going after new lines*;\n"
							"}";*/
int qc_size=0;

//pointers copied directly from argv
static char * input_path;
static char * output_path;
static char * defined_cc;	//defined c compiler for automatic compilation

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

//Primary utilities{{{

static inline int pushToOutput(char* symbol,int size){{{
	//increases the size of the output allocation while
	//copying the same amount of bytes to the allocation
	//basically just a copy but with the expansion of the
	//allocation.
	
	result_code=	realloc(result_code,result_size+size);
	bcopy(symbol,&result_code[result_size],size);
	result_size+=	size;
	return 0;
}}}

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

static inline int handleComment(char *start){{{
	int i=0;
	if (start[i+1]=='*'){//if block comment
		i+=2;

		//find the size of the comment
		while(start[i]!='*' && start[i+1]!=';') i++;

		//push to C  replacing qcc comments with C comments
		if ((mode_flags&mode_trim_comments)==0){
			{char *temp="/*";pushToOutput(temp,2);}
			pushToOutput(start+2,i-2);
			{char *temp="*/";pushToOutput(temp,2);}
		}
		
		//
		i+=1;

	}
	else{//else if line comment
		
		//find the size of the comment
		while(start[i]!='\n') i++; 

		//push to C, translating all the while
		if ((mode_flags&mode_trim_comments)==0){
			{char *temp="//";pushToOutput(temp,2);}
			pushToOutput(start+1,i);
		}
	}

	return i;
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
	
	//keep going while there is no endcap and these are valid
	}while (	
				//while no type is defined
			(	c!='b'	&&	c!='B'	&&
				c!='w'	&&	c!='W'	&&
				c!='d'	&&	c!='D'	&&
				c!='q'	&&	c!='Q'	&&
				c!='h'	&&	c!='f'	&&
				c!='l'	&&	c!='v')	&&
				//and modifiers are still being defined
				(c=='x' ||	c=='y'	||
				 c=='z'	||	c=='p')
			);

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

//Misc utilities{{{

int dumpToFile(){{{
	//dumps the whole output to a given file
	int f=open(output_path,O_WRONLY|O_CREAT,0777);

	//if write failed, then...
	write(f,result_code,result_size);

	close(f);
	return 0;
}}}

int pullFromFile(char *path){{{

	//initialize main buffer
	qc_code=malloc(4096);

	//initialize local buffer
	int ti;char tca[4096];
	
	int f=open(path,O_RDWR);

	do{	
		//read input
		ti=read(f,tca,4096);

		//if read ended
		if (ti!=0){
//		printf("%i\n",ti);

		//increase main buff alloc and copy local to it
		qc_code=realloc(qc_code,qc_size+ti);
		bcopy(tca,qc_code+qc_size,ti);

		//realloc by adding whole 4kib pages
		qc_size+=ti;
		}

	}while(ti!=0);
	close(f);
	return 0;
}}}

int printUsage(){{{
	static char * usage=
		"qcc qcc_file.qc [-nc] [-nw] [-p] [-o output.c] [-c [gcc,tcc,etc]] [-h]\n"
		"\t-nc\t:\tNo comments output to C.\n"
		"\t-nw\t:\tNo whitespace output to C.\n"
		"\t-p\t:\tPrint C code to standard out.\n"
		"\t-o\t:\tOutputs the end file to the given path.\n"
		"\t-c\t:\tUNIMPLIMENTED | automatically compiles the resulting\n"
		"\t\t\tC code with an optionally specified compiler.\n"
		"\t-h\t:\tPrint this message\n";
	write(1,usage,strlen(usage));
}}}

//}}}

int main(int argc, char **argv){{{

	//command line args parsing {{{
	
	if (argc<2){printUsage();return 1;}

	{char input_defined=0;
	for (int i=1;i<argc;i++){
		if (argv[i][0]=='-'){
			switch (argv[i][1]){

				case 'n':
					if (argv[i][2]=='c') mode_flags|=mode_trim_comments;
					else if (argv[i][2]=='w') mode_flags|=mode_trim_whitespace;
					break;

				case 'p':
					if (mode_flags&mode_do_compile){
						printf("Incompatible flags \"-p\" and \"-c\"\n");
						return 1;}
					if (mode_flags&mode_defined_output){
						printf("Incompatible flags \"-p\" and \"-o\"\n");
						return 1;}
					mode_flags|=mode_print_code;break;

				case 'c':
					if (mode_flags&mode_print_code){
						printf("Incompatible flags \"-c\" and \"-p\"\n");
						return 1;}
					if (argv[i][2]=='c'){ defined_cc=argv[i+1];i++; }
					mode_flags|=mode_do_compile;break;

				case 'o':
					if (mode_flags&mode_print_code){
						printf("Incompatible flags \"-o\" and \"-p\"\n");
						return 1;}
					mode_flags|=mode_defined_output;
					output_path=argv[i+1]; i++; break;
				default:
					printf("Argument %i - %s not recognized. Ignoring\n",i,argv[i]);
					break;

			}
		}else{
			if(input_defined){
				printf("Too many input files defined\n");
				return 1;
			}
			else{
				input_path=argv[i]; input_defined=1;
			}
		}
	}
	}

	//}}}

	//input file reading
	pullFromFile(input_path);

	//initialization of output storage buffer
	result_code=malloc(1);

	//main parsing loop{{{
	for (int i=0;i<strlen(qc_code);i++){

		//If there's a new arglist and a new scope, that
		//means that this curly bracket is needed in the C
		//output.
		if(parse_flags&prs_was_arg&&parse_flags&prs_new_scope){
			char *c="{";pushToOutput(c,1),parse_flags^=prs_was_arg;}

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
							continue;}
						
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

			case ';':	
						i+=handleComment(temp);
						continue;

			default:	
						
		}
	}//}}}

	//output{{{
	
	//gets mode_flags without the trimming modes
	switch (mode_flags&0x1c){
		case 0x4:	//only print
			printf(result_code);
			break;

		case 0x8:	//write transpile to file given
			dumpToFile();
			break;

		case 0x18:	//compile to given output file

		case 0x10:	//compile to default file path

		case 0x0:	//write to default file path

	}//}}}
	
	return 0;
}}}
