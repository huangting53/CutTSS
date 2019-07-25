#include <iostream>
#include "CutTSS.h"
#include <cmath>
using namespace std;

CutTSS::CutTSS(int k1, int threshold1, std::vector<Rule>& classifier){

    k=k1;      //0:SA, 1:DA 切割维度
    threshold = pow(2,threshold1);  //默认24
//    for (int i=0; i<MAXDIMENSIONS; i++){
//        dim[i] = 0;
//        if((k & (1 << i)) > 0) dim[i] = 1;
//    }
    this->rules = classifier;
    numrules = classifier.size();
    nodeSet = new CutTSSNode[MAXNODES+1];
    root = 1;

    pass=1;
    max_depth=0;
    Total_Rule_Size=0;
    Total_Array_Size=0;
    Leaf_Node_Count=0;
    NonLeaf_Node_Count=0;
    Total_Tuple_Count=0;
    total_ficuts_memory_in_KB=0;
    total_tss_memory_in_KB=0;
    total_memory_in_KB=0;


    for(int i=1; i<=MAXNODES; i++) {
        nodeSet[i].child = (int*)malloc(sizeof(int));
        nodeSet[i].ntuples = 0;
    }

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

    }

    nodeSet[root].classifier = classifier;

    nodeSet[root].depth = 1; //层数
    nodeSet[root].flag = 1; //cut
    nodeSet[root].ntuples = 0;

    freelist = 2;
    /*for (int i = 2; i < MAXNODES; i++)
        nodeSet[i].child[0] = i+1;
    nodeSet[MAXNODES].child[0] = Null;*/
}

CutTSS::~CutTSS() {
    delete [] nodeSet;
}

void CalcCuts(CutTSSNode* node, int k, int threshold){
    int done=0;

    while(!done){
        if(node->ncuts < MAXCUTS && (node->field[k].high - node->field[k].low) > threshold) //大于最大切割份数，或者子空间小于阈值时，不再加倍切割份数
            node->ncuts=node->ncuts*2;
        else
            done=1;
    }

}

void CutTSS::ConstructClassifier(const vector<Rule>& rules)
{
    int nr;
    int empty;
    int u,v;
    int s = 0;
    unsigned int r1, lo1, hi1;
    int flag;

    qNode.push(root);

    while(!qNode.empty()){
        v=qNode.front();
        qNode.pop();

        if(nodeSet[v].flag==1){
            CalcCuts(&nodeSet[v],k,threshold);
            if(nodeSet[v].ncuts < MAXCUTS) nodeSet[v].flag=2;  //tss
        }

        if(nodeSet[v].flag==1) //FiCuts stage
        {
            if(nodeSet[v].nrules <= binth){ //叶子节点
                nodeSet[v].isleaf = 1;
                Total_Rule_Size+= nodeSet[v].nrules;
                Leaf_Node_Count++;
                if(max_depth<(nodeSet[v].depth+nodeSet[v].nrules))
                    max_depth=nodeSet[v].depth+nodeSet[v].nrules;
            }
            else{  //非叶子节点
                NonLeaf_Node_Count++;
                //printf("\n>> trie.cpp line 395 ------------ nodeSet[v].ncuts  %d  %d \n", nodeSet[v].ncuts[0],nodeSet[v].ncuts[1]);
                nodeSet[v].child = (int *)realloc(nodeSet[v].child, nodeSet[v].ncuts * sizeof(int));

                Total_Array_Size += nodeSet[v].ncuts;

                r1 = (nodeSet[v].field[k].high - nodeSet[v].field[k].low)/nodeSet[v].ncuts;
                lo1 = nodeSet[v].field[k].low;
                hi1 = lo1 + r1;

                for(int i = 0; i < nodeSet[v].ncuts; i++) {

                    empty = 1;
                    nr = 0;
                    for (const Rule r : nodeSet[v].classifier) {
                        flag = 1;

                        if (r.range[k][0] > hi1 || r.range[k][1] < lo1) {
                                flag = 0;
                        }

                        if (flag == 1) {
                            empty = 0;
                            nr++;
                        }
                    }

                    if (!empty) {
                        nodeSet[v].child[i] = freelist;
                        u = freelist;
                        freelist++;//下一个节点
                        //freelist = nodeSet[freelist].child[0]; //下一个节点
                        nodeSet[u].nrules = nr;
                        nodeSet[u].depth = nodeSet[v].depth + 1;
                        if (nr <= binth) { //叶子节点
                            nodeSet[u].isleaf = 1;
                            Total_Rule_Size += nr;
                            Leaf_Node_Count++;

                            if (max_depth < (nodeSet[u].depth + nr))
                                max_depth = nodeSet[u].depth + nr;

                        } else {
                            nodeSet[u].isleaf = 0;
                            nodeSet[u].flag = 1;  //cut

                            if (pass < nodeSet[u].depth) {
                                pass = nodeSet[u].depth;
                            }

                            qNode.push(u); //加入队列，等待切割
                        }

                        for (int t = 0; t < MAXDIMENSIONS; t++) {   //更新子节点区间范围
                            if (t == k) {
                                nodeSet[u].field[t].low = lo1;
                                nodeSet[u].field[t].high = hi1;
                            } else {
                                nodeSet[u].field[t].low = nodeSet[v].field[t].low;
                                nodeSet[u].field[t].high = nodeSet[v].field[t].high;
                            }
                        }

                        //更新子节点规则集
                        for (const Rule r : nodeSet[v].classifier) {
                            flag = 1;

                            if (r.range[k][0] > hi1 || r.range[k][1] < lo1) {
                                flag = 0;
                            }

                            if (flag == 1) {
                                nodeSet[u].classifier.push_back(r);
                            }
                        }

                    } else //empty
                        nodeSet[v].child[i] = Null;

                    lo1 = hi1 + 1;
                    hi1 = lo1 + r1;
                }
            }
        } else{ //TSS stage
            if(nodeSet[v].nrules <= binth){  //叶子节点
                nodeSet[v].isleaf = 1;
                Total_Rule_Size+= nodeSet[v].nrules;
                Leaf_Node_Count++;
                if(max_depth<(nodeSet[v].depth+nodeSet[v].nrules))
                    max_depth=nodeSet[v].depth+nodeSet[v].nrules;
            }
            else{ //非叶子节点
                NonLeaf_Node_Count++;

                nodeSet[v].ptss.ConstructClassifier(nodeSet[v].classifier);
                nodeSet[v].ntuples = nodeSet[v].ptss.NumTables();
                nodeSet[v].depth += nodeSet[v].ntuples; //+= tuplelist.num
                Total_Tuple_Count += nodeSet[v].ntuples;
                total_tss_memory_in_KB += nodeSet[v].ptss.MemSizeBytes();

                if(max_depth<nodeSet[v].depth)
                    max_depth=nodeSet[v].depth;

            }

        }

    }

}

