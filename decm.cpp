#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <io.h>
#include "dirent.h"

#include "decm.h"
#include "get_cpl.h"
#include "mxf_demux.h"
#include "aes.h"
#include "wave.h"

#define MAX_INSTANCE 1
static int current_instance = 0;

#define MAJOR 1
#define MINOR 0
static char libver[9];


struct mxf_context
{
	res_path_t res_path;
	dcp_para_t dcp_para;
	cpl_info_t cpl_info;

	int current_reel;
	uint8_t aes_key_video[16];
	uint8_t aes_key_audio[16];
};

static const uint8_t mxf_essence_element_key[] = { 0x06,0x0e,0x2b,0x34,0x01,0x02,0x01,0x01,0x0d,0x01,0x03,0x01 };
static const uint8_t checkv[16] = { 0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b };

static int mxf_disk_get_assetmap_path(const char *dirname, char *path, int pathmaxlen)
{
	DIR *dir = NULL;
	struct dirent * ptr = NULL;
	int ret = -1;

	dir = opendir(dirname);
	if (dir == NULL)
	{
		perror("opendir");
	}
	else
	{
		while ((ptr = readdir(dir)) != NULL)
		{
			if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))
				continue;

			if (_stricmp(ptr->d_name, "ASSETMAP") == 0 ||
				_stricmp(ptr->d_name, "ASSETMAP.xml") == 0 ||
				_stricmp(ptr->d_name, "assetmap") == 0 ||
				_stricmp(ptr->d_name, "assetmap.xml") == 0)
			{
				_snprintf(path, pathmaxlen-1, "%s/%s", dirname, ptr->d_name);
				printf("The AssetMap = %s\n", path);
				ret = 0;
				break;
			}
		}
		closedir(dir);
	}
	return ret;
}

static int mxf_disk_get_volindex_path(const char *dirname, char *path, int pathmaxlen)
{
	DIR *dir = NULL;
	struct dirent * ptr = NULL;
	int ret = -1;

	dir = opendir(dirname);
	if (dir == NULL)
	{
		perror("opendir");
	}
	else
	{
		while ((ptr = readdir(dir)) != NULL)
		{
			if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))
				continue;

			if (_stricmp(ptr->d_name, "VOLINDEX") == 0 ||
				_stricmp(ptr->d_name, "VOLINMDEX.xml") == 0 ||
				_stricmp(ptr->d_name, "volindex") == 0 ||
				_stricmp(ptr->d_name, "volindex.xml") == 0)
			{
				_snprintf(path, pathmaxlen - 1, "%s/%s", dirname, ptr->d_name);
				printf("The Volindex = %s\n", path);
				ret = 0;
				break;
			}
		}
		closedir(dir);
	}
	return ret;
}

static void getCplData(const char * cpluuid, char cpldata[16])
{
	int n;
	char cplstr[64];
	int data[16];
	for (n = 0; n<(int)strlen(cpluuid); n++)
	{
		cplstr[n] = tolower(cpluuid[n]);
	}
	cplstr[n] = '\0';
	sscanf(cplstr, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		&data[0], &data[1], &data[2], &data[3],
		&data[4], &data[5], &data[6], &data[7],
		&data[8], &data[9], &data[10], &data[11],
		&data[12], &data[13], &data[14], &data[15]);
	for (n = 0; n<16; n++)
	{
		cpldata[n] = (char)data[n];
		//fprintf(stderr, "%02x ", (unsigned char)cpldata[n]);
	}
	//fprintf(stderr, "\n");
}


