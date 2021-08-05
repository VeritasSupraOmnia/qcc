I need to increase the amount of time I spend programming assembly, not in C.
Due to the fact that C is slow to write but fast and virtual machine languages
are generally inefficient but very fast to write, bloated and oriented around
scripting, I need to develop a language and transpiler/compiler-invoker that
takes the syntactic beauty of lisp and map it onto C - 1 to 1.5. I say 1 to 1.5
because the C source will be larger than the lisp source, but I don't want it to
be by much. I am not trying to have runtime manipulation or dynamic anything. I
want basically C with better syntax and a paintjob. I want to have similar macro
capabilities to lisps but with the ability to map pretty directly onto C code so
you can know what you are actually writing when you write it. Carp doesn't do
this. No other lisp compiler - interpreter I know of does this. I don't need a
fancy stdlib. I want to use the standard C lib alone. I, eventually, want
integrated inline assembly.  None of this bullshit where you have to declare
shit beforehand. I want to be able to write (mov rax value) and move value into
the rax register. 

Because this isn't a real lisp and doesn't try to be, it's main purpose is to
enable people to write a lot of quality C very quickly with little drawback, I
want to call this QC - QuickC. This isn't necessarily faster performing, so to
speak, I want to set up the transpiler so that you are very much capable of
writing, if you know what you are doing, very exact C very quickly. For this
end, I wannt to also have temporary file buffers, in-file compiler selection and
flag passing, as well post-compile auto-execution and scripting support for gcc
and other script-less compilers, by calling the output file in filesystem
mounted memory from the current position and subsequently removing it after
executing it, 


Compiler_Invokation_and_Control:
	
	cmd:	qcc helloworld.qc
	output:	helloworld.c
	
	cmd:	qcc helloworld.qc -o helloworld.h
	output:	helloworld.h

	cmd:	qcc helloworld.qc -o helloworld
	rout:	helloworld.qc -> qcc -> helloworld.c -> cc -> helloworld
	output:	helloworld 

	cmd:	qcc helloworld.qc
	source:	(iflags exec)
	rout:	helloworld.qc -> qcc -> libgccjit -> execution_of_compiled_code

Typing:
	Obviously I want static typing. I'm thinking I will do something like:

	whole:	#dtestvalue
	type:	#d
	name:	testvalue
		
	Basically, the '#' will tell the transpiler that you are declaring a new variable. The 'd'
	is a signed int which is the bytes used in the integer. This is the typing
	mechanism. It will keep searching for a type until it finds a match, it runs out of possible
	remaining types for the current characters in the type string or it hits a reserved
	character or whitespace. If it doesn't find a type after this it will screech and print
	"unknown type: #thisisnotarealtype at line: 69 in file cum.qc" or something similar.


