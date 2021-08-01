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

//Symbol flags {{{
//for keeping the symbol array state (in "Main data" section)

//Array for keeping track of symbol argument counts and flags.
#define sflgsz U1
sflgsz *symb_flags;//same count as "symbol" detailed in section: Main data

//for determining the amount of bits to shift the access of symbol elements right to get the symbol argument count
#define symb_flags_size 3

//for getting a mask of the flags alone to AND them, just in case...
//This is probably not necessary
#define symb_flags_fmask (-1>>((sizeof(sflgsz)*8)-symb_flags_size))

//whether symbol is a function or, if zero, a variable ID of some kind
#define symb_is_func		0x01
//has any arguments at all
#define symb_has_arguments		0x02
//might have more than count arguments
#define symb_func_is_variadic		0x04

//to get the count, you must shift


//}}}

//parsing flags {{{
//for keeping the runtime state of the parser


//tell the parser to increase scope after the next iteration
//because a new curly bracket is needed
#define prs_new_scope		0x01
//tell the parser that a new argument or indentifier is
//needed and should do those transformation operations.
#define prs_new_args		0x02

#define prs_new_id			0x04
//tell the parser that a new sexpr, which might just be a contiguous symbol
#define prs_new_sexpr		0x1000
#define prs_in_sexpr		0x2000
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
#define err_no_type_for_varlist		0x01
#define err_type_does_not_exist		0x02
#define err_string_has_no_end		0x04
#define err_char_const_has_no_end	0x04

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

//Arrays for keeping track of symbols, which are striped within the main array
//to create sub arrays of their starting symbols. symbol_array[0] is a
//lowercase "a". symbol_array[1] is a lowercase "b" and so on. This can be
//easily calculated and the size of the array is simply checked when a new
//symbol push location is calculated and the array is re-alloced only when the
//wanted index is out of bounds for a number of 4096 byte blocks that are
//needed to take the allocation to a safe buffer amount past the current
//location, probably at least a page ahead of the number.
char ** symbol_array;
//symbol table size
int symbol_array_size;

//I might want to add a non-striped bleed over 3d char array so that past a
//certain memory usage or stripe usage disparity, symbols only increase the
//memory for themselves and not others, which might not use them.

//where the actual storage happens - in blocks of 4096 which each have a type
//when the pushSymbol function doesn't have enough space left in the current
//block for the symbol's type, a new current block is created and the fill, bit
//and current block arrays are updated.
char * block_array;

//these are bit arrays of which blocks contain which type of symbol
/* var		-	variable blocks
 * arr		-	local array blocks                                               
 * pnt		-	generic pointer blocks which function very similarly to array blocks 
 * str		-	struct blocks                                                    
 * fun		-	function blocks                                                  
 * typ		-	type assignment blocks                                           
 * def		-	no-argument C macros                                             
 * dfn		-	argument C macros - "fn" like function                           
 * mac		-	qcc macros                                                       
 */
//bit arrays detailing which blocks have which 
U8 ** block_bit_array;
//current block for any given type
char ** current_type_block;
//malloced array detailing the current fill of each current block per type, in
//above order.
U2 * block_fill_array;


//for tracking sexpr level
int sexpr_level=0;
int scope=0;

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

static inline int pushSymbol(char *symbol,int size,int type){{{
	//pushes a s
}}}

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

static inline int maybeId(char *start){{{
	//returns whether a current symbol might indicate an
	//identifier
	if(((	*start	)<=	0x5a	&&	
	   (	*start	)>	0x40) 	||
	   ((	*start	)<=	0x7a	&&	
	   (	*start	)>	0x60))	return 1;
	return 0;
}}}

static inline int isWhitespace(char *start){{{
	//just tells of the next character is whitespace
	if(*start==' '||*start=='\n'||*start=='\t')
		return 1;
	return 0;
}}}

static inline int sizeOfAlphaNum(char *start){{{
	//finds size of alphanumeric symbol
	int i=0;char c=start[i];

	//while 0-9, a-z or A-Z, then increment
	while(	(	c	<=	0x5a	&&	c	>	0x40 )	||
			(	c	<=	0x7a	&&	c	>	0x60 )	||
			(	c	<=	0x39	&&	c	>=	0x30 )	||
				c	==	'_'	)
			i++, c=start[i];	
			
	return i;
}}}

static inline int toSymbol(char *start){{{
	//Returns byte offset to when the next symbol is,
	//ignoring spaces, tabs and newlines and adds the
	//whitespace to the C code.
	int i=0;char c=start[i];
	while (c==' '||c=='\t'||c=='\n'){
	i++;c=start[i];}
	if((mode_flags&mode_trim_whitespace)==0)
			pushToOutput(start,i);
	else pushToOutput(" ",1);
	return i;//returns the amount to increase the parser increment counter.
}}}