int decm_open(void ** ppdecm, const char * dcp_path, const char * kdm_path)
{
	unsigned int j;
	int ret;
	char path[MAX_PATH_LEN];
	assetmap_info_t assetmap = { 0, };
	KLVPacket klv_audio;
	KLVPacket klv_video;
	int needkdm = 0;
	int decmerrno = 0;
	struct mxf_context *pContext = NULL;

	memset(&assetmap, 0, sizeof(assetmap_info_t));

	if (current_instance == MAX_INSTANCE)
	{
		decmerrno = -M_ERR_MAX_INSTANCE;
		goto err__;
	}

	pContext = (mxf_context *)malloc(sizeof(struct mxf_context));
	if (pContext == NULL)
	{
		decmerrno = -M_ERR_MALLOC;
		goto err__;
	}
	memset(pContext, 0, sizeof(struct mxf_context));

	//step 1
	ret = mxf_disk_get_assetmap_path(dcp_path, path, sizeof(path));
	if (ret < 0)
	{
		decmerrno = -M_ERR_NO_ASSETMAP;
		goto err__;
	}
	_snprintf_s(pContext->res_path.Assetmap, sizeof(pContext->res_path.Assetmap), "%s", path);

	ret = mxf_disk_get_volindex_path(dcp_path, path, sizeof(path));
	if (ret < 0)
	{
		return -M_ERR_NO_VOLINDEX;
		goto err__;
	}
	_snprintf_s(pContext->res_path.VolumeIndex, sizeof(pContext->res_path.VolumeIndex), "%s", path);


	//step2
	ret = GetAssetMap(pContext->res_path.Assetmap, &assetmap);
	if (ret < 0)
	{
		ClearAssetMap(&assetmap);
		decmerrno = -M_ERR_WRONG_ASSETMAP;
		goto err__;
	}

	if (0)  //tiaoshi info
	{
		fprintf(stderr, "[%s] [%llu] [%u]\n", assetmap.path_, assetmap.file_size_, assetmap.asset_num_);
		for (j = 0; j<assetmap.asset_num_; j++)
		{
			fprintf(stderr, "[%s] [%s] [Type:%d] [%llu] [complete:%d] [PKLUUID:%s]\n",
				assetmap.p_asset_[j].path_,
				assetmap.p_asset_[j].uuid_,
				assetmap.p_asset_[j].type_,
				assetmap.p_asset_[j].file_size_,
				assetmap.p_asset_[j].is_complete_,
				assetmap.p_asset_[j].pkl_uuid_);
		}
	}
	for (j = 0; j<assetmap.asset_num_; j++)
	{
		if (assetmap.p_asset_[j].type_ == ASSET_PKL)
		{
			snprintf(pContext->res_path.PKL, sizeof(pContext->res_path.PKL), "%s", assetmap.p_asset_[j].path_);
			break;
		}
	}
	if (j == assetmap.asset_num_)
	{
		decmerrno = -M_ERR_NO_PKL;
		goto err__;
	}
	for (j = 0; j<assetmap.asset_num_; j++)
	{
		if (assetmap.p_asset_[j].type_ == ASSET_CPL)
		{
			snprintf(pContext->res_path.CPL, sizeof(pContext->res_path.CPL), "%s", assetmap.p_asset_[j].path_);
			break;
		}
	}
	if (j == assetmap.asset_num_)
	{
		decmerrno = -M_ERR_NO_CPL;
		goto err__;
	}

	//step3
	for (j = 0; j<assetmap.asset_num_; j++)
	{
		if (assetmap.p_asset_[j].type_ == ASSET_CPL)
		{
			ret = GetCplInfo(j, &assetmap, &(pContext->cpl_info));
			if (ret <0)
			{
				ClearCplInfo(&(pContext->cpl_info));
				continue;
			}
			break;
		}
	}
	if (j == assetmap.asset_num_) /* 没有找到 */
	{
		decmerrno = -M_ERR_PARSE_CPL;
		goto err__;
	}
	//fprintf(stderr, "GetCplInfo OK\n");
	ClearAssetMap(&assetmap);
	if (1) //tiaoshi info
	{
		fprintf(stderr, "\n");
		fprintf(stderr, "[%s] [%s] [%s]\n", pContext->cpl_info.uuid_, pContext->cpl_info.content_title_, pContext->cpl_info.content_kind_);
		fprintf(stderr, "[%s] [%s] [%s] [complete:%d] [reelnum:%u]\n",
			pContext->cpl_info.assetmap_path_, pContext->cpl_info.pkl_path_, pContext->cpl_info.path_, pContext->cpl_info.is_complete_, pContext->cpl_info.reel_num_);
		for (j = 0; j<pContext->cpl_info.reel_num_; j++)
		{
			fprintf(stderr, "[Reel%u V] [%s] [%s] [%d/%d] [%lu] [%lu] [%lu] [%d/%d] [3D:%d] [keyid:%s]\n",
				j,
				pContext->cpl_info.p_reel_[j].v_path_,
				pContext->cpl_info.p_reel_[j].v_uuid_,
				pContext->cpl_info.p_reel_[j].v_editrate_.Numerator, pContext->cpl_info.p_reel_[j].v_editrate_.Denominator,
				pContext->cpl_info.p_reel_[j].v_intrinsic_duration_,
				pContext->cpl_info.p_reel_[j].v_entrypoint_,
				pContext->cpl_info.p_reel_[j].v_duration_,
				pContext->cpl_info.p_reel_[j].v_framerate_.Numerator, pContext->cpl_info.p_reel_[j].v_framerate_.Denominator,
				pContext->cpl_info.p_reel_[j].is_stereoscopic_,
				pContext->cpl_info.p_reel_[j].v_keyid_);
			fprintf(stderr, "[Reel%u A] [%s] [%s] [%d/%d] [%lu] [%lu] [%lu] [keyid:%s]\n",
				j,
				pContext->cpl_info.p_reel_[j].a_path_,
				pContext->cpl_info.p_reel_[j].a_uuid_,
				pContext->cpl_info.p_reel_[j].a_editrate_.Numerator, pContext->cpl_info.p_reel_[j].a_editrate_.Denominator,
				pContext->cpl_info.p_reel_[j].a_intrinsic_duration_,
				pContext->cpl_info.p_reel_[j].a_entrypoint_,
				pContext->cpl_info.p_reel_[j].a_duration_,
				pContext->cpl_info.p_reel_[j].a_keyid_);
		}
		fprintf(stderr, "\n");
	}
	if (pContext->cpl_info.is_complete_ == 0)
	{
		decmerrno = -M_ERR_CPL_NOT_COMPLETE;
		goto err__;
	}

	pContext->dcp_para.num_reels = pContext->cpl_info.reel_num_;
	for (j = 0; j<pContext->cpl_info.reel_num_; j++)
	{
		uint64_t start_v, end_v;
		uint64_t start_a, end_a;
		mxf_get_position(pContext->cpl_info.p_reel_[j].v_path_, &start_v, &end_v);
		mxf_get_position(pContext->cpl_info.p_reel_[j].a_path_, &start_a, &end_a);

		if (1) {
			int tmp_fd = 0;
			tmp_fd = open(pContext->cpl_info.p_reel_[j].v_path_, O_RDONLY);
			if (tmp_fd == -1)
			{
				perror("open");
				decmerrno = -M_ERR_OPEN_VIDEO_REEL;
				goto err__;
			}
			mxf_read_header(tmp_fd, &klv_video);
			//fprintf(stderr, "ParseVideoInfo: \n\tcodecid:%d\n", klv_video.codecID);
			close(tmp_fd);

			tmp_fd = open(pContext->cpl_info.p_reel_[j].a_path_, O_RDONLY);
			if (tmp_fd == -1)
			{
				perror("open");
				decmerrno = -M_ERR_OPEN_AUDIO_REEL;
				goto err__;
			}
			mxf_read_header(tmp_fd, &klv_audio);
			//fprintf(stderr, "ParseAudioInfo: \n\tcodecid:%d\n\tchannels:%d\n\tbits_per_sample:%d\n\tsampling_rate:%d\n", klv_audio.codecID, klv_audio.channels, klv_audio.bits_per_sample, klv_audio.sampling_rate);
			close(tmp_fd);
		}

		pContext->dcp_para.reels[j].video.EntryPoint = pContext->cpl_info.p_reel_[j].v_entrypoint_;
		pContext->dcp_para.reels[j].video.duration = pContext->cpl_info.p_reel_[j].v_duration_;
		pContext->dcp_para.reels[j].video.IntrinsicDuration = pContext->cpl_info.p_reel_[j].v_intrinsic_duration_;
		pContext->dcp_para.reels[j].video.EditRateNum = pContext->cpl_info.p_reel_[j].v_editrate_.Numerator;
		pContext->dcp_para.reels[j].video.EditRateDen = pContext->cpl_info.p_reel_[j].v_editrate_.Denominator;
		pContext->dcp_para.reels[j].video.mxf_name = (char *)malloc(strlen(pContext->cpl_info.p_reel_[j].v_path_) + 1);
		if (pContext->dcp_para.reels[j].video.mxf_name)
		{
			strcpy(pContext->dcp_para.reels[j].video.mxf_name, pContext->cpl_info.p_reel_[j].v_path_);
		}
		else
		{
			decmerrno = -M_ERR_MALLOC;
			goto err__;
		}
		pContext->dcp_para.reels[j].video.ofs_start = start_v;
		pContext->dcp_para.reels[j].video.ofs_end = end_v;
		pContext->dcp_para.reels[j].video.num_uuid = 1;
		memcpy(pContext->dcp_para.reels[j].video.uuid[0], klv_video.descriptor, 16);
		pContext->dcp_para.reels[j].video.encryption[0] = strlen(pContext->cpl_info.p_reel_[j].v_keyid_) ? 1 : 0;
		if (pContext->dcp_para.reels[j].video.encryption[0]) needkdm = 1;
		//pContext->dcp_para.reels[j].video.format
		if (klv_video.codecID == CODEC_ID_MPEG2VIDEO) pContext->dcp_para.reels[j].video.format = CODEC_MPEG2VIDEO;
		else if (klv_video.codecID == CODEC_ID_H264) pContext->dcp_para.reels[j].video.format = CODEC_H264;
		else pContext->dcp_para.reels[j].video.format = CODEC_UNKNOWN;
		pContext->dcp_para.reels[j].video.pcm_rate = 0;
		pContext->dcp_para.reels[j].video.pcm_bits = 0;
		pContext->dcp_para.reels[j].video.pcm_nchn = 0;

		pContext->dcp_para.reels[j].audio.EntryPoint = pContext->cpl_info.p_reel_[j].a_entrypoint_;
		pContext->dcp_para.reels[j].audio.duration = pContext->cpl_info.p_reel_[j].a_duration_;
		pContext->dcp_para.reels[j].audio.IntrinsicDuration = pContext->cpl_info.p_reel_[j].a_intrinsic_duration_;
		pContext->dcp_para.reels[j].audio.EditRateNum = pContext->cpl_info.p_reel_[j].a_editrate_.Numerator;
		pContext->dcp_para.reels[j].audio.EditRateDen = pContext->cpl_info.p_reel_[j].a_editrate_.Denominator;
		pContext->dcp_para.reels[j].audio.mxf_name = (char *)malloc(strlen(pContext->cpl_info.p_reel_[j].a_path_) + 1);
		if (pContext->dcp_para.reels[j].audio.mxf_name)
		{
			strcpy(pContext->dcp_para.reels[j].audio.mxf_name, pContext->cpl_info.p_reel_[j].a_path_);
		}
		else
		{
			decmerrno = -M_ERR_MALLOC;
			goto err__;
		}
		pContext->dcp_para.reels[j].audio.ofs_start = start_a;
		pContext->dcp_para.reels[j].audio.ofs_end = end_a;
		pContext->dcp_para.reels[j].audio.num_uuid = 1;
		memcpy(pContext->dcp_para.reels[j].audio.uuid[0], klv_audio.descriptor, 16);
		pContext->dcp_para.reels[j].audio.encryption[0] = strlen(pContext->cpl_info.p_reel_[j].a_keyid_) ? 1 : 0;
		if (pContext->dcp_para.reels[j].audio.encryption[0]) needkdm = 1;
		if (klv_audio.codecID == CODEC_ID_PCM) pContext->dcp_para.reels[j].audio.format = CODEC_PCM;
		else if (klv_audio.codecID == CODEC_ID_AC3) pContext->dcp_para.reels[j].audio.format = CODEC_AC3;
		else pContext->dcp_para.reels[j].audio.format = CODEC_UNKNOWN;
		if (pContext->dcp_para.reels[j].audio.format == CODEC_PCM)
		{
			pContext->dcp_para.reels[j].audio.pcm_rate = klv_audio.sampling_rate;
			pContext->dcp_para.reels[j].audio.pcm_bits = klv_audio.bits_per_sample;
			pContext->dcp_para.reels[j].audio.pcm_nchn = klv_audio.channels;
		}
		else
		{
			pContext->dcp_para.reels[j].audio.pcm_rate = 0;
			pContext->dcp_para.reels[j].audio.pcm_bits = 0;
			pContext->dcp_para.reels[j].audio.pcm_nchn = 0;
		}
	}
	fprintf(stderr, "decm_open ok\n");
	*ppdecm = pContext;
	current_instance++;
	return 0;
err__:
	//if (pContext) decm_destory(pContext);
	*ppdecm = NULL;
	return decmerrno;
}

