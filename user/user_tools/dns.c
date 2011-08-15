/* 
 * DNS Parser 
 * Ref: tools.ietf.org/html/rfc1035
 * Written by: cooky451 - 8/15/2011
 * Last change: cooky451 - 8/15/2011
 * Todo: Parser for authority, additional. (See dns_header)
 *       Parser for RR's: NS, MD, MF, SOA, MB, MG, MR, NULL, 
 *           WKS, PTR, HINFO, MINFO, MX, TXT. (See dns_qtype)
 * Comments: Take a look at Ref: 2.3.4. Size limits. (Bottom of file)
 */

#include "stdint.h"
#include "string.h"
#include "dns.h"

const int dns_port = 53;

void dns_copyInverse(void* dst, const void* src, size_t size)
{ // same as htons() + memcpy()
    size_t i;
    for (i = 0; size--; ++i)
        *((char*)dst + i) = *((char*)src + size);
}

void dns_fillHeaderWithFlags(dns_header *header, const dns_flags *flags)
{
    header->flags = (flags->QR << 15) | (flags->OPCODE << 11) | 
                    (flags->AA << 10) | (flags->TC << 9) | 
                    (flags->RD << 8) | (flags->RA << 7) | 
                    (flags->Z << 4) | (flags->RCODE << 0);
}

size_t dns_writeHeaderToBuffer(char *buf, size_t buf_size, 
    const dns_header *header)
{
    if (buf_size >= 12)
    {
        dns_copyInverse(buf + 0, &header->id, 2);
        dns_copyInverse(buf + 2, &header->flags, 2);
        dns_copyInverse(buf + 4, &header->qdcount, 2);
        dns_copyInverse(buf + 6, &header->ancount, 2);
        dns_copyInverse(buf + 8, &header->nscount, 2);
        dns_copyInverse(buf + 10, &header->arcount, 2);
        return 12;
    }
    return 0;
}

size_t dns_writeQuestionToBuffer(char *buf, size_t buf_size, 
    const dns_question *question)
{
    size_t need_space = strlen(question->qname) + 6;
    if (buf_size >= need_space && need_space < 256 + 4)
    {
        // "www.henkessoft.de" -> "\x03www\x0Ahenkessoft\0x02de"
        // Ref: 4.1.2. Question section format, -> QNAME
        //      
        char *p;
        uint16_t n;
        strcpy(buf + 1, question->qname);
        while ((p = strchr(buf + 1, '.')))
        {
            if (p - buf - 1 < 0x100)
            {
                *buf = p - buf - 1;
                buf = p;
            }
            else
            {
                return 0;
            }
        }
        if ((n = strlen(buf + 1)) < 64)
        {
            *buf = (char)n;
            buf += n + 1;
            *buf++ = '\0';
            dns_copyInverse(buf + 0, &question->qtype, 2);
            dns_copyInverse(buf + 2, &question->qclass, 2);
            return need_space;
        }
    }
    return 0;
}

size_t dns_createSimpleQueryBuffer(char *buf, size_t buf_size, 
    const dns_header *header, const dns_question *question)
{
    int w = dns_writeHeaderToBuffer(buf, buf_size, header);
    if (w)
    {
        int v = dns_writeQuestionToBuffer(buf + w, buf_size - w, question);
        if (v)
        {
            return w + v;
        }
    }
    return 0;
}

size_t dns_createSimpleQuery(char *buf, size_t buf_size, 
    const char *url, uint16_t id)
{
    if (strlen(url) < 256)
    {
        dns_flags flags;
        dns_header header;
        dns_question question;
        flags.AA = 0;
        flags.OPCODE = 0; // standard query
        flags.QR = 0;
        flags.RA = 0;
        flags.RCODE = 0;
        flags.RD = 1; // recursion desired
        flags.TC = 0;
        flags.Z = 0;
        header.ancount = 0;
        header.arcount = 0;
        dns_fillHeaderWithFlags(&header, &flags);
        header.id = id;
        header.nscount = 0;
        header.qdcount = 1; // one query
        question.qclass = dns_class_IN; // internet
        strcpy(question.qname, url);
        question.qtype = dns_type_A; // IPv4 addr
        return dns_createSimpleQueryBuffer(buf, 
            buf_size, &header, &question);
    }
    return 0;
}

const char* dns_parseHeader(dns_header *header, 
    const char *buf, size_t buf_size)
{
    if (buf_size >= 12)
    {
        dns_copyInverse(&header->id, buf + 0, 2);
        dns_copyInverse(&header->flags, buf + 2, 2);
        dns_copyInverse(&header->qdcount, buf + 4, 2);
        dns_copyInverse(&header->ancount, buf + 6, 2);
        dns_copyInverse(&header->nscount, buf + 8, 2);
        dns_copyInverse(&header->arcount, buf + 10, 2);
        return buf + 12;
    }
    return 0;
}

const char* dns_parseName(char *dst, const char *buf, 
    size_t buf_size, const char *pos)
{
    size_t size = buf_size - (pos - buf);
    size_t written = 0;
    const char *p = 0;
    while (size)
    {
        unsigned char i = *pos++;
        for (; i != 0 && i < size && i < 64 && 
            i + written < 256; i = *pos++)
        {
            size -= i + 1;
            written += i + 1;
            while (i--)
                *dst++ = *pos++;
            *dst++ = '.';
        }
        if (i == 0)
        {
            *(--dst) = '\0';
            return p != 0 ? p : pos;
        }
        if (((i & 192) == 192) && size >= 2 )
        { // pointer found
            p = p ? p : pos + 1; // only the first time
            pos = buf + (((i << 8) | *pos) & 0x3FFF);
            if (pos < buf + buf_size)
            {
                size = buf_size - (pos - buf);
                continue;
            }
        }
        break;
    }
    return 0;
}

const char* dns_parseQuestion(dns_question *question, 
    const char *buf, size_t buf_size, const char *pos)
{
    if (buf_size)
    {
        const char *p = dns_parseName(question->qname, 
            buf, buf_size, pos);
        if (p && buf_size - (p - buf) >= 4)
        {
            dns_copyInverse(&question->qtype, p + 0, 2);
            dns_copyInverse(&question->qclass, p + 2, 2);
            return p + 4;
        }
    }
    return 0;
}

const char* dns_parseResource(dns_resource* resource, 
    const char* buf, size_t buf_size, const char* pos)
{
    if (buf_size)
    {
        const char* p = dns_parseName(resource->name, 
            buf, buf_size, pos);
        if (p && buf_size - (p - buf) >= 10)
        {
            dns_copyInverse(&resource->type, p + 0, 2);
            dns_copyInverse(&resource->dns_class, p + 2, 2);
            dns_copyInverse(&resource->ttl, p + 4, 4);
            dns_copyInverse(&resource->rdlength, p + 8, 2);
            p += 10;
            if (buf_size - (p - buf) >= resource->rdlength)
            {
                resource->rdata = p;
                return p + resource->rdlength;
            }
        }
    }
    return 0;
}

/* Ref:
 * 2.3.4. Size limits
 * 
 * Various objects and parameters in the DNS have size limits.  They are
 * listed below.  Some could be easily changed, others are more
 * fundamental.
 * 
 * labels          63 octets or less
 * 
 * names           255 octets or less
 * 
 * TTL             positive values of a signed 32 bit number.
 * 
 * UDP messages    512 octets or less 
 */
