/*-----------------------------------------------------------------------------
 *  
 *  Name:		CutTSS.cpp
 *  Description:	CutTSS packet classification algorithm
 *  Version:		1.0 (release)
 *  Author:		huangting
 *  Date:		11/13/2018
 *   
 *-----------------------------------------------------------------------------*/

#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<list>
#include <sys/time.h>
#include <string.h>

#include"./CutSplit/CutSplit.h"
#include "./PartitionSort/PartitionSort.h"

using namespace std;



FILE *fpr = fopen("./ruleset/fw1_10k", "r");           // ruleset file
FILE *fpt = fopen("./ruleset/fw1_10k_trace", "r");           // test trace file for cuttss
FILE *fpu;//ruleset for update insert

int bucketSize = 8;   // leaf threashold
int threshold = 16;   // Assume T_SA=T_DA=threshold

map<int,int> pri_id;
int rand_update[MAXRULES]; //random generate rule id
int max_pri[4] = {-1,-1,-1,-1};

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  loadrule
 *  Description:  load rules from file
 * =====================================================================================
 */
vector<Rule> loadrule(FILE *fp)
{
    unsigned int tmp;
    unsigned sip1,sip2,sip3,sip4,smask;
    unsigned dip1,dip2,dip3,dip4,dmask;
    unsigned sport1,sport2;
    unsigned dport1,dport2;
    unsigned protocal,protocol_mask;
    unsigned ht, htmask;
    int number_rule=0; //number of rules

    vector<Rule> rule;

    while(1){

        Rule r;
        std::array<Point,2> points;

        if(fscanf(fp,"@%d.%d.%d.%d/%d\t%d.%d.%d.%d/%d\t%d : %d\t%d : %d\t%x/%x\t%x/%x\n",
                  &sip1, &sip2, &sip3, &sip4, &smask, &dip1, &dip2, &dip3, &dip4, &dmask, &sport1, &sport2,
                  &dport1, &dport2,&protocal, &protocol_mask, &ht, &htmask)!= 18) break;


        if(smask == 0){
            points[0] = 0;
            points[1] = 0xFFFFFFFF;
        }else if(smask > 0 && smask <= 8){
            tmp = sip1<<24;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-smask)) - 1;
        }else if(smask > 8 && smask <= 16){
            tmp = sip1<<24; tmp += sip2<<16;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-smask)) - 1;
        }else if(smask > 16 && smask <= 24){
            tmp = sip1<<24; tmp += sip2<<16; tmp +=sip3<<8;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-smask)) - 1;
        }else if(smask > 24 && smask <= 32){
            tmp = sip1<<24; tmp += sip2<<16; tmp += sip3<<8; tmp += sip4;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-smask)) - 1;
        }else{
            printf("Src IP length exceeds 32\n");
            exit(-1);
        }
        r.range[0] = points;

        if(dmask == 0){
            points[0] = 0;
            points[1] = 0xFFFFFFFF;
        }else if(dmask > 0 && dmask <= 8){
            tmp = dip1<<24;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-dmask)) - 1;
        }else if(dmask > 8 && dmask <= 16){
            tmp = dip1<<24; tmp +=dip2<<16;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-dmask)) - 1;
        }else if(dmask > 16 && dmask <= 24){
            tmp = dip1<<24; tmp +=dip2<<16; tmp+=dip3<<8;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-dmask)) - 1;
        }else if(dmask > 24 && dmask <= 32){
            tmp = dip1<<24; tmp +=dip2<<16; tmp+=dip3<<8; tmp +=dip4;
            points[0] = tmp;
            points[1] = points[0] + (1<<(32-dmask)) - 1;
        }else{
            printf("Dest IP length exceeds 32\n");
            exit(-1);
        }
        r.range[1] = points;

        points[0] = sport1;
        points[1] = sport2;
        r.range[2] = points;

        points[0] = dport1;
        points[1] = dport2;
        r.range[3] = points;

        if(protocol_mask == 0xFF){
            points[0] = protocal;
            points[1] = protocal;
        }else if(protocol_mask== 0){
            points[0] = 0;
            points[1] = 0xFF;
        }else{
            printf("Protocol mask error\n");
            exit(-1);
        }
        r.range[4] = points;

        r.prefix_length[0] = smask;
        r.prefix_length[1] = dmask;
        r.id = number_rule;
        //r.priority = number_rule;

        rule.push_back(r);

        number_rule++;
    }

    //printf("the number of rules = %d\n", number_rule);
    int max_pri = number_rule-1;
    for(int i=0;i<number_rule;i++){
        rule[i].priority = max_pri - i;
        pri_id.insert(pair<int,int>(rule[i].priority,rule[i].id));
        /*printf("%u: %u:%u %u:%u %u:%u %u:%u %u:%u %d\n", i,
          rule[i].range[0][0], rule[i].range[0][1],
          rule[i].range[1][0], rule[i].range[1][1],
          rule[i].range[2][0], rule[i].range[2][1],
          rule[i].range[3][0], rule[i].range[3][1],
          rule[i].range[4][0], rule[i].range[4][1],
          rule[i].priority);*/
    }
    pri_id.insert(pair<int,int>(-1,-1));

    return rule;
}