int decm_get_resource_path(void *pdecm, res_path_t *pres_path)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	memcpy(pres_path, &(pContext->res_path), sizeof(pContext->res_path));
	return 0;
}

int decm_get_name(void * pdecm, char *name, int name_len_max)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	int titlelen = strlen(pContext->cpl_info.content_title_);
	if (titlelen + 1 > name_len_max)
	{
		return -1;
	}
	strcpy(name, pContext->cpl_info.content_title_);
	return 0;
}

static void decm_destory(void *pdecm)
{
	if (pdecm)
	{
		struct mxf_context *pContext = (struct mxf_context *)pdecm;
		unsigned int j;
		for (j = 0; j<(unsigned int)(pContext->dcp_para.num_reels); j++)
		{
			if (pContext->dcp_para.reels[j].video.mxf_name)
			{
				free(pContext->dcp_para.reels[j].video.mxf_name);
				pContext->dcp_para.reels[j].video.mxf_name = NULL;
			}
			if (pContext->dcp_para.reels[j].audio.mxf_name)
			{
				free(pContext->dcp_para.reels[j].audio.mxf_name);
				pContext->dcp_para.reels[j].audio.mxf_name = NULL;
			}
		}
		ClearCplInfo(&pContext->cpl_info);
		free(pContext);
	}

	return;
}

