/*-----------------------------------------------------------------------------
 *  
 *  Name:		CutSplit.c
 *  Description:	CutSplit packet classification algorithm
 *  Version:		1.0 (release)
 *  Author:		Wenjun Li (wenjunli@pku.edu.cn)	 
 *  Date:		4/15/2018
 *   
 *-----------------------------------------------------------------------------*/


#include "CutSplit.h"


//FILE *fpr = fopen("./ipc1_32k", "r");           // ruleset file
//FILE *fpt = fopen("./ipc1_32k_trace", "r");           // test trace file
//int bucketSize = 8;   // leaf threashold
//int threshold = 16;   // Assume T_SA=T_DA=threshold


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  loadrule
 *  Description:  load rules from file
 * =====================================================================================
 */
int loadrule1(FILE *fp,pc_rule *rule)
{
   int tmp;
   unsigned sip1,sip2,sip3,sip4,smask;
   unsigned dip1,dip2,dip3,dip4,dmask;
   unsigned sport1,sport2;
   unsigned dport1,dport2;
   unsigned protocal,protocol_mask;
   unsigned ht, htmask;
   int number_rule=0; //number of rules

   while(1){
      if(fscanf(fp,"@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t%d : %d\t%d : %d\t%x/%x\t%x/%x\n",
                &sip1, &sip2, &sip3, &sip4, &smask, &dip1, &dip2, &dip3, &dip4, &dmask, &rule[number_rule].field[2].low, &rule[number_rule].field[2].high,
                &rule[number_rule].field[3].low, &rule[number_rule].field[3].high,&protocal, &protocol_mask, &ht, &htmask)!= 18) break;


   if(smask == 0){
      rule[number_rule].field[0].low = 0;
      rule[number_rule].field[0].high = 0xFFFFFFFF;
    }else if(smask > 0 && smask <= 8){
      tmp = sip1<<24;
      rule[number_rule].field[0].low = tmp;
      rule[number_rule].field[0].high = rule[number_rule].field[0].low + (1<<(32-smask)) - 1;
    }else if(smask > 8 && smask <= 16){
      tmp = sip1<<24; tmp += sip2<<16;
      rule[number_rule].field[0].low = tmp; 	
      rule[number_rule].field[0].high = rule[number_rule].field[0].low + (1<<(32-smask)) - 1;	
    }else if(smask > 16 && smask <= 24){
      tmp = sip1<<24; tmp += sip2<<16; tmp +=sip3<<8; 
      rule[number_rule].field[0].low = tmp; 	
      rule[number_rule].field[0].high = rule[number_rule].field[0].low + (1<<(32-smask)) - 1;			
    }else if(smask > 24 && smask <= 32){
      tmp = sip1<<24; tmp += sip2<<16; tmp += sip3<<8; tmp += sip4;
      rule[number_rule].field[0].low = tmp; 
      rule[number_rule].field[0].high = rule[number_rule].field[0].low + (1<<(32-smask)) - 1;	
    }else{
      printf("Src IP length exceeds 32\n");
      return 0;
    }
    if(dmask == 0){
      rule[number_rule].field[1].low = 0;
      rule[number_rule].field[1].high = 0xFFFFFFFF;
    }else if(dmask > 0 && dmask <= 8){
      tmp = dip1<<24;
      rule[number_rule].field[1].low = tmp;
      rule[number_rule].field[1].high = rule[number_rule].field[1].low + (1<<(32-dmask)) - 1;
    }else if(dmask > 8 && dmask <= 16){
      tmp = dip1<<24; tmp +=dip2<<16;
      rule[number_rule].field[1].low = tmp; 	
      rule[number_rule].field[1].high = rule[number_rule].field[1].low + (1<<(32-dmask)) - 1;	
    }else if(dmask > 16 && dmask <= 24){
      tmp = dip1<<24; tmp +=dip2<<16; tmp+=dip3<<8;
      rule[number_rule].field[1].low = tmp; 	
      rule[number_rule].field[1].high = rule[number_rule].field[1].low + (1<<(32-dmask)) - 1;			
    }else if(dmask > 24 && dmask <= 32){
      tmp = dip1<<24; tmp +=dip2<<16; tmp+=dip3<<8; tmp +=dip4;
      rule[number_rule].field[1].low = tmp; 	
      rule[number_rule].field[1].high = rule[number_rule].field[1].low + (1<<(32-dmask)) - 1;	
    }else{
      printf("Dest IP length exceeds 32\n");
      return 0;
    }
    if(protocol_mask == 0xFF){
      rule[number_rule].field[4].low = protocal;
      rule[number_rule].field[4].high = protocal;
    }else if(protocol_mask== 0){
      rule[number_rule].field[4].low = 0;
      rule[number_rule].field[4].high = 0xFF;
    }else{
      printf("Protocol mask error\n");
      return 0;
    }

       rule[number_rule].id = number_rule;
       number_rule++;
   }
  
   /* 
   printf("the number of rules = %d\n", number_rule);
     for(int i=0;i<number_rule;i++){
      printf("%u: %u:%u %u:%u %u:%u %u:%u %u:%u\n", i,
        rule[i].field[0].low, rule[i].field[0].high, 
        rule[i].field[1].low, rule[i].field[1].high,
        rule[i].field[2].low, rule[i].field[2].high,
        rule[i].field[3].low, rule[i].field[3].high, 
        rule[i].field[4].low, rule[i].field[4].high);}
   */

  return number_rule;  
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  count_length
 *  Description:  record length of field and correponding size
 * =====================================================================================
 */
void count_length1(int number_rule,pc_rule *rule,field_length *field_length_ruleset)
{
   unsigned temp_size=0;
   unsigned temp_value=0;
   //unsigned temp0=0;

   for(int i=0;i<number_rule;i++) //计算每个规则各维度的length和size（log2(length+1)）
   {
       for(int j=0;j<5;j++)  //record field length in field_length_ruleset[i]
       {
          field_length_ruleset[i].length[j]=rule[i].field[j].high-rule[i].field[j].low;    
          if(field_length_ruleset[i].length[j]==0xffffffff)
             field_length_ruleset[i].size[j]=32; //for address *
          else 
          {
             temp_size=0;
             temp_value=field_length_ruleset[i].length[j]+1;   //0xf +1 4
             while((temp_value=temp_value/2)!=0)
                temp_size++;  
             //for port number 
             if((field_length_ruleset[i].length[j]+1 - pow(2,temp_size))!=0) //10 3 11-8=3 4
               temp_size++; 

             field_length_ruleset[i].size[j]=temp_size;
          }
       }
   }
}




/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  partition_v2 (version2), More balanced, from HybridCuts
 *  Description:  partition ruleset into subsets based on address field(2 dim.)
 * =====================================================================================
 */
void partition_V2(pc_rule *rule,pc_rule* subset[3],int num_subset[3],int number_rule,field_length *field_length_ruleset,int threshold_value[2])
{
    int bucketSize = 8;
  int num_small_tmp[number_rule];//统计小规则集
  for(int i=0;i<number_rule;i++){
      num_small_tmp[i]=0;
      for(int k=0;k<2;k++) //统计每个规则的小域数目sip dip
         if(field_length_ruleset[i].size[k] <= threshold_value[k]) 
            num_small_tmp[i]++; 
  }


  int count0=0;  
  for(int i=0;i<number_rule;i++)
     if(num_small_tmp[i]==0)
        subset[0][count0++]=rule[i]; //大规则集
  num_subset[0]=count0;

  int count1=0;
  int count2=0;
  int count1_tmp=0; 
  int count2_tmp=0; 



  int index[number_rule];
  int index_con=0;
  list<range> element_SA; 
  element_SA.clear();     
  list<range> element_DA;    
  element_SA.clear(); 
  range check;
  int same1=0;
  int same2=0;
  int n1=pow(2,20);
  int n2=pow(2,12);
  int* sa;
  sa=(int*)calloc(n1,sizeof(int)); 
  int* da;
  da=(int*)calloc(n1,sizeof(int)); 

  for(int i=0;i<n1;i++)
  {
     sa[i]=0;da[i]=0;
  }
     int index_sa=0;
     int index_da=0;
     int index_low=0;
     int index_high=0;
     int cnum=0;
     int flag=0;
     int sum1=0;
     int sum2=0;
  
  //a new partition method 
  for(int i=0;i<number_rule;i++)
  {
      if((count0<=8)&&num_small_tmp[i]==0) {
        if((rule[i].field[0].high-rule[i].field[0].low)<(rule[i].field[1].high-rule[i].field[1].low))
        {
           index_low=rule[i].field[0].low/n2;
           index_high=rule[i].field[0].high/n2;
           cnum=index_high-index_low;

           if(cnum>1)
             {
              for(int y=0;y<cnum+1;y++)
                 sa[index_low+y]++;
             }
           else{
                index_sa=rule[i].field[0].low/n2;
                sa[index_sa]++;
               }
           subset[1][count1_tmp++]=rule[i];
        }
        else{
          index_low=rule[i].field[1].low/n2;
          index_high=rule[i].field[1].high/n2;
          cnum=index_high-index_low;

          if(cnum>1)
            {
             for(int y=0;y<cnum+1;y++)
                da[index_low+y]++;
            }
          else{
               index_da=rule[i].field[1].low/n2;
               da[index_da]++;
               }

          subset[2][count2_tmp++]=rule[i];
        }
      }

      index_sa=0;
      index_da=0;
      sum1=0;
      sum2=0;
      flag=0;
      cnum=0;
      index_high=0;
      index_low=0;

      if((num_small_tmp[i]==1)&&(field_length_ruleset[i].size[0]<=threshold_value[0]))
        {
        index_low=rule[i].field[0].low/n2;
        index_high=rule[i].field[0].high/n2;
        cnum=index_high-index_low;

        if(cnum>1)
           {
            for(int y=0;y<cnum+1;y++)
               sa[index_low+y]++;
           }
        else{
            index_sa=rule[i].field[0].low/n2;
            sa[index_sa]++;
            }
    
          subset[1][count1_tmp++]=rule[i];   
         }

      if((num_small_tmp[i]==1)&&(field_length_ruleset[i].size[1]<=threshold_value[1])) 
         {
         index_low=rule[i].field[1].low/n2;
         index_high=rule[i].field[1].high/n2;
         cnum=index_high-index_low;

         if(cnum>1)
            {
             for(int y=0;y<cnum+1;y++)
               da[index_low+y]++;
            }
         else{
             index_da=rule[i].field[1].low/n2;
             da[index_da]++;
             }

          subset[2][count2_tmp++]=rule[i];
         }
  }

   for(int i=0;i<number_rule;i++)
   {
      index_sa=0;
      index_da=0;
      sum1=0;
      sum2=0;
      flag=0;
      cnum=0;
      index_high=0;
      index_low=0;

      if(num_small_tmp[i]==2)
      {
        index_low=rule[i].field[0].low/n2;
        index_high=rule[i].field[0].high/n2;
        cnum=index_high-index_low;
        
        for(int e=0;e<=cnum;e++)
            {
             if(sa[index_low+e]>bucketSize)
             sum1++;
            }

        index_low=rule[i].field[1].low/n2;
        index_high=rule[i].field[1].high/n2;
        cnum=index_high-index_low;
        for(int e=0;e<=cnum;e++)
            {
             if(da[index_low+e]>bucketSize)
             sum2++;
            }
 
        if(sum1<sum2)
          { 
          index_low=rule[i].field[0].low/n2;
          index_high=rule[i].field[0].high/n2;
          cnum=index_high-index_low;

          if(cnum>1)
             {
             for(int y=0;y<cnum+1;y++)
                sa[index_low+y]++;
             }
           else{
               index_sa=rule[i].field[0].low/n2;
               sa[index_sa]++;
               }
    
           subset[1][count1_tmp++]=rule[i];  
           }
        else if(sum1>sum2)
            {
            index_low=rule[i].field[1].low/n2;
            index_high=rule[i].field[1].high/n2;
            cnum=index_high-index_low;

            if(cnum>1)
              {
               for(int y=0;y<cnum+1;y++)
                 da[index_low+y]++;
              }
            else{
                 index_da=rule[i].field[1].low/n2;
                 da[index_da]++;
                 }

            subset[2][count2_tmp++]=rule[i];
            } 
        else 
            {
             if(count1_tmp<=count2_tmp)
               {
               index_low=rule[i].field[0].low/n2;
               index_high=rule[i].field[0].high/n2;
               cnum=index_high-index_low;

               if(cnum>1)
                 {
                 for(int y=0;y<cnum+1;y++)
                    sa[index_low+y]++;
                 }
                else{
                    index_sa=rule[i].field[0].low/n2;
                    sa[index_sa]++;
                    }
    
                subset[1][count1_tmp++]=rule[i];  
                }
              else
                 {
                 index_low=rule[i].field[1].low/n2;
                 index_high=rule[i].field[1].high/n2;
                 cnum=index_high-index_low;

                 if(cnum>1)
                    {
                     for(int y=0;y<cnum+1;y++)
                        da[index_low+y]++;
                     }
                 else{
                     index_da=rule[i].field[1].low/n2;
                     da[index_da]++;
                     }

                 subset[2][count2_tmp++]=rule[i];
                 }
              }
      }
   }

   count1=count1_tmp;
   count2=count2_tmp; 
   num_subset[1]=count1; 
   num_subset[2]=count2;
 
   //printf("Big_subset:%d\tSa_subset:%d\tDa_subset:%d\n\n",count0,count1,count2);

/*
   printf("***********************big_ruleset*******************************************\n");
        if(num_subset[0]!=0)
           dump_ruleset(subset[0],num_subset[0]);
        else
           printf("empty!\n");
   printf("***********************SA_ruleset********************************************\n");
        dump_ruleset(subset[1],num_subset[1]);
   printf("***********************DA_ruleset********************************************\n");
        dump_ruleset(subset[2],num_subset[2]);
*/
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  partition_v3 (version3)
 *  Description:  partition ruleset into subsets based on address field(2 dim.)
 * =====================================================================================
 */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  partition_v3 (version3)
 *  Description:  partition ruleset into subsets based on address field(2 dim.)
 * =====================================================================================
 */
void partition_V3(pc_rule *rule,pc_rule* subset[4],int num_subset[4],int number_rule,field_length *field_length_ruleset,int threshold_value[2])
{  //划分后的规则子集 0:sa,da规则子集 1:sa规则子集 2:da规则子集 3:big
    int num_small_tmp[number_rule];
    for(int i=0;i<number_rule;i++){ //统计每个规则的小域数目sip dip
        num_small_tmp[i]=0;
        for(int k=0;k<2;k++)
            if(field_length_ruleset[i].size[k] <= threshold_value[k])
                num_small_tmp[i]++;
    }

    int count_sa_da=0;
    int count_sa=0;
    int count_da=0;
    int count_big=0;  //大规则集
    for(int i=0;i<number_rule;i++){
        if(num_small_tmp[i]==0)
            subset[3][count_big++]=rule[i];
        else if(num_small_tmp[i]==2)
            subset[0][count_sa_da++]=rule[i];
        else if(num_small_tmp[i]==1){
            if(field_length_ruleset[i].size[0]<=threshold_value[0]) //sip为小域
                subset[1][count_sa++]=rule[i];
            else if(field_length_ruleset[i].size[1]<=threshold_value[1]) //dip为小域
                subset[2][count_da++]=rule[i];
        }
    }

    num_subset[0]=count_sa_da;
    num_subset[1]=count_sa;
    num_subset[2]=count_da;
    num_subset[3]=count_big;

    //printf("Sa_Da_subset:%d\tSa_subset:%d\tDa_subset:%d\tBig_subset:%d\n\n",count_sa_da,count_sa,count_da,count_big);

/*
   printf("***********************big_ruleset*******************************************\n");
        if(num_subset[0]!=0)
           dump_ruleset(subset[0],num_subset[0]);
        else
           printf("empty!\n");
   printf("***********************SA_ruleset********************************************\n");
        dump_ruleset(subset[1],num_subset[1]);
   printf("***********************DA_ruleset********************************************\n");
        dump_ruleset(subset[2],num_subset[2]);
*/

}


void CutSplit(FILE* fpr,FILE* fpt)
{
    int bucketSize = 8;   // leaf threashold
    int threshold = 16;   // Assume T_SA=T_DA=threshold
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::duration<double> elapsed_seconds;
    std::chrono::duration<double,std::milli> elapsed_milliseconds;

    pc_rule *rule;  //规则集
    int number_rule=0;
    //parseargs(argc, argv);
    char test1;
    while((test1=fgetc(fpr))!=EOF)
        if(test1=='@')
            number_rule++;
    rewind(fpr);
    rule = (pc_rule *)calloc(number_rule, sizeof(pc_rule));
    number_rule=loadrule1(fpr,rule);
    //printf("the number of rules = %d\n", number_rule);
    fclose(fpr);
    //dump_ruleset(rule,number_rule);

    field_length field_length_ruleset[number_rule];
    count_length1(number_rule,rule,field_length_ruleset); //统计规则的各个域的长度

    pc_rule* subset_4[4];  //划分后的规则子集 0:sa,da规则子集 1:sa规则子集 2:da规则子集 3:big
    for(int n=0;n<4;n++)
        subset_4[n]=(pc_rule *)malloc(number_rule*sizeof(pc_rule));
    int num_subset_4[4]={0,0,0,0};  //各规则子集的大小
    int threshold_value[2]={threshold,threshold};   //T_SA=T_DA=threshold 小域阈值

    partition_V3(rule,subset_4,num_subset_4,number_rule,field_length_ruleset,threshold_value); //规则集划分

    //printf("\n num_sa = %d   num_da = %d   num_big = %d \n", num_subset_3[1],num_subset_3[2],num_subset_3[0]);

    start = std::chrono::steady_clock::now();
    //construct classifier
    trie T_sa_da(num_subset_4[0],bucketSize,rule,subset_4[0],threshold,3); //SIP、DIP小域规则子集cut+split生成决策树
    trie T_sa(num_subset_4[1],bucketSize,rule,subset_4[1],threshold,1); //DIP小域规则子集cut+split生成决策树
    trie T_da(num_subset_4[2],bucketSize,rule,subset_4[2],threshold,2); //DIP小域规则子集cut+split生成决策树
    hs_node_t* big_node = (hs_node_t *) malloc(sizeof(hs_node_t));
    if(num_subset_4[3] > 0){  //大规则集hypersplit生成决策树
        hstrie T(num_subset_4[3],subset_4[3], bucketSize, big_node);
        /*printf("***Big rules(using HyperSplit):***\n");
        printf(">>RESULTS:");
        printf("\n>>number of rules: %d", T.result.num_rules);
        printf("\n>>number of children: %d", T.result.num_childnode);
        printf("\n>>worst case tree depth: %d", T.result.wst_depth);
        printf("\n>>average tree depth: %f", T.result.avg_depth);
        printf("\n>>number of tree nodes:%d", T.result.num_tree_node);
        printf("\n>>number of leaf nodes:%d", T.result.num_leaf_node);
        printf("\n>>total memory: %f(KB)", T.result.total_mem_kb);
        printf("\n>>Byte/rule: %f", (T.result.total_mem_kb*1024)/T.result.num_rules);
        printf("\n***SUCCESS in building HyperSplit sub-tree for big rules***\n\n");*/

    }
    end = std::chrono::steady_clock::now();
    elapsed_milliseconds = end - start;
    //printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());


    int header[MAXDIMENSIONS];
    int match_sa_da, match_sa, match_da, match_big, fid;
    int matchid;
    unsigned int proto_mask;
    int mem_acc[1] = {0};
    start = std::chrono::steady_clock::now();
    if(fpt != NULL){
        int i=0, j=0;
        while(fscanf(fpt,"%u %u %d %d %d %u %d\n",
                     &header[0], &header[1], &header[2], &header[3], &header[4], &proto_mask, &fid) != Null){
            i++;
            //printf("\n>> packet %d\n", i);
            matchid = match_sa_da = match_sa = match_da = match_big = -1;

            match_sa_da = T_sa_da.trieLookup(header);
            match_sa = T_sa.trieLookup(header);
            match_da = T_da.trieLookup(header);
            if(num_subset_4[3] > 0) match_big = LookupHSTree(rule,big_node,header,mem_acc);
            //printf("\nmatch_sa = %d   match_da = %d   match_big = %d\n", match_sa, match_da, match_big);

            if(match_sa_da != -1) matchid = match_sa_da;
            if((matchid == -1) || (match_sa != -1 && match_sa < matchid)) matchid = match_sa;
            if((matchid == -1) || (match_da != -1 && match_da < matchid)) matchid = match_da;
            if((matchid == -1) || (match_big != -1 && match_big < matchid)) matchid = match_big;
            //printf("match_id = %d   fid = %d\n", matchid, fid);

            if(matchid == -1){
                printf("? packet %d match NO rule, should be %d\n", i, fid);
                j++;
            }else if(matchid == fid){
                //printf("\nmatch_id = %d   fid = %d   -----   packet %d match rule %d\n", matchid, fid, i, matchid);
            }else if(matchid > fid){
                //printf("? packet %d match lower priority rule %d, should be %d\n", i, matchid, fid);
                j++;
            }else{
                //printf("! packet %d match higher priority rule %d, should be %d\n", i, matchid, fid);
            }
        }
        end = std::chrono::steady_clock::now();
        elapsed_seconds = end - start;
        printf("\t%d packets are classified, %d of them are misclassified\n", i, j);
        printf("\tTotal classification time: %f s\n", elapsed_seconds.count());
        printf("\tAverage classification time: %f us\n", elapsed_seconds.count()*1000000 / i);
        printf("\tThroughput: %f Mpps\n", 1 / (elapsed_seconds.count()*1000000 / i));

        int total_query = T_sa_da.TablesQueried()+T_sa.TablesQueried()+T_da.TablesQueried()+mem_acc[0];
        printf("\tTotal memory access: %d\n", total_query);
        printf("\tAverage memory access: %f\n", 1.0 * total_query / i);
    }else{
        printf("\nNo packet trace input\n");
    }

    free(rule);free(big_node);
    for(int n=0;n<4;n++)
        free(subset_4[n]);

}


