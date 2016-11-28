#pragma once
#ifndef INCLUDE_DECM_H
#define INCLUDE_DECM_H

#include <stdint.h>

#define MAX_PATH_LEN 1024

typedef unsigned int uint;
typedef unsigned char uchar;

/*
	errno:返回值加上符号，比如return -M_ERR_NO_ASSETMAP
*/

enum  Error
{
	M_SUCCESS = 0,
	M_ERR_UNKNOWN = 1,
	M_ERR_NO_ASSETMAP,
	M_ERR_NO_VOLINDEX,
	M_ERR_WRONG_ASSETMAP,
	M_ERR_NO_PKL,
	M_ERR_NO_CPL,
	M_ERR_PARSE_CPL,
	M_ERR_CPL_NOT_COMPLETE,
	M_ERR_OPEN_VIDEO_REEL,
	M_ERR_OPEN_AUDIO_REEL = 10,
	M_ERR_NO_KDMFILE,
	M_ERR_NO_RIGHT_KDM,
	M_ERR_KDM_NV_BEFORE,
	M_ERR_KDM_NV_AFTER,
	M_ERR_MALLOC,
	M_ERR_DCRY_WRONG_TYPE,
	M_ERR_DCRY_NO_AUDIO_KEYID,
	M_ERR_DCRY_NO_VIDEO_KEYID,
	M_ERR_ESSENCE_ELEMENT_KEY,
	M_ERR_ESSENCE_SIZE = 20,
	M_ERR_ENC_SIZE,
	M_ERR_AES_CHECK,
	M_ERR_SOCKET_CREATE,
	M_ERR_SOCKET_CONNECT,
	M_ERR_SOCKET_SEND,
	M_ERR_SOCKET_READ,
	M_ERR_SOCKET_SN_TOO_LONG,
	M_ERR_SOCKET_MSG_LEN,
	M_ERR_SOCKET_MSG_PARSE,
	M_ERR_SOCKET_INVALID_SN = 30,
	M_ERR_SOCKET_INVALID_VER,
	M_ERR_SOCKET_INVALID_RESULT,
	M_ERR_SOCKET_INVALID_LOCAL_TIME,
	M_ERR_SERVER_INFO,
	M_ERR_PREPARE,
	M_ERR_MAX_INSTANCE,
};

enum Codec 
{
	CODEC_MPEG2VIDEO = 0,
	CODEC_H264 = 1,
	CODEC_PCM = 3,
	CODEC_AC3 = 4,
	CODEC_UNKNOWN = 999,
};

enum DataType
{
	AUDIO = 0,
	VIDEO = 1,
};

typedef struct
{
	char Assetmap[MAX_PATH_LEN];
	char CPL[MAX_PATH_LEN];
	char PKL[MAX_PATH_LEN];
	char KDM[MAX_PATH_LEN];
	char VolumeIndex[MAX_PATH_LEN];
}res_path_t;

typedef struct
{
	uint EntryPoint;            //入口时间，其类型定义待定
	uint duration;              //持续时间
	uint IntrinsicDuration;     //内在持续时间
	uint EditRateNum;          //编辑速率分子
	uint EditRateDen;          //编辑速率分母
	char *mxf_name;            //相应的mxf文件
	uint64_t ofs_start;        //BODY起始位置，单位字节
	uint64_t ofs_end;         //FOOTER起始位置，单位字节，等于BODY结束位置+1
	unsigned char uuid[4][16];//该类型资产所对应的KEY，可能出现多个KEY对应，如同时存在加密和不加密
	int encryption[4];       //显示哪个uuid包为加密，0：不加密，1：加密
	int num_uuid;            //对应的有效UUID的数量，预留，应该只有1个，其他UUID非音视频数据
	int format;              //0: MPEG-2; 1: H.264; 3: PCM; 4: AC3
	int pcm_rate;            //仅音频有效，音频采样率,对视频则=0
	int pcm_bits;            //仅音频有效, 采样位数，对视频则=0
	int pcm_nchn;            //通道数，音频通道数，对视频则=1
}track_t;

typedef struct
{
	track_t video;          //相应的video资产信息
	track_t audio;          //相应的audio资产信息
}reel_t;

typedef struct
{
	reel_t reels[128];      //按照播放顺序排列mxf播放表
	int num_reels;          //数量
}dcp_para_t;

#ifdef __cplusplus
extern "C"
{
#endif //
	int decm_open(void ** ppdecm, const char * dcp_path, const char * kdm_path);

	int decm_get_resource_path(void *pdecm, res_path_t *res_path);

	int decm_get_name(void *pdecm, char* name, int name_len_max);

	int decm_close(void *pdecm);

	int decm_get_paras(void *pdecm, dcp_para_t * dcp_para);

	void decm_set_current_reel(void *pdecm, int idx);

	int decm_dcry(void *pdecm, uchar *in, int in_len, uchar *out, int *out_len, int out_len_max, int type);

	int decm_get_index_position(void *pdecm, int current_reel, int index, uint64_t *vpos, uint64_t *apos);

	const char * decm_version(void);
#ifdef __cplusplus
}
#endif // 


#endif // !INCLUDE_DECM_H
