// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#ifndef PRAGMA_H
#define PRAGMA_H

#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

class StringUtils
{
public:
    static string strip(string s);
    static string getParenValue(string s);
    static vector<string> tokenize(string s, string split);
};

enum PragmaOption {GRID,IMAGE_MEM,LOCAL_MEM,CONSTANT_MEM,PIXEL,BOUNDARY_COND,GRID_SIZE,CONSTANT_MEM_CAND};

class Pragma
{
public:
    Pragma(string pragmaString);
    bool operator <(const Pragma& lhs) const;

    PragmaOption option;
    set<string> values;
    map<string,string> pairValues;
    int x;
    int y;
    string str();

private:
    //Ugh...
    const map<string, PragmaOption> pragmaStringToOption = {{"grid", GRID},
                                                           {"image_mem",IMAGE_MEM},
                                                           {"local_mem",LOCAL_MEM},
                                                           {"constant_mem",CONSTANT_MEM},
                                                           {"constant_mem_cand",CONSTANT_MEM_CAND},
                                                           {"pixel",PIXEL},
                                                           {"boundary_cond",BOUNDARY_COND},
                                                           {"grid_size",GRID_SIZE}
                                                          };

    const map<PragmaOption, string> pragmaOptionToString = {{GRID, "grid"},
                                                           {IMAGE_MEM, "image_mem"},
                                                           {LOCAL_MEM, "local_mem"},
                                                           {CONSTANT_MEM, "constant_mem"},
                                                           {CONSTANT_MEM_CAND, "constant_mem_cand"},
                                                           {PIXEL, "pixel"},
                                                           {BOUNDARY_COND, "boundary_cond"},
                                                           {GRID_SIZE, "grid_size"}
                                                          };
    PragmaOption findPragmaOption();
    void parseValues();
    void parseValuePairs();
    void parseGridSize();

    string pragmaString;

};

#endif // PRAGMA_H