int decm_close(void * pdecm)
{
	if (pdecm)
	{
		char cpldata[16];
		struct mxf_context *pContext = (struct mxf_context *)pdecm;
		getCplData(pContext->cpl_info.uuid_, cpldata);
		decm_destory(pdecm);
		current_instance--;
	}
	return 0;
}

int decm_get_paras(void *pdecm, dcp_para_t * pdcp_para)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	memcpy(pdcp_para, &(pContext->dcp_para), sizeof(pContext->dcp_para));
	return 0;
}

static uint64_t klv_decode_ber_len(uchar ** pp)
{
	uchar * p = *pp;
	uint64_t size = (uint64_t)(*p);
	p++;
	if (size & 0x80) { /* long form */
		int bytes_num = size & 0x7f;
		/* SMPTE 379M 5.3.4 guarantee that bytes_num must not exceed 8 bytes */
		if (bytes_num > 8)
		{
			*pp = p;
			return -1;
		}
		size = 0;
		while (bytes_num--)
			size = size << 8 | (*p++);
	}
	*pp = p;
	return size;
}

static uint64_t get_be64_num(uchar ** pp)
{
	uchar * p = *pp;
	uint32_t valh;
	uint32_t vall;
	uint64_t val;
	valh = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
	vall = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	val = ((uint64_t)valh << 32) + vall;
	p += 8;
	*pp = p;
	return val;
}

