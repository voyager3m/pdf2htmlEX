
#include "OutlineRec.h"

using namespace pdf2htmlEX;

void OutlineRec::add_text(const Unicode *u, int len)
{
    if (u && len > 0 ) {
        text.reserve(len);
        text.assign(u, u + len);
    }
}