// Copyright (c) 2016, Thomas L. Falch
// For conditions of distribution and use, see the accompanying LICENSE and README files

// This file is part of the ImageCL source-to-source compiler
// developed at the Norwegian University of Science and technology


#include "pragma.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

Pragma::Pragma(string pragmaString) : pragmaString(pragmaString)
{
    findPragmaOption();

    switch(option){
    case GRID:
    case LOCAL_MEM:
    case CONSTANT_MEM:
    case CONSTANT_MEM_CAND:
    case IMAGE_MEM:
        parseValues();
        break;
    case PIXEL:
    case BOUNDARY_COND:

        parseValuePairs();
        break;
    case GRID_SIZE:
        parseGridSize();
        break;
    }
}

PragmaOption Pragma::findPragmaOption()
{
    int matchlength = 0;
    for(pair<string,PragmaOption>  sp : pragmaStringToOption){
        if(pragmaString.find(sp.first) != string::npos){
            if(sp.first.length() > matchlength){
                this->option = sp.second;
                matchlength = sp.first.length();
            }
        }
    }
}

void Pragma::parseValues()
{
    vector<string> rawValues = StringUtils::tokenize(StringUtils::strip(StringUtils::getParenValue(pragmaString)), ",");

    for(string s : rawValues){
        this->values.insert(StringUtils::strip(s));
    }

}

void Pragma::parseValuePairs()
{
    vector<string> rawValuePairs = StringUtils::tokenize(StringUtils::strip(StringUtils::getParenValue(pragmaString)), ",");

    for(string rawValuePair : rawValuePairs){
        vector<string> p = StringUtils::tokenize(rawValuePair, ":");

        this->pairValues[StringUtils::strip(p[0])] = StringUtils::strip(p[1]);
    }
}

void Pragma::parseGridSize()
{
    vector<string> rawValues = StringUtils::tokenize(StringUtils::strip(StringUtils::getParenValue(pragmaString)), ",");

    this->x = stoi(StringUtils::strip(rawValues[0]));
    this->y = stoi(StringUtils::strip(rawValues[1]));
}


bool Pragma::operator <(const Pragma& lhs) const
{
    return this->pragmaString < lhs.pragmaString;
}

string Pragma::str()
{
    ostringstream s;


    s << pragmaOptionToString.at(option) << " : ";

    switch(option){
    case GRID:
    case LOCAL_MEM:
    case CONSTANT_MEM:
    case CONSTANT_MEM_CAND:
    case IMAGE_MEM:
        for(string v : values){
            s << v << ", ";
        }
        break;
    case PIXEL:
    case BOUNDARY_COND:
        for(pair<string, string> ps : pairValues){
            s << ps.first << ":" << ps.second << ", ";
        }
        break;
    case GRID_SIZE:
        s << this->x << "," << this->y;
        break;
    }

    return s.str();
}


string StringUtils::strip(string s)
{
    //There must be a better way...
    while(s.at(0) == ' '){
        s = s.substr(1,s.size()-1);
    }

    while(s.at(s.size()-1) == ' '){
        s = s.substr(0, s.size()-1);
    }

    return s;
}

string StringUtils::getParenValue(string s)
{
    int openParen = s.find("(");
    int closeParen = s.find(")");

    if(openParen < s.size() && closeParen <s.size())
        return s.substr(openParen +1, (closeParen-openParen) -1);

    return "";
}

vector<string> StringUtils::tokenize(string s, string split){

    vector<string> tokens;

    size_t splitPos = s.find(split);
    while(splitPos != string::npos){
        string token = s.substr(0,splitPos);
        if(token.length() > 0){
            tokens.push_back(token);
        }
        s = s.substr(splitPos+split.length(), s.length()-1);
        splitPos = s.find(split);
    }
    if(s.length() > 0){
        tokens.push_back(s);
    }

    return tokens;
}
