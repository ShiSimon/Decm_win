#include <stdio.h>
#include "decm.h"

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

int main(int argc, char * argv[])
{
	int j;
	int ret = 0;
	void *pdecm;
	const char * dcpdir = "E:/hdmovlib/BHSJ";
	const char * kdmdir = NULL;
	res_path_t res_path;
	char dcpname[256];
	dcp_para_t dcp_para;
	printf("%s\n", dcpdir);
	ret = decm_open(&pdecm, dcpdir, kdmdir);
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
	for (j = 0; j<dcp_para.num_reels; j++)
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
	decm_close(pdecm);
	return 0;
}