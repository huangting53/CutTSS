#pragma once
#ifndef  _CUTSPLIT_H
#define  _CUTSPLIT_H


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<math.h>
#include<list>
#include"trie.h"
#include <sys/time.h>

int loadrule1(FILE *fp,pc_rule *rule);
void count_length1(int number_rule,pc_rule *rule,field_length *field_length_ruleset);
void partition_V2(pc_rule *rule,pc_rule* subset[3],int num_subset[3],int number_rule,field_length *field_length_ruleset,int threshold_value[2]);
void CutSplit(FILE* fpr,FILE* fpt);

#endif 
