#include <stdio.h>
#include <string.h>
#include <string>
#include <malloc.h>
#include <memory>
#include "dirent.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "base64.h"
#include <io.h>
#include "tinyxml.h"
#include "rsa.h"
#include "kdm.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#define BUF_SIZE 4096
static uint8_t aes_key[16] = {0x71,0x99,0xE4,0x07,0x7C,0xD2,0x60,0xCB,0xF5,0xE0,0xCB,0x12,0xD5,0xF1,0x27,0x31};

static int HexToNum(char c);
static int ScanHexOctet( unsigned char octxt[], const char *str, int sz);

static unsigned long long get_filesize(char * fn)
{
    int fd;
    struct _stat64  buf;
    DIR  * dir;
    /*测试是否目录*/
    dir= opendir(fn);
    if(dir) {
        closedir(dir);
        return 0;
    }

    fd= _open(fn, O_RDONLY);
    if(fd== -1) {
        //perror("open");
        //fprintf(stderr,"open %s failed!\n",fn);
        return 0;
    }
    _close (fd);
    _stat64(fn, &buf);
    fflush(stdout);
    return buf.st_size;
}

static int aes_decrypt(uint8_t *in, uint8_t *out, uint32_t length, uint8_t *key)
{
    int ret = 0 ;
    AES_KEY round_key;
    uint8_t iv[16] = {0x5B,0x5B,0x58,0x78,0x20,0xFF,0x90,0xE4,0x22,0x66,0xC7,0x03,0xCB,0xAE,0xEA,0xE1};

    ret = AES_set_decrypt_key(key, 128, &round_key);
    if(ret < 0)return -1;

    AES_cbc_encrypt(in, out, length, &round_key, iv, AES_DECRYPT);
    return 0;
}

static int HexToNum(char c)
{
    switch(c)
    {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    default: return -1;
    }
}

static int ScanHexOctet( unsigned char octxt[], const char *str, int sz)
{
    int i = 0;
    int j = 0;
    for(i=0;i<sz && *str != 0;i++, str++)
    {
        int h = HexToNum(*str);
        if(h == -1) return 0;
        str++;
        i++;
        if(!(i<sz  && *str != 0)) break;
        int l = HexToNum(*str);
        if(l == -1) return 0;
        octxt[j++] = (unsigned char)((h<<4) | l);
    }
    if( i<sz)
    {
        return 0;
    }
    return 1;
}

static int get_key_buffer(char *&buf, unsigned int &len, const char *keyfile)
{
    FILE * fp_in = NULL;        /* 输入加密文件*/
    uint32_t file_size,orig_size,aes_size;     /* 加密的文件的大小*/
	uint8_t *buf_in, *buf_out;
    //uint32_t orig_size = 0;     /* 原始文件的大小*/
    //uint32_t aes_size = 0;      /* 需要解密的大小*/
    uint8_t del_size =  0;      /* 填充的大小*/
    //uint8_t *buf_out = NULL;    /* 解密后的buf */
    uint8_t md5[16];            /* md5校验结果 */
    uint8_t *p = NULL;
    int ret = 0;

    /* 获取信息 读取文件*/
    file_size = (uint32_t)get_filesize((char*)keyfile);
    if(file_size < 16)
    {
//        fprintf(stderr, "%s is a dir or size(%u) is not correct\n", keyfile, file_size);
        return -1;
    }
	aes_size = file_size - 16;
    buf_in = (uint8_t *)malloc(file_size);
    if(buf_in == NULL)
    {
        perror("malloc in");
        ret = -1;
        goto cleanup;
    }
    buf_out = (uint8_t *)malloc(aes_size);
    if(buf_out == NULL)
    {
        perror("malloc out");
        ret = -1;
        goto cleanup;
    }

    fp_in = fopen(keyfile,"rb");
    if(fp_in == NULL)
    {
//        fprintf(stderr,"cannot open %s\n",keyfile);
        ret = -1;
        goto cleanup;
    }

    p = buf_in;
    while(!feof(fp_in))
    {
        int rd = fread(p, 1, BUF_SIZE, fp_in);
        p += rd;
    }
    if( (uint32_t)(p - buf_in) != file_size)
    {
//        fprintf(stderr, "read error,read num:%d\n", p-buf_in);
        ret = -1;
        goto cleanup;
    }
    else
    {
        fclose(fp_in);
        fp_in = NULL;
    }
    /* 先校验文件 */
    EVP_Digest(buf_in, aes_size, md5, NULL, EVP_md5(), NULL);
//    print_key("md5:", md5);
    if(memcmp(md5, buf_in+aes_size, 16) != 0)
    {
//        fprintf(stderr, "md5 is not right\n");
        ret = -1;
        goto cleanup;
    }

    /* 解密 */
    ret = aes_decrypt(buf_in, buf_out, aes_size, aes_key);
    if(ret < 0)
    {
//        fprintf(stderr, "aes error\n");
        ret = -1;
        goto cleanup;
    }

    del_size = *(buf_out+aes_size-1);
    //fprintf(stderr, "del_size=%u\n", del_size);
    orig_size = aes_size - del_size;

    len = orig_size;
    buf = (char *)buf_out;

cleanup:
    if(buf_in)
    {
        free(buf_in);buf_in = NULL;
    }

    if(ret <0 && buf_out)
    {
        free(buf_out);buf_out = NULL;
    }

    if(fp_in)
    {
        fclose(fp_in);fp_in = NULL;
    }

    return ret;
}

