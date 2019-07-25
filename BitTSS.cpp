#include <iostream>
#include <stdlib.h>
#include <cmath>
#include "BitTSS.h"
using namespace std;

BitTSS::BitTSS(int k1, int threshold1, std::vector<Rule> &classifier){

    k = k1;      //0:SA, 1:DA
    threshold = threshold1;  //默认24
    this->rules = classifier;
    numrules = classifier.size();
    nodeSet = new BitTSSNode[MAXNODES+1];
    root = 1;

    pass=1;
    max_depth=0;
    Total_Rule_Size=0;
    Total_Array_Size=0;
    Leaf_Node_Count=0;
    NonLeaf_Node_Count=0;
    Total_Tuple_Count=0;
    total_bit_memory_in_KB=0;
    total_tss_memory_in_KB=0;
    total_memory_in_KB=0;

    //初始化根节点信息
    nodeSet[root].depth = 1; //层数
    nodeSet[root].isleaf = 0;
    nodeSet[root].nrules = numrules;
    nodeSet[root].classifier = classifier;
    nodeSet[root].flag = 1; //bit
    nodeSet[root].ntuples = 0;

    freelist = 2;
    /*for (int i = 2; i < MAXNODES; i++)
        nodeSet[i].child[0] = i+1;
    nodeSet[MAXNODES].child[0] = Null;*/
}

BitTSS::~BitTSS(){
    delete [] nodeSet;
}

//strategy1: select one best bit
int BitSelect1(BitTSSNode* node, int k, int threshold){

    int minDiff = node->nrules;
    int count_left_child = 0,count_right_child = 0;
    int bitSelect = -1;

    for(int i=0;i<32-threshold;i++){
        if(node->bitFlag[i] == 0){
            count_left_child = 0;
            count_right_child = 0;
            for(int j=0;j<node->nrules;j++){
                if(((node->classifier[j].range[k][0] >> (31-i)) & 1) == 0) count_left_child++;
                else count_right_child++;
            }
            int absDiff = abs(count_right_child - count_left_child);
            if(minDiff > absDiff){
                minDiff = absDiff;
                bitSelect = i;
            }
        }
    }
    if(bitSelect != -1){
        node->sbit = bitSelect;
        node->bitFlag[bitSelect] = 1;
    }

    return bitSelect;
}

//strategy2: select top b optimal bits
int BitSelect2(BitTSSNode* node, int k, int threshold){

    int minDiff = node->nrules;
    int count_left_child = 0,count_right_child = 0;
    int bitSelect = -1;
    int flag = -1;

    for(int t=0;t<maxBits;t++){
        bitSelect = -1;
        minDiff = node->nrules;
        for(int i=0;i<32-threshold;i++){
            if(node->bitFlag[i] == 0){
                count_left_child = 0;
                count_right_child = 0;
                for(int j=0;j<node->nrules;j++){
                    if(((node->classifier[j].range[k][0] >> (31-i)) & 1) == 0) count_left_child++;
                    else count_right_child++;
                }
                int absDiff = abs(count_right_child - count_left_child);
                if(minDiff >= absDiff){
                    minDiff = absDiff;
                    bitSelect = i;
                    flag = 1;
                }
            }
        }
        if(bitSelect != -1){
            node->sbits[t] = bitSelect;
            node->bitFlag[bitSelect] = 1;
            node->ncuts = node->ncuts * 2;
        }
    }

//    for(int t=0;t<maxBits;t++){
//        bitSelect = -1;
//        minDiff = node->nrules;
//        for(int i=0;i<32-threshold;i++){
//            if(node->bitFlag[i] == 0){
//                bitSelect = i;
//                flag = 1;
//                break;
//            }
//        }
//        if(bitSelect != -1){
//            node->sbits[t] = bitSelect;
//            node->bitFlag[bitSelect] = 1;
//            node->ncuts = node->ncuts * 2;
//        }
//    }

    return flag;
}

