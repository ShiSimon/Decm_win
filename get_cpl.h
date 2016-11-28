/*
** get_cpl.h
**
** Made by pfgu
** Login   <pfgu@pfgu.r52>
**
** Started on  Thu Aug 21 14:28:21 2008 pfgu
** Last update Thu Aug 21 14:28:21 2008 pfgu
*/

#ifndef   	GET_CPL_H_
#define   	GET_CPL_H_

#define ERR_CPL_NO_CPL_XML    1
#define ERR_CPL_NO_XML_FOUND  2
#define ERR_CPL_GLOB_ERR      3
#define ERR_CPL_BAD_CPL_ID    4
#define ERR_CPL_NILL_CPL_ID   5
#define ERR_CPL_NO_CPL_ID     6
#define ERR_CPL_LOAD_XML      7

#define ERR_ASSET_LOAD_XML    8
#define ERR_ASSET_NO_ASSET    9

#define ERR_ASSET_NO_PKL_FN      10
#define ERR_ASSET_NO_PKL_FS      11
#define ERR_ASSET_NO_PKL_UUID    12
#define ERR_ASSET_NO_CPL_FN      13
#define ERR_ASSET_NO_CPL_FS      14
#define ERR_ASSET_NO_CPL_UUID    15
#define ERR_ASSET_NO_AUDIO_FN    16
#define ERR_ASSET_NO_AUDIO_FS    17
#define ERR_ASSET_NO_AUDIO_UUID  18
#define ERR_ASSET_NO_VIDEO_FN    19
#define ERR_ASSET_NO_VIDEO_FS    20
#define ERR_ASSET_NO_VIDEO_UUID  21

#define ERR_PKL_NO_ASSET  22
#define ERR_PKL_LOAD_XML  23 

#define ERR_ASSET_NOT_PKL    24

#define MAX_ASSETMAP_NUM    512       /* ��������ASSETMAP���� */
#define MAX_XML_SIZE       5*1024*1024   /* 5M ���ϵ�XML�ļ���ȥ�� */


/* uuid:����ǰ׺36�ֽڣ�ǰ׺9�ֽ� */

#ifdef __cplusplus
extern "C"
{
#endif

    struct Rational
    {
        int Numerator;
        int Denominator;
    };
    
    struct Reel
    {
        char *v_path_;          /* ��̬���� */
        char v_uuid_[64];
        struct Rational v_editrate_;
        unsigned long v_intrinsic_duration_;
        unsigned long v_entrypoint_;
        unsigned long v_duration_;
        struct Rational v_framerate_;
        int is_stereoscopic_;   /* �Ƿ���3D */
        char v_keyid_[64];
        
        char *a_path_;          /* ��̬���� */
        char a_uuid_[64];
        struct Rational a_editrate_;
        unsigned long a_intrinsic_duration_;
        unsigned long a_entrypoint_;
        unsigned long a_duration_;
        char a_keyid_[64];
    };
        
    typedef struct cpl_info
    {
        char uuid_[64];         /* cpl uuid */
        char content_title_[64]; /* ��Ӱ�� */
        char content_kind_[32];
    
        struct Reel *p_reel_;   /* reel�ṹ������ ��Ҫ��̬����*/
        unsigned int reel_num_; /* �м���reel һ���Է���*/
        
        char *assetmap_path_;   /* ��Ӧ��ASSETMAP����·�� ��̬���� */
        char *pkl_path_;        /* ��Ӧ��PKL����·�� ��̬���� */
        char *path_;            /* ��CPL�ľ���·�� ��̬���� */
        int is_complete_;       /*  �����Ӱ�Ƿ����� */
    }cpl_info_t;
    
    enum
    {
        ASSET_UNKNOW=0,
        ASSET_PKL,
        ASSET_CPL,
        ASSET_AUDIO,
        ASSET_VIDEO,
        ASSET_SUB               /* ��Ļ */
    };

    struct asset
    {
        char *path_;
        char uuid_[64];
        int type_;               /* ASSET TYPE */
        unsigned long long file_size_;
        int is_complete_;       /*  �Ƿ����� */
        char pkl_uuid_[64];        /* �����ĸ�PKL */
    };
    
    typedef struct assetmap_info
    {
        char *path_;            /* assetmap����·�� */
        struct asset *p_asset_; /* asset�ṹ���� һ���Զ�̬����*/
        unsigned int asset_num_;       /* �м���asset */
        unsigned long long file_size_; /* assetmap���ļ���С */
    }assetmap_info_t;

    unsigned long long getFileSize(const char * fn);

    /* 
     * ���ݵ�ǰ��ASSETMAP ����CPL�ļ������ã����cpl_info_t�ṹ��
     * p_infoָ����ڴ�����������
     * cpl_info_t �е�һЩ�ṹʹ�� malloc �����ڴ棬ʹ�����Ҫ����
     * ClearCplInfo ������ cpl_info_t �ṹ�з�����ڴ档
     * @param[in]  index ��Ҫ�����CPL�ļ���p_assetmap�е�����
     * @param[in]  p_assetmap �ʲ��ļ��б�
     * @param[out] p_info �õ���CPL ��Ϣ
     * @return 0:�ɹ� <0ʧ��
     *
     */
    int GetCplInfo( int index, assetmap_info_t *p_assetmap ,cpl_info_t *p_info);

    /* ����ʹ�ù��� cpl_info_t
     *
     * ���� GetCplInfo ��� cpl_info_t �ṹ����ʹ�ñ�������������
     * �ͷ�һЩ��̬������ڴ�
     */
    int ClearCplInfo( cpl_info_t *p_info);

    
    /* 
       �ݹ����dir�����е�ASSETMAP�����ҵ��ĸ�����û���򷵻�0
    */
    unsigned int FindAssetMapPath(const char *dir, char *path[MAX_ASSETMAP_NUM]);
    
    /*  
        �ͷŷ����ASSETMAP·����Ϣ
    */
    int ClearAssetMapPath(char *path[MAX_ASSETMAP_NUM]);
    
    
    /* 
     * ���������ASSETMAP·������ȡ���е�ASSET��Ϣ
     * p_assetmapָ����ڴ��Ѿ������
     * assetmap_info_t �е�һЩ�ṹʹ�� malloc �����ڴ棬ʹ�����Ҫ����
     * ClearAssetMap ������ṹ�з�����ڴ档
     * ����:0�ɹ��� <0ʧ��
     */
    int GetAssetMap( const char *assetmap_path, assetmap_info_t *p_assetmap);
    int ClearAssetMap( assetmap_info_t *p_assetmap);
    
    /* s:�������ڵĺ���,err_ret:����ķ���ֵ */
    void PrintErrInfo(const char *s, int err_ret);
    
#ifdef __cplusplus
}
#endif

#endif 	    /* !GET_CPL_H_ */
