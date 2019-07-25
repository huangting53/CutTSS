

int CutTSS::ClassifyAPacket(const Packet &packet){

	int cnode = 1;
	int flag_tss = 0;
	int match_id = -1;	
	unsigned int cchild = 0;
	unsigned int numb = 32;

	switch(speedUpFlag) // speedUpFlag=3:sa&da,1:sa, 2:da, others:for !5-tuple rules
	{		
		case 1:{
			while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode
				numb -= 4;
				cchild = (packet[0]>>numb) & 15;	
				cnode = nodeSet[cnode].child[cchild];  //移动到子节点
				queryCount[0]++;
				
				if(cnode == Null) break;
				if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
					flag_tss = 1;
					break;
				}					
			}
			break;
		}
		case 2:{
			while(nodeSet[cnode].isleaf != 1){  //找到匹配的叶子节点cnode
				numb -= 4;
				cchild = (packet[1]>>numb) & 15;
				cnode = nodeSet[cnode].child[cchild];  //移动到子节点
				queryCount[0]++;
				
				if(cnode == Null) break;
				if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
					flag_tss = 1;
					break;
				}					
			}
			break;
			
		}
		case 3:{
			while(nodeSet[cnode].isleaf != 1){   //找到匹配的叶子节点cnode
				numb -= 4;
				cchild = ((packet[0]>>numb) & 15)*nodeSet[cnode].ncuts[1] + ((packet[1]>>numb) & 15);
				cnode = nodeSet[cnode].child[cchild];  //移动到子节点
				queryCount[0]++;
				if(cnode == Null) break;
				if(nodeSet[cnode].flag == 2 && nodeSet[cnode].isleaf != 1) {
					flag_tss = 1;
					break;
				}							
			}
			break;		

		}
		default: break;
			
	}


	if(cnode != Null && flag_tss == 1){ //tss stage
		//queryCount[2]++;
		match_id = nodeSet[cnode].ptss.ClassifyAPacket(packet); //priority
	}
	else if(cnode != Null && flag_tss == 0 && nodeSet[cnode].isleaf == 1){ //cut stage
		//queryCount[1]++;
		for(int i = 0; i < nodeSet[cnode].nrules; i++){
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