//区别对待video,audio，先按video做
int decm_dcry(void *pdecm, uchar *in, int in_len, uchar *out, int *out_len, int out_len_max, int type)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	KLVPacket tmpklv;
	KLVPacket *klv = &tmpklv;
	uchar * p = in;
	uint64_t size;
	uint64_t plaintext_size;

	if (type != AUDIO && type != VIDEO)
	{
		return -M_ERR_DCRY_WRONG_TYPE;
	}
	if (type == AUDIO && strlen(pContext->cpl_info.p_reel_[pContext->current_reel].a_keyid_) == 0)
	{
		return -M_ERR_DCRY_NO_AUDIO_KEYID;
	}
	if (type == VIDEO && strlen(pContext->cpl_info.p_reel_[pContext->current_reel].v_keyid_) == 0)
	{
		return -M_ERR_DCRY_NO_VIDEO_KEYID;
	}

	// crypto context
	size = klv_decode_ber_len(&p);
	p += size;
	// plaintext offset
	size = klv_decode_ber_len(&p);
	plaintext_size = get_be64_num(&p);
	// source klv key
	size = klv_decode_ber_len(&p);

	memcpy(klv->key, p, 16);
	p += 16;

	if (memcmp(klv->key, mxf_essence_element_key, sizeof(mxf_essence_element_key)) != 0)
	{
		//fprintf(stderr, "mxf_decrypt_triplet: not a mxf_essence_element_key\n");
		return -M_ERR_ESSENCE_ELEMENT_KEY;
	}

	// source size
	klv_decode_ber_len(&p);
	klv->orig_size = get_be64_num(&p);
	if (klv->orig_size < plaintext_size)
	{
		//fprintf(stderr, "mxf_decrypt_triplet: orig_size < plaintext_size\n");
		return -M_ERR_ESSENCE_SIZE;
	}

	// enc. code
	size = klv_decode_ber_len(&p);
	if (size < 32 || size - 32 < klv->orig_size)
	{
		//fprintf(stderr, "mxf_decrypt_triplet: size < 32 || size - 32 < orig_size\n");
		return -M_ERR_ENC_SIZE;
	}

	memcpy(klv->ivec, p, 16);
	p += 16;
	memcpy(klv->checkbuf, p, 16);
	p += 16;

	klv->plain_offset = (offset_t)(p - in);
	klv->plain_size = plaintext_size;

	size -= 32;
	size -= plaintext_size;

	klv->aes_offset = klv->plain_offset + plaintext_size;
	klv->aes_size = size;

	{
		uint32_t i;
		uint8_t aes_key[16];

		if (type == VIDEO)
		{
			memcpy(aes_key, pContext->aes_key_video, 16);
		}
		else if (type == AUDIO)
		{
			memcpy(aes_key, pContext->aes_key_audio, 16);
		}
		else
		{
			return -M_ERR_DCRY_WRONG_TYPE;
		}

		{
			struct AVAES aesc;
			av_aes_init(&aesc, aes_key, 128, 1);
			av_aes_crypt(&aesc, klv->checkbuf, klv->checkbuf, 1, klv->ivec, 1);
			if (memcmp(klv->checkbuf, checkv, 16))
			{
				//fprintf(stderr, "probably incorrect decryption key\n");
				return -M_ERR_AES_CHECK;
			}
		}

		for (i = 0; i<klv->plain_size; i++)
		{
			out[i] = *(p + i);
		}
		p += klv->plain_size;
		{
			struct AVAES aesc;
			av_aes_init(&aesc, aes_key, 128, 1);
			av_aes_crypt(&aesc, out + klv->plain_size, p, klv->aes_size / 16, klv->ivec, 1);
		}
		*out_len = klv->orig_size;
	}

	return 0;
}

