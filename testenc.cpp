#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "mxf.h"
#include "wave.h"
#include "decm.h"

#define REC_DIR                 "E:/record"
#define REC_VIDEO_NAME          "rec_video_file"
#define REC_AUDIO_NAME          "rec_audio_file"

static int rec_on = 1;

static char * getuuidbuf(unsigned char * uuid, char * buf)
{
	sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
		uuid[0], uuid[1], uuid[2], uuid[3],
		uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11],
		uuid[12], uuid[13], uuid[14], uuid[15]);
	return buf;
}

static const char * getformat(int format)
{
	switch (format)
	{
	case CODEC_MPEG2VIDEO:
		return "MPEG-2";
	case CODEC_H264:
		return "H.264";
	case CODEC_PCM:
		return "PCM";
	case CODEC_AC3:
		return "AC3";
	default:
		return "UNKNOWN";
	}
}

int main()
{
	int j;
	int ret;
	void *pdecm;
	const char *dcpdir = "E:/hdmovlib/BHSJ";
	const char *kdmpath = NULL;
	res_path_t res_path;
	char dcpname[256];
	dcp_para_t dcp_para;

	ret = decm_open(&pdecm, dcpdir, kdmpath);
	if (ret < 0)
	{
		fprintf(stderr, "open dcp error:%d\n", ret);
		return -2;
	}

	ret = decm_get_resource_path(pdecm, &res_path);
	if (ret < 0)
	{
		fprintf(stderr, "get resource path error: %d!\n", ret);
		return -3;
	}
	fprintf(stderr, "DCP Resource Path:\n");
	fprintf(stderr, "\t\tAssetmap:%s\n", res_path.Assetmap);
	fprintf(stderr, "\t\tCPL:%s\n", res_path.CPL);
	fprintf(stderr, "\t\tKDM:%s\n", res_path.KDM);
	fprintf(stderr, "\t\tPKL:%s\n", res_path.PKL);
	fprintf(stderr, "\t\tVolumeIndex:%s\n", res_path.VolumeIndex);
	fprintf(stderr, "\n");

	ret = decm_get_name(pdecm, dcpname, sizeof(dcpname));
	if (ret < 0)
	{
		fprintf(stderr, "get dcp name error: %d\n", ret);
		return -4;
	}
	fprintf(stderr, "DCP Name:\n\t\t%s\n\n", dcpname);

	ret = decm_get_paras(pdecm, &dcp_para);
	if (ret < 0)
	{
		fprintf(stderr, "get dc paras error: %d\n", ret);
		return -5;
	}
	fprintf(stderr, "DCP Total Reels: %d\n", dcp_para.num_reels);

	for (j = 0; j < dcp_para.num_reels; j++)
	{
		int k;
		fprintf(stderr, "\tReel%d:\n", j);

		fprintf(stderr, "\t\t\tvideo:\n");
		fprintf(stderr, "\t\t\tEntryPoint:%u\n", dcp_para.reels[j].video.EntryPoint);
		fprintf(stderr, "\t\t\tduration:%u\n", dcp_para.reels[j].video.duration);
		fprintf(stderr, "\t\t\tIntrinsicDuration:%u\n", dcp_para.reels[j].video.IntrinsicDuration);
		fprintf(stderr, "\t\t\tEditRate %d/%d\n", dcp_para.reels[j].video.EditRateNum, dcp_para.reels[j].video.EditRateDen);
		fprintf(stderr, "\t\t\tmxf_name:%s\n", dcp_para.reels[j].video.mxf_name);
		fprintf(stderr, "\t\t\tofs_start:%llu\n", (unsigned long long)dcp_para.reels[j].video.ofs_start);
		fprintf(stderr, "\t\t\tofs_end:%llu\n", (unsigned long long)dcp_para.reels[j].video.ofs_end);
		for (k = 0; k<dcp_para.reels[j].video.num_uuid; k++)
		{
			char buf[256];
			fprintf(stderr, "\t\t\tuuid[%d]:%s\n", k, getuuidbuf(dcp_para.reels[j].video.uuid[k], buf));
			fprintf(stderr, "\t\t\tencryption[%d]:%d\n", k, dcp_para.reels[j].video.encryption[k]);
		}
		fprintf(stderr, "\t\t\tnum_uuid:%d\n", dcp_para.reels[j].video.num_uuid);
		fprintf(stderr, "\t\t\tformat:%s\n", getformat(dcp_para.reels[j].video.format));
		fprintf(stderr, "\t\t\tpcm_rate:%d\n", dcp_para.reels[j].video.pcm_rate);
		fprintf(stderr, "\t\t\tpcm_bits:%d\n", dcp_para.reels[j].video.pcm_bits);
		fprintf(stderr, "\t\t\tpcm_nchn:%d\n", dcp_para.reels[j].video.pcm_nchn);

		fprintf(stderr, "\t\t\taudio:\n");
		fprintf(stderr, "\t\t\tEntryPoint:%u\n", dcp_para.reels[j].audio.EntryPoint);
		fprintf(stderr, "\t\t\tduration:%u\n", dcp_para.reels[j].audio.duration);
		fprintf(stderr, "\t\t\tIntrinsicDuration:%u\n", dcp_para.reels[j].audio.IntrinsicDuration);
		fprintf(stderr, "\t\t\tEditRate %d/%d\n", dcp_para.reels[j].audio.EditRateNum, dcp_para.reels[j].audio.EditRateDen);
		fprintf(stderr, "\t\t\tmxf_name:%s\n", dcp_para.reels[j].audio.mxf_name);
		fprintf(stderr, "\t\t\tofs_start:%llu\n", (unsigned long long)dcp_para.reels[j].audio.ofs_start);
		fprintf(stderr, "\t\t\tofs_end:%llu\n", (unsigned long long)dcp_para.reels[j].audio.ofs_end);
		for (k = 0; k<dcp_para.reels[j].audio.num_uuid; k++)
		{
			char buf[256];
			fprintf(stderr, "\t\t\tuuid[%d]:%s\n", k, getuuidbuf(dcp_para.reels[j].audio.uuid[k], buf));
			fprintf(stderr, "\t\t\tencryption[%d]:%d\n", k, dcp_para.reels[j].audio.encryption[k]);
		}
		fprintf(stderr, "\t\t\tnum_uuid:%d\n", dcp_para.reels[j].audio.num_uuid);
		fprintf(stderr, "\t\t\tformat:%s\n", getformat(dcp_para.reels[j].audio.format));
		fprintf(stderr, "\t\t\tpcm_rate:%d\n", dcp_para.reels[j].audio.pcm_rate);
		fprintf(stderr, "\t\t\tpcm_bits:%d\n", dcp_para.reels[j].audio.pcm_bits);
		fprintf(stderr, "\t\t\tpcm_nchn:%d\n", dcp_para.reels[j].audio.pcm_nchn);
	}


	//decode and save
	for (j = 0; j < dcp_para.num_reels; j++)
	{
		int idx = 0;
		FILE * fp_v;
		FILE * fp_a;
		FILE * fp_v_rec;
		FILE * fp_a_rec;
		char *paudio = NULL;
		char *pvideo = NULL;
		int audiosize = 0;
		int videosize = 0;

		if (rec_on)
		{
			char filename[256];
			
			if (dcp_para.num_reels > 1)
			{
				snprintf(filename, sizeof(filename), "%s/%s%d", REC_DIR, REC_VIDEO_NAME, j);
				fp_v_rec = fopen(filename, "w+");
				snprintf(filename, sizeof(filename), "%s/%s%d", REC_DIR, REC_AUDIO_NAME, j);
				fp_a_rec = fopen(filename, "w+");
			}
			else
			{
				snprintf(filename, sizeof(filename), "%s/%s", REC_DIR, REC_VIDEO_NAME);
				fp_v_rec = fopen(filename, "w+");
				snprintf(filename, sizeof(filename), "%s/%s", REC_DIR, REC_AUDIO_NAME);
				fp_a_rec = fopen(filename, "w+");

			}
			if (dcp_para.reels[j].audio.format == CODEC_PCM)
			{
				if (dcp_para.reels[j].audio.pcm_nchn == 6)
				{
					_fseeki64(fp_a_rec, sizeof(struct WAVE_HEADER_EXTENSIBLE), SEEK_SET);
				}
				else
				{
					_fseeki64(fp_a_rec, sizeof(struct WAVE_HEADER), SEEK_SET);
				}
			}
		}

		//1. set current reel
		decm_set_current_reel(pdecm, j);

		//2. open video and audio
		fp_v = fopen(dcp_para.reels[j].video.mxf_name, "rb");
		if (fp_v == NULL)
		{
			perror("fopen");
			//打开失败处理
		}
		fp_a = fopen(dcp_para.reels[j].audio.mxf_name, "rb");
		if (fp_a == NULL)
		{
			perror("fopen");
			//打开失败处理            
		}

		//3. skip video header and audio header
		if (_fseeki64(fp_v, dcp_para.reels[j].video.ofs_start, SEEK_SET) < 0)
		{
			perror("fseek_64");
			//错误处理
		}
		if (_fseeki64(fp_a, dcp_para.reels[j].audio.ofs_start, SEEK_SET) < 0)
		{
			perror("fseek_64");
			//错误处理
		}

		idx = 0;
		while (1)
		{
			int ret;
			KLVPkt klv;
			//read video mxf packet
			ret = mxf_read_es_packet(fp_v, &klv);
			if (ret < 0)
			{
				if (ret == -1)
				{
					fprintf(stderr, "cannot read video packet.\n");
				}
				break;
			}
			//fprintf(stderr, "video%d[O:%lld L:%llu E:%lld]\n", idx, (long long int)klv.offset, (long long unsigned int)klv.length, (long long int)klv.end);

			pvideo = (char *)malloc(klv.length);
			videosize = fread(pvideo, 1, klv.length, fp_v);
			if (dcp_para.reels[j].video.encryption[0])
			{
				//视频加密
				decm_dcry(pdecm, (uchar *)pvideo, klv.length, (uchar *)pvideo, &videosize, klv.length, VIDEO);
			}

			//read audio mxf packet
			ret = mxf_read_es_packet(fp_a, &klv);
			if (ret < 0)
			{
				if (ret == -1)
				{
					fprintf(stderr, "cannot read audio packet.\n");
				}
				break;
			}
			//fprintf(stderr, "audio%d[O:%lld L:%lld E:%lld]\n", idx, (long long int)klv.offset, (long long unsigned int)klv.length, (long long int)klv.end);

			paudio = (char *)malloc(klv.length);
			audiosize = fread(paudio, 1, klv.length, fp_a);
			if (dcp_para.reels[j].audio.encryption[0])
			{
				//音频加密
				decm_dcry(pdecm, (uchar *)paudio, klv.length, (uchar *)paudio, &audiosize, klv.length, AUDIO);
			}

			if (rec_on)
			{
				fwrite(paudio, 1, audiosize, fp_a_rec);
				fwrite(pvideo, 1, videosize, fp_v_rec);
			}

			if (paudio) free(paudio);
			if (pvideo) free(pvideo);

			idx++;

			if ((unsigned long long)_ftelli64(fp_v) >= (unsigned long long)(dcp_para.reels[j].video.ofs_end) ||
				(unsigned long long)_ftelli64(fp_a) >= (unsigned long long)(dcp_para.reels[j].audio.ofs_end))
			{
				fprintf(stderr, "end of file.\n");
				break;
			}
		}

		if (rec_on)
		{
			if (dcp_para.reels[j].audio.format == CODEC_PCM)
			{
				if (dcp_para.reels[j].audio.pcm_nchn == 6)
				{
					struct WAVE_HEADER_EXTENSIBLE wave_header;
					memset(&wave_header, 0, sizeof(wave_header));
					fill_wav_header_extensible(&wave_header, (unsigned long long)_ftelli64(fp_a_rec) - 68,
						dcp_para.reels[j].audio.pcm_nchn,
						dcp_para.reels[j].audio.pcm_bits,
						dcp_para.reels[j].audio.pcm_rate);
					_fseeki64(fp_a_rec, 0, SEEK_SET);
					fwrite(&wave_header, 1, sizeof(wave_header), fp_a_rec);
				}
				else
				{
					struct WAVE_HEADER wave_header;
					memset(&wave_header, 0, sizeof(wave_header));
					fill_wav_header(&wave_header, (unsigned long long)_ftelli64(fp_a_rec) - 44,
						dcp_para.reels[j].audio.pcm_nchn,
						dcp_para.reels[j].audio.pcm_bits,
						dcp_para.reels[j].audio.pcm_rate);
					_fseeki64(fp_a_rec, 0, SEEK_SET);
					fwrite(&wave_header, 1, sizeof(wave_header), fp_a_rec);
				}
			}

			if (fp_v_rec) fclose(fp_v_rec);
			if (fp_a_rec) fclose(fp_a_rec);
		}

		fclose(fp_v);
		fclose(fp_a);
	}
	decm_close(pdecm);

	return 0;
}