int PrintUuid(char uuid_buf[45], const unsigned char uuid[16], int flag)
{
    const char *fmt = 0;
    if( flag & UUID_NO_PREFIX)
    {
        if( flag & UUID_NO_HYPHEN)
        {
            fmt = "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x";
        }
        else
        {
            fmt = "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x";
        }
    }
    else
    {
        if( flag & UUID_NO_HYPHEN)
        {
            fmt = "urn:uuid:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x";
        }
        else
        {
            fmt = "urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x";
        }
    }

    snprintf( uuid_buf, 45, fmt,
              uuid[0],uuid[1],uuid[2],uuid[3],
              uuid[4],uuid[5],
              uuid[6],uuid[7],
              uuid[8],uuid[9],
              uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15]);
    return 0;
}

int ParseUuid(unsigned char uuid[16], const char *uuid_str)
{
    // urn:uuid:2d68a8b6-a0df-4242-a8dc-ffbfbe126c40
    const char *p = strstr( uuid_str, "urn:uuid:");
    if( p == 0)
    {
        return 0;
    }
    p += 9;                     // skip urn:uuid:
    if(!ScanHexOctet(uuid, p, 8)) return 0;
    p += 8;
    if(*p!='-') return 0;
    p++;

    if(!ScanHexOctet(uuid+4, p, 4)) return 0;
    p += 4;
    if(*p!='-') return 0;
    p++;

    if(!ScanHexOctet(uuid+6, p, 4)) return 0;
    p += 4;
    if(*p!='-') return 0;
    p++;


    if(!ScanHexOctet(uuid+8, p, 4)) return 0;
    p += 4;
    if(*p!='-') return 0;
    p++;

    if(!ScanHexOctet(uuid+10, p, 12)) return 0;

    // print it;
#if 0
    fprintf( stderr, "ParseUuid:%s->", uuid_str);
    for( int i=0; i < 16; i++)
    {
        fprintf(stderr, "%02X", uuid[i]);
    }
    fprintf( stderr,"\n");
#endif
    return 1;
}

int ParseUtc( unsigned long long *utc, const char *utc_str)
{
    // 2004-05-01T13:20:00-08:00
    int year, month, day, hour, min, sec, dist_hour, dist_min;
    if( 8 != sscanf( utc_str, "%d-%d-%dT%d:%d:%d%d:%d",
                     &year, &month, &day,
                     &hour, &min, &sec,
                     &dist_hour, &dist_min))
    {
        fprintf( stderr, "utc_str=\"%s\"\n", utc_str);
        return -1;
    }
    //fprintf( stderr, "%d %d %d %d %d %d %d %d\n", year, month, day, hour, min, sec, dist_hour, dist_min);
    struct tm tm_utc;
    if( year < 1900) return -1;
    if( month < 1 || month > 12) return -1;
    if( day < 1 || day > 31) return -1;
    if( hour < 0 || hour > 23) return -1;
    if( min < 0 || min > 59) return -1;
    if( sec < 0 || sec > 59) return -1;
    if( dist_hour < 0 || dist_hour > 23) return -1;
    if( dist_min < 0 || dist_min > 59) return -1;
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month -1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hour;
    tm_utc.tm_min = min;
    tm_utc.tm_sec = sec;

    time_t time_utc = mktime(&tm_utc); // time_utc: the local time

    // TODO: set the time dist!!

    *utc = (unsigned long long)time_utc;
    return 0;
}

KDM::KDM()
{
    memset(msg_uuid,0,sizeof(msg_uuid));
    memset(cpl_uuid,0,sizeof(cpl_uuid));
    nv_after = 0;
    nv_before = 0;
}

KDM::~KDM()
{
    Close();
}

void KDM::Close()
{
    memset(msg_uuid,0,sizeof(msg_uuid));
    memset(cpl_uuid,0,sizeof(cpl_uuid));
    nv_after = 0;
    nv_before = 0;
    list_dec_key_.clear();
    content_title.clear();
}