static inline int sizeOfString(char *start){{{
	//gets size of string, including quotes and escapes \" properly as well as \\" and \\\"
	U1 escaped=0;int i=0;
	while (start[i]!='\"' || (start[i]=='\"' && (escaped!=0))){

		if (start[i]=='\n') {error_flags|=err_string_has_no_end;return 0;}

		//an escape means 1 character skipped exit checking
		if (escaped){
			escaped^=escaped;
			i++;
			//this needs to be done even during escapes
			if (start[i]=='\n') {error_flags|=err_string_has_no_end;return 0;}
		}
		if (start[i]=='\\') escaped=1;
		i++;

	}
	return i;
}}}

static inline int sizeOfCharConst(char *start){{{
	//gets size of char const (can be 3 or 4 chars long in source)
	if (start[1]=='\\'){
		if (start[3]!='\''){error_flags|=err_char_const_has_no_end;return 0;}
		return 4;
	}else{
		if (start[2]!='\''){error_flags|=err_char_const_has_no_end;return 0;}
		return 3;
	}
}}}

//Symbol addition{{{

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

static inline int addConstant(char *start){{{
	//adds a constant to the output C code
	int i=0,temp;
	switch (*start){
		
		//cases '0' to '9'
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			//all numeric constants start with a digit even if they contain a letter
			temp=sizeOfAlphaNum(start),
			pushToOutput(start,temp),
			i+=temp;
			break;

		case '\"':	
			temp=sizeOfString(start),
			pushToOutput(start,temp),
			i+=temp;
			break;

		case '\'':
			temp=sizeOfCharConst(start),
			pushToOutput(start,temp),
			i+=temp;
			break;
	}
	return i;
}}}

static inline int addReturn(char *start){{{
	//checks if at the start of a whitespace terminated ret statement
	//if so, add the C return statement as well as the constant that follows
	//If is qc ret statement and has whitespace for return value
	if (*start=='r' && start[1]=='e' && start[2]=='t' && isWhitespace(start+3)){
		//push C return
		{char *temp="return";pushToOutput(temp,6);}
		//move past whitespace to constant
		int i=3;i+=toSymbol(start+i);
		//TODO: add symbol checker here so that variables and functions can be returned.
		i+=addConstant(start+i);	i--;//decrement needed
		{char *temp=";";pushToOutput(temp,1);}
		return i;
	}
	return 0;
}}}

static inline int addBreak(char *start){{{
	//adds a C "break" from the QC "brk"
	//needs exactly "brk" and can't be concatenated to other alphanum symbols
	//without destroying the meaning
	if (*start=='b' && start[1]=='r' && start[2]=='k' && sizeOfAlphaNum(start)==3){
		{char *temp="break;";pushToOutput(temp,6);}
		return 2;
	}return 0;
}}}

static inline int addContinue(char *start){{{
	//adds a C "continue;" from the QC "con"
	if (*start=='c' && start[1]=='o' && start[2]=='n' && sizeOfAlphaNum(start)==3){
		{char *temp="continue;";pushToOutput(temp,9);}
		return 2;
	}return 0;
	
}}}

static inline int addCase(char *start){{{
	//adds a C "case %const:" from the QC "ca %const"
	if (*start=='c' && start[1]=='a' && sizeOfAlphaNum(start)==2){
		{char *temp="case";pushToOutput(temp,4);}//push initial
		int i=2;i+=toSymbol(start+i);
		i+=addConstant(start+i);
		{char *temp=":";pushToOutput(temp,1);}
		return --i;//decrement needed
	}return 0;
}}}

static inline int addGoto(char*start){{{
	//adds a C "goto %label" from the QC "jmp %label"
	if (*start=='j' && start[1]=='m' && start[2]=='p' && sizeOfAlphaNum(start)==3){
		{char *temp="goto";pushToOutput(temp,4);}
		int i=3;i+=toSymbol(start+i);
		int size=sizeOfAlphaNum(start+i);
		pushToOutput(start+i,size);i+=size;
		{char *temp=";";pushToOutput(temp,1);}
		return --i;//decrement needed
	}return 0;

}}}

static inline int addInclude(char *start){{{
	//adds a C "#include <stdio.h>" or "#include \"custom.h\"" from the QC
	//"incl <stdio>" or "incl <stdio.h>" or "incl \"custom.h\""
	
	if (*start=='i' && start[1]=='n' && start[2]=='c' && start[3]=='l' && 
			sizeOfAlphaNum(start)==4){
		{char *temp="#include";pushToOutput(temp,8);}
		int i=4;	i+=toSymbol(start+i);	int size=0;
		while (0==isWhitespace(start+i+size)) size++;
		pushToOutput(start+i,size);
		return i+size-1;
	}
	return 0;
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
	while(	(start[i]<=0x5a	&&	
			start[i]>0x40 	||
			(start[i]<=0x7a	&&	
			start[i]>0x60)	||
			start[i]=='_'))	i++;

	//remind the parser that it is no longer in "ID" state
	//but was, just.
	pushToOutput(start,i);
	return i;
}}}

