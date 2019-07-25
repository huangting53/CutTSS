#pragma once
#ifndef  _TRIE_H
#define  _TRIE_H

#include<stdio.h>
#include<stdlib.h>
#include<queue>

#include "hs.h"
#include "stdinc.h"

using namespace std;

class trie {
	struct nodeItem {
		bool	isleaf;
		//pc_rule	*rulelist;
		int	nrules;
		range	field[MAXDIMENSIONS];
		int	*ruleid;
		unsigned int	ncuts[MAXDIMENSIONS];
		hs_node_t* rootnode;
		int*	child;
		int	layNo;
		int	flag;     //Cutting or Splitting
	};
public:

  	int	binth;                 
	int	pass;			// max trie level
	int	k;                      //dim. of small
	int dim[MAXDIMENSIONS];
	int	freelist;		// first nodeItem on free list
	unsigned int	threshold;

	int	Total_Rule_Size;	// number of rules stored
	int	Total_Array_Size;
	int	Leaf_Node_Count;
	int	NonLeaf_Node_Count;
	float	total_ficuts_memory_in_KB;       
	float	total_hs_memory_in_KB;
	float	total_memory_in_KB;


	int	max_depth;
	int	numrules;
	pc_rule	*rule;  //整个规则集
	int	root;		// root of trie
	nodeItem *nodeSet;	// base of array of NodeItems

	int queryCount[1] = {0};

public:
       queue<int> qNode;	//queue for node
       trie(int, int, pc_rule*, pc_rule*,int, int);
	   ~trie();
       void  count_np_ficut(nodeItem*);
       void createtrie();
	   int trieLookup(int*);
	   int TablesQueried()
	   {
		   return queryCount[0];
	   }
};

#endif 
