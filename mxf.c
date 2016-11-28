#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "mxf.h"

#define IS_KLV_KEY(x, y) (!memcmp(x, y, sizeof(y)))
#define PRINT_KEY(s, x) fprintf(stderr, "%s %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", s, \
                             (x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5], (x)[6], (x)[7], (x)[8], (x)[9], (x)[10], (x)[11], (x)[12], (x)[13], (x)[14], (x)[15])

static const uint8_t mxf_klv_key[]                         = { 0x06,0x0e,0x2b,0x34 }; //for sync
static const uint8_t mxf_essence_element_key[]             = { 0x06,0x0e,0x2b,0x34,0x01,0x02,0x01,0x01,0x0d,0x01,0x03,0x01 };
static const uint8_t mxf_encrypted_triplet_key[]           = { 0x06,0x0e,0x2b,0x34,0x02,0x04,0x01,0x07,0x0d,0x01,0x03,0x01,0x02,0x7e,0x01,0x00 };
static const uint8_t mxf_footer_partition_pack_key[]       = { 0x06,0x0e,0x2b,0x34,0x02,0x05,0x01,0x01,0x0d,0x01,0x02,0x01,0x01,0x04 };

static uint8_t get_byte(FILE * fp)
{
    unsigned char buf[1];
    uint8_t one_byte;
    size_t tmp_ret = 0;
    tmp_ret = fread(buf, 1, 1, fp);
    if(tmp_ret != 1)
    {
    }
    one_byte = (uint8_t)(buf[0]);
    return one_byte;
}

static void get_buffer(FILE * fp, unsigned char *buf, int size)
{
    size_t tmp_ret = 0;
    tmp_ret = fread(buf, 1, size, fp);
    if(tmp_ret != (size_t)size)
    {
    }
    return;    
}

//寻找4字节头,指针移到size后
//return:  1:找到了对应的key  0:没找到 
static int mxf_read_sync(FILE * fp, const uint8_t *key, int size)
{
    int i, b;
    for (i = 0; i < size && !feof(fp); i++)
    {
        b = get_byte(fp);
        if (b == key[0])
            i = 0;
        else if (b != key[i])
            i = -1;
    }
    return i == size;
}

//读取KLV结构的length
//return: int64_t:length  -1:bytes_num超过8字节  
static uint64_t klv_decode_ber_length(FILE * fp)
{
    uint64_t size = get_byte(fp);
    if (size & 0x80) { /* long form */
        int bytes_num = size & 0x7f;
        /* SMPTE 379M 5.3.4 guarantee that bytes_num must not exceed 8 bytes */
        if (bytes_num > 8)
            return -1;
        size = 0;
        while (bytes_num--)
            size = size << 8 | get_byte(fp);
    }
    return size;
}

//获取KLV包的信息
//retrun:  -1:bytes_num超过8字节 0:成功 -2:没找到同步头
static int klv_read_packet(FILE * fp, KLVPkt *klv)
{
    if (!mxf_read_sync(fp, mxf_klv_key, 4))
        return -2;
    klv->offset = _ftelli64(fp) - 4;
    memcpy(klv->key, mxf_klv_key, 4);
    get_buffer(fp, klv->key + 4, 12);
    klv->length = klv_decode_ber_length(fp);
    klv->end = _ftelli64(fp) + klv->length;
    return (int64_t)(klv->length) == -1 ? -1 : 0;
}

int mxf_read_es_packet(FILE * fp, KLVPkt *klv)
{
    int ret;
        
    while (!feof(fp)) 
    {
        if ((ret = klv_read_packet(fp, klv)) < 0)
            return -1;

#ifdef _DEBUG
        PRINT_KEY("read packet", klv->key);
#endif

        if (IS_KLV_KEY(klv->key, mxf_encrypted_triplet_key))
        {
            klv->type = TRIPLET_PACKET;
            return 0;
        }
        else if (IS_KLV_KEY(klv->key, mxf_essence_element_key))
        {
            klv->type = ESSENCE_PACKET;
            //klv->plain_size = 0;
            //klv->aes_size = 0;
            /* check for 8 channels AES3 element */
            if (klv->key[12] == 0x06 && klv->key[13] == 0x01 && klv->key[14] == 0x10)
            {
               
                fprintf(stderr,"mxf_read_packet:now AES_key\n");
                goto skip;
            } 
            else
            {
                return 0;                
            }
        }
        else if (IS_KLV_KEY(klv->key, mxf_footer_partition_pack_key))
        {
            fprintf(stderr, "end of the file\n");
            return -2;
        }
        else
        {
            //是别的key
            fprintf(stderr,"mxf_read_packet: unknow packet\n");
        }
    skip:
        _fseeki64(fp, klv->length, SEEK_CUR);
    }
    return -2;
}
