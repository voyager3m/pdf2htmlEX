/*
 * Parameters
 *
 * Wang Lu
 * 2012.08.03
 */


#ifndef PARAM_H__
#define PARAM_H__

#include <string>
#include <sstream>

namespace pdf2htmlEX {

struct Param
{
    // pages
    int first_page, last_page;

    // dimensions
    double zoom;
    double fit_width, fit_height;
    int use_cropbox;
    double desired_dpi;
    double actual_dpi;
    double max_dpi;
    double text_dpi;

    // output
    int embed_css;
    int embed_font;
    int embed_image;
    int embed_javascript;
    int embed_outline;
    int split_pages;
    std::string dest_dir;
    std::string css_filename;
    std::string page_filename;
    std::string outline_filename;
    int process_nontext;
    int process_outline;
    int process_annotation;
    int process_form;
    int correct_text_visibility;
    int printing;
    int fallback;
    int tmp_file_size_limit;

    // fonts
    int embed_external_font;
    std::string font_format;
    int decompose_ligature;
    int turn_off_ligatures;
    int auto_hint;
    std::string external_hint_tool;
    int stretch_narrow_glyph;
    int squeeze_wide_glyph;
    int override_fstype;
    int process_type3;

    // text
    double h_eps, v_eps;
    double space_threshold;
    double font_size_multiplier;
    int space_as_offset;
    int tounicode;
    int optimize_text;

    // background image
    std::string bg_format;
    int svg_node_count_limit;
    int svg_embed_bitmap;

    // encryption
    std::string owner_password, user_password;
    int no_drm;

    // misc.
    int clean_tmp;
    std::string data_dir;
    std::string poppler_data_dir;
    std::string tmp_dir;
    int debug;
    int proof;
    int quiet;
    int memstat; // add cpu and mem stat to console output
    int disable_ref; // disable reference table in output file
    int tags; // process tags

    bool use_console_pipeline; // use console pipeline for input/output file

    std::string input_filename, output_filename;

    void dump(std::ostream &s) const;
};

} // namespace pdf2htmlEX

#endif //PARAM_h__