const char * decm_version(void)
{
	snprintf(libver, sizeof(libver), "%d.%d.%s", MAJOR, MINOR, "windowstest");
	return libver;
}

void decm_set_current_reel(void *pdecm, int idx)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	int i, j;
	pContext->current_reel = idx;
	i = pContext->current_reel;
	//if (strlen(pContext->cpl_info.p_reel_[i].v_keyid_)) /*  这个reel video需要KEYID */
	//{
	//	for (j = 0; j<pContext->kdm_info.key_num_; j++) /* 遍历所有找到的key */
	//	{
	//		if (strcmp(pContext->kdm_info.pKey_[j].uuid_,
	//			pContext->cpl_info.p_reel_[i].v_keyid_) == 0)
	//		{
	//			memcpy(pContext->aes_key_video, pContext->kdm_info.pKey_[j].aes_key_, 16);
	//			break; /* 找到了 break */
	//		}
	//	}
	//	if (j == pContext->kdm_info.key_num_)
	//	{
	//		//open时曾判定过，不应该执行到此处，属于有BUG
	//		//fprintf(stderr, "video key of reel[%d] not found quit...\n", i);
	//		return;
	//	}
	//}

	//if (strlen(pContext->cpl_info.p_reel_[i].a_keyid_)) /*  这个reel audio需要KEYID */
	//{
	//	for (j = 0; j<pContext->kdm_info.key_num_; j++) /* 遍历所有找到的key */
	//	{
	//		if (strcmp(pContext->kdm_info.pKey_[j].uuid_,
	//			pContext->cpl_info.p_reel_[i].a_keyid_) == 0)
	//		{
	//			memcpy(pContext->aes_key_audio, pContext->kdm_info.pKey_[j].aes_key_, 16);
	//			break; /* 找到了 break */
	//		}
	//	}
	//	if (j == pContext->kdm_info.key_num_)
	//	{
	//		//open时曾判定过，不应该执行到此处，属于有BUG
	//		//fprintf(stderr, "audio key of reel[%d] not found quit...\n", i);
	//		return;
	//	}
	//}
}

int decm_get_index_position(void *pdecm, int current_reel, int index, uint64_t *vpos, uint64_t *apos)
{
	struct mxf_context *pContext = (struct mxf_context *)pdecm;
	int ret;
	//int mxf_get_index_position(const char *filename, uint32_t index, uint64_t *pos);

	ret = mxf_get_index_position(pContext->cpl_info.p_reel_[current_reel].v_path_, (uint32_t)index, vpos);
	if (ret < 0)
	{
		return -1;
	}

	ret = mxf_get_index_position(pContext->cpl_info.p_reel_[current_reel].a_path_, (uint32_t)index, apos);
	if (ret < 0)
	{
		return -2;
	}

	return 0;
}