/*-----------------------------------------------------------------------------
 *  
 *  Name:		trie.c for CutSplit
 *  Description:	trie construction for CutSplit: Pre-Cutting + Post-Splitting
 *  Version:		1.0 (release)
 *  Author:		Wenjun Li (wenjunli@pku.edu.cn)	 
 *  Date:		4/15/2018
 * 
 *-----------------------------------------------------------------------------*/

/*#include <stdio.h>
#include <stdlib.h>

#include<queue>
#include<list>
#include"CutSplit.h"
#include"hs.h"*/
#include "trie.h"

using namespace std;

trie::trie(int numrules1, int binth1, pc_rule* rule1, pc_rule* rule2, int threshold1, int k1)
{
   numrules = numrules1;  
   binth = binth1;
   rule = rule1;
   k=k1;      //0:SA, 1:DA 切割维度
   for (int i=0; i<MAXDIMENSIONS; i++){
       dim[i] = 0;
       if((k & (1 << i)) > 0) dim[i] = 1;
   }
   threshold = pow(2,threshold1);  //默认24
   nodeSet = new nodeItem[MAXNODES+1]; 
   root = 1;  

   pass=1;
   max_depth=0;
   Total_Rule_Size=0;
   Total_Array_Size=0;
   Leaf_Node_Count=0;
   NonLeaf_Node_Count=0;
   total_ficuts_memory_in_KB=0;
   total_hs_memory_in_KB=0;
   total_memory_in_KB=0;


   for(int i=1; i<=MAXNODES; i++) 
      nodeSet[i].child = (int*)malloc(sizeof(int)); 

    //初始化根节点信息
   nodeSet[root].isleaf = 0;
   nodeSet[root].nrules = numrules;

   for (int i=0; i<MAXDIMENSIONS; i++){
        nodeSet[root].field[i].low = 0;
        if(i<2)
           nodeSet[root].field[i].high = 0xffffffff;
        else if(i==4) 
           nodeSet[root].field[i].high = 255;
        else 
           nodeSet[root].field[i].high = 65535;

       nodeSet[root].ncuts[i] = 0;
   }

  nodeSet[root].ruleid = (int*)calloc(numrules, sizeof(int));
  for(int i=0; i<numrules; i++)
      nodeSet[root].ruleid[i] = rule2[i].id;  //here


  nodeSet[root].layNo = 1; //层数
  nodeSet[root].flag = 1; //cut

  freelist = 2;
  for (int i = 2; i < MAXNODES; i++)
      nodeSet[i].child[0] = i+1;
  nodeSet[MAXNODES].child[0] = Null;

  createtrie();

}
trie::~trie() {
  delete [] nodeSet;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  count_np_ficut
 *  Description:  count np_ficut in pre-cutting stage (using FiCut[from HybridCuts] for one two field)
 * =====================================================================================
 */
void trie::count_np_ficut(nodeItem *v)
{
    int done=0;

    for(int i=0;i<MAXDIMENSIONS;i++){
        v->ncuts[i] = 1;
        if(dim[i]){
            done = 0;
            while(!done){
                if(v->ncuts[i] < MAXCUTS && (v->field[i].high - v->field[i].low) > threshold) //大于最大切割份数，或者子空间小于阈值时，不再加倍切割份数
                    v->ncuts[i]=v->ncuts[i]*2;
                else
                    done=1;
            }
        }
    }
}

void trie::createtrie()
{
    int nr;
    int empty;
    unsigned int r[MAXDIMENSIONS], lo[MAXDIMENSIONS], hi[MAXDIMENSIONS];
    int u,v;
    int s = 0;
    int i[MAXDIMENSIONS];
    int flag,index;

    qNode.push(root);

    while(!qNode.empty()){
        v=qNode.front();
        qNode.pop();

        if(nodeSet[v].flag==1){
            count_np_ficut(&nodeSet[v]);
            for(int t=0;t<MAXDIMENSIONS;t++){
                if(dim[t]){
                    if(nodeSet[v].ncuts[t] < MAXCUTS) //空间小于阈值
                        nodeSet[v].flag=2;  //tss
                }
            }
        }

        if(nodeSet[v].flag==1) //FiCuts stage
        {
            if(nodeSet[v].nrules <= binth){ //叶子节点
                nodeSet[v].isleaf = 1;
                Total_Rule_Size+= nodeSet[v].nrules;
                Leaf_Node_Count++;
                if(max_depth<(nodeSet[v].layNo+nodeSet[v].nrules))
                    max_depth=nodeSet[v].layNo+nodeSet[v].nrules;
            }
            else{  //非叶子节点
                NonLeaf_Node_Count++;
                //printf("\n>> trie.cpp line 395 ------------ nodeSet[v].ncuts  %d  %d \n", nodeSet[v].ncuts[0],nodeSet[v].ncuts[1]);
                nodeSet[v].child = (int *)realloc(nodeSet[v].child, nodeSet[v].ncuts[0] * nodeSet[v].ncuts[1] * sizeof(int));

                Total_Array_Size += (nodeSet[v].ncuts[0] * nodeSet[v].ncuts[1]);

                index = 0;

                r[0] = (nodeSet[v].field[0].high - nodeSet[v].field[0].low)/nodeSet[v].ncuts[0]; //切割后区间长度
                lo[0] = nodeSet[v].field[0].low;
                hi[0] = lo[0] + r[0];
                for(i[0] = 0; i[0] < nodeSet[v].ncuts[0]; i[0]++){  //第0维sip

                    r[1] = (nodeSet[v].field[1].high - nodeSet[v].field[1].low)/nodeSet[v].ncuts[1];
                    lo[1] = nodeSet[v].field[1].low;
                    hi[1] = lo[1] + r[1];
                    for(i[1] = 0; i[1] < nodeSet[v].ncuts[1]; i[1]++){ //第1维dip

                        r[2] = (nodeSet[v].field[2].high - nodeSet[v].field[2].low)/nodeSet[v].ncuts[2];
                        lo[2] = nodeSet[v].field[2].low;
                        hi[2] = lo[2] + r[2];
                        for(i[2] = 0; i[2] < nodeSet[v].ncuts[2]; i[2]++){ //第2维

                            r[3] = (nodeSet[v].field[3].high - nodeSet[v].field[3].low)/nodeSet[v].ncuts[3];
                            lo[3] = nodeSet[v].field[3].low;
                            hi[3] = lo[3] + r[3];
                            for(i[3] = 0; i[3] < nodeSet[v].ncuts[3]; i[3]++){ //第3维

                                r[4] = (nodeSet[v].field[4].high - nodeSet[v].field[4].low)/nodeSet[v].ncuts[4];
                                lo[4] = nodeSet[v].field[4].low;
                                hi[4] = lo[4] + r[4];
                                for(i[4] = 0; i[4] < nodeSet[v].ncuts[4]; i[4]++){ //第4维

                                    empty = 1;
                                    nr = 0;
                                    for(int j=0; j<nodeSet[v].nrules; j++){  //统计各个子节点区间所包含的规则数
                                        flag = 1;
                                        for(int t = 0; t < MAXDIMENSIONS; t++){
                                            if(rule[nodeSet[v].ruleid[j]].field[t].low > hi[t] || rule[nodeSet[v].ruleid[j]].field[t].high < lo[t]){
                                                flag = 0;
                                                break;
                                            }
                                        }
                                        if(flag == 1){
                                            empty = 0;
                                            nr++;
                                        }
                                    }

                                    if(!empty){
                                        nodeSet[v].child[index] = freelist;
                                        u=freelist;
                                        //freelist++;//下一个节点
                                        freelist = nodeSet[freelist].child[0]; //下一个节点
                                        nodeSet[u].nrules = nr;
                                        nodeSet[u].layNo=nodeSet[v].layNo+1;
                                        if(nr <= binth){ //叶子节点
                                            nodeSet[u].isleaf = 1;
                                            Total_Rule_Size+= nr;
                                            Leaf_Node_Count++;

                                            if(max_depth<(nodeSet[u].layNo+nr))
                                                max_depth=nodeSet[u].layNo+nr;

                                        }
                                        else{
                                            nodeSet[u].isleaf = 0;
                                            nodeSet[u].flag=1;  //cut

                                            if(pass<nodeSet[u].layNo)
                                                pass=nodeSet[u].layNo;

                                            qNode.push(u); //加入队列，等待切割
                                        }

                                        for (int t=0; t<MAXDIMENSIONS; t++){   //更新子节点区间范围
                                            if(dim[t] == 1){
                                                nodeSet[u].field[t].low = lo[t];
                                                nodeSet[u].field[t].high= hi[t];
                                            }else{
                                                nodeSet[u].field[t].low = nodeSet[v].field[t].low;
                                                nodeSet[u].field[t].high= nodeSet[v].field[t].high;
                                            }
                                        }

                                        s = 0;
                                        nodeSet[u].ruleid = (int *)calloc(nodeSet[u].nrules, sizeof(int));
                                        for(int j=0; j<nodeSet[v].nrules; j++){
                                            flag = 1;
                                            for(int t = 0; t < MAXDIMENSIONS; t++){
                                                if(dim[t] == 1){
                                                    if(rule[nodeSet[v].ruleid[j]].field[t].low > hi[t] || rule[nodeSet[v].ruleid[j]].field[t].high < lo[t]){
                                                        flag = 0;
                                                        break;
                                                    }
                                                }
                                            }
                                            if(flag == 1){
                                                nodeSet[u].ruleid[s] = nodeSet[v].ruleid[j];
                                                s++;
                                            }
                                        }
                                    }
                                    else //empty
                                        nodeSet[v].child[index] = Null;

                                    index ++;

                                    lo[4] = hi[4] + 1;  //下一个子节点区间范围
                                    hi[4] = lo[4] + r[4];
                                }
                                lo[3] = hi[3] + 1;
                                hi[3] = lo[3] + r[3];
                            }
                            lo[2] = hi[2] + 1;
                            hi[2] = lo[2] + r[2];
                        }
                        lo[1] = hi[1] + 1;
                        hi[1] = lo[1] + r[1];
                    }
                    lo[0] = hi[0] + 1;
                    hi[0] = lo[0] + r[0];
                }
            }
        } else{ //HyperSplit stage
           if(nodeSet[v].nrules <= binth){  //叶子节点
               nodeSet[v].isleaf = 1;

               Total_Rule_Size+= nodeSet[v].nrules;
               Leaf_Node_Count++;
               if(max_depth<(nodeSet[v].layNo+nodeSet[v].nrules))
                   max_depth=nodeSet[v].layNo+nodeSet[v].nrules;
           }
           else{
               NonLeaf_Node_Count++;
               pc_rule* subset_node;
               subset_node = (pc_rule *)malloc(nodeSet[v].nrules*sizeof(pc_rule));
               for(int m=0;m<nodeSet[v].nrules;m++){
                   subset_node[m].id = nodeSet[v].ruleid[m];
                   for(int k = 0; k < MAXDIMENSIONS; k++){
                       subset_node[m].field[k].low = rule[nodeSet[v].ruleid[m]].field[k].low;
                       //if(subset_node[m].field[k].low < nodeSet[v].field[k].low) subset_node[m].field[k].low = nodeSet[v].field[k].low;
                       subset_node[m].field[k].high = rule[nodeSet[v].ruleid[m]].field[k].high;
                       //if(subset_node[m].field[k].high > nodeSet[v].field[k].high) subset_node[m].field[k].high = nodeSet[v].field[k].high;
                   }
               }

               nodeSet[v].rootnode = (hs_node_t *) malloc(sizeof(hs_node_t));
               hstrie Ttmp(nodeSet[v].nrules,subset_node, binth, nodeSet[v].rootnode);
               //printf("\nline 280 ------------ k=%d   d2s=%u   thresh=0x%8x\n", k, nodeSet[v].rootnode->d2s,nodeSet[v].rootnode->thresh);

               total_hs_memory_in_KB += Ttmp.result.total_mem_kb;

               nodeSet[v].layNo += Ttmp.result.wst_depth;

               if(max_depth<nodeSet[v].layNo)
                   max_depth=nodeSet[v].layNo;

           }

       }

   }


  total_ficuts_memory_in_KB=double(Total_Rule_Size*PTR_SIZE+Total_Array_Size*PTR_SIZE+LEAF_NODE_SIZE*Leaf_Node_Count+TREE_NODE_SIZE*NonLeaf_Node_Count)/1024;
  total_memory_in_KB = total_ficuts_memory_in_KB + total_hs_memory_in_KB;

  /*if(numrules>binth){
      if(k==1)
          printf("\n\t***SA Subset Tree(using FiCuts + HyperSplit):***\n");
      if(k==2)
          printf("\n\t***DA Subset Tree(using FiCuts + HyperSplit):***\n");
      if(k==3)
          printf("\n\t***SA_DA Subset Tree(using FiCuts + HyperSplit):***\n");
      printf(">>RESULTS:");
      printf("\n>>number of rules:%d",numrules);
      printf("\n>>worst case tree level: %d",pass);
      printf("\n>>worst case tree depth: %d",max_depth);
      printf("\n>>total memory(Pre-Cutting): %f(KB)",total_ficuts_memory_in_KB);
      printf("\n>>total memory(Post_Splitting): %f(KB)",total_hs_memory_in_KB);
      printf("\n>>total memory: %f(KB)",total_memory_in_KB);
      printf("\n>>Byte/rule: %f",double(total_memory_in_KB*1024)/numrules);
      printf("\n***SUCCESS in building %d-th CutSplit sub-tree(0_sa, 1_da)***\n\n",k%3);

  }*/

}


unsigned int get_nbits1(unsigned int n)
{
    int		k = 0;

    while (n >>= 1)
        k++;
    return	k;
}
unsigned int get_pow1(unsigned int n)
{
    int		k = 1;

    while (n--)
        k*=2;
    return	k;
}

int trie::trieLookup(int* header){

    int index[MAXDIMENSIONS];
    int cchild, cover;
    int cuts[MAXDIMENSIONS];
    int cvalue[MAXDIMENSIONS];
    int cnode = root;
    int match = 0;
    int nbits[MAXDIMENSIONS];
    int i,j,t;
    int flag_hs = 0;
    int match_id = -1;

    for(i = 0; i< MAXDIMENSIONS; i++){ //初始化每个维度索引长度 sip:32 dip:32 sport:16 dport:16 protocol:8
        if(i == 4) index[i] = 8;
        else if(i >= 2) index[i] = 16;
        else index[i] = 32;
    }

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        for(i = 0; i< MAXDIMENSIONS; i++){nbits[i] = 0;} //初始化各个维度计算子节点索引的比特位数
        for(i = 0; i< MAXDIMENSIONS; i++){
            if(dim[i] == 1){
                cuts[i] = nodeSet[cnode].ncuts[i];//该节点的切割份数
                nbits[i] = get_nbits1(cuts[i]);

                cvalue[i] = 0; //初始化各个维度子节点索引值
                for(t = index[i]; t > index[i] - nbits[i]; t--){
                    if((header[i] & 1<<(t-1)) != 0){
                        cvalue[i] += (int)get_pow1(t-index[i]+nbits[i]-1); //计算每个维度的子节点索引值
                    }
                }
            }else{
                cvalue[i] = 0;
            }
        }

        cchild = 0;
        for(i = 0; i< MAXDIMENSIONS-1; i++){  //计算多维度切割下子节点的索引值  如各维度切割份数分别为 8 4 2 1 1 各维度子节点索引值为 5 3 1 0 0
            for(j = i+1; j < MAXDIMENSIONS; j++){  //则多维度切割下子节点索引值为 5*(4*2*1*1)+3*(2*1*1)+1*(1*1)+0*1+0
                cvalue[i] *= nodeSet[cnode].ncuts[j];
            }
            cchild += cvalue[i];
        }
        cchild += cvalue[MAXDIMENSIONS-1]; //0

        cnode = nodeSet[cnode].child[cchild];  //移动到子节点
        queryCount[0]++;

        if(cnode == Null) break;
        if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
            flag_hs = 1;
            break;
        }

        for(i = 0; i< MAXDIMENSIONS; i++){ //移除前nbits位，下次的子节点索引计算从nbits位后开始
            index[i] -= nbits[i];
        }

    }

    if(cnode != Null && flag_hs == 1){
        match_id = LookupHSTree(rule, nodeSet[cnode].rootnode, header, queryCount);
    }
    if(cnode != Null && flag_hs == 0 && nodeSet[cnode].isleaf == 1){
        for(i = 0; i < nodeSet[cnode].nrules; i++){
            queryCount[0]++;
            cover = 1;
            for(j = 0; j < MAXDIMENSIONS; j++){   //k维都匹配才为match
                if(rule[nodeSet[cnode].ruleid[i]].field[j].low > header[j] ||
                   rule[nodeSet[cnode].ruleid[i]].field[j].high < header[j]){
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
    if(match == 1) {
        match_id = nodeSet[cnode].ruleid[i]; //返回匹配的规则id
    }

    return  match_id;

}