unsigned int get_nbits(unsigned int n)
{
    int		k = 0;

    while (n >>= 1)
        k++;
    return	k;
}
unsigned int get_pow(unsigned int n)
{
    int		k = 1;

    while (n--)
        k*=2;
    return	k;
}


int CutTSS::ClassifyAPacket(const Packet &packet){
    //queryCount[4]++;

    int cchild, cover;
    int cnode = 1;
    int match = 0;
    int nbits;
    int i,j,t;
    int flag_tss = 0;
    int match_id = -1;

    int index[MAXDIMENSIONS] = {32,32,16,16,8};

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        //nbits = get_nbits(nodeSet[cnode].ncuts);
        nbits = 4;
        cchild = 0;
        for(i = index[k]; i > index[k] - nbits; i--){   //计算子节点索引cchild 如高位10为2^1，第2个子节点；如高位11为2^1+2^0，第3个子节点
            if((packet[k] & 1<<(i-1)) != 0){
                cchild += (int)pow(2,i-index[k]+nbits-1);
            }
        }

        cnode = nodeSet[cnode].child[cchild];  //移动到子节点
        queryCount[0]++;

        if(cnode == Null) break;
        if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
            flag_tss = 1;
            break;
        }

        index[k] -= nbits;  //移除前nbits位，下次的子节点索引计算从nbits位后开始

    }

    if(cnode != Null && flag_tss == 1){ //tss stage
        //queryCount[2]++;
        match_id = nodeSet[cnode].ptss.ClassifyAPacket(packet); //priority
    }
    else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //cut stage

        //queryCount[1]++;

//        for(const Rule& r : nodeSet[cnode].classifier) {
//            queryCount[0]++;
//            if (r.MatchesPacket(packet)) {
//                match_id = r.priority;
//                break;
//            }
//        }
        int n = nodeSet[cnode].classifier.size();
        for(int i = 0; i < n; i++){
            queryCount[0]++;
            if(packet[0] >= nodeSet[cnode].classifier[i].range[0][LowDim] && packet[0] <= nodeSet[cnode].classifier[i].range[0][HighDim] &&
               packet[1] >= nodeSet[cnode].classifier[i].range[1][LowDim] && packet[1] <= nodeSet[cnode].classifier[i].range[1][HighDim] &&
               packet[2] >= nodeSet[cnode].classifier[i].range[2][LowDim] && packet[2] <= nodeSet[cnode].classifier[i].range[2][HighDim] &&
               packet[3] >= nodeSet[cnode].classifier[i].range[3][LowDim] && packet[3] <= nodeSet[cnode].classifier[i].range[3][HighDim] &&
               packet[4] >= nodeSet[cnode].classifier[i].range[4][LowDim] && packet[4] <= nodeSet[cnode].classifier[i].range[4][HighDim]){
                match_id = nodeSet[cnode].classifier[i].priority;
                break;
            }
        }

    }

    return  match_id;
}