int KDM::Open(const char *kdm_fn,int thestorage)
{
    int i = 0;
    int ret = 0;
    std::auto_ptr<TiXmlDocument> kdm_doc(new TiXmlDocument());
 //选择kdm的存储路径，1是本地，2是nfs
    if(1)
    {
        if(!(kdm_doc->LoadFile(kdm_fn)))
        {
            printf("kdm open error!\n");
            return -1001;
        }
    }
    TiXmlHandle xml_handle(kdm_doc.get());
    TiXmlText *p_txt =
            xml_handle.
            FirstChild("DCinemaSecurityMessage").
            FirstChild("AuthenticatedPublic").
            FirstChild("MessageId").
            FirstChild().Text();
    if( p_txt == 0)                        //有的含有etm:
        {
            p_txt = xml_handle.
                FirstChild("etm:DCinemaSecurityMessage").
                FirstChild("etm:AuthenticatedPublic").
                FirstChild("etm:MessageId").
                FirstChild().Text();
            //fprintf( stderr, "ERR:Kdm::Open: MessageId not found\n");
            if(p_txt == 0)
            {
                return -1003;
            }
        }
    if( !ParseUuid(msg_uuid, p_txt->Value()))
        {
//            fprintf( stderr, "ERR:Kdm::Open: \"%s\' not a msg-uuid\n", p_txt->Value());
            return -1004;
        }
    TiXmlElement * p_xml_kdm = NULL;
    p_xml_kdm =  xml_handle.
                   FirstChild("DCinemaSecurityMessage").
                   FirstChild("AuthenticatedPublic").
                   FirstChild("RequiredExtensions").
                   Element();
    p_txt = p_xml_kdm->
            FirstChild("CompositionPlaylistId")->
            FirstChild()->
            ToText();
    if( p_txt == 0)
    {
        fprintf( stderr, "ERR:Kdm::Open: CPL uuid not found\n");
        return -1005;
    }
    if( !ParseUuid(cpl_uuid, p_txt->Value()))
    {
        fprintf( stderr, "ERR:Kdm::Open: \"%s\" not a cpl-uuid\n", p_txt->Value());
        return -1005;
    }
    p_txt = p_xml_kdm->
            FirstChild("ContentTitleText")->
            FirstChild()->
            ToText();
    content_title = p_txt->Value();
    p_txt = p_xml_kdm->
            FirstChild("ContentKeysNotValidBefore")->
            FirstChild()->
            ToText();
    if( ParseUtc( &nv_before, p_txt->Value()) < 0)
    {
        return -1006;
    }
    p_txt = p_xml_kdm->
            FirstChild("ContentKeysNotValidAfter")->
            FirstChild()->
            ToText();
    if( ParseUtc( &nv_after, p_txt->Value()) < 0)
    {
        return -1006;
    }
    TiXmlNode * p_node = NULL;
    TiXmlNode * p_keyid_list = NULL;
    TiXmlElement *p_key = NULL;
    TiXmlElement *p_elem = NULL;
    p_keyid_list = p_xml_kdm->FirstChild("KeyIdList");
    if(p_keyid_list == 0)
    {
        //fprintf(stderr, "getKdmInfo: key id list not found[%s:%d]" );
        return -1007;
    }
    p_node = p_keyid_list->FirstChild("KeyId");
    p_key = p_node->ToElement();
    p_txt = p_key->FirstChild()->ToText();
    KdmEncKey enc_key;
    if( !ParseUuid(enc_key.key_uuid_, p_txt->Value()))
    {
        fprintf(stderr, "ERR:Kdm::Open: key id [%d] \"%s\" not a uuid\n", i, p_txt->Value());
        ret = -1008;
    }
    list_enc_key_.push_back(enc_key);
    p_elem = xml_handle.
                FirstChild("DCinemaSecurityMessage").
                FirstChild("AuthenticatedPrivate").
                FirstChild("enc:EncryptedKey").Element();
    ret = 0;
    i = 0;
    std::list<KdmEncKey>::iterator iter_begin, iter_end = list_enc_key_.end();
    for(p_key = p_elem, iter_begin = list_enc_key_.begin();
            iter_begin != iter_end && p_key != 0;
            i++, iter_begin ++, p_key = p_key->NextSiblingElement("enc:EncryptedKey"))
        {

            TiXmlElement *p_cd = p_key->FirstChildElement( "enc:CipherData");
            if( p_cd == 0)
            {
                fprintf( stderr, "ERR:Kdm::Open: cipher data[%d] not found\n", i);
                ret = -1009;
                break;
            }
            TiXmlElement *p_cv = p_cd->FirstChildElement("enc:CipherValue");
            if( p_cv == 0)
            {
                fprintf( stderr, "ERR:Kdm::Open: cipher value[%d] not found\n", i);
                ret = -1009;
                break;
            }
            p_txt = p_cv->FirstChild()->ToText();
            if( p_txt == 0)
            {
                fprintf( stderr, "ERR:Kdm::Open: cipher value[%d] not found\n",i);
                ret = -1009;
                break;
            }
            unsigned int out_size = 0,alloc_size = 0;
            void *p_data = base64_dec( p_txt->Value(), &out_size, &alloc_size);
            if( p_data == 0 || out_size < 256)
            {
                if( p_data)
                {
                    free(p_data);
                }
                fprintf( stderr, "ERR:Kdm::Open: cipher value[%d] bad format out=%u, alloc=%u\n", i, out_size, alloc_size);
                ret = -1009;
                break;
            }
            KdmEncKey &enc_key = *iter_begin;
            memcpy( enc_key.enc_key_, p_data, 256);
            free(p_data);
        }
    if( ret == 0)
        {
            if( iter_begin != iter_end)
            {
                fprintf(stderr, "WRN:Kdm::Open: key id more than cipher value, ignored\n");
                while(iter_begin != iter_end)
                {
                    std::list<KdmEncKey>::iterator iter = iter_begin;
                    ++iter_begin;
                    list_enc_key_.erase(iter);
                }
            }
            else if( p_key != 0 )
            {
                fprintf( stderr, "WRN:Kdm::Open: key id less than cipher value[%d], ignored\n", i);
            }
        }
        else
        {
            return ret;
        }
    return 0;
}

