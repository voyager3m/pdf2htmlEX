/*
 * general.cc
 *
 * Handling general stuffs
 *
 * Copyright (C) 2012,2013,2014 Lu Wang <coolwanglu@gmail.com>
 */

#include <cstdio>
#include <ostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>
#include <sys/resource.h>

#include <GlobalParams.h>

#include "pdf2htmlEX-config.h"
#include "HTMLRenderer.h"
#include "HTMLTextLine.h"
#include "Base64Stream.h"

#include "BackgroundRenderer/BackgroundRenderer.h"

#include "util/namespace.h"
#include "util/ffw.h"
#include "util/math.h"
#include "util/path.h"
#include "util/css_const.h"
#include "util/encoding.h"
#include "util/misc.h"

namespace pdf2htmlEX {

using std::fixed;
using std::flush;
using std::ostream;
using std::max;
using std::min_element;
using std::vector;
using std::abs;
using std::cerr;
using std::endl;

HTMLRenderer::HTMLRenderer(const char* progPath, Param & param)
    :OutputDev()
    ,param(param)
    ,html_text_page(param, all_manager)
    ,preprocessor(param)
    ,tmp_files(param)
    ,covered_text_detector(param)
    ,tracer(param)
{
    if(!(param.debug))
    {
        //disable error messages of poppler
        globalParams->setErrQuiet(true);
    }

    ffw_init(progPath, param.debug);

    cur_mapping.resize(0x10000);
    cur_mapping2.resize(0x100);
    width_list.resize(0x10000);

    /*
     * For these states, usually the error will not be accumulated
     * or may be handled well (whitespace_manager)
     * So we can set a large eps here
     */
    all_manager.vertical_align.set_eps(param.v_eps);
    all_manager.whitespace    .set_eps(param.h_eps);
    all_manager.left          .set_eps(param.h_eps);
    /*
     * For other states, we need accurate values
     * optimization will be done separately
     */
    all_manager.font_size   .set_eps(EPS);
    all_manager.letter_space.set_eps(EPS);
    all_manager.word_space  .set_eps(EPS);
    all_manager.height      .set_eps(EPS);
    all_manager.width       .set_eps(EPS);
    all_manager.bottom      .set_eps(EPS);

    tracer.on_char_drawn =
            [this](cairo_t *cairo, double * box) { covered_text_detector.add_char_bbox(cairo, box); };
    tracer.on_char_clipped =
            [this](cairo_t *cairo, double * box, int partial) { covered_text_detector.add_char_bbox_clipped(cairo, box, partial); };
    tracer.on_non_char_drawn =
            [this](cairo_t *cairo, double * box, int what) { covered_text_detector.add_non_char_bbox(cairo, box, what); };
}

HTMLRenderer::~HTMLRenderer()
{
    ffw_finalize();
}

#define MAX_DIMEN 9000



void HTMLRenderer::process(PDFDoc *doc)
{
    cur_doc = doc;
    cur_catalog = doc->getCatalog();
    xref = doc->getXRef();

    pre_process(doc);

    /* get outline records */
    if (doc && doc->getOutline()) {
        dump_outline(&outline_recs, doc->getOutline()->getItems(), param.first_page, param.last_page, 1);
    }


    if (doc && doc->getStructTreeRoot()) {
        parse_treeroot(doc->getStructTreeRoot(), param.first_page, param.last_page);
    }

    ///////////////////
    // Process pages

    if(param.process_nontext)
    {
        bg_renderer = BackgroundRenderer::getBackgroundRenderer(param.bg_format, this, param);
        if(!bg_renderer)
            throw "Cannot initialize background renderer, unsupported format";
        bg_renderer->init(doc);

        fallback_bg_renderer = BackgroundRenderer::getFallbackBackgroundRenderer(this, param);
        if (fallback_bg_renderer)
            fallback_bg_renderer->init(doc);
    }

    int page_count = (param.last_page - param.first_page + 1);
    for(int i = param.first_page; i <= param.last_page ; ++i)
    {
        param.actual_dpi = param.desired_dpi;
        param.max_dpi = 72 * MAX_DIMEN / max(doc->getPageCropWidth(i), doc->getPageCropHeight(i));

        if (param.actual_dpi > param.max_dpi) {
            param.actual_dpi = param.max_dpi;
            printf("Warning:Page %d clamped to %f DPI\n", i, param.actual_dpi);
        }

        if (param.tmp_file_size_limit != -1 && tmp_files.get_total_size() > param.tmp_file_size_limit * 1024) {
            if(param.quiet == 0)
                cerr << "Stop processing, reach max size\n";
            break;
        }

        if (param.quiet == 0) {
          cerr << "Working: " << (i-param.first_page) << "/" << page_count;
          if(param.memstat != 0) {
            struct rusage r;
            if (getrusage(RUSAGE_SELF, &r) == 0) { // OK
              cerr <<  prints (" cpu:%3ld.%03lds mem: %7ld KB", r.ru_utime.tv_sec, r.ru_utime.tv_usec/1000, r.ru_maxrss);
            }
          }
          cerr << endl;
        }

        if(param.split_pages)
        {
            // copy the string out, since we will reuse the buffer soon
            string filled_template_filename = (char*)str_fmt(param.page_filename.c_str(), i);
            auto page_fn = str_fmt("%s/%s", param.dest_dir.c_str(), filled_template_filename.c_str());
            f_curpage = new ofstream((char*)page_fn, ofstream::binary);
            if(!(*f_curpage))
                throw string("Cannot open ") + (char*)page_fn + " for writing";
            set_stream_flags((*f_curpage));

            cur_page_filename = filled_template_filename;
        }

        doc->displayPage(this, i,
                text_zoom_factor() * DEFAULT_DPI, text_zoom_factor() * DEFAULT_DPI,
                0,
                (!(param.use_cropbox)),
                true,  // crop
                false, // printing
                nullptr, nullptr, nullptr, nullptr);

        if (param.desired_dpi != param.actual_dpi) {
            printf("Page %d DPI change %.1f => %.1f\n", i, param.desired_dpi, param.actual_dpi);
        }

        if(param.split_pages)
        {
            delete f_curpage;
            f_curpage = nullptr;
        }
    }
    if(page_count >= 0 && param.quiet == 0) {
      cerr << "Working: " << page_count << "/" << page_count;
      if(param.memstat != 0) {
        struct rusage r;
        if (getrusage(RUSAGE_SELF, &r) == 0) { // OK
          cerr <<  prints (" cpu:%3ld.%03lds mem: %7ld KB", r.ru_utime.tv_sec, r.ru_utime.tv_usec/1000, r.ru_maxrss);
        }
      }
    }

    if(param.quiet == 0)
        cerr << endl;

    ////////////////////////
    // Process Outline
    if(param.process_outline)
        process_outline();

    post_process();

    bg_renderer = nullptr;
    fallback_bg_renderer = nullptr;

    if(param.quiet == 0)
        cerr << endl;
}

void HTMLRenderer::setDefaultCTM(const double *ctm)
{
    memcpy(default_ctm, ctm, sizeof(default_ctm));
}

void HTMLRenderer::startPage(int pageNum, GfxState *state, XRef * xref)
{
    covered_text_detector.reset();
    tracer.reset(state);

    this->pageNum = pageNum;

    html_text_page.set_page_size(state->getPageWidth(), state->getPageHeight());

    reset_state();
}

void HTMLRenderer::endPage() {
    long long wid = all_manager.width.install(html_text_page.get_width());
    long long hid = all_manager.height.install(html_text_page.get_height());

    (*f_curpage)
        << "<div id=\"" << CSS::PAGE_FRAME_CN << pageNum
            << "\" class=\"" << CSS::PAGE_FRAME_CN
            << " " << CSS::WIDTH_CN << wid
            << " " << CSS::HEIGHT_CN << hid
            << "\" data-page-no=\"" << pageNum << "\">"
        << "<div class=\"" << CSS::PAGE_CONTENT_BOX_CN
            << " " << CSS::PAGE_CONTENT_BOX_CN << pageNum
            << " " << CSS::WIDTH_CN << wid
            << " " << CSS::HEIGHT_CN << hid
            << "\">";


    /*
     * When split_pages is on, f_curpage points to the current page file
     * and we want to output empty frames in f_pages.fs
     */
    if(param.split_pages)
    {
        f_pages.fs
            << "<div id=\"" << CSS::PAGE_FRAME_CN << pageNum
                << "\" class=\"" << CSS::PAGE_FRAME_CN
                << " " << CSS::WIDTH_CN << wid
                << " " << CSS::HEIGHT_CN << hid
                << "\" data-page-no=\"" << pageNum
                << "\" data-page-url=\"";

        writeAttribute(f_pages.fs, cur_page_filename);
        f_pages.fs << "\">";
    }

    if(param.process_nontext)
    {
        if (bg_renderer->render_page(cur_doc, pageNum))
        {
            bg_renderer->embed_image(pageNum);
        }
        else if (fallback_bg_renderer)
        {
            if (fallback_bg_renderer->render_page(cur_doc, pageNum))
                fallback_bg_renderer->embed_image(pageNum);
        }
    }

    // dump all text
    html_text_page.dump_text(*f_curpage, cur_doc, pageNum, &outline_recs);
    html_text_page.dump_css(f_css.fs);
    html_text_page.clear();

    // process form
    if(param.process_form)
        process_form(*f_curpage);
    
    // process links before the page is closed
    if (param.disable_ref == 0) {
      cur_doc->processLinks(this, pageNum);
    }

    // close box
    (*f_curpage) << "</div>";

    // dump info for js
    // TODO: create a function for this
    // BE CAREFUL WITH ESCAPES
    {
        (*f_curpage) << "<div class=\"" << CSS::PAGE_DATA_CN << "\" data-data='{";

        //default CTM
        (*f_curpage) << "\"ctm\":[";
        for(int i = 0; i < 6; ++i)
        {
            if(i > 0) (*f_curpage) << ",";
            (*f_curpage) << round(default_ctm[i]);
        }
        (*f_curpage) << "]";

        (*f_curpage) << "}'></div>";
    }

    // close page
    (*f_curpage) << "</div>" << endl;

    if(param.split_pages)
    {
        f_pages.fs << "</div>" << endl;
    }
}

void HTMLRenderer::pre_process(PDFDoc * doc)
{
    preprocessor.process(doc);

    /*
     * determine scale factors
     */
    {
        vector<double> zoom_factors;

        if(is_positive(param.zoom))
        {
            zoom_factors.push_back(param.zoom);
        }

        if(is_positive(param.fit_width))
        {
            zoom_factors.push_back((param.fit_width) / preprocessor.get_max_width());
        }

        if(is_positive(param.fit_height))
        {
            zoom_factors.push_back((param.fit_height) / preprocessor.get_max_height());
        }

        double zoom = (zoom_factors.empty() ? 1.0 : (*min_element(zoom_factors.begin(), zoom_factors.end())));

        text_scale_factor1 = max<double>(zoom, param.font_size_multiplier);
        text_scale_factor2 = zoom / text_scale_factor1;
    }

    // we may output utf8 characters, so always use binary
    {
        /*
         * If embed-css
         * we have to keep the generated css file into a temporary place
         * and embed it into the main html later
         *
         * otherwise
         * leave it in param.dest_dir
         */

        auto fn = (param.embed_css)
            ? str_fmt("%s/__css", param.tmp_dir.c_str())
            : str_fmt("%s/%s", param.dest_dir.c_str(), param.css_filename.c_str());

        if(param.embed_css)
            tmp_files.add((char*)fn);

        f_css.path = (char*)fn;
        f_css.fs.open(f_css.path, ofstream::binary);
        if(!f_css.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_css.fs);
    }

    if (param.process_outline)
    {
        /*
         * The logic for outline is similar to css
         */

        auto fn = (param.embed_outline)
            ? str_fmt("%s/__outline", param.tmp_dir.c_str())
            : str_fmt("%s/%s", param.dest_dir.c_str(), param.outline_filename.c_str());

        if(param.embed_outline)
            tmp_files.add((char*)fn);

        f_outline.path = (char*)fn;
        f_outline.fs.open(f_outline.path, ofstream::binary);
        if(!f_outline.fs)
            throw string("Cannot open") + (char*)fn + " for writing";

        // might not be necessary
        set_stream_flags(f_outline.fs);
    }

    {
        /*
         * we have to keep the html file for pages into a temporary place
         * because we'll have to embed css before it
         *
         * Otherwise just generate it
         */
        auto fn = str_fmt("%s/__pages", param.tmp_dir.c_str());
        tmp_files.add((char*)fn);

        f_pages.path = (char*)fn;
        f_pages.fs.open(f_pages.path, ofstream::binary);
        if(!f_pages.fs)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(f_pages.fs);
    }

    if(param.split_pages)
    {
        f_curpage = nullptr;
    }
    else
    {
        f_curpage = &f_pages.fs;
    }
}

void HTMLRenderer::post_process(void)
{
    dump_css();
    
    // close files if they opened
    if (param.process_outline)
    {
        f_outline.fs.close();
    }
    f_pages.fs.close();
    f_css.fs.close();

    // build the main HTML file
    ofstream output;
    {
        auto fn = str_fmt("%s/%s", param.dest_dir.c_str(), param.output_filename.c_str());
        output.open((char*)fn, ofstream::binary);
        if(!output)
            throw string("Cannot open ") + (char*)fn + " for writing";
        set_stream_flags(output);
    }

    // apply manifest
    ifstream manifest_fin((char*)str_fmt("%s/%s", param.data_dir.c_str(), MANIFEST_FILENAME.c_str()), ifstream::binary);
    if(!manifest_fin)
        throw "Cannot open the manifest file";

    bool embed_string = false;
    string line;
    long line_no = 0;
    while(getline(manifest_fin, line))
    {
        // trim space at both sides
        {
            static const char * whitespaces = " \t\n\v\f\r";
            auto idx1 = line.find_first_not_of(whitespaces);
            if(idx1 == string::npos)
            {
                line.clear();
            }
            else
            {
                auto idx2 = line.find_last_not_of(whitespaces);
                assert(idx2 >= idx1);
                line = line.substr(idx1, idx2 - idx1 + 1);
            }
        }

        ++line_no;

        if(line == "\"\"\"")
        {
            embed_string = !embed_string;
            continue;
        }

        if(embed_string)
        {
            output << line << endl;
            continue;
        }

        if(line.empty() || line[0] == '#')
            continue;


        if(line[0] == '@')
        {
            embed_file(output, param.data_dir + "/" + line.substr(1), "", true);
            continue;
        }

        if(line[0] == '$')
        {
            if(line == "$css")
            {
                embed_file(output, f_css.path, ".css", false);
            }
            else if (line == "$outline")
            {
                if (param.process_outline && param.embed_outline)
                {
                    ifstream fin(f_outline.path, ifstream::binary);
                    if(!fin)
                        throw "Cannot open outline for reading";
                    output << fin.rdbuf();
                    output.clear(); // output will set fail big if fin is empty
                }
            }
            else if (line == "$pages")
            {
                ifstream fin(f_pages.path, ifstream::binary);
                if(!fin)
                    throw "Cannot open pages for reading";
                output << fin.rdbuf();
                output.clear(); // output will set fail bit if fin is empty
            }
            else
            {
                cerr << "Warning: manifest line " << line_no << ": Unknown content \"" << line << "\"" << endl;
            }
            continue;
        }

        cerr << "Warning: unknown line in manifest: " << line << endl;
    }
}

void HTMLRenderer::set_stream_flags(std::ostream & out)
{
    // we output all ID's in hex
    // browsers are not happy with scientific notations
    out << hex << fixed;
}

void HTMLRenderer::dump_css (void)
{
    all_manager.transform_matrix.dump_css(f_css.fs);
    all_manager.vertical_align  .dump_css(f_css.fs);
    all_manager.letter_space    .dump_css(f_css.fs);
    all_manager.stroke_color    .dump_css(f_css.fs);
    all_manager.word_space      .dump_css(f_css.fs);
    all_manager.whitespace      .dump_css(f_css.fs);
    all_manager.fill_color      .dump_css(f_css.fs);
    all_manager.font_size       .dump_css(f_css.fs);
    all_manager.bottom          .dump_css(f_css.fs);
    all_manager.height          .dump_css(f_css.fs);
    all_manager.width           .dump_css(f_css.fs);
    all_manager.left            .dump_css(f_css.fs);
    all_manager.bgimage_size    .dump_css(f_css.fs);

    // print css
    if(param.printing)
    {
        double ps = print_scale();
        f_css.fs << CSS::PRINT_ONLY << "{" << endl;
        all_manager.transform_matrix.dump_print_css(f_css.fs, ps);
        all_manager.vertical_align  .dump_print_css(f_css.fs, ps);
        all_manager.letter_space    .dump_print_css(f_css.fs, ps);
        all_manager.stroke_color    .dump_print_css(f_css.fs, ps);
        all_manager.word_space      .dump_print_css(f_css.fs, ps);
        all_manager.whitespace      .dump_print_css(f_css.fs, ps);
        all_manager.fill_color      .dump_print_css(f_css.fs, ps);
        all_manager.font_size       .dump_print_css(f_css.fs, ps);
        all_manager.bottom          .dump_print_css(f_css.fs, ps);
        all_manager.height          .dump_print_css(f_css.fs, ps);
        all_manager.width           .dump_print_css(f_css.fs, ps);
        all_manager.left            .dump_print_css(f_css.fs, ps);
        all_manager.bgimage_size    .dump_print_css(f_css.fs, ps);
        f_css.fs << "}" << endl;
    }
}

void HTMLRenderer::embed_file(ostream & out, const string & path, const string & type, bool copy)
{
    string fn = get_filename(path);
    string suffix = (type == "") ? get_suffix(fn) : type;

    auto iter = EMBED_STRING_MAP.find(suffix);
    if(iter == EMBED_STRING_MAP.end())
    {
        cerr << "Warning: unknown suffix: " << suffix << endl;
        return;
    }

    const auto & entry = iter->second;

    if(param.*(entry.embed_flag))
    {
        ifstream fin(path, ifstream::binary);
        if(!fin)
            throw string("Cannot open file ") + path + " for embedding";
        out << entry.prefix_embed;

        if(entry.base64_encode)
        {
            out << Base64Stream(fin);
        }
        else
        {
            out << endl << fin.rdbuf();
        }
        out.clear(); // out will set fail big if fin is empty
        out << entry.suffix_embed << endl;
    }
    else
    {
        out << entry.prefix_external;
        writeAttribute(out, fn);
        out << entry.suffix_external << endl;

        if(copy)
        {
            ifstream fin(path, ifstream::binary);
            if(!fin)
                throw string("Cannot copy file: ") + path;
            auto out_path = param.dest_dir + "/" + fn;
            ofstream out(out_path, ofstream::binary);
            if(!out)
                throw string("Cannot open file ") + path + " for embedding";
            out << fin.rdbuf();
            out.clear(); // out will set fail big if fin is empty
        }
    }
}

const std::string HTMLRenderer::MANIFEST_FILENAME = "manifest";

std::string HTMLRenderer::UnicodeToUTF8(const Unicode *str, int len)
{
    typedef std::codecvt<char32_t,char,std::mbstate_t> facet_type;
    std::string result;
    std::locale mylocale;
    const facet_type& myfacet = std::use_facet<facet_type>(mylocale);

    // prepare objects to be filled by codecvt::out :
    char* pstr= new char [(len+1) * 5];        // the destination buffer
    std::mbstate_t mystate = std::mbstate_t(); // the shift state object
    const char32_t* pwc;                        // from_next
    char* pc;                                  // to_next

    // call codecvt::out (translate characters):
    facet_type::result myresult = myfacet.out (mystate,
        (const char32_t *)str, (const char32_t *)str + len , pwc,
        pstr, pstr+len*5, pc);

    if (myresult==facet_type::ok) {
        result = std::string((const char *)pstr, pc - pstr);;
    }

    delete[] pstr;
    return result;
}




// dump-outline
void HTMLRenderer::dump_outline(std::map<int, std::vector<OutlineRec>> *outline, const std::vector<OutlineItem *> *items, int firstpage, int lastpage, int deep)
{
    if (items && outline && items->size() > 0) {
        for (auto i : *items) {
            auto act = i->getAction();
            if (act && act->getKind() == LinkActionKind::actionGoTo)   {

                auto * link =  dynamic_cast<const LinkGoTo*>(act);
                std::unique_ptr<LinkDest> dest = nullptr;
                if(auto _ = link->getDest()) {
                    dest = std::unique_ptr<LinkDest>(new LinkDest(*_));
                } else if (auto _ = link->getNamedDest()) {
                    dest = cur_catalog->findDest(_);
                }

                if (dest) {
                    int pagenum = 0;
                    if (dest->isPageRef()) {
                        auto pageref = dest->getPageRef();
                        pagenum = cur_catalog->findPage(pageref);
                    } else {
                        pagenum = dest->getPageNum();
                    }

                    if (pagenum >= firstpage && pagenum <= lastpage) {
                        OutlineRec rec {0.000001, 0.000001};
                        rec.title = UnicodeToUTF8(i->getTitle(), i->getTitleLength());
                        rec.level = deep;
                        rec.used = false;
                        if (dest->getKind() == destXYZ) {
                            rec.left    = dest->getLeft();
                            rec.top     = dest->getTop();
                            rec.bottom  = dest->getBottom();
                        }
                        rec.add_text(i->getTitle(), i->getTitleLength());
                        (*outline)[pagenum].push_back(rec);
                        //printf("get outline record: x=%f top=%f bottom=%f l=%d pagenum=%d title=%s\n", rec.left, rec.top, rec.bottom, deep, pagenum, rec.title.c_str());
                    }
                    dest.reset();
                }
            }
            
            /* check for kids */
            if (i->hasKids()) {
                dump_outline(outline, i->getKids(), firstpage, lastpage, deep+1);
            }
        }
    }
}





// go_child    
void HTMLRenderer::go_child (const StructElement *el, HTMLTextLine::MCItem parent_item)
{
    int mcid = el->getMCID();
    HTMLTextLine::MCItem item;
    // static int level = 0;
    // level++;    
    // cerr << std::string(level * 2, ' ') << "mcid=" << el->getMCID()  << " type=" << el->getTypeName() << " numchild=" << el->getNumChildren() << " " ;
    // for (unsigned i = 0; i < el->getNumAttributes(); i++) {
    //     auto attr =  el->getAttribute(i);
    //     auto v = attr->getValue();
    //     cerr << attr->getTypeName() << "=[(" << v->getTypeName() << ") ";
    //     v->print(stderr);
    //     cerr << "] ";
    // }
    // cerr << " content="<< el->isContent() << " oref=" << el->isObjectRef() << " blk=" << el->isBlock();
    // if (el->getActualText()) { cerr << endl << std::string(level*2, ' ') << "+ acttext=[" << el->getActualText()->c_str() << "]"; }
    // if (el->getAltText())    { cerr << endl << std::string(level*2, ' ') << "+ altext=[" << el->getAltText()->c_str() << "]";     }
    // cerr << endl;

    if (el->getType() == StructElement::MCID) {
        if (parent_item) {
            if (mc_items.count(mcid) > 0) {
                //cerr << "EE DUPLICATE MCID " << mcid << endl;
            }
            mc_items[mcid] = parent_item;
        }
    } else if (!el->isContent() && el->getType() != StructElement::Document) {
        item.add_parent(parent_item);
        item.id = mcid;
        item.type = el->getTypeName();
        if (el->getAltText()) {
            item.alt_text = el->getAltText()->toStr();
        }
        if (el->getActualText()) {
            item.actual_text = el->getActualText()->c_str();
        }
        if (el->getType() == StructElement::Table) {
            // table may contain summary
            for (unsigned i = 0; i < el->getNumAttributes(); i++) {
                auto attr =  el->getAttribute(i);
                auto v = attr->getValue();
                if (v && v->isString()) {
                    if (attr->getType() == Attribute::Summary) {
                        item.summary = v->getString()->c_str();
                    }
                }
            }
        }
    }
    for (unsigned i = 0; i < el->getNumChildren(); i++) {
        go_child(el->getChild(i), item);
    }
    //level--;
}


// parse_treeroot get tags from treeroot (get page and coords on page)
void HTMLRenderer::parse_treeroot(const StructTreeRoot * treeroot, int firstpage, int lastpage)
{

    for (unsigned i = 0; i < treeroot->getNumChildren(); i++) {
        if (treeroot->getChild(i)->getType() == StructElement::Document) {
            auto elem = treeroot->getChild(i);
            HTMLTextLine::MCItem item;
            go_child(elem, item);
        }
    }
}


// endMarkedContent
void HTMLRenderer::endMarkedContent(GfxState *state)
{

}

// beginMarkedContent
void HTMLRenderer::beginMarkedContent(const char *name, Dict *properties)
{
    if (properties) {
        for (auto i = 0; i < properties->getLength(); i++) {
            std::string strmcid{"MCID"};
            if (properties->getKey(i) == strmcid) {
                auto v = properties->getVal(i);
                //cerr << "MC " << name << " val ";  v.print(stderr); cerr << " " << v.isIntOrInt64() << endl;
                if (v.isIntOrInt64()) {
                    int mcid = v.getInt();
                    if (mc_items.count(mcid) > 0) {
                        auto line = html_text_page.get_cur_line();
                        if (line) {
                          line->setMCItem(mc_items[mcid]);
                        } else {
                          std::cerr << "Line is NULL\n\n";
                        }
                        //cerr << mcid << " " << mc_items[mcid].type << endl;
                    }
                }
            }
        }
    }
}


}// namespace pdf2htmlEX