//Compute Standard Deviation
double costFunc(BitTSSNode* node, int *cur_sbits, int k){
    double variance = 0, stdDev = 0;
    int maxchilds = (int)pow(2,maxBits);
    int numRules[maxchilds] = {0};
    double meanValue = node->nrules/(double)maxchilds;
    //printf("nrules = %d  meanValue = %f\n",node->nrules,meanValue);
    for(const Rule& r : node->classifier){
        int index = 0;
        int cnode = 0;
        for(int j = 0;j < maxBits;j++){
            cnode += ((r.range[k][0] >> (31-cur_sbits[j])) & 1) * ((int)pow(2,index));
            index++;
        }
        numRules[cnode]++;
    }
    /*int numchilds = 0;
    for(int i = 0;i < maxchilds; i++){
        if(numRules[i] > 0) numchilds++;
    }*/

    for(int i = 0;i < maxchilds; i++){
        //if(numRules[i] > 0)
        variance += (numRules[i]-meanValue)*(numRules[i]-meanValue);
    }
    variance = variance/maxchilds;
    stdDev = sqrt(variance);
    //printf("variance = %f stdDev = %f \n",variance,stdDev);

    return stdDev;
}

//strategy3: violence algorithm to find the best b bits
int BitSelect3(BitTSSNode* node, int k, int threshold){

    double minStdDev = MAXNODES; //TODO
    double curCost = 0;
    int flag = -1;
    for(int i=0;i<32-threshold-3;i++){
        if(node->bitFlag[i]==0){
            for(int j=i+1;j<32-threshold-2;j++){
                if(node->bitFlag[j]==0){
                    for(int h=j+1;h<32-threshold-1;h++){
                        if(node->bitFlag[h]==0){
                            for(int t=h+1;t<32-threshold;t++){
                                if(node->bitFlag[t]==0){
                                    int cur_sbits[maxBits] = {i,j,h,t};
                                    curCost = costFunc(node,cur_sbits,k);
                                    if(curCost < minStdDev){
                                        flag = 1;
                                        minStdDev = curCost;
                                        memcpy(node->sbits,cur_sbits, sizeof(node->sbits));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }
    for(int h=0;h<maxBits;h++){
        node->bitFlag[node->sbits[h]] = 1;
        node->ncuts = node->ncuts * 2;
    }

    return flag;
}


//strategy1: select one best bit
/*void BitTSS::ConstructClassifier(const vector<Rule>& rules)
{
    int u,v;
    int left_child_count = 0,right_child_count = 0;
    int bitSelect;

    qNode.push(root);

    while(!qNode.empty()){
        v=qNode.front();
        qNode.pop();

        if(nodeSet[v].flag==1){
            if(nodeSet[v].depth > (32-threshold)) nodeSet[v].flag = 2;//tss   (32-threshold)          //TODO
            else{
                bitSelect = BitSelect1(&nodeSet[v],k,threshold); //bit
                if(bitSelect == -1) { //tss
                    nodeSet[v].flag = 2;
                    //printf("bitSelect = %d\n",bitSelect);
                }

            }
        }

        if(nodeSet[v].flag==1) //bit stage
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
                Total_Array_Size += 2;
                //printf("nodeSet[v].sbit = %d\n",nodeSet[v].sbit);

                left_child_count = 0;
                right_child_count = 0;
                for (const Rule r : nodeSet[v].classifier) {
                    if(((r.range[k][0] >> (31-nodeSet[v].sbit)) & 1) == 0) left_child_count++;
                    else right_child_count++;
                }
                //left child
                if(left_child_count > 0){
                    nodeSet[v].child0 = freelist;
                    u=freelist;
                    freelist++;//下一个节点
                    //freelist = nodeSet[freelist].child[0]; //下一个节点
                    nodeSet[u].nrules = left_child_count;
                    nodeSet[u].depth=nodeSet[v].depth+1;
                    if(left_child_count <= binth) { //叶子节点
                        nodeSet[u].isleaf = 1;
                        Total_Rule_Size += left_child_count;
                        Leaf_Node_Count++;

                        if (max_depth < (nodeSet[u].depth + left_child_count))
                            max_depth = nodeSet[u].depth + left_child_count;
                    }
                    else{
                        nodeSet[u].isleaf = 0;
                        nodeSet[u].flag=1;  //bit

                        if(pass<nodeSet[u].depth){
                            pass=nodeSet[u].depth;
                        }

                        qNode.push(u); //加入队列，等待切割
                    }

                    //更新子节点规则集
                    for (const Rule r : nodeSet[v].classifier) {
                        if(((r.range[k][0] >> (31-nodeSet[v].sbit)) & 1) == 0) nodeSet[u].classifier.push_back(r);
                    }
                    //更新子节点bigFlag
                    memcpy(nodeSet[u].bitFlag, nodeSet[v].bitFlag, sizeof(nodeSet[u].bitFlag));

                }else //empty
                    nodeSet[v].child0 = Null;

                //right child
                if(right_child_count > 0){
                    nodeSet[v].child1 = freelist;
                    u=freelist;
                    freelist++;//下一个节点
                    //freelist = nodeSet[freelist].child[0]; //下一个节点
                    nodeSet[u].nrules = right_child_count;
                    nodeSet[u].depth=nodeSet[v].depth+1;
                    if(right_child_count <= binth) { //叶子节点
                        nodeSet[u].isleaf = 1;
                        Total_Rule_Size += right_child_count;
                        Leaf_Node_Count++;

                        if (max_depth < (nodeSet[u].depth + right_child_count))
                            max_depth = nodeSet[u].depth + right_child_count;
                    }
                    else{
                        nodeSet[u].isleaf = 0;
                        nodeSet[u].flag=1;  //bit

                        if(pass<nodeSet[u].depth){
                            pass=nodeSet[u].depth;
                        }

                        qNode.push(u); //加入队列，等待切割
                    }

                    //更新子节点规则集
                    for (const Rule r : nodeSet[v].classifier) {
                        if(((r.range[k][0] >> (31-nodeSet[v].sbit)) & 1) == 1) nodeSet[u].classifier.push_back(r);
                    }
                    //更新子节点bigFlag
                    memcpy(nodeSet[u].bitFlag, nodeSet[v].bitFlag, sizeof(nodeSet[u].bitFlag));

                }else //empty
                    nodeSet[v].child1 = Null;

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
                Total_Tuple_Count += nodeSet[v].ntuples;
                total_tss_memory_in_KB += nodeSet[v].ptss.MemSizeBytes();

                if(max_depth < (nodeSet[v].depth + nodeSet[v].ntuples))
                    max_depth = nodeSet[v].depth + nodeSet[v].ntuples; //+= tuplelist.num

            }

        }

    }

}*/

//strategy2: select top b optimal bits
//strategy3: violence search to find the best b bits
void BitTSS::ConstructClassifier(const vector<Rule>& rules)
{
    int u,v;
    int flag;
    int nr;
    int empty;

    qNode.push(root);

    while(!qNode.empty()){
        v=qNode.front();
        qNode.pop();

        if(nodeSet[v].flag==1){ //((32-threshold)/4)
            if(nodeSet[v].depth > ((32-threshold)/4)) nodeSet[v].flag = 2;//tss   (32-threshold)          //TODO
            else{
                flag = BitSelect2(&nodeSet[v],k,threshold); //bit
                if(flag == -1) { //tss
                    nodeSet[v].flag = 2;
                    //printf("bitSelect = %d\n",bitSelect);
                }

            }
        }

        if(nodeSet[v].flag==1) //bit stage
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
                Total_Array_Size += nodeSet[v].ncuts;
                //printf("nodeSet[v].sbit = %d\n",nodeSet[v].sbit);
                //printf("nodeSet[v].ncuts = %d\n",nodeSet[v].ncuts);
                nodeSet[v].child = (int *)malloc(sizeof(int)*nodeSet[v].ncuts);

                for(int i = 0;i < nodeSet[v].ncuts; i++){
                    nodeSet[v].child[i] = freelist;
                    u = freelist;
                    freelist++;

                    for(const Rule& r : nodeSet[v].classifier){
                        int index = 0;
                        int cnode = 0;
                        for(int j = 0;j < maxBits;j++){
                            if(nodeSet[v].sbits[j] != -1){
                                cnode += ((r.range[k][0] >> (31-nodeSet[v].sbits[j])) & 1) * ((int)pow(2,index));
                                index++;
                            }
                        }
                        if(cnode == i){
                            nodeSet[u].classifier.push_back(r);
                        }
                    }
                    nodeSet[u].nrules = nodeSet[u].classifier.size();
                    if(nodeSet[u].nrules > 0){
                        nodeSet[u].depth = nodeSet[v].depth+1;

                        if(nodeSet[u].nrules <= binth) { //叶子节点
                            nodeSet[u].isleaf = 1;
                            Total_Rule_Size += nodeSet[u].nrules;
                            Leaf_Node_Count++;

                            if (max_depth < (nodeSet[u].depth + nodeSet[u].nrules))
                                max_depth = nodeSet[u].depth + nodeSet[u].nrules;
                        }
                        else{
                            nodeSet[u].isleaf = 0;
                            nodeSet[u].flag=1;  //bit

                            if(pass<nodeSet[u].depth){
                                pass=nodeSet[u].depth;
                            }

                            qNode.push(u); //加入队列，等待切割
                        }

                        //更新子节点bigFlag
                        memcpy(nodeSet[u].bitFlag, nodeSet[v].bitFlag, sizeof(nodeSet[u].bitFlag));

                    }else //empty
                        nodeSet[v].child[i] = Null;


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
                Total_Tuple_Count += nodeSet[v].ntuples;
                total_tss_memory_in_KB += nodeSet[v].ptss.MemSizeBytes();

                if(max_depth < (nodeSet[v].depth + nodeSet[v].ntuples))
                    max_depth = nodeSet[v].depth + nodeSet[v].ntuples; //+= tuplelist.num

            }

        }

    }

}

//strategy1: select one best bit
/*int BitTSS::ClassifyAPacket(const Packet &packet){
    //queryCount[4]++;

    int cnode = 1;
    int flag_tss = 0;
    int match_id = -1;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        if(((packet[k] >> (31-nodeSet[cnode].sbit)) & 1) == 0) cnode = nodeSet[cnode].child0;
        else cnode = nodeSet[cnode].child1;

        queryCount[0]++;

        if(nodeSet[cnode].flag == 2) {
            flag_tss = 1;
            break;
        }
    }

    if(cnode != Null && flag_tss == 1){ //tss stage
        //queryCount[2]++;
        match_id = nodeSet[cnode].ptss.ClassifyAPacket(packet); //priority
    }
    else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //bitSelect stage

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
}*/

//strategy2: select top k best bits
//strategy3: violence search to find the best k bits
int BitTSS::ClassifyAPacket(const Packet &packet){
    //queryCount[4]++;

    int cnode = 1;
    int flag_tss = 0;
    int match_id = -1;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        int index = 0;
        int cchild = 0;
//        for(int j = 0;j < maxBits;j++){
//            if(nodeSet[cnode].sbits[j] != -1){
//                cchild += ((packet[k] >> (31-nodeSet[cnode].sbits[j])) & 1) * ((int)pow(2,index));
//                index++;
//            }
//        }

        cchild = ((packet[k] >> (31-nodeSet[cnode].sbits[0])) & 1) + ((packet[k] >> (31-nodeSet[cnode].sbits[1])) & 1) * 2
                + ((packet[k] >> (31-nodeSet[cnode].sbits[2])) & 1) * 4 + ((packet[k] >> (31-nodeSet[cnode].sbits[3])) & 1) * 8;

        cnode = nodeSet[cnode].child[cchild];
        queryCount[0]++;

        if(cnode == Null) break;
        if(nodeSet[cnode].flag == 2) {
            flag_tss = 1;
            break;
        }
    }

    if(cnode != Null && flag_tss == 1){ //tss stage
        //queryCount[2]++;
        match_id = nodeSet[cnode].ptss.ClassifyAPacket(packet); //priority
    }
    else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //bitSelect stage

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

/*//strategy1: select one best bit
void BitTSS::DeleteRule(const Rule& delete_rule) {
    //TODO
    int cnode = 1;
    int flag_tss = 0;
    int i,j;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        if(((delete_rule.range[k][0] >> (31-nodeSet[cnode].sbit)) & 1) == 0) cnode = nodeSet[cnode].child0;
        else cnode = nodeSet[cnode].child1;

        if(nodeSet[cnode].flag == 2) {
            flag_tss = 1;
            break;
        }
    }
    //cnode != Null && flag_tss == 1
    if(flag_tss){
        nodeSet[cnode].ptss.DeleteRule(delete_rule);
    }
    //cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1
    else {
        if(nodeSet[cnode].nrules > 0){
            int size = nodeSet[cnode].classifier.size();
            for(i=0;i<size;i++){
                if(nodeSet[cnode].classifier[i].id == delete_rule.id) break;
            }
            nodeSet[cnode].classifier.erase(nodeSet[cnode].classifier.begin()+i);
            nodeSet[cnode].nrules--;
        }
    }
}
//strategy1: select one best bit
void BitTSS::InsertRule(const Rule& insert_rule) {

    int cnode = 1;
    int flag_tss = 0;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        if(((insert_rule.range[k][0] >> (31-nodeSet[cnode].sbit)) & 1) == 0) cnode = nodeSet[cnode].child0;
        else cnode = nodeSet[cnode].child1;

        if(nodeSet[cnode].flag == 2) {
            flag_tss = 1;
            break;
        }
    }

    if(cnode != Null && flag_tss == 1){ //tss stage
        nodeSet[cnode].ptss.InsertRule(insert_rule);
    }
    else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //bitSelect stage

        nodeSet[cnode].nrules++;
        nodeSet[cnode].classifier.push_back(insert_rule);

        if(nodeSet[cnode].nrules > binth){ //TODO bitSelect or tss???
            //printf("new tss node!\n");
            nodeSet[cnode].isleaf = 0;
            nodeSet[cnode].flag = 2;
            nodeSet[cnode].ptss.ConstructClassifier(nodeSet[cnode].classifier);
        }

    }

}*/

//strategy2/3
void BitTSS::DeleteRule(const Rule& delete_rule) {
    //TODO
    int cnode = 1;
    int flag_tss = 0;
    int i,j;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        int index = 0;
        int cchild = 0;
//        for(int j = 0;j < maxBits;j++){
//            if(nodeSet[cnode].sbits[j] != -1){
//                cchild += ((packet[k] >> (31-nodeSet[cnode].sbits[j])) & 1) * ((int)pow(2,index));
//                index++;
//            }
//        }

        cchild = ((delete_rule.range[k][0] >> (31-nodeSet[cnode].sbits[0])) & 1) + ((delete_rule.range[k][0] >> (31-nodeSet[cnode].sbits[1])) & 1) * 2
                 + ((delete_rule.range[k][0] >> (31-nodeSet[cnode].sbits[2])) & 1) * 4 + ((delete_rule.range[k][0] >> (31-nodeSet[cnode].sbits[3])) & 1) * 8;

        cnode = nodeSet[cnode].child[cchild];

        if(nodeSet[cnode].flag == 2) {
            flag_tss = 1;
            break;
        }
    }
    //cnode != Null && flag_tss == 1
    if(flag_tss){
        nodeSet[cnode].ptss.DeleteRule(delete_rule);
    }
    //cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1
    else {
        if(nodeSet[cnode].nrules > 0){
            int size = nodeSet[cnode].classifier.size();
            for(i=0;i<size;i++){
                if(nodeSet[cnode].classifier[i].id == delete_rule.id) break;
            }
            nodeSet[cnode].classifier.erase(nodeSet[cnode].classifier.begin()+i);
            nodeSet[cnode].nrules--;
        }
    }
}
//strategy2/3
void BitTSS::InsertRule(const Rule& insert_rule) {

    int cnode = 1;
    int flag_tss = 0;
    int cchild = 0;
    int u;

    while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

        int index = 0;
        cchild = 0;
//        for(int j = 0;j < maxBits;j++){
//            if(nodeSet[cnode].sbits[j] != -1){
//                cchild += ((insert_rule[k] >> (31-nodeSet[cnode].sbits[j])) & 1) * ((int)pow(2,index));
//                index++;
//            }
//        }

        cchild = ((insert_rule.range[k][0] >> (31-nodeSet[cnode].sbits[0])) & 1) + ((insert_rule.range[k][0] >> (31-nodeSet[cnode].sbits[1])) & 1) * 2
                 + ((insert_rule.range[k][0] >> (31-nodeSet[cnode].sbits[2])) & 1) * 4 + ((insert_rule.range[k][0] >> (31-nodeSet[cnode].sbits[3])) & 1) * 8;

        u = nodeSet[cnode].child[cchild];

        if(u == Null) break;
        if(nodeSet[u].flag == 2) {
            flag_tss = 1;
            break;
        }

        cnode = u;
    }
    if(u == Null){
        nodeSet[cnode].child[cchild] = freelist;
        u = freelist;
        freelist++;
        nodeSet[u].classifier.push_back(insert_rule);
        nodeSet[u].nrules++;
    }
    //u != Null && flag_tss == 1
    else if(flag_tss){ //tss stage
        nodeSet[u].ptss.InsertRule(insert_rule);
    }
    //u != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1
    else { //bitSelect stage

        nodeSet[cnode].nrules++;
        nodeSet[cnode].classifier.push_back(insert_rule);

        if(nodeSet[cnode].nrules > binth){ //TODO tss
            //printf("new tss node!\n");
            nodeSet[cnode].isleaf = 0;
            nodeSet[cnode].flag = 2;
            nodeSet[cnode].ptss.ConstructClassifier(nodeSet[cnode].classifier);
        }

    }

}