int KDM::Dec(const char *pri_fn)
{
    //pri_fn = "D:/QT Test/build-BestMoviePlayer-Desktop_Qt_5_4_2_MinGW_32bit-Debug/release/privatekey/SMT00002.prikey.enc";
    int ret = 0;
    char *key_buf = NULL;
    unsigned int key_len = 0;
    int i = 0;
    char utc_buf[26];
    std::list<KdmEncKey>::iterator iter, iter_end = list_enc_key_.end();

    if( !IsOpen())
    {
        ret = -1010;
        goto end__;
    }
    if( IsDec())
    {
        goto end__;
    }
        // st get key buffer
    ret = get_key_buffer(key_buf, key_len, pri_fn);
    if(ret < 0)
    {
        fprintf(stderr, "ERR:Kdm::Dec: get key buffer err\n");
        ret = -1010;
        goto end__;
    }
    for( iter = list_enc_key_.begin(), i=0; iter != iter_end; ++iter, ++i)
    {
        KdmDecKey dec_key;
        unsigned char dec_buf[256];
        int dec_size = 0;
        memset( dec_buf, 0, sizeof(dec_buf));
        dec_size = rsa_decrypt_from_buffer(key_buf, key_len, iter->enc_key_, dec_buf);

        if( dec_size < 0)
        {
            fprintf( stderr, "ERR:Kdm::Dec: decryt[%d] fail\n", i);
            ret = -1011;
            goto end__;
        }
        if( dec_size == 138)
        {
            memcpy( dec_key.cpl_uuid_, dec_buf + 36 , 16);
            memcpy( dec_key.key_uuid_, dec_buf + 56 , 16);
            memcpy( utc_buf, dec_buf + 72, 25);
            utc_buf[25] = 0;
            if( ParseUtc(&(dec_key.nv_before), utc_buf) < 0)
            {
                ret = -1012;
                goto end__;
            }
    memcpy( utc_buf, dec_buf + 97, 25);
    utc_buf[25] = 0;
    if( ParseUtc(&(dec_key.nv_after), utc_buf) < 0)
    {
        ret = -1013;
        goto end__;
    }
    memcpy( dec_key.aes_key_, dec_buf + 122, 16);
        }
        else if(dec_size == 134)
        {
            memcpy( dec_key.cpl_uuid_, dec_buf + 36 , 16);
            memcpy( dec_key.key_uuid_, dec_buf + 52 , 16);

            memcpy( utc_buf, dec_buf + 68, 25);
            utc_buf[25] = 0;
            if( ParseUtc(&(dec_key.nv_before), utc_buf) < 0)
            {
                ret = -1014;
                goto end__;
            }

            memcpy( utc_buf, dec_buf + 93, 25);
            utc_buf[25] = 0;
            if( ParseUtc(&(dec_key.nv_after), utc_buf) < 0)
            {
                ret = -1013;
                goto end__;
            }

            memcpy( dec_key.aes_key_, dec_buf + 118, 16);
        }
        else
        {
            fprintf( stderr, "ERR:Kdm::Dec: invalid decryped size=%d [%d]\n", dec_size, i);
            ret = -1015;
            goto end__;
        }
        list_dec_key_.push_back(dec_key);
    }

end__:
    if(key_buf)
    {
        free(key_buf);key_buf = NULL;
    }
    return ret;
}
