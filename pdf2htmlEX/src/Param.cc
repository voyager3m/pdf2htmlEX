/*
 * Parameters
 *
 * Alex ZHmudovskyi (voyager3m@gmail.com)
 * 18.12.2022
 */


#include "Param.h"

using namespace std;
namespace pdf2htmlEX {

#define S(S, A) S << "  " << #A << "=" << A << std::endl;

void Param::dump(std::ostream &s) const
{
    s << endl << "pages" << endl;
    S(s, first_page);
    S(s, last_page);

    s << endl << "dimensions" << endl;
    S(s, zoom);
    S(s, fit_width);
    S(s, fit_height);
    S(s, use_cropbox);
    S(s, desired_dpi);
    S(s, actual_dpi);
    S(s, max_dpi);
    S(s, text_dpi);

    s << endl << "output" << endl;
    S(s, embed_css);
    S(s, embed_font);
    S(s, embed_image);
    S(s, embed_javascript);
    S(s, embed_outline);
    S(s, split_pages);
    S(s, dest_dir);
    S(s, css_filename);
    S(s, page_filename);
    S(s, outline_filename);
    S(s, process_nontext);
    S(s, process_outline);
    S(s, process_annotation);
    S(s, process_form);
    S(s, correct_text_visibility);
    S(s, printing);
    S(s, fallback);
    S(s, tmp_file_size_limit);

    s << endl << "fonts" << endl;
    S(s, embed_external_font);
    S(s, font_format);
    S(s, decompose_ligature);
    S(s, turn_off_ligatures);
    S(s, auto_hint);
    S(s, external_hint_tool);
    S(s, stretch_narrow_glyph);
    S(s, squeeze_wide_glyph);
    S(s, override_fstype);
    S(s, process_type3);

    s << endl << "text" << endl;
    S(s, h_eps)
    S(s, v_eps);
    S(s, space_threshold);
    S(s, font_size_multiplier);
    S(s, space_as_offset);
    S(s, tounicode);
    S(s, optimize_text);

    s << endl << "background image" << endl;
    S(s, bg_format);
    S(s, svg_node_count_limit);
    S(s, svg_embed_bitmap);

    s << endl << "encryption" << endl;
    S(s, owner_password)
    S(s, user_password);
    S(s, no_drm);

    s << endl << "misc." << endl;
    S(s, clean_tmp);
    S(s, data_dir);
    S(s, poppler_data_dir);
    S(s, tmp_dir);
    S(s, debug);
    S(s, proof);
    S(s, quiet);
    S(s, memstat); // add cpu and mem stat to console output
    S(s, disable_ref); // disable reference table in output file
    S(s, tags); // process tags

    s << endl << "use console pipeline for input/output file" << endl;
    S(s, use_console_pipeline); // 

    s << endl << "files" << endl;
    S(s, input_filename);
    S(s, output_filename);
};

} // namespace pdf2htmlEX

