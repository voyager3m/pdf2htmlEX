#include "MCItem.h"

#define _OPEN_SYS_ITOA_EXT
#include <stdlib.h>


namespace pdf2htmlEX {

// add_parent
//void MCItem::add_parent(const MCItem &item) 
//{
//    parent_type         = item.type;
//    parent_alt_text     = item.alt_text;
//    parent_summary      = item.summary;
//    parent_actual_text  = item.actual_text;   
//    parent_id           = item.id;
//}

// dump
void MCItem::dump(std::ostream & out) const
{
    out << " data-tag-id=\"" << id << "\" ";

/*
    out << " data-tag-type=\"" << type << "\" ";
    if (!alt_text.empty()) {
        out << "data-tag-alttext=\"" << Base64Stream(alt_text) << "\" ";
    }
    if (!actual_text.empty()) {
        out << "data-tag-actualtext=\"" << Base64Stream(actual_text) << "\" ";
    }
    if (!summary.empty()) {
        out << "data-tag-summary=\"" << Base64Stream(summary) << "\" ";
    }
    if (print_parent && is_parent()) {
        out << "data-tag-parent-type=\"" << parent_type << "\" ";
        out << "data-tag-parent-id=\"" << parent_id << "\" ";
        if (!parent_alt_text.empty()) {
            out << "data-tag-partent-alttext=\"" << Base64Stream(parent_alt_text) << "\" ";
        }
        if (!parent_actual_text.empty()) {
            out << "data-tag-parent-actualtext=\"" << Base64Stream(parent_actual_text) << "\" ";
        }
        if (!parent_summary.empty()) {
            out << "data-tag-parent-summary=\"" << Base64Stream(parent_summary) << "\" ";
        }
    }
    */
}


// getJson
Json::Value MCItem::getJson() const
{
    Json::Value val;
    val["type"] = type;
    if (!alt_text.empty()) val["alt_text"] = alt_text;
    if (!actual_text.empty()) val["act_text"] = actual_text;
    if (!title.empty()) val["title"] = title;
    if (!summary.empty()) val["summary"] = summary;

    char buf[16];
    snprintf(buf, sizeof(buf), "%x", id);
    val["id"] = buf;

    //Json::Value parent;
    //parent["type"] = parent_type;
    //parent["alt_text"] = parent_alt_text;
    //parent["act_text"] = parent_actual_text;
    //parent["summary"] = parent_summary;
    //parent["id"] = parent_id;
    //val["parent"] = parent;
    if (!children.empty()) {
      Json::Value ch_arr(Json::arrayValue);
      for (auto const &i: children) {
        ch_arr.append(i.getJson());
      }
      val["children"] = ch_arr;
    }
    return val;
}

}