void CutTSS::DeleteRule(const Rule& delete_rule) {
    //TODO

    int cchild, cover;
    int cnode = 1;
    int match = 0;
    int nbits;
    int i,j,t;
    int flag_tss = 0;
    int match_id = -1;

    int index[MAXDIMENSIONS] = {32,32,16,16,8};

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode
        //nbits = get_nbits(nodeSet[cnode].ncuts);
        nbits = 4;
        cchild = 0;
        for(i = index[k]; i > index[k] - nbits; i--){   //计算子节点索引cchild 如高位10为2^1，第2个子节点；如高位11为2^1+2^0，第3个子节点
            if((delete_rule.range[k][0] & 1<<(i-1)) != 0){
                cchild += (int)pow(2,i-index[k]+nbits-1);
            }
        }

        cnode = nodeSet[cnode].child[cchild];  //移动到子节点

        if(cnode == Null) break;
        if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
            flag_tss = 1;
            break;
        }

        index[k] -= nbits;  //移除前nbits位，下次的子节点索引计算从nbits位后开始

    }
   if(cnode != Null && flag_tss == 1){
        nodeSet[cnode].ptss.DeleteRule(delete_rule);
    }
    else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){

        if(nodeSet[cnode].nrules > 0){
            for(i=0;i<nodeSet[cnode].classifier.size();i++){
                if(nodeSet[cnode].classifier[i].id == delete_rule.id) break;
            }
            /*for(j=i;j<(nodeSet[cnode].classifier.size()-1);j++){
                nodeSet[cnode].classifier[j]=nodeSet[cnode].classifier[j+1];
            }
            nodeSet[cnode].classifier.pop_back();*/

            nodeSet[cnode].classifier.erase(nodeSet[cnode].classifier.begin()+i);

            nodeSet[cnode].nrules--;
        }

    }
}

void CutTSS::InsertRule(const Rule& insert_rule) {

    int cchild, cover;
    int match = 0;
    int nbits;
    int i,j,t;
    int flag_tss = 0;
    int match_id = -1;
    int v = 1,u;

    int index[MAXDIMENSIONS] = {32,32,16,16,8};

    while(nodeSet[v].isleaf != 1){  //找到匹配的叶子节点cnode

        //nbits = get_nbits(nodeSet[cnode].ncuts);
        nbits = 4;
        cchild = 0;
        for(i = index[k]; i > index[k] - nbits; i--){   //计算子节点索引cchild 如高位10为2^1，第2个子节点；如高位11为2^1+2^0，第3个子节点
            if((insert_rule.range[k][0] & 1<<(i-1)) != 0){
                cchild += (int)pow(2,i-index[k]+nbits-1);
            }
        }

        u = nodeSet[v].child[cchild];  //移动到子节点

        if(u == Null) break;
        if(nodeSet[u].flag == 2 && nodeSet[u].isleaf != 1) {
            flag_tss = 1;
            break;
        }

        index[k] -= nbits;  //移除前nbits位，下次的子节点索引计算从nbits位后开始

        v = u;

    }
    if(u == Null){
        printf("the node is empty, create a new node!\n");

        nodeSet[v].child[cchild] = freelist;
        u=freelist;
        freelist++;//下一个节点
        nodeSet[u].nrules = 1;
        nodeSet[u].classifier.push_back(insert_rule);
        nodeSet[u].flag = 1;
        nodeSet[u].isleaf = 1;

    }
    else if(u != Null && flag_tss == 1){
        nodeSet[u].ptss.InsertRule(insert_rule);
    }
    else if(u != Null && flag_tss == 0 && nodeSet[u].isleaf == 1){

        nodeSet[u].nrules++;
        nodeSet[u].classifier.push_back(insert_rule);

        if(nodeSet[u].nrules > binth){ //TODO cut or tss???
            nodeSet[u].isleaf = 0;
            nodeSet[u].flag = 2;
            nodeSet[u].ptss.ConstructClassifier(nodeSet[u].classifier);
        }

    }

}

