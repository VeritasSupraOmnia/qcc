//for saving keywords (return, if, while, do, for, etc.)
typedef struct keyword_pair{
	char * qc;
	char * c;
}key_pair;

key_pair newKeyPair(char * qc, char * c ){
	key_pair t;
	t.qc=qc;
	t.c=c;
	return t;
}

//for high level organization of keyword pairs
typedef struct key_pair_context{
	key_pair * kpa;
	unsigned long long length;
}kp_cntxt;

//for ease of refactoring
#define kpc_increment 4096
#define kpc_increment_indexes kpc_increment/sizeof(kp_cntxt)
#define kpc_index_size kpc_increment / kpc_increment_indexes 

int initKeyPairContext(kp_cntxt kpc){
	kpc.kpa=malloc(kpc_increment);
	kpc.length=0;
}

int pushKeywordPair(kp_cntxt kpc,key_pair kp){
	//I can assume that this will only increase indexes
	if (kpc.length%kpc_increment_indexes==0)
		kpc.kpa=realloc(
			kpc.kpa,kpc.length*kpc_index_size+kpc_increment);
	kpc.kpa[kpc.length]=kp,kpc.length++;
	return 0;
}