std::vector<Packet> loadpacket(FILE *fp)
{
    unsigned int header[MAXDIMENSIONS];
    unsigned int proto_mask, fid;
    int number_pkt=0; //number of packets
    std::vector<Packet> packets;

    while(1){
        if(fscanf(fp,"%u %u %d %d %d %u %d\n",
                     &header[0], &header[1], &header[2], &header[3], &header[4], &proto_mask, &fid) == Null) break;
        Packet p;
        p.push_back(header[0]);
        p.push_back(header[1]);
        p.push_back(header[2]);
        p.push_back(header[3]);
        p.push_back(header[4]);
        p.push_back(fid);

        packets.push_back(p);
        number_pkt++;
    }

    /*printf("the number of packets = %d\n", number_pkt);
    for(int i=0;i<number_pkt;i++){
        printf("%u: %u %u %u %u %u %u\n", i,
               packets[i][0],
               packets[i][1],
               packets[i][2],
               packets[i][3],
               packets[i][4],
               packets[i][5]);}*/

    return packets;
}

void parseargs(int argc, char *argv[])
{
    int	c;
    bool	ok = 1;
    while ((c = getopt(argc, argv, "b:t:r:e:s:u:p:h")) != -1){
        switch (c) {
            case 'b':
                bucketSize = atoi(optarg);
                break;
            case 't':
                threshold = atoi(optarg);
                break;
            case 'r':
                fpr = fopen(optarg, "r");
                break;
            case 'e':
                fpt = fopen(optarg, "r");
                break;
            case 'u':
                fpu = fopen(optarg, "r");
                break;
            case 'h':
                printf("CutTSS [-b bucketSize][-t threshold(assume T_SA=T_DA)][-r ruleset][-e trace][-u update]\n");
                printf("mail me: wenjunli@pku.edu.cn\n");
                exit(1);
                break;
            default:
                ok = 0;
        }
    }

    if(bucketSize <= 0 || bucketSize > MAXBUCKETS){
        printf("bucketSize should be greater than 0 and less than %d\n", MAXBUCKETS);
        ok = 0;
    }
    if(threshold < 0 || threshold > 32){
        printf("threshold should be greater than 0 and less than 32\n");
        ok = 0;
    }
    if(fpr == NULL){
        printf("can't open ruleset file\n");
        ok = 0;
    }
    if (!ok || optind < argc){
        fprintf (stderr, "CutTSS [-b bucketSize][-t threshold(assume T_SA=T_DA)][-r ruleset][-e trace][-u update]\n");
        fprintf (stderr, "Type \"CutTSS -h\" for help\n");
        exit(1);
    }

    printf("************CutTSS: version 1.0******************\n");
    printf("Bucket Size =  %d\n", bucketSize);
    printf("Threshold = %d,%d\n", threshold,threshold);
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  count_length
 *  Description:  record length of field and corresponding size
 * =====================================================================================
 */
void count_length(int number_rule,vector<Rule> rule,field_length *field_length_ruleset)
{
    unsigned temp_size=0;
    unsigned temp_value=0;
    //unsigned temp0=0;

    for(int i=0;i<number_rule;i++) //计算每个规则各维度的length和size（log2(length+1)）
    {
        for(int j=0;j<5;j++)  //record field length in field_length_ruleset[i]
        {
            field_length_ruleset[i].length[j]=rule[i].range[j][1]-rule[i].range[j][0];
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
//void partition_v2(vector<Rule> rule,vector<Rule> subset[4],int num_subset[4],int number_rule,field_length *field_length_ruleset,int threshold_value[2])
//{
//    int num_small_tmp[number_rule];//统计小规则集
//    for(int i=0;i<number_rule;i++){
//        num_small_tmp[i]=0;
//        for(int k=0;k<2;k++) //统计每个规则的小域数目sip dip
//            if(field_length_ruleset[i].size[k] <= threshold_value[k])
//                num_small_tmp[i]++;
//    }
//
//
//    //划分后的规则子集 1:sa规则子集 2:da规则子集 3:big
//    int count_big=0;
//    for(int i=0;i<number_rule;i++) {
//        if(num_small_tmp[i] == 0) {
//            subset[3].push_back(rule[i]);
//            count_big++;
//        }
//    }
//    num_subset[3]=count_big;
//
//    int count_sa=0;
//    int count_da=0;
//    int count1_tmp=0;
//    int count2_tmp=0;
//
//    int index[number_rule];
//    int index_con=0;
//    list<range> element_SA;
//    element_SA.clear();
//    list<range> element_DA;
//    element_SA.clear();
//    range check;
//    int same1=0;
//    int same2=0;
//    int n1=pow(2,20);
//    int n2=pow(2,12);
//    int* sa;
//    sa=(int*)calloc(n1,sizeof(int));
//    int* da;
//    da=(int*)calloc(n1,sizeof(int));
//
//    for(int i=0;i<n1;i++)
//    {
//        sa[i]=0;da[i]=0;
//    }
//    int index_sa=0;
//    int index_da=0;
//    int index_low=0;
//    int index_high=0;
//    int cnum=0;
//    int flag=0;
//    int sum1=0;
//    int sum2=0;
//
//    //a new partition method
//    for(int i=0;i<number_rule;i++)
//    {
//        if((count_big<=8)&&num_small_tmp[i]==0) {
//            if((rule[i].range[0][1]-rule[i].range[0][0])<(rule[i].range[1][1]-rule[i].range[1][0]))
//            {
//                index_low=rule[i].range[0][0]/n2;
//                index_high=rule[i].range[0][1]/n2;
//                cnum=index_high-index_low;
//
//                if(cnum>1)
//                {
//                    for(int y=0;y<cnum+1;y++)
//                        sa[index_low+y]++;
//                }
//                else{
//                    index_sa=rule[i].range[0][0]/n2;
//                    sa[index_sa]++;
//                }
//                subset[1].push_back(rule[i]);count1_tmp++;
//            }
//            else{
//                index_low=rule[i].range[1][0]/n2;
//                index_high=rule[i].range[1][1]/n2;
//                cnum=index_high-index_low;
//
//                if(cnum>1)
//                {
//                    for(int y=0;y<cnum+1;y++)
//                        da[index_low+y]++;
//                }
//                else{
//                    index_da=rule[i].range[1][0]/n2;
//                    da[index_da]++;
//                }
//                subset[2].push_back(rule[i]);count2_tmp++;
//            }
//        }
//
//        index_sa=0;
//        index_da=0;
//        sum1=0;
//        sum2=0;
//        flag=0;
//        cnum=0;
//        index_high=0;
//        index_low=0;
//
//        if((num_small_tmp[i]==1)&&(field_length_ruleset[i].size[0]<=threshold_value[0]))
//        {
//            index_low=rule[i].range[0][0]/n2;
//            index_high=rule[i].range[0][1]/n2;
//            cnum=index_high-index_low;
//
//            if(cnum>1)
//            {
//                for(int y=0;y<cnum+1;y++)
//                    sa[index_low+y]++;
//            }
//            else{
//                index_sa=rule[i].range[0][0]/n2;
//                sa[index_sa]++;
//            }
//
//            subset[1].push_back(rule[i]);count1_tmp++;
//        }
//
//        if((num_small_tmp[i]==1)&&(field_length_ruleset[i].size[1]<=threshold_value[1]))
//        {
//            index_low=rule[i].range[1][0]/n2;
//            index_high=rule[i].range[1][1]/n2;
//            cnum=index_high-index_low;
//
//            if(cnum>1)
//            {
//                for(int y=0;y<cnum+1;y++)
//                    da[index_low+y]++;
//            }
//            else{
//                index_da=rule[i].range[1][0]/n2;
//                da[index_da]++;
//            }
//
//            subset[2].push_back(rule[i]);count2_tmp++;
//        }
//    }
//
//    for(int i=0;i<number_rule;i++)
//    {
//        index_sa=0;
//        index_da=0;
//        sum1=0;
//        sum2=0;
//        flag=0;
//        cnum=0;
//        index_high=0;
//        index_low=0;
//
//        if(num_small_tmp[i]==2)
//        {
//            index_low=rule[i].range[0][0]/n2;
//            index_high=rule[i].range[0][1]/n2;
//            cnum=index_high-index_low;
//
//            for(int e=0;e<=cnum;e++)
//            {
//                if(sa[index_low+e]>bucketSize)
//                    sum1++;
//            }
//
//            index_low=rule[i].range[1][0]/n2;
//            index_high=rule[i].range[1][1]/n2;
//            cnum=index_high-index_low;
//            for(int e=0;e<=cnum;e++)
//            {
//                if(da[index_low+e]>bucketSize)
//                    sum2++;
//            }
//
//            if(sum1<sum2)
//            {
//                index_low=rule[i].range[0][0]/n2;
//                index_high=rule[i].range[0][1]/n2;
//                cnum=index_high-index_low;
//
//                if(cnum>1)
//                {
//                    for(int y=0;y<cnum+1;y++)
//                        sa[index_low+y]++;
//                }
//                else{
//                    index_sa=rule[i].range[0][0]/n2;
//                    sa[index_sa]++;
//                }
//
//                subset[1].push_back(rule[i]);count1_tmp++;
//            }
//            else if(sum1>sum2)
//            {
//                index_low=rule[i].range[1][0]/n2;
//                index_high=rule[i].range[1][1]/n2;
//                cnum=index_high-index_low;
//
//                if(cnum>1)
//                {
//                    for(int y=0;y<cnum+1;y++)
//                        da[index_low+y]++;
//                }
//                else{
//                    index_da=rule[i].range[1][0]/n2;
//                    da[index_da]++;
//                }
//
//                subset[2].push_back(rule[i]);count2_tmp++;
//            }
//            else
//            {
//                if(count1_tmp<=count2_tmp)
//                {
//                    index_low=rule[i].range[0][0]/n2;
//                    index_high=rule[i].range[0][1]/n2;
//                    cnum=index_high-index_low;
//
//                    if(cnum>1)
//                    {
//                        for(int y=0;y<cnum+1;y++)
//                            sa[index_low+y]++;
//                    }
//                    else{
//                        index_sa=rule[i].range[0][0]/n2;
//                        sa[index_sa]++;
//                    }
//
//                    subset[1].push_back(rule[i]);count1_tmp++;
//                }
//                else
//                {
//                    index_low=rule[i].range[1][0]/n2;
//                    index_high=rule[i].range[1][1]/n2;
//                    cnum=index_high-index_low;
//
//                    if(cnum>1)
//                    {
//                        for(int y=0;y<cnum+1;y++)
//                            da[index_low+y]++;
//                    }
//                    else{
//                        index_da=rule[i].range[1][0]/n2;
//                        da[index_da]++;
//                    }
//
//                    subset[2].push_back(rule[i]);count2_tmp++;
//                }
//            }
//        }
//    }
//
//    count_sa=count1_tmp;
//    count_da=count2_tmp;
//    num_subset[1]=count_sa;
//    num_subset[2]=count_da;
//
//    printf("Sa_subset:%d\tDa_subset:%d\tBig_subset:%d\n\n",count_sa,count_da,count_big);
//
//}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  partition_v3 (version3)
 *  Description:  partition ruleset into subsets based on address field(2 dim.)
 * =====================================================================================
 */
void partition_v3(vector<Rule> rule,vector<Rule> subset[4],int num_subset[4],int number_rule,field_length *field_length_ruleset,int threshold_value[2])
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
        if(num_small_tmp[i]==0){
            subset[3].push_back(rule[i]);
            count_big++;
            if(rule[i].priority>max_pri[3]) max_pri[3]=rule[i].priority;
        }
        else if(num_small_tmp[i]==2){
            subset[0].push_back(rule[i]);
            count_sa_da++;
            if(rule[i].priority>max_pri[0]) max_pri[0]=rule[i].priority;
        }
        else if(num_small_tmp[i]==1){
            if(field_length_ruleset[i].size[0]<=threshold_value[0]) {
                subset[1].push_back(rule[i]);
                count_sa++;
                if(rule[i].priority>max_pri[1]) max_pri[1]=rule[i].priority;
            }//sip为小域
            else if(field_length_ruleset[i].size[1]<=threshold_value[1]){
                subset[2].push_back(rule[i]);
                count_da++;
                if(rule[i].priority>max_pri[2]) max_pri[2]=rule[i].priority;
            } //dip为小域

        }
    }

    num_subset[0]=count_sa_da;
    num_subset[1]=count_sa;
    num_subset[2]=count_da;
    num_subset[3]=count_big;

    printf("Sa_Da_subset:%d\tSa_subset:%d\tDa_subset:%d\tBig_subset:%d\n\n",count_sa_da,count_sa,count_da,count_big);

}


int main(int argc, char* argv[])
{

    parseargs(argc, argv);

    vector<Rule> rule;
    vector<Packet> packets;

    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::duration<double> elapsed_seconds;
    std::chrono::duration<double,std::milli> elapsed_milliseconds;


    if(fpr != NULL){
        rule = loadrule(fpr);
        int number_rule = rule.size();
        printf("the number of rules = %d\n", number_rule);

        //CutTSS
        field_length *field_length_ruleset = (field_length*)malloc(sizeof(field_length)*number_rule);
        count_length(number_rule,rule,field_length_ruleset); //统计规则的各个域的长度

        vector<Rule> subset_4[4];  //划分后的规则子集 0:sa,da规则子集 1:sa规则子集 2:da规则子集 3:big
        int num_subset_4[4]={0,0,0,0};  //各规则子集的大小
        int threshold_value[2]={threshold,threshold};   //T_SA=T_DA=threshold 小域阈值

        partition_v3(rule,subset_4,num_subset_4,number_rule,field_length_ruleset,threshold_value); //四个规则集划分sa_da sa da big
        //partition_v2(rule,subset_4,num_subset_4,number_rule,field_length_ruleset,threshold_value); //三个规则集划分sa da big

        //printf("\n\n**************** Construction ****************\n\n");

        printf("CutTSS\n");

        CutTSS ct_sa_da(3,threshold,subset_4[0]);
        CutTSS ct_sa(1,threshold,subset_4[1]);
        CutTSS ct_da(2,threshold,subset_4[2]);
        PriorityTupleSpaceSearch ptss;
        //TSS ptss;

        start = std::chrono::steady_clock::now();
        //construct classifier
        if(num_subset_4[0] > 0) ct_sa_da.ConstructClassifier(subset_4[0]);
        if(num_subset_4[1] > 0) ct_sa.ConstructClassifier(subset_4[1]);
        if(num_subset_4[2] > 0) ct_da.ConstructClassifier(subset_4[2]);
        if(num_subset_4[3] > 0) ptss.ConstructClassifier(subset_4[3]);
        end = std::chrono::steady_clock::now();
        elapsed_milliseconds = end - start;

        //ct_sa_da.prints();
        //ct_sa.prints();
        //ct_da.prints();
        //printf("\t***PTSS for big ruleset:***\n");
        //ptss.prints();
        //printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());


        //printf("\nPTSS\n");

        PriorityTupleSpaceSearch ptss1;
        //TSS ptss1;

        start = std::chrono::steady_clock::now();
        ptss1.ConstructClassifier(rule); //construct
        end = std::chrono::steady_clock::now();
        elapsed_milliseconds = end - start;

        //ptss1.prints();
        //printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());
        //ptss1.CntLinkedSize();
        //printf("\thashCf_true: %d   hashCf_false: %d\n",ptss1.HashCfTRUE(),ptss1.HashCfFALSE());
        //printf("\tavg_linked_size: %f   worst_linked_size: %d\n",ptss1.GetAvgLinkedSize(),ptss1.GetWorstLinkedSize());



        //printf("\nPartitionSort\n");

        PartitionSort ps;

        start = std::chrono::steady_clock::now();
        ps.ConstructClassifier(rule); //construct
        end = std::chrono::steady_clock::now();
        elapsed_milliseconds = end - start;

        //printf("\tConstruction time: %f ms\n", elapsed_milliseconds.count());


        if(fpt != NULL){
            printf("\n\n**************** Classification ****************\n\n");
            packets = loadpacket(fpt);
            int number_pkt = packets.size();
            //printf("the number of packets = %d\n", number_pkt);

            int match_miss = 0;
            const int trials = 1;
            printf("\ttotal packets: %d\n", packets.size()*trials);
            printf("CutTSS\n");
            std::chrono::duration<double> sum_time(0);
            int match_pri = -1;
			vector<int> results;
            int results1[number_pkt];
			Packet p;

			
            for (int t = 0; t < trials; t++) {
                start = std::chrono::steady_clock::now();
				for(int i=0;i<number_pkt;i++){
					p=packets[i];					
					
                    match_pri = -1;
	
                    if(num_subset_4[0] > 0) match_pri = ct_sa_da.ClassifyAPacket(p);
                    if(match_pri < max_pri[1] && num_subset_4[1] > 0) match_pri = max(match_pri, ct_sa.ClassifyAPacket(p));
                    if(match_pri < max_pri[2] && num_subset_4[2] > 0) match_pri = max(match_pri, ct_da.ClassifyAPacket(p));
                    if(match_pri < max_pri[3] && num_subset_4[3] > 0) match_pri = max(match_pri, ptss.ClassifyAPacket(p));
		
                    //results.push_back(pri_id[match_pri]);
					results1[i] = number_rule-1 - match_pri;

                }
                end = std::chrono::steady_clock::now();
                elapsed_seconds = end - start;
                sum_time += elapsed_seconds;
            }
			
            printf("\tTotal classification time: %f s\n", sum_time.count() / trials);
			printf("\tAverage classification time: %f us\n", sum_time.count() * 1e6 / (trials * packets.size()));
			printf("\tThroughput: %f Mpps\n", 1 / (sum_time.count() * 1e6 / (trials * packets.size())));
			

			
			
			
            match_miss = 0;
            for(int i = 0;i < number_pkt;i++){
                if(results1[i] == -1) match_miss++;
                else if(packets[i][5] < results1[i]) match_miss++;
                //if(packets[i][5] != results1[i]) match_miss++;
            }
            printf("\t%d packets are classified, %d of them are misclassified\n", number_pkt, match_miss);

            int total_query = ct_sa_da.TablesQueried()+ct_sa.TablesQueried()+ct_da.TablesQueried()+ptss.TablesQueried();
            printf("\tTotal tables queried: %d\n", total_query);
            printf("\tAverage tables queried: %f\n", 1.0 * total_query / (trials * packets.size()));

            //printf("ct_sa_da\n\t%d: match cut packets count: %d match tss packets count: %d no match packets count: %d\n", ct_sa_da.totalPktsCount(),ct_sa_da.cutPktCount(),ct_sa_da.tssPktCount(),ct_sa_da.noMatchCount());
            //printf("ct_sa\n\t%d: match cut packets count: %d match tss packets count: %d no match packets count: %d\n", ct_sa.totalPktsCount(),ct_sa.cutPktCount(),ct_sa.tssPktCount(),ct_sa.noMatchCount());
            //printf("ct_da\n\t%d: match cut packets count: %d match tss packets count: %d no match packets count: %d\n", ct_da.totalPktsCount(),ct_da.cutPktCount(),ct_da.tssPktCount(),ct_da.noMatchCount());

			
			
			
			
            printf("PTSS\n");
            std::chrono::duration<double> sum_time1(0);
            results.clear();
            for (int t = 0; t < trials; t++) {
                start = std::chrono::steady_clock::now();
                //for (auto const &p : packets) {										
				for(int i=0;i<number_pkt;i++){
					p=packets[i];
					
                    //results.push_back(pri_id[ptss1.ClassifyAPacket(p)]);
					
					
					results1[i] = number_rule-1 - ptss1.ClassifyAPacket(p);
                }
                end = std::chrono::steady_clock::now();
                elapsed_seconds = end - start;
                sum_time1 += elapsed_seconds;
            }
            printf("\tTotal classification time: %f s\n", sum_time1.count() / trials);
			printf("\tAverage classification time: %f us\n", sum_time1.count() * 1e6 / (trials * packets.size()));
			printf("\tThroughput: %f Mpps\n", 1 / (sum_time1.count() * 1e6 / (trials * packets.size())));
			
			
			
            match_miss = 0;
            for(int i = 0;i < number_pkt;i++){
                if(results1[i] == -1) match_miss++;
                else if(packets[i][5] < results1[i]) match_miss++;
                //if(packets[i][5] != results[i]) match_miss++;
            }
            printf("\t%d packets are classified, %d of them are misclassified\n", number_pkt, match_miss);
            printf("\tTotal tables queried: %d\n", ptss1.TablesQueried());
            printf("\tAverage tables queried: %f\n", 1.0 * ptss1.TablesQueried() / (trials * packets.size()));


            printf("PartitionSort\n");
            std::chrono::duration<double> sum_time2(0);
            results.clear();
            for (int t = 0; t < trials; t++) {
                start = std::chrono::steady_clock::now();
                for (auto const &p : packets) {
                    results.push_back(pri_id[ps.ClassifyAPacket(p)]);
                }
                end = std::chrono::steady_clock::now();
                elapsed_seconds = end - start;
                sum_time2 += elapsed_seconds;
            }
            printf("\tTotal classification time: %f s\n", sum_time2.count() / trials);
			printf("\tAverage classification time: %f us\n", sum_time2.count() * 1e6 / (trials * packets.size()));
			printf("\tThroughput: %f Mpps\n", 1 / (sum_time2.count() * 1e6 / (trials * packets.size())));
			
			
            match_miss = 0;
            for(int i = 0;i < number_pkt;i++){
                if(results[i] == -1) match_miss++;
                else if(packets[i][5] < results[i]) match_miss++;
                //if(packets[i][5] != results[i]) match_miss++;
            }
            printf("\t%d packets are classified, %d of them are misclassified\n", number_pkt, match_miss);
            printf("\tTotal tables queried: %d\n", ps.TablesQueried());
            printf("\tAverage tables queried: %f\n", 1.0 * ps.TablesQueried() / (trials * packets.size()));

            rewind(fpr);rewind(fpt);
            printf("CutSplit\n");

            CutSplit(fpr,fpt);
        }



        if(fpr != NULL){
            printf("\n\n**************** Update ****************\n\n");
            srand((unsigned)time(NULL));
            for(int ra=0;ra<MAXRULES;ra++){ //1000000
                rand_update[ra] = rand()%2; //0:insert 1:delete
            }
            int insert_num = 0, delete_num = 0;
            int number_update = min(number_rule,MAXRULES);

            printf("CutTSS\n");
            start = std::chrono::steady_clock::now();
            for(int ra=0;ra<number_update;ra++){
                int smask = rule[ra].prefix_length[0];
                int dmask = rule[ra].prefix_length[1];
                if(rand_update[ra] == 0)//0:insert
                {
                    if((smask>=threshold) && (dmask>=threshold)) ct_sa_da.InsertRule(rule[ra]);
                    else if((smask>=threshold) && (dmask<=threshold)) ct_sa.InsertRule(rule[ra]);
                    else if((smask<=threshold) && (dmask>=threshold)) ct_da.InsertRule(rule[ra]);
                    else if((smask<=threshold) && (dmask<=threshold)) ptss.InsertRule(rule[ra]);
                    insert_num++;
                } else{//1:delete
                    if((smask>=threshold) && (dmask>=threshold)) ct_sa_da.DeleteRule(rule[ra]);
                    else if((smask>=threshold) && (dmask<=threshold)) ct_sa.DeleteRule(rule[ra]);
                    else if((smask<=threshold) && (dmask>=threshold)) ct_da.DeleteRule(rule[ra]);
                    else if((smask<=threshold) && (dmask<=threshold)) ptss.DeleteRule(rule[ra]);
                    delete_num++;
                }
            }
            end = std::chrono::steady_clock::now();
            elapsed_seconds = end - start;
            printf("\t%d rules update: insert_num = %d delete_num = %d\n", number_update,insert_num,delete_num);
            printf("\tUpdate time: %f s\n", elapsed_seconds.count());


            printf("PTSS\n");
            insert_num = 0, delete_num = 0;
            start = std::chrono::steady_clock::now();
            for(int ra=0;ra<number_update;ra++){
                if(rand_update[ra] == 0)//0:insert
                {
                    ptss1.InsertRule(rule[ra]);
                    insert_num++;
                } else{//1:delete
                    ptss1.DeleteRule(rule[ra]);
                    delete_num++;
                }
            }
            end = std::chrono::steady_clock::now();
            elapsed_seconds = end - start;
            printf("\t%d rules update: insert_num = %d delete_num = %d\n", number_update,insert_num,delete_num);
            printf("\tUpdate time: %f s\n", elapsed_seconds.count());
        }

    }

    //fclose(fpr);fclose(fpt);

    return 0;

}


