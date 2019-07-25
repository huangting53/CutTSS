/*-----------------------------------------------------------------------------
 *  
 *  Name:		hs.c
 *  Description:	hypersplit packet classification algorithm
 *  Version:		1.0 (release)
 *  Author:		Wenjun Li (wenjunli@pku.edu.cn)	 
 *  Date:		4/15/2018
 *
 *  comments1:		Modified work based on the code of hypersplit from Yaxuan Qi
 *  comments2:		We simply use hypersplit for big rules in this version. However, its performance may be very bad for big rule sets(e.g., 100K). 
 *                      A simple way to resolve this problem: increase threshold value to reduce the number of big rules(e.g. from 2^16 to 2^24,2^28).
 *                      Another more general way is to introduce a few cuttings before splitting to reduce set size, which will be given in future versions. 
 *
 *-----------------------------------------------------------------------------*/

//#include "stdinc.h"
#include <stdio.h>
#include <stdlib.h>
#include"hs.h"
#include <sys/time.h>
using namespace std;


hstrie::hstrie(int number, pc_rule* subset, int binth1, hs_node_t* node)
{
  gChildCount = 0;
  gNumTreeNode = 0;
  gNumLeafNode = 0;
  gWstDepth = 0;
  gAvgDepth = 0;
  gNumTotalNonOverlappings = 0;
  binth = binth1;

  rule_set_t ruleset;
  ruleset.num = number;
  ruleset.ruleList = (rule_t*) malloc(ruleset.num * sizeof(rule_t));

  for (int i = 0; i < ruleset.num; i++) { //节点规则集
	  ruleset.ruleList[i].pri = subset[i].id;
	  for (int j = 0; j < DIM; j++) {
		  ruleset.ruleList[i].range[j][0] = subset[i].field[j].low;
		  ruleset.ruleList[i].range[j][1] = subset[i].field[j].high;
	  }
  }

 
  struct timeval gStartTime,gEndTime;
  gettimeofday(&gStartTime,NULL);
  // build hyper-split tree 
  //hs_node_t rootnode;
//  printf("\n\n>>Building HyperSplit tree (%u rules, 5-tuple)", ruleset.num);

  BuildHSTree(&ruleset, node, 0);
/*#ifdef	LOOKUP
  LookupHSTree(&ruleset, node);
#endif*/
  gettimeofday(&gEndTime,NULL);

  //result of hypersplit tree
  result.num_rules = number;
  result.num_childnode = gChildCount;
  result.wst_depth = gWstDepth;
  result.avg_depth = (float) gAvgDepth/gChildCount;
  result.num_tree_node = gNumTreeNode;
  result.num_leaf_node = gNumLeafNode;
  result.total_mem_kb = (double)(gNumTreeNode*8 + gNumLeafNode*4)/1024;

/*
  printf("\n\n>>RESULTS:");
  printf("\n>>number of children: %d", gChildCount);
  printf("\n>>worst case tree depth: %d", gWstDepth);
  printf("\n>>average tree depth: %f", (float) gAvgDepth/gChildCount);
  printf("\n>>number of tree nodes:%d", gNumTreeNode);
  printf("\n>>number of leaf nodes:%d", gNumLeafNode);
  printf("\n>>total memory: %f(KB)", (double)(gNumTreeNode*8 + gNumLeafNode*4)/1024);
  printf("\n>>preprocessing time: %ld(ms)", 1000*(gEndTime.tv_sec - gStartTime.tv_sec)
			+ (gEndTime.tv_usec - gStartTime.tv_usec)/1000);
  printf("\n\n>>SUCCESS in building HyperSplit tree :-)\n\n");
*/
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  Compare
 *  Description:  for qsort
 *     Comments:  who can make it better?
 * =====================================================================================
 */
int SegPointCompare (const void * a, const void * b)
{
  if ( *(unsigned int*)a < *(unsigned int*)b )
      return -1;
  else if ( *(unsigned int*)a == *(unsigned int*)b )
      return 0;
  else 
      return 1;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  BuildHSTree
 *  Description:  building hyper-splitting tree via recursion
 * =====================================================================================
 */
int hstrie::BuildHSTree (rule_set_t* ruleset, hs_node_t* currNode, unsigned int depth)
{
    /*Update Leaf node*/
    if(ruleset->num <= binth) //生成叶子节点
    {
		currNode->d2s = 0;
		currNode->depth = depth;
		currNode->thresh = 0;
		currNode->child[0] = NULL;
		currNode->child[1] = NULL;
		currNode->ruleset = ruleset;

        //printf("\n>>LEAF-NODE: matching rule %d", currNode->ruleset->ruleList[0].pri);

		gChildCount ++;
		gNumLeafNode ++;
		if (gNumLeafNode % 1000000 == 0)
			printf(".");
			//printf("\n>>#%8dM leaf-node generated", gNumLeafNode/1000000);
		if (gWstDepth < depth)
			gWstDepth = depth;
		gAvgDepth += depth;
		return	SUCCESS;

    }


	/* generate segments for input filtset */
	unsigned int	dim, num, pos;
	unsigned int	maxDiffSegPts = 1;	/* maximum different segment points */
	unsigned int	d2s = 0;		/* dimension to split (with max diffseg) */
	unsigned int   thresh;
	unsigned int	range[2][2];	/* sub-space ranges for child-nodes */
	unsigned int	*segPoints[DIM];
	unsigned int	*segPointsInfo[DIM];
	unsigned int	*tempSegPoints;
	unsigned int	*tempRuleNumList;
	float			hightAvg, hightAll;
	rule_set_t		*childRuleSet;

#ifdef	DEBUG
	/*if (depth > 10)	exit(0);*/
	printf("\n\n>>BuildHSTree at depth=%d", depth);
	printf("\n>>Current Rules:");
	for (num = 0; num < ruleset->num; num++) {
		printf ("\n>>%5dth Rule:", ruleset->ruleList[num].pri);
		for (dim = 0; dim < DIM; dim++) {
			printf (" [%-8x, %-8x]", ruleset->ruleList[num].range[dim][0], ruleset->ruleList[num].range[dim][1]);
		}
	}
#endif /* DEBUG */
	
	/*Generate Segment Points from Rules*/
	for (dim = 0; dim < DIM; dim ++) {
		/* N rules have 2*N segPoints */
		segPoints[dim] = (unsigned int*) malloc ( 2 * ruleset->num * sizeof(unsigned int));
		segPointsInfo[dim] = (unsigned int*) malloc ( 2 * ruleset->num * sizeof(unsigned int));
		for (num = 0; num < ruleset->num; num ++) {
			segPoints[dim][2*num] = ruleset->ruleList[num].range[dim][0];
			segPoints[dim][2*num + 1] = ruleset->ruleList[num].range[dim][1];
		}
	}
	/*Sort the Segment Points*/
	for(dim = 0; dim < DIM; dim ++) {
		qsort(segPoints[dim], 2*ruleset->num, sizeof(unsigned int), SegPointCompare);
	}

	/*Compress the Segment Points, and select the dimension to split (d2s)*/
	tempSegPoints  = (unsigned int*) malloc(2 * ruleset->num * sizeof(unsigned int)); 
	hightAvg = 2*ruleset->num + 1;
	for (dim = 0; dim < DIM; dim ++) {
		unsigned int	i, j;
		unsigned int	*hightList;
		unsigned int	diffSegPts = 1; /* at least there are one different segment point */
		tempSegPoints[0] = segPoints[dim][0];
		for (num = 1; num < 2*ruleset->num; num ++) {
			if (segPoints[dim][num] != tempSegPoints[diffSegPts-1]) {
				tempSegPoints[diffSegPts] = segPoints[dim][num];
				diffSegPts ++;
			}
		}
		/*Span the segment points which is both start and end of some rules*/
		pos = 0;
		for (num = 0; num < diffSegPts; num ++) {
			int	i;
			int ifStart = 0;
			int	ifEnd	= 0;
			segPoints[dim][pos] = tempSegPoints[num];
			for (i = 0; i < ruleset->num; i ++) {
				if (ruleset->ruleList[i].range[dim][0] == tempSegPoints[num]) {
					/*printf ("\n>>rule[%d] range[0]=%x", i, ruleset->ruleList[i].range[dim][0]);*/
					/*this segment point is a start point*/
					ifStart = 1;
					break;
				}
			}
			for (i = 0; i < ruleset->num; i ++) {
				if (ruleset->ruleList[i].range[dim][1] == tempSegPoints[num]) {
					/*printf ("\n>>rule[%d] range[1]=%x", i, ruleset->ruleList[i].range[dim][1]);*/
					/* this segment point is an end point */
					ifEnd = 1;
					break;
				}
			}
			if (ifStart && ifEnd) {
				segPointsInfo[dim][pos] = 0;
				pos ++;
				segPoints[dim][pos] = tempSegPoints[num];
				segPointsInfo[dim][pos] = 1;
				pos ++;
			}
			else if (ifStart) {
				segPointsInfo[dim][pos] = 0;
				pos ++;
			}
			else {
				segPointsInfo[dim][pos] = 1;
				pos ++;
			}

		}

		/* now pos is the total number of points in the spanned segment point list */

		if (depth == 0) {
			gNumNonOverlappings[dim] = pos;
			gNumTotalNonOverlappings *= (unsigned long long) pos;
		}

#ifdef	DEBUG
		printf("\n>>dim[%d] segs: ", dim);
		for (num = 0; num < pos; num++) {
			/*if (!(num % 10))	printf("\n");*/
			printf ("%x(%u)	", segPoints[dim][num], segPointsInfo[dim][num]);
		}
#endif /* DEBUG */
		
		if (pos >= 3) {  //pos为segment points的数目
			hightAll = 0;
			hightList = (unsigned int *) malloc(pos * sizeof(unsigned int));
			for (i = 0; i < pos-1; i++) {
				hightList[i] = 0;
				for (j = 0; j < ruleset->num; j++) {
					if (ruleset->ruleList[j].range[dim][0] <= segPoints[dim][i] \
							&& ruleset->ruleList[j].range[dim][1] >= segPoints[dim][i+1]) {
						hightList[i]++; //segment i 的权重，即覆盖segment i 的规则数
						hightAll++;  //当前维度的segments的权重和 规则数总和
					}
				}
			}
			//当前维度的segment平均权重较小
			if (hightAvg > hightAll/(pos-1)) {	/* possible choice for d2s, pos-1 is the number of segs */
				float hightSum = 0;
				
				/* select current dimension */
				d2s = dim;
				hightAvg = hightAll/(pos-1);
				
				/* the first segment MUST belong to the left child */
				//更新二分阈值thresh
				hightSum += hightList[0];
				for (num = 1; num < pos-1; num++) {  /* pos-1 >= 2; seg# = num */
					if (segPointsInfo[d2s][num] == 0) 
						thresh = segPoints[d2s][num] - 1;
					else
						thresh = segPoints[d2s][num];
		
					if (hightSum > hightAll/2) { //加权segments平分
						break;
					}
					hightSum += hightList[num];
				}
				/*printf("\n>>d2s=%u thresh=%x\n", d2s, thresh);*/
				//更新二分子区间的范围
				range[0][0] = segPoints[d2s][0];
				range[0][1] = thresh;
				range[1][0] = thresh + 1;
				range[1][1] = segPoints[d2s][pos-1];
			}
			/* print segment list of each dim */
#ifdef	DEBUG
			printf("\n>>hightAvg=%f, hightAll=%f, segs=%d", hightAll/(pos-1), hightAll, pos-1);
			for (num = 0; num < pos-1; num++) {
				printf ("\nseg%5d[%8x, %8x](%u)	", 
						num, segPoints[dim][num], segPoints[dim][num+1], hightList[num]);
			}
#endif /* DEBUG */
			free(hightList);
		} /* pos >=3 */

		//printf("\n>>298: maxDiffSegPts = %d    pos = %d", maxDiffSegPts,pos);

		if (maxDiffSegPts < pos) {
			maxDiffSegPts = pos;
			//printf("\n>>300: maxDiffSegPts = %d", maxDiffSegPts);
		}
	}
	free(tempSegPoints);


	/*Update Leaf node*/
	if (maxDiffSegPts <= 2) {
		currNode->d2s = 0;
		currNode->depth = depth;
		currNode->thresh = 0; //当不同segment points的数量不超过2时，叶子节点的第一个规则的优先级（最高）赋值给thresh
		currNode->child[0] = NULL;
		currNode->child[1] = NULL;
		currNode->ruleset = ruleset;

		for (dim = 0; dim < DIM; dim ++) {
			free(segPoints[dim]);
			free(segPointsInfo[dim]);
		}

		//printf("\n>>LEAF-NODE: matching rule %d", ruleset->ruleList[0].pri);

		gChildCount ++;
		gNumLeafNode ++;
		if (gNumLeafNode % 1000000 == 0)
			printf(".");
		/*printf("\n>>#%8dM leaf-node generated", gNumLeafNode/1000000);*/
		if (gWstDepth < depth)
			gWstDepth = depth;
		gAvgDepth += depth;
		return	SUCCESS;
	}


	/*Update currNode*/	
	/*Binary split along d2s*/


#ifdef DEBUG
	/* split info */
	printf("\n>>d2s=%u; thresh=0x%8x, range0=[%8x, %8x], range1=[%8x, %8x]",
			d2s, thresh, range[0][0], range[0][1], range[1][0], range[1][1]);
#endif /* DEBUG */


	if (range[1][0] > range[1][1]) {
		printf("\n>>maxDiffSegPts=%d  range[1][0]=%x  range[1][1]=%x", 
				maxDiffSegPts, range[1][0], range[1][1]);
		printf("\n>>fuck\n"); exit(0);
	}


	for (dim = 0; dim < DIM; dim ++) {
		free(segPoints[dim]);
		free(segPointsInfo[dim]);
	}
	
	gNumTreeNode ++;
	currNode->d2s = d2s;
	currNode->depth = depth;
	currNode->thresh = thresh;
	currNode->child[0] = (hs_node_t *) malloc(sizeof(hs_node_t));
	//printf("\n>>line 354 ----------- currNode->d2s = %u\n", currNode->d2s);

	/*Generate left child rule list*/
	tempRuleNumList = (unsigned int*) malloc(ruleset->num * sizeof(unsigned int)); /* need to be freed */
	pos = 0;
	for (num = 0; num < ruleset->num; num++) { // low<=end && high>=start 统计左子节点规则数目
		if (ruleset->ruleList[num].range[d2s][0] <= range[0][1]
		&&	ruleset->ruleList[num].range[d2s][1] >= range[0][0]) {
			tempRuleNumList[pos] = num;
			pos++;
		}
	}
	childRuleSet = (rule_set_t*) malloc(sizeof(rule_set_t));
	childRuleSet->num = pos;
	childRuleSet->ruleList = (rule_t*) malloc( childRuleSet->num * sizeof(rule_t) );
	for (num = 0; num < childRuleSet->num; num++) {
		childRuleSet->ruleList[num] = ruleset->ruleList[tempRuleNumList[num]];
		/* in d2s dim, the search space needs to be trimmed off */ //更新左子节点规则集
		if (childRuleSet->ruleList[num].range[d2s][0] < range[0][0])
			childRuleSet->ruleList[num].range[d2s][0] = range[0][0];
		if (childRuleSet->ruleList[num].range[d2s][1] > range[0][1])
			childRuleSet->ruleList[num].range[d2s][1] = range[0][1];
	}
	free(tempRuleNumList);

	BuildHSTree(childRuleSet, currNode->child[0], depth+1); //生成左子节点
 

/*#ifndef	LOOKUP
	free(currNode->child[0]);
	free(childRuleSet->ruleList);
	free(childRuleSet);
#endif*/

	/*Generate right child rule list*/
	currNode->child[1] = (hs_node_t *) malloc(sizeof(hs_node_t));
	tempRuleNumList = (unsigned int*) malloc(ruleset->num * sizeof(unsigned int)); /* need to be free */
	pos = 0;
	for (num = 0; num < ruleset->num; num++) {  // low<=end && high>=start
		if (ruleset->ruleList[num].range[d2s][0] <= range[1][1]
		&&	ruleset->ruleList[num].range[d2s][1] >= range[1][0]) {
			tempRuleNumList[pos] = num;
			pos++;
		}
	}

	childRuleSet = (rule_set_t*) malloc(sizeof(rule_set_t));
	childRuleSet->num = pos;
	childRuleSet->ruleList = (rule_t*) malloc( childRuleSet->num * sizeof(rule_t) );
	for (num = 0; num < childRuleSet->num; num++) {
		childRuleSet->ruleList[num] = ruleset->ruleList[tempRuleNumList[num]];
		/* in d2s dim, the search space needs to be trimmed off */
		if (childRuleSet->ruleList[num].range[d2s][0] < range[1][0])
			childRuleSet->ruleList[num].range[d2s][0] = range[1][0];
		if (childRuleSet->ruleList[num].range[d2s][1] > range[1][1])
			childRuleSet->ruleList[num].range[d2s][1] = range[1][1];
	}
	
	free(tempRuleNumList);  

	BuildHSTree(childRuleSet, currNode->child[1], depth+1);
/*#ifndef	LOOKUP
	free(currNode->child[1]);
	free(childRuleSet->ruleList);
	free(childRuleSet);
#endif*/
		
	return	SUCCESS;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  LookupHSTtree
 *  Description:  test the hyper-split-tree with give 4-tuple packet
 * =====================================================================================
 */
int LookupHSTree(pc_rule* rule, hs_node_t* rootnode, int* header, int *queryCount)
{

	int cover = 1;
	int match = 0;
	int i,k;

	hs_node_t* node = rootnode;

	while (node->child[0] != NULL) {
		queryCount[0]++;
		if (header[node->d2s] <= (node->thresh)){
			node = node->child[0];
		}
		else{
			node = node->child[1];
		}

	}
	if(node != NULL){
		for(i = 0; i < node->ruleset->num; i++){
			queryCount[0]++;
			cover = 1;
			for(k = 0; k < MAXDIMENSIONS; k++){   //k维都匹配才为match
				if(node->ruleset->ruleList[i].range[k][0] > header[k] ||
				    node->ruleset->ruleList[i].range[k][1] < header[k]){
					cover = 0;
					break;
				}
			}
			if(cover == 1){
				match = 1;
				break;
			}
		}
	}


	if(match == 1){
		//printf("\n>>Matched Rule %d\n", node->ruleset->ruleList[i].pri);
		return node->ruleset->ruleList[i].pri;  //返回匹配的规则id
	}else{
		return -1;
	}

}