int addFuncName(char* symbol,int size){{{
	
}}}

static inline int handleComment(char *start){{{
	int i=0;
	if (start[i+1]=='*'){//if block comment

		//move past comment start
		i+=2;

		//find the size of the rest of the comment
		while(start[i]!='*' && start[i+1]!=';') i++;

		//if the mode tells you to push comments
		if ((mode_flags&mode_trim_comments)==0){
			//C block comment start
			{char *temp="/*";pushToOutput(temp,2);}
			//comment text (-2 for actual size)
			pushToOutput(start+2,i-2);
			//C block comment end
			{char *temp="*/";pushToOutput(temp,2);}
		}
		
		//move past the last ';'
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

static inline int addArgs(char *start){{{
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


static inline int addIf(char *start){{{
	//adds an if statement to the C out
	if (*start=='i' && start[1]=='f' && sizeOfAlphaNum(start)==2){
		{char *temp="if";pushToOutput(temp,2);}
		int i=2;i+=toSymbol(start+i);
		parse_flags|=prs_new_sexpr;
		return i-1;
	}
	return 0;
}}}

static inline int addElse(char *start){{{
	//adds an else statement to the C out
}}}

static inline int addSwitch(char *start){{{
	//adds a switch statement to the C out
}}}

static inline int addFor(char *start){{{
	//adds a for loop to the C out
}}}

static inline int addWhile(char *start){{{
	//adds a while loop to the C out
}}}

static inline int addDo(char *start){{{
	//adds a do statement to the C out
}}}


//}}}

//}}}

//Misc utilities{{{

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

		//State Dependent Parsing{{{
		
		//If there's a new arglist and a new scope, that
		//means that this curly bracket is needed in the C
		//output.
		if(parse_flags&prs_was_arg&&parse_flags&prs_new_scope){
			char *c="{";pushToOutput(c,1),parse_flags^=prs_was_arg|prs_new_scope;}

		//For receiving symbol data for this iteration to
		//add to the result_code string.
		char *temp=&qc_code[i];

		//for dealing with whitespace
		if (isWhitespace(temp)){
			i+=toSymbol(temp),parse_flags|=prs_ws_sep_sym;}

		if (parse_flags & prs_new_id && maybeId(temp)){
			//If a previous declaration says to expect a new
			//identifier in the qc source
			int size=addId(temp);
			i+=size;

			parse_flags|=prs_was_id;
			parse_flags^=prs_new_id;
		}

		temp=&qc_code[i];
		if (isWhitespace(temp))
			i+=toSymbol(temp),parse_flags|=prs_ws_sep_sym;

		if (parse_flags&prs_new_sexpr){
			{char *c="(";pushToOutput(c,1);}
			if (*temp =='(') sexpr_level++;
			else parse_flags|=prs_in_sexpr;
			parse_flags^=prs_new_sexpr;
		}

		//}}}

		//Main Parsing Switch{{{

		//check for special characters
		temp=&qc_code[i];
		switch(*temp){ 
			case '#':
						i+=addType(temp+1);
						parse_flags|=prs_new_id;
						continue;
			case '{': 	
						//do passthrough
						if(*(temp+1)==':'){	i+=addPassthrough(temp+2)+1;
							continue;}
						
				 		parse_flags|=prs_new_scope;scope++;
						continue;

			case '}':
						scope--;
						pushToOutput(temp,1);
						continue;
			case '(':	
						sexpr_level++;
						pushToOutput(temp,1);
						continue;
			case ')':	
						sexpr_level--;
						pushToOutput(temp,1);
						continue;

			case '[':
						i+=addArgs(temp+1);
						parse_flags|=prs_was_arg;
						continue;

			case ';':	
						i+=handleComment(temp);
						continue;

			case 'r':	{	int is=addReturn(temp);	//check for return statment
							if (is){i+=is;continue;}
							else {goto symbparse;}	}
			case 'b':	
						{	int is=addBreak(temp);	//check for break statement
							if (is){i+=is;continue;}
							else {goto symbparse;}	}
			case 'c':	{	int is=addContinue(temp);//check for continue statement
							if (is){i+=is;continue;}
							else{ 
								is=addCase(temp);//check for case statement
								if (is){i+=is;continue;}
							goto symbparse;}}
						//IDEA: implement ca '1'->'3' to represent the following:
						//		case '1': case '2': case '3': break;
			case 'j':	{	int is=addGoto(temp);	//check for goto statement
					   		if (is){i+=is;continue;}
					   		else {goto symbparse;}	}
						//IDEA: Make jne, je, jlz, jg, etc to allow for easy if statements.
			case 'i':	{	int is=addInclude(temp);
							if (is){i+=is;continue;}
							else { is=addIf(temp);
								if(is){i+=is;continue;}
							goto symbparse;}}
goto symbparse;
			symbparse:	//Do generic (non keyword) symbol parsing

			default:	
						
		}
	}
	//}}}

	//}}}

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
