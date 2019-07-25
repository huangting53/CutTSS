

#pragma once

#include "./OVS/TupleSpaceSearch.h"
#include "./TSS/tss.h"


class range
{
public:
	unsigned low;
	unsigned high;
};

class field_length
{
public:
	unsigned length[5];
	int size[5];
	int flag_smallest[4];
};

struct CutTSSNode{
	int	depth;
	bool isleaf;

	std::vector<Rule> classifier;
	int nrules;
	int* child;

	range field[MAXDIMENSIONS];
	//int ncuts[MAXDIMENSIONS]; //number of cuts
	int ncuts = 1;

	int flag = 1; //cut 1 or tss 2
	PriorityTupleSpaceSearch ptss;
    //TSS ptss;
	int ntuples;
};


class CutTSS : public PacketClassifier {
public:
	CutTSS(int k1,int threshold1, std::vector<Rule>& classifier);
	~CutTSS();
	virtual void ConstructClassifier(const std::vector<Rule>& rules);
	virtual int ClassifyAPacket(const Packet& packet);
	virtual void DeleteRule(const Rule& delete_rule);
	virtual void InsertRule(const Rule& insert_rule);
	virtual Memory MemSizeBytes() const { return 0; } // TODO
	virtual int MemoryAccess() const { return 0; } // TODO
	virtual size_t NumTables() const { return 1; }
	virtual size_t RulesInTable(size_t tableIndex) const { return rules.size(); }

	void prints(){

		//total_ficuts_memory_in_KB = double(Total_Rule_Size*RULE_SIZE+Total_Array_Size*NODE_SIZE+Leaf_Node_Count*LEAF_NODE_SIZE+NonLeaf_Node_Count*TREE_NODE_SIZE)/1024;
		total_ficuts_memory_in_KB = double(Total_Array_Size*NODE_SIZE)/1024;
		total_tss_memory_in_KB = double(total_tss_memory_in_KB)/1024;
		total_memory_in_KB = total_ficuts_memory_in_KB + total_tss_memory_in_KB;

		if(numrules>binth){
			if(k==1)
				printf("\n\t***SA Subset Tree(using FiCuts + TSS):***\n");
			if(k==2)
				printf("\n\t***DA Subset Tree(using FiCuts + TSS):***\n");
			if(k==3)
				printf("\n\t***SA_DA Subset Tree(using FiCuts + TSS):***\n");
			printf("\tRESULTS:");
			printf("\n\tnumber of rules:%d",numrules);
			printf("\n\tnumber of rules in leaf nodes:%d",Total_Rule_Size);
			printf("\n\tworst case tree level: %d",pass);
			printf("\n\tworst case tree depth: %d",max_depth);
			printf("\n\ttotal memory(Pre-Cutting): %f(KB)",total_ficuts_memory_in_KB);
			printf("\n\ttotal memory(Post-TSS): %f(KB)",total_tss_memory_in_KB);
			printf("\n\ttotal memory: %f(KB)",total_memory_in_KB);
			printf("\n\tByte/rule: %f",double(total_memory_in_KB*1024)/numrules);
			printf("\n\t***SUCCESS in building %d-th CutTSS sub-tree(0_sa, 1_da)***\n\n",k%3);

		}
	}

    int TablesQueried()
    {
		for(int i=1; i<=MAXNODES; i++) {
			if(nodeSet[i].flag == 2){
				queryCount[0] += nodeSet[i].ptss.TablesQueried();
			}
		}
	    return queryCount[0];
    }

    int cutPktCount() const { return queryCount[1]; }
    int tssPktCount() const { return queryCount[2]; }
    int noMatchCount() const { return queryCount[3]; }
    int totalPktsCount() const { return queryCount[4]; }

private:
	std::vector<Rule> rules;
	CutTSSNode *nodeSet;	// base of array of NodeItems
	int k;
	unsigned int threshold;
	//int dim[MAXDIMENSIONS];


    std::queue<int> qNode;	//queue for node
    int	max_depth = 0;
    int numrules = 0;
    int root = 1;
    int	binth = 8;
    int	pass;			// max trie level
    int	freelist;		// first nodeItem on free list

    int queryCount[5] = {0,0,0,0,0}; //0:query tuples count  1:match packet at cut stage count  2:match packet at tss stage count 3:no match packets count 4:total packets processed

	int	Total_Rule_Size;	// number of rules stored in leaf
	int	Total_Array_Size;   // number of tree nodes
	int	Leaf_Node_Count;
	int	NonLeaf_Node_Count;
	int Total_Tuple_Count;
	double	total_ficuts_memory_in_KB;
	double	total_tss_memory_in_KB;
	double	total_memory_in_KB;
};
