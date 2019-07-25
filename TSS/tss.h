#ifndef  _TSS_H
#define  _TSS_H

#include "../OVS/ElementaryClasses.h"
#include <ght_hash_table.h>
#include <stdint.h>

#define MAXTUPLES 10000
#define FILTERSIZE 18
#define HT_SIZE 1000

/*rule key*/
typedef struct rule_key{ //5-tuple
    uint32_t src_ip; // source IP address
    uint32_t dst_ip; //destination IP address
}rule_key_t;

typedef struct rule_entry{
    Rule r;
    struct rule_entry *next;
}rule_entry_t;

typedef struct entry{
    int size;  //entry对应规则数目
    rule_entry_t *head;
}entry_t;

typedef struct tuple{
    uint32_t siplen;     //sip dip 域前缀长度
    uint32_t diplen;
    ght_hash_table_t* tss_table;//ght_hash_table_t* filtertable
}tuple_t;



class TSS : public PacketClassifier {
public:

    virtual void ConstructClassifier(const std::vector<Rule>& rules);
    virtual int ClassifyAPacket(const Packet& packet);
    virtual void DeleteRule(const Rule& delete_rule);
    virtual void InsertRule(const Rule& insert_rule);
    virtual Memory MemSizeBytes() const { return 0; } // TODO
    virtual int MemoryAccess() const { return 0; } // TODO
    virtual size_t NumTables() const { return tupleList.size(); }
    virtual size_t RulesInTable(size_t tableIndex) const { return rules.size(); }

    int TablesQueried() const {	return queryCount; }
    int HashCfTRUE() const { return hashCf_true; }
    int HashCfFALSE() const { return hashCf_false; }

    void CntLinkedSize();
    double GetAvgLinkedSize() const { return avgsize; }
    int GetWorstLinkedSize() const { return worstsize; }

private:
    std::vector<Rule> rules;
    std::vector<tuple_t> tupleList;
    int queryCount = 0;
    int hashCf_true = 0;
    int hashCf_false = 0;
    double avgsize = 0;
    int worstsize = 0;
};

#endif