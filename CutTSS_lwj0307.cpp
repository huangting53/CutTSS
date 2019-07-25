#include "CutTSS.h"
using namespace std;

CutTSS::CutTSS(int k1, int threshold1, std::vector<Rule>& classifier){

	k=k1;      //0:SA, 1:DA 切割维度
	threshold = pow(2,threshold1);  //默认24
	for (int i=0; i<MAXDIMENSIONS; i++){
		dim[i] = 0;
		if((k & (1 << i)) > 0) dim[i] = 1;
	}
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

		nodeSet[root].ncuts[i] = 1;
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

void CalcCuts(CutTSSNode* node, int dim[MAXDIMENSIONS], int threshold){
	int done=0;

	for(int i=0;i<MAXDIMENSIONS;i++){
		node->ncuts[i] = 1;
		if(dim[i]){
			done = 0;
			while(!done){
				if(node->ncuts[i] < MAXCUTS && (node->field[i].high - node->field[i].low) > threshold) //大于最大切割份数，或者子空间小于阈值时，不再加倍切割份数
					node->ncuts[i]=node->ncuts[i]*2;
				else
					done=1;
			}
		}
	}
}

void CutTSS::ConstructClassifier(const vector<Rule>& rules)
{
	int nr;
	int empty;
	unsigned int r[MAXDIMENSIONS], lo[MAXDIMENSIONS], hi[MAXDIMENSIONS];
	int u,v;
	int s = 0;
	int i[MAXDIMENSIONS];
	int flag,index;
	
int num_leaf_rule = 0;
int num_tss_rule = 0;

	qNode.push(root);

	while(!qNode.empty()){
		v=qNode.front();
		qNode.pop();

		if(nodeSet[v].flag==1){
			CalcCuts(&nodeSet[v],dim,threshold);
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
num_leaf_rule += nodeSet[v].nrules;
				nodeSet[v].isleaf = 1;
				Total_Rule_Size+= nodeSet[v].nrules;
				Leaf_Node_Count++;
				if(max_depth<(nodeSet[v].depth+nodeSet[v].nrules))
					max_depth=nodeSet[v].depth+nodeSet[v].nrules;
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
									for (const Rule r : nodeSet[v].classifier) {
										flag = 1;
										for(int t = 0; t < MAXDIMENSIONS; t++){
											if(r.range[t][0] > hi[t] || r.range[t][1] < lo[t]){
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
										freelist++;//下一个节点
										//freelist = nodeSet[freelist].child[0]; //下一个节点
										nodeSet[u].nrules = nr;
										nodeSet[u].depth=nodeSet[v].depth+1;
										if(nr <= binth){ //叶子节点
num_leaf_rule += nr;
											nodeSet[u].isleaf = 1;
											Total_Rule_Size+= nr;
											Leaf_Node_Count++;

											if(max_depth<(nodeSet[u].depth+nr))
												max_depth=nodeSet[u].depth+nr;

										}
										else{
											nodeSet[u].isleaf = 0;
											nodeSet[u].flag=1;  //cut

											if(pass<nodeSet[u].depth){
												pass=nodeSet[u].depth;
											}

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

										//更新子节点规则集
										for (const Rule r : nodeSet[v].classifier) {
											flag = 1;
											for(int t = 0; t < MAXDIMENSIONS; t++){
												if(r.range[t][0] > hi[t] || r.range[t][1] < lo[t]){
													flag = 0;
													break;
												}
											}
											if(flag == 1){
												nodeSet[u].classifier.push_back(r);
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
		} else{ //TSS stage
			if(nodeSet[v].nrules <= binth){  //叶子节点
num_leaf_rule += nodeSet[v].nrules;
				nodeSet[v].isleaf = 1;
				Total_Rule_Size+= nodeSet[v].nrules;
				Leaf_Node_Count++;
				if(max_depth<(nodeSet[v].depth+nodeSet[v].nrules))
					max_depth=nodeSet[v].depth+nodeSet[v].nrules;
			}
			else{ //非叶子节点
				NonLeaf_Node_Count++;
num_tss_rule += nodeSet[v].nrules;

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
printf("num_leaf_rule = %d, num_tss_rule=%d\n",num_leaf_rule,num_tss_rule);

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



int Classify(const CutTSSNode* nodeSet, const Packet& packet, int dim[MAXDIMENSIONS], int *queryCount) {

	//queryCount[4]++;

	int cchild, cover;
	int cuts[MAXDIMENSIONS];
	int cvalue[MAXDIMENSIONS];
	int cnode = 1;
	int match = 0;
	int nbits[MAXDIMENSIONS];
	int i,j,t;
	int flag_tss = 0;
	int match_id = -1;

    int index[MAXDIMENSIONS] = {32,32,16,16,8};
	/*for(i = 0; i< MAXDIMENSIONS; i++){ //初始化每个维度索引长度 sip:32 dip:32 sport:16 dport:16 protocol:8
		if(i == 4) index[i] = 8;
		else if(i >= 2) index[i] = 16;
		else index[i] = 32;
	}*/

	while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode

		for(i = 0; i< MAXDIMENSIONS; i++){nbits[i] = 0;} //初始化各个维度计算子节点索引的比特位数
		for(i = 0; i< MAXDIMENSIONS; i++){
			if(dim[i] == 1){
				//cuts[i] = nodeSet[cnode].ncuts[i];//该节点的切割份数 16
				//nbits[i] = get_nbits(cuts[i]);
                nbits[i] = 4;

				cvalue[i] = 0; //初始化各个维度子节点索引值
				for(t = index[i]; t > index[i] - nbits[i]; t--){
					if((packet[i] & 1<<(t-1)) != 0){
						cvalue[i] += (int)pow(2, t-index[i]+nbits[i]-1); //计算每个维度的子节点索引值
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
			flag_tss = 1;
			break;
		}

		for(i = 0; i< MAXDIMENSIONS; i++){ //移除前nbits位，下次的子节点索引计算从nbits位后开始
			index[i] -= nbits[i];
		}

	}

	if(cnode != Null && flag_tss == 1){ //tss stage
		//queryCount[2]++;
		match_id = nodeSet[cnode].ptss.ClassifyAPacket(packet); //priority
	}
	else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //cut stage

		//queryCount[1]++;
/*lee	
		for(const Rule& r : nodeSet[cnode].classifier) {
			queryCount[0]++;
			if (r.MatchesPacket(packet)) {
				match_id = r.priority;
				break;
			}
		}
*/
		

	
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

int CutTSS::ClassifyAPacket(const Packet &packet){
	int p = Classify(nodeSet, packet, dim, queryCount);
	return p;
}

void CutTSS::DeleteRule(const Rule& delete_rule) {
	//TODO
	int index[MAXDIMENSIONS];
	int cchild, cover;
	int cuts[MAXDIMENSIONS];
	int cvalue[MAXDIMENSIONS];
	int cnode = 1;
	int match = 0;
	int nbits[MAXDIMENSIONS];
	int i,j,t;
	int flag_tss = 0;
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
				nbits[i] = get_nbits(cuts[i]);

				cvalue[i] = 0; //初始化各个维度子节点索引值
				for(t = index[i]; t > index[i] - nbits[i]; t--){
					if((delete_rule.range[i][0] & 1<<(t-1)) != 0){
						cvalue[i] += (int)get_pow(t-index[i]+nbits[i]-1); //计算每个维度的子节点索引值
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

		if(cnode == Null) break;
		if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
			flag_tss = 1;
			break;
		}

		for(i = 0; i< MAXDIMENSIONS; i++){ //移除前nbits位，下次的子节点索引计算从nbits位后开始
			index[i] -= nbits[i];
		}

	}
	if(cnode == Null){
		printf("the node is empty, delete failed!\n");
	}
	else if(cnode != Null && flag_tss == 1){
		nodeSet[cnode].ptss.DeleteRule(delete_rule);
	}
	else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){

		if(nodeSet[cnode].nrules > 0){
			for(i=0;i<nodeSet[cnode].classifier.size();i++){
				if(nodeSet[cnode].classifier[i].id == delete_rule.id) break;
			}
			for(j=i;j<(nodeSet[cnode].classifier.size()-1);j++){
				nodeSet[cnode].classifier[j]=nodeSet[cnode].classifier[j+1];
			}
			nodeSet[cnode].classifier.pop_back();

            /*if (i != (nodeSet[cnode].classifier.size()-1))
				nodeSet[cnode].classifier[i] = std::move(nodeSet[cnode].classifier[nodeSet[cnode].classifier.size() - 1]);
			nodeSet[cnode].classifier.pop_back();*/

			//nodeSet[cnode].classifier.erase(find(begin(nodeSet[cnode].classifier), end(nodeSet[cnode].classifier), delete_rule));

			nodeSet[cnode].nrules--;
		}

	}
}

void CutTSS::InsertRule(const Rule& insert_rule) {

	int index[MAXDIMENSIONS];
	int cchild, cover;
	int cuts[MAXDIMENSIONS];
	int cvalue[MAXDIMENSIONS];
	int match = 0;
	int nbits[MAXDIMENSIONS];
	int i,j,t;
	int flag_tss = 0;
	int match_id = -1;
	int v = 1,u;

	for(i = 0; i< MAXDIMENSIONS; i++){ //初始化每个维度索引长度 sip:32 dip:32 sport:16 dport:16 protocol:8
		if(i == 4) index[i] = 8;
		else if(i >= 2) index[i] = 16;
		else index[i] = 32;
	}

	while(nodeSet[v].isleaf != 1){  //找到匹配的叶子节点cnode

		for(i = 0; i< MAXDIMENSIONS; i++){nbits[i] = 0;} //初始化各个维度计算子节点索引的比特位数
		for(i = 0; i< MAXDIMENSIONS; i++){
			if(dim[i] == 1){
				cuts[i] = nodeSet[v].ncuts[i];//该节点的切割份数
				nbits[i] = get_nbits(cuts[i]);

				cvalue[i] = 0; //初始化各个维度子节点索引值
				for(t = index[i]; t > index[i] - nbits[i]; t--){
					if((insert_rule.range[i][0] & 1<<(t-1)) != 0){
						cvalue[i] += (int)get_pow(t-index[i]+nbits[i]-1); //计算每个维度的子节点索引值
					}
				}
			}else{
				cvalue[i] = 0;
			}
		}

		cchild = 0;
		for(i = 0; i< MAXDIMENSIONS-1; i++){  //计算多维度切割下子节点的索引值  如各维度切割份数分别为 8 4 2 1 1 各维度子节点索引值为 5 3 1 0 0
			for(j = i+1; j < MAXDIMENSIONS; j++){  //则多维度切割下子节点索引值为 5*(4*2*1*1)+3*(2*1*1)+1*(1*1)+0*1+0
				cvalue[i] *= nodeSet[v].ncuts[j];
			}
			cchild += cvalue[i];
		}
		cchild += cvalue[MAXDIMENSIONS-1]; //0

		u = nodeSet[v].child[cchild];  //移动到子节点

		if(u == Null) break;
		if(nodeSet[u].flag == 2 && nodeSet[u].isleaf != 1) {
			flag_tss = 1;
			break;
		}

		for(i = 0; i< MAXDIMENSIONS; i++){ //移除前nbits位，下次的子节点索引计算从nbits位后开始
			index[i] -= nbits[i];
		}
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