QC:	{#di 3}
C:	int i=3;
	
	Above is an example of the assignment syntax. The curly brackets do assignment.
	
QC:	#di
C:	int i;

	if you don't need an assignment, you can just leave it as is in the list.
	Both
	
	The following are the standard nc l
	data type names and what they represent.
	
	Integers:
	b	:	signed byte
	B	:	unsigned byte
	w	:	signed word
	W	:	unsigned word
	d	:	signed doubleword
	D	:	unsigned doubleword
	q	:	signed quadword
	Q	:	unsigned quadword
	Floats:
	h	:	2byte half floating point
	f	:	4byte single floating point
	l	:	8byte long floating point
	vector infixes:
	xb	:	16 signed bytes
	xQ	:	16 unsigned quadwords
	yw	:	16 signed words
	yD	:	8 unsigned words
	zf	:	16 single floats
	zB	:	64 unsigned bytes
	other:
	v	:	void
	pointer infixes:
	pb	:	pointer to byte
	ppb	:	pointer to pointer to byte

	Vector data types are explained later.

	You can declare a list of variables of the same type without by just by enclosing them in the
	sexpressions(yeah, baby).

QC:	(#Qa b c d e f g)
C:	unsigned long long a,b,c,d,e,f,g;

	You can also initialize a listt of variables.
QC:	{#Qa b c d e f g:1 2 3 4 . 6 7}
C:	unsigned long long a=1,b=2,c=3,d=4,e,f=6,g=7;

	The '.' skips assignment of a variable  in the variable enumeration
	list. '...' on their own without touching another value in the
	assignment list would skip 3 whole variable assignments. This function
	is arbitrary.  insufficient amount of assignments for the variables in
	the lisp will result in the rest of the variables being un-initialized.
	':' separates symbol enumeration with value assignment.  This can be
	right next to both variable names and values, with not separating
	whitespace, because it isn't an operator and isn't allowed within
	variable names.

	Also, as you see in the below example, the basic assignment operator
	will still work for basic C functionality. The difference is the
	equality sign is separated by whitespace. You also no longer need the
	separator, as that is needed more for readability than accurate
	transpiling. This is slower to write and mimics the functionality of the
	curly brackets.  This should be used primarily for clairity sake.

QC:	#di (=: #qj i)
C:	int i; long long j = (long long)i;
	
	The above example shows how to cast an already declared variable to a
	different type. No reason to overcomplicate things. I need the cast
	declarator on the left of the type so that you can start your variable
	name with a 'c'. If I put it after, the compiler couldn't know that I
	wanted to cast or if I was about to refer to Elizabeth Warren properly.

	NOTE: THE DECLARATION FLAGS CAN COME IN ANY ORDER BUT THEY MUST COME AFTER THE DECLARATION
	INITIALISER AND BEFORE THE TYPE SPECIFICATION 
	#pdi

QC:	{#dv 3}
C:	int v=3;

	Use the defining curly brackets (it's not scope like in C), to tell the
	transpiler that you are trying to define something.

QC:	3#dv
C:	int v=3;

	Enabling even shorter declarative assignment, this functions based off the
	fact that all data is declared with a non alphabetical symbol. 

QC:	3v-
C:	v-=3;

QC:	v++
C:	v++;

	
Assign-Operation Symbols:
|= ^= &=
C:	++ -- += -= *= /= 
QC:	<<<	-	i<<=i1;//I will make new cuda syntax for QC later. 
	>>>	-	i>>=i1;
	*|	-	multiply by itself.
	+|	-	added by itself.

	Appending the last '#' in a declaration prefix with a 'r' will prepend
	the variable with the qualifier register, which only really means that
	on higher 'O' levels it will trade the variable to memory later than the
	other variables, but cannot be passed via a pointer. It's useful, imo,
	mostly for large functions with a lot of variables that need to be saved
	and you want to tell the compiler which variables are most important to
	keep in register and which aren't. Example:
	
	QC:
	#rQregister_var
	C:
	register long long register_var:

QC:	(##newstruct #qquadval #wwordval #Ququadval)
C:	struct newstruct{
		long long quadval;short wordval;unsigned long long uquadval;
	};

	The above syntax uses the double pound '##' to create a typedef struct
	with the given members.

QC:	{##newstruct #qquadval #wordval #Ququadval:1 2 3}	
C:	struct newstruct{long long quadval;short wordval;unsigned long long uquadval;
	};newstruct newstruct;newstruct.quadval=1;newstruct.wordval=2;newstruct.uquadval=3;

	This shows how to declare a typedef struct, say within a function to
	better organize things, then immediately declare a new instance of said
	struct and initiate its values.

QC:	{#newstructn . 2}{3n.uquadval}
C:	newstruct n;n.wordval=2;n.uquadval=3
	

QC:	#newstructn 1_2_3
C:	newstruct n;n.quadval=1;n.wordval=2;n.uquadval=3;

QC:	#newstruct 1 2 3
C:	newstruct newstruct;newstruct.quadval=1;newstruct.wordval=2;newstruct.uquadval=3;
	
QC:	'di 8
C:	int i[8];

QC:	8'di
C:	int i[8];
	
	This declares an array of signed, 4byte integers with 8 indexes.  There
	is not the same problem as in C where you need total enclosure. Space
	between parts of a list are guarenteed during declaration or the
	transpiler will assume you are talking about the next symbol, so this is
	not a big deal, this array declarator that does not enclose the array
	count so there is no chance of over-inclusion bugs that escape to the
	underlying C here. I can use commas here because listing things is
	guarenteed whitespace. Unlike C, the comma isn't used other than for
	data management of some kind. It can be used for array addressing as
	well.

	Multidimensional array declaration

QC:	`di 8_9
C:	int i[8][9];

QC:	8_9`di
C:	int i[8][9];
	
	The '_' is used to split dimensions of an array and can be used because constants. The dimension parameters MUST not be split by whitespace.

	Multidimensional array initialazation

QC:	8_9#di`=
		1,..,9_1,..,9_1,..,9_1,..,9
		1,..,9_1,..,9_1,..,9_1,..,9 
		
C:	int i[8][9]={
		{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8},
		{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8},{1,2,3,4,5,6,7,8}}
		

	The transpiler can know what is what because of the placement of the
	commas. #di ,8 for example, has whitespace between the comma and the
	declaration and before the array size initialization.


QC:	{#da,8 b c,4:0,++,7 . 4,..,1}
	{b a,3}
C:	int a[8]={0,1,2,3,4,5,6,7},b,c={4,0,0,1};
	b=a[3];

	Array value assignment incrementers/decrementers. These things allow you
	to declare arrays with programmatically incremented or decremented
	values. These use standard QC (not regular C) incrementer/decrementer
	symbols which can be attached in the same way as C's can
	
QC:	0,++,7#da,8
C:	int a[8]={0,1,2,3,4,5,6,7};

	

VECTOR:	

QC:	#yQvector
C:	unsigned long long vector[4];
	
	Due to intrinsics being cringe, and the fact that I may or may not want
	to use them at all, using an array of the bytes of the simd array
	divided by the size of each sub-element. Since future intrinsic support
	is highly dubious, since asm is to be integrated within the base syntax,
	I don't need that weird array bullshit. The only thing is that if you
	want to compute ymm sizes, you basically need to do so manually.

QC:	#yQvector,8
C:	unsigned long long vector[32];
	
	The vector array is just does a multiply on the array size of a single
	variable of the same vector register size. The below equation details
	the process by which the transpiler derives this value of '32'.

Equat:	( register_size / variable_size ) * index_count = base_C_array_index_count
	( 32            / 8             ) * 8           = 32

	Doing it this way makes processing requires said processing to be done
	in assembly but that's what something I wanted to do anyway with this
	language: make assembly language a an integral part of the programming
	experience. Making the rest of the language better and vastly reducing
	the mental and hand-cost of inline assembly will make it viable.

Pointers:
	QC:
	(=#*da &(=#db 4))
	C:
	int b=4;
	int *a=&b;

Scope(closure):
	
	I think that if I basically use curly brackets to represent closure, I
	can make a 100 percent guarentee that the scope will match. This is not
	what I want, however. I want the transpiler to optimize this scoping
	better, checking the dependencies and avoiding curly brackets safely
	without sacrificing perfectly coherent transience.
	
Comments:
	;This is a line comment
	;*	This is a 
		comment block	*;
Functions:
	Declaration:
	{$dtestfunction[#darg1 arg2](ret(+ arg1 arg2))}
	Which would transpile to:
	int testfunction(int arg1,int arg2){return arg1+arg2};

	Obviously, the function is declared much like a C function except the
	arguments are in square brackets and the first curly bracket is before
	its C counterpart. Other than that, it's the same. Curly brackets are
	unlike in lisps. If you use them, you are telling the compiler you are
	defining something of some sort. this can be implicit, such as in: 

	for 0#di i<100 i++ testfunction 1 2
	for(int i=0;i<100;i++)testfunction(1,2);

	Because expressions are optional within the list, you can have this
	interesting and quite fast way to call a function. If you have a variadic
	function, use the parentheses like so:

	for 0#di i<100 i++ (testfunction 1 2)
	for(int i=0;i<100;i++)testfunction(1,2);

	This tells the transpiler that the function is calling 

Expressions:
	Expressions work exactly the same way they do in C except without needing
	semicolons.

QC:	i=i|2&&f==3 ret 0
C:	i=i|2&&f==3;return 0;

	The way C does expressions is pretty much fine with me and I'm not gonna
	change most of it. What I will change is Turnary operations...

QC:	i?i=4:i=5 ret 0
C:	i?i=4:(i=5); return 0;

Integration:
	
	As this is meant to be a fast way to write C, I also want the ability to
	switch between C and QC within the same file. Given that I am setting
	the syntax up to be highly interoperable with C, I should be able to
	switch mid function but not mid expression.  I should, theoretically, be
	able to switch back and forth between them.  So, QC assumes you are
	starting in QC lang then uses '{:' and ':}', like the block comments but
	with curly braces and colons, to switch to pure C mode.  When I make
	this, I don't think I will be spending much time in 'C' mode but I don't
	know what will be better in what form and I need a way to use C to
	suppliment the lang while I develop it. It's also the case that not
	having that fine grained control over the mid-level programming media is
	highly short-sighted due to the gimping of control the programmer might
	very easily feel from not having the ability to emit fairly exact C and
	fairly exact assembly, thereby.

	Because this is mostly a preprocessor not a true compiler, I don't need
	to worry so much about the verbosely emitted C being technically
	incorrect in certain situations as long as it relates directly to the
	preprocessor. Case and point, defining something like '(:' and ':)' to
	go back to QC from inside pure C will, while technically not being pure
	C, allow a mostly rationally sound, but completely unheard of outside of
	strings, sequence of characters to represent a way to distinguish nested
	lang swaps.

	
QC:	{#da 4}
	{:int i=5+a;:}
C:	int a=4;
	int i=5+a;
	
QC:	{:int i=5;
	(:{#da 4}:):}
C:	int i=4




ASSEMBLY:
	WARNING:	AT&T SYNTAX on C examples
	NOTE:		QC USES INTEL SYNTAX BY DEFAULT
	
QC:	(mov a b)
C:	__asm__ (
		"mov %1, %0\t\n"
		:"=r" (a)
		:"r" (b)
		);
	
QC:	(mov a b)
	(add a c)
C:	__asm__ (
		"mov %1, %0\t\n"
		"add %2, %0"
		:"=r" (a)
		:"r" (b), "r" (c)
		);

	As is clear from this, one can, with a little bit of fanangling, write assembly code very
	easily (and without touching at&t). I don't want basically any separation between a called function
	and an assembly instruction. I might add a compiler flag to add intrins later but intrins
	are pretty fucking cringe, ngl, so I might scrap that idea.

MACROS:

QC:	{#mTestMacro 3#di}
C:	#define Testmacro int i=3;

QC:	{#mTestMacro[type id] 3#typeid
C:	#define Testmacro(type,val) type id=3;

QC:	{

GOAL:
	The number one goal of this project is to make a lisp-like skin for C, not to make a mostly
	C compatible lisp compiler. I hate lispfags so goddamn much. They have 1 really great idea,
	and one terrible idea. From the fact that these people refused to do static typing to the
	need for 'code as manipulatable as data' to be mantained. 
