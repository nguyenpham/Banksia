#ifndef TBPROBE_H
#define TBPROBE_H

#include <stdio.h>

#include "../base/base.h"

typedef uint64_t Key;

//int TB_probe_wdl(Pos *pos, int *success);
//int TB_probe_dtz(Pos *pos, int *success);
//Value TB_probe_dtm(Pos *pos, int wdl, int *success);
//int TB_root_probe_wdl(Pos *pos, RootMoves *rm);
//int TB_root_probe_dtz(Pos *pos, RootMoves *rm);
//int TB_root_probe_dtm(Pos *pos, RootMoves *rm);
//void TB_expand_mate(Pos *pos, RootMove *move);

#define TB_PIECES 7

class SyzygyTb
{
public:
    static int TB_MaxCardinality;
    static int TB_MaxCardinalityDTM;
    
    void TB_init(char *path);
    void TB_free(void);

    // functions
private:
    Key calc_key_from_pcs(int *pcs, bool flip);
    
    void init_tb(char *str);
    bool test_tb(const char *str, const char *suffix);
    
    Key calc_key_from_pieces(uint8_t *piece, int num);
    FD open_tb(const char *str, const char *suffix);
    bool test_tb(const char *str, const char *suffix);
    void* map_tb(const char *name, const char *suffix, map_t *mapping);
    
    // variants
private:
    
    std::mutex tbMutex;
    int initialized = 0;
    int numPaths = 0;
    
    std::string pathString;
    std::vector<std::string> paths;
    
    int tbNumPiece, tbNumPawn;
    int numWdl, numDtm, numDtz;
};

#endif
