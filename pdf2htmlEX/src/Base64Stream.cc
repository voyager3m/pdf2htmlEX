#include "Base64Stream.h"
#include <sstream>

namespace pdf2htmlEX {

using std::ostream;


Base64Stream::Base64Stream(std::istream & in) : in(&in)
{
    need_clear = false;
}


Base64Stream::Base64Stream(const std::string  & str)
{ 
    in = new std::istringstream(str);
    need_clear = true;
}


Base64Stream::~Base64Stream() {
    if (in && need_clear) {
        delete in;
    }
}


ostream & Base64Stream::dumpto(ostream & out)
{
    unsigned char buf[3];
    while(in->read((char*)buf, 3))
    {
        out << base64_encoding[(buf[0] & 0xfc)>>2]
            << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)]
            << base64_encoding[((buf[1] & 0x0f)<<2) | ((buf[2] & 0xc0)>>6)]
            << base64_encoding[(buf[2] & 0x3f)];
    } 
    auto cnt = in->gcount();
    if(cnt > 0)
    {
        for(int i = cnt; i < 3; ++i)
            buf[i] = 0;

        out << base64_encoding[(buf[0] & 0xfc)>>2]
            << base64_encoding[((buf[0] & 0x03)<<4) | ((buf[1] & 0xf0)>>4)];

        if(cnt > 1)
        {
            out << base64_encoding[(buf[1] & 0x0f)<<2];
        }
        else
        {
            out <<  '=';
        }
        out << '=';
    }

    return out;
}

const char * Base64Stream::base64_encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

} //namespace pdf2htmlEX
