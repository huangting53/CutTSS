
#pragma once
#ifndef  _HS_H
#define  _HS_H

//#include "CutSplit.h"
#include "../CutTSS.h"
/* for 5-tuple classification */
#define DIM			5
//#define	DEBUG

/* for function return value */
#define SUCCESS		1
#define FAILURE		0
#define TRUE		1
#define FALSE		0
#define Null -1

/* locate rule files */
#define RULEPATH "./rules/"

/* for bitmap */
#define MAXFILTERS	115536 /* support 64K rules */	
#define WORDLENGTH	32	/* for 32-bit system */	
#define BITMAPSIZE	256 /* MAXFILTERS/WORDLENGTH */

#define MAXDIMENSIONS 5
#define MAXBUCKETS 40
#define MAXNODES 5000000
#define MAXCUTS  16
#define PTR_SIZE 4
#define HEADER_SIZE 4
#define LEAF_NODE_SIZE 4
#define TREE_NODE_SIZE 8


/*-----------------------------------------------------------------------------
 *  structure
 *-----------------------------------------------------------------------------*/
struct FILTER						
{
	unsigned int cost;		
	unsigned int dim[DIM][2];
	unsigned char act;	
};

struct FILTSET
{
	unsigned int	numFilters;	
	struct FILTER	filtArr[MAXFILTERS];
};


struct TPOINT
{
	unsigned int value;
	unsigned char flag;	
};

struct FRAGNODE
{
	unsigned int start;
	unsigned int end;
	struct FRAGNODE *next;
};

struct FRAGLINKLIST
{
	unsigned int fragNum;
	struct FRAGNODE *head;
};

struct TFRAG
{
	unsigned int value;						// end point value
	unsigned int cbm[BITMAPSIZE];					// LENGTH * SIZE bits, CBM
};
						// released after tMT[2] is generated

struct FRAG
{
	unsigned int value;
};


struct CES
{
	unsigned short eqID;					// 2 byte, eqID;
	unsigned int  cbm[BITMAPSIZE];	
	struct	CES *next;								// next CES
};

struct LISTEqS
{
	unsigned short	nCES;					// number of CES
	struct			CES *head;								// head pointer of LISTEqS 
	struct			CES *rear;								// pointer to end node of LISTEqS
};


struct PNODE
{
	unsigned short	cell[65536];			// each cell stores an eqID
	struct			LISTEqS listEqs;					// list of Eqs
};


/*for hyper-splitting tree*/
typedef	struct rule_s
{
	unsigned int	pri;
	unsigned int	range[DIM][2];
} rule_t;

typedef struct rule_set_s
{
	unsigned int	num; /* number of rules in the rule set */
	rule_t*			ruleList; /* rules in the set */
} rule_set_t;

typedef	struct seg_point_s
{
	unsigned int	num;	/* number of segment points */
	unsigned int*	pointList;	/* points stores here */
} seg_point_t;

typedef struct segments_s
{
	unsigned int	num;		/* number of segment */
	unsigned int	range[2];	/* segment */
} segments_t;

typedef	struct search_space_s
{
	unsigned int	range[DIM][2];
} search_space_t;

typedef struct hs_node_s
{
    unsigned int		d2s;		/* dimension to split, 2bit is enough */
    unsigned int		depth;		/* tree depth of the node, x bits supports 2^(2^x) segments */
	unsigned int		thresh;		/* thresh value to split the current segments */
	rule_set_t* ruleset;
	struct hs_node_s*	child[2];	/* pointer to child-node, 2 for binary split */
} hs_node_t;

class pc_rule
{
public:
	int id;
	range field[MAXDIMENSIONS];
};

class hs_result  //result of hypersplit tree
{
public:
	int num_rules;
	int num_childnode;
	int wst_depth;
	float avg_depth;
	int num_tree_node;
	int num_leaf_node;
	float total_mem_kb;
};

class hstrie {

public:
unsigned int	binth;
unsigned int	gChildCount;
unsigned int	gNumTreeNode;
unsigned int	gNumLeafNode;
unsigned int	gWstDepth;
unsigned int	gAvgDepth;
unsigned int	gNumNonOverlappings[DIM];
unsigned long long	gNumTotalNonOverlappings;
hs_result result;

public:

hstrie(int number, pc_rule* subset, int binth, hs_node_t* node);

/* build hyper-split-tree */
int BuildHSTree(rule_set_t* ruleset, hs_node_t* node, unsigned int depth); /* main */


};

/* lookup hyper-split-tree */
int LookupHSTree(pc_rule* rule, hs_node_t* rootnode, int* header, int *queryCount);

#endif 
