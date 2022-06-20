/*
 * Header file for OutlineRec - save outline info to html attribuutes
 * Copyright (C) 2022 Alex Zhmudovskyi (voyager3m@gmail.com)
 */

#ifndef OUTLINE_REC_H__
#define OUTLINE_REC_H__

#include <vector>
#include <map>
#include <string>
#include <poppler/CharTypes.h>

namespace pdf2htmlEX {


// for outline records (inline in html)
struct OutlineRec {
    double              left;
    double              top;
    std::string         title; // UTF-8 base64 encoded string
    int                 level;
    std::vector<int>    text;

    void add_text(const Unicode *u, int len);
};

typedef std::vector<OutlineRec>         OutlineRecVec;
typedef std::map<int, OutlineRecVec>    OutlineRecMap;


} //namespace pdf2htmlEX 
#endif //OUTLINE_REC_H__
