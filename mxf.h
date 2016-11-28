#ifndef _DECM_MXF_H
#define _DECM_MXF_H

#include <stdint.h>

typedef uint8_t UID[16];
typedef int64_t offset_t;

typedef enum packet_type
{
    TRIPLET_PACKET,//encrypted triplet
    ESSENCE_PACKET,//essence element
}PACKET_TYPE;

typedef struct
{
    PACKET_TYPE type;//包的类型
    UID key;
    offset_t offset;//key的offset
    uint64_t length;//value的length
    offset_t end;//KLV的END位置
} KLVPkt;

#ifdef __cplusplus
extern "C"
{
#endif //
	int mxf_read_es_packet(FILE * fp, KLVPkt *klv);

#ifdef __cplusplus
}
#endif // 
#endif
