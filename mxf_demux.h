#ifndef MXF_DEMUX_H
#define MXF_DEMUX_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef uint8_t UID[16];
typedef int64_t offset_t;

typedef enum packet_type
{
    TRIPLET_PACKET,//encrypted triplet
    ESSENCE_PACKET,//essence element
}PACKET_TYPE;

typedef struct {
    UID uid;
    unsigned matching_len;
    int id;
} MXFCodecUL;

enum CodecID {
    CODEC_ID_NONE= 0x0,
    
    /* video codecs */
    CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
    CODEC_ID_MPEG4,
    CODEC_ID_RAWVIDEO,
    CODEC_ID_DVVIDEO,
    CODEC_ID_JPEG2000,
    CODEC_ID_DNXHD,

    /* various PCM "codecs" */
    CODEC_ID_PCM_S16LE= 0x10000,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_PCM_ALAW,
    CODEC_ID_PCM,

    /* audio codecs */
    CODEC_ID_MP2= 0x15000,
    CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,

    /* added codecs */
    CODEC_ID_H264 = 0x20000,
};

typedef struct
{
    PACKET_TYPE type;//��������

    UID key;
    offset_t offset;//key��offset
    uint64_t length;//value��length

    uint8_t ivec[16];//iv
    uint8_t aes_key[16];//key
    uint8_t checkbuf[16];//����CHECK KEY

    offset_t plain_offset;//����offset
    uint32_t plain_size;
    offset_t aes_offset;//����offset
    uint32_t aes_size;

    uint32_t orig_size;//�������ǰ�Ĵ�С

    offset_t end;//KLV��ENDλ��

    int channels;               /* ��Ƶ������ */
    int bits_per_sample;        /* ��Ƶÿ������ռ�õ�bitλ */
    int sampling_rate;          /* ��Ƶ������ */

    int codecID;                /* enum CodecID���� */
    UID descriptor;             /* ES������ */
    
    int found;                  /* if index found or not */
    uint32_t index;             /* for input */
    uint64_t pos;               /* for output */
} KLVPacket;

//retrurn:  -1:bytes_num����8�ֽ�  0:�ɹ�
int mxf_read_header(int fd, KLVPacket *klv);

//return:  -2:�ļ�����  0:�ɹ���ȡһ��ES�� -1:bytes_num����8�ֽ� -3:triplet error
int mxf_read_packet(int fd, KLVPacket *klv);

//����һ��packet
//return:  -2:�ļ�����  0:�ɹ����� -1:bytes_num����8�ֽ�
int mxf_skip_packet(int fd, KLVPacket *klv);

/* filename:mxf file path */
/* ret <0:error  =0:ok */
int mxf_get_index_position(const char *filename, uint32_t index, uint64_t *pos);

/* filename:mxf file path */
/* ret <0:error  =0:ok */
int mxf_get_position(const char *filename, uint64_t *pbody, uint64_t *pfooter);

#ifdef __cplusplus
}
#endif

#endif
