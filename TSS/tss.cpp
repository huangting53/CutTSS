#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tss.h"
using namespace std;


void TSS::CntLinkedSize(){
    int totalEntrys = 0;

    for(int i=0;i<tupleList.size();i++){
        const void  *pv, *pk;
        ght_iterator_t iterator;
        int size = ght_size(tupleList[i].tss_table);
        totalEntrys += size;
        //if(size > HT_SIZE) hashCf_true += (size-HT_SIZE);
        printf("ght size:%d\n",size);
        for (pv = (entry_t *) ght_first(tupleList[i].tss_table, &iterator, &pk); pv;
             pv = (entry_t *) ght_next(tupleList[i].tss_table, &iterator, &pk)) { //get the next element
            entry_t *pvt = (entry_t *)pv;
            if(pvt->size > worstsize) worstsize = pvt->size;
        }
    }
    avgsize = (double)rules.size()/totalEntrys;
}

void TSS::ConstructClassifier(const std::vector<Rule>& r){

    for (const auto& Rule : r) {
        InsertRule(Rule);
    }

    rules = r;
}
int TSS::ClassifyAPacket(const Packet& packet){
    int i,j,k;
    int match_id = -1;
    for(i = 0; i < tupleList.size(); i++){

        queryCount++;

        rule_key_t *pktKey = (rule_key_t*)malloc(sizeof(rule_key_t));
        if(tupleList[i].siplen == 32){
            pktKey->src_ip = packet[0];
        } else if(tupleList[i].siplen == 0){
            pktKey->src_ip = 0;
        } else{
            pktKey->src_ip = (packet[0] >> (32-tupleList[i].siplen)) << (32-tupleList[i].siplen);
        }

        if(tupleList[i].diplen == 32){
            pktKey->dst_ip = packet[1];
        } else if(tupleList[i].diplen == 0){
            pktKey->dst_ip = 0;
        } else{
            pktKey->dst_ip = (packet[1] >> (32-tupleList[i].diplen)) << (32-tupleList[i].diplen);
        }

        ght_hash_table_t * tuplehash = tupleList[i].tss_table;
        entry_t *tmp=(entry_t*)ght_get(tuplehash, sizeof(rule_key_t), pktKey);
        if(tmp) {
            //printf("get hash value\n");
            rule_entry_t *myRule = tmp->head;
            for(j = 0; j < tmp->size; j++){
                queryCount++;
                if(myRule->r.MatchesPacket(packet)){
                    match_id = std::max(match_id, myRule->r.priority);
                    break;
                }
                myRule = myRule->next;
            }
        }
    }

    return  match_id;
}
void TSS::DeleteRule(const Rule& delete_rule){
    int i,j;
    int flag=1;

    for(i=0; i<tupleList.size(); i++){
        if((delete_rule.prefix_length[0] == tupleList[i].siplen) && (delete_rule.prefix_length[1] == tupleList[i].diplen)){
            ght_hash_table_t * tuplehash = tupleList[i].tss_table;
            rule_key_t *ruleKey = (rule_key_t*)malloc(sizeof(rule_key_t));
            ruleKey->src_ip = delete_rule.range[0][0];
            ruleKey->dst_ip = delete_rule.range[1][0];

            entry_t *newEntry = (entry_t*)ght_get(tuplehash,sizeof(rule_key_t),ruleKey);
            if(newEntry == NULL){
                printf("the entry is empty!\n");
            } else{
                rule_entry_t *tmp = newEntry->head;
                for (j = 0; j < newEntry->size; j++) {
                    if(tmp->r.id == delete_rule.id){
                        flag = 0;
                        break;
                    }
                    tmp = tmp->next;
                }
                if(flag == 0){
                    if(newEntry->size==1 || j==0){
                        newEntry->head = newEntry->head->next;
                    }
                    else if(newEntry->size > 1){
                        rule_entry_t *tmp1 = newEntry->head;
                        rule_entry_t *tmp2 = newEntry->head->next;
                        while(j-1){
                            tmp1 = tmp1->next;
                            tmp2 = tmp2->next;
                            j--;
                        }
                        tmp1->next = tmp2->next;
                    }

                    newEntry->size--;
                }

            }

            break;
        }
    }

    if(flag == 1){  //规则i与之前的所有tuple都不匹配
        printf("delete error: no match rule!\n");
    }
}
void TSS::InsertRule(const Rule& insert_rule){

    int i;

    tuple_t currenttuple;
    int flag;

    currenttuple.siplen = insert_rule.prefix_length[0];
    currenttuple.diplen = insert_rule.prefix_length[1];
    flag = 1;
    for(i=0; i<tupleList.size(); i++){
        if((currenttuple.siplen == tupleList[i].siplen) && (currenttuple.diplen == tupleList[i].diplen)){
            ght_hash_table_t * tuplehash = tupleList[i].tss_table;
            rule_key_t *ruleKey = (rule_key_t*)malloc(sizeof(rule_key_t));
            ruleKey->src_ip = insert_rule.range[0][0];
            ruleKey->dst_ip = insert_rule.range[1][0];

            entry_t *newEntry = (entry_t*)ght_get(tuplehash,sizeof(rule_key_t),ruleKey);
            if(newEntry == NULL){
                newEntry = (entry_t*)malloc(sizeof(entry_t));
                newEntry->size = 1;
                newEntry->head = (rule_entry_t*)malloc(sizeof(rule_entry_t));
                memcpy(&newEntry->head->r,&insert_rule, sizeof(Rule));
                newEntry->head->next = NULL;
                ght_insert(tuplehash,newEntry,sizeof(rule_key_t),ruleKey);
            } else{

                if((insert_rule.range[0][0]==newEntry->head->r.range[0][0]) && (insert_rule.range[1][0]==newEntry->head->r.range[1][0])){
                    hashCf_false++;
                } else{
                    hashCf_true++;
                }

                rule_entry_t *tmp = newEntry->head;
                while(tmp->next != NULL){
                    tmp = tmp->next;
                }
                tmp->next = (rule_entry_t*)malloc(sizeof(rule_entry_t));
                tmp = tmp->next;
                memcpy(&tmp->r,&insert_rule, sizeof(Rule));
                tmp->next = NULL;
                newEntry->size++;
            }

            flag = 0;
            break;
        }
    }

    if(flag == 1){  //规则i与之前的所有tuple都不匹配，则新建tuple
        currenttuple.tss_table = ght_create(HT_SIZE);
        if(currenttuple.tss_table == NULL)
        {
            printf("create tuple hash table error!\n");
            exit(-1);
        }
        ght_hash_table_t * tuplehash = currenttuple.tss_table;
        rule_key_t *ruleKey = (rule_key_t*)malloc(sizeof(rule_key_t));
        ruleKey->src_ip = insert_rule.range[0][0];
        ruleKey->dst_ip = insert_rule.range[1][0];

        entry_t *newEntry = (entry_t*)malloc(sizeof(entry_t));
        newEntry->size = 1;
        newEntry->head = (rule_entry_t*)malloc(sizeof(rule_entry_t));
        memcpy(&newEntry->head->r,&insert_rule, sizeof(Rule));
        newEntry->head->next = NULL;

        ght_insert(tuplehash,newEntry,sizeof(rule_key_t),ruleKey);

        tupleList.push_back(currenttuple);
    }

}
