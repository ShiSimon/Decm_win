#ifndef KDM_H
#define KDM_H

#include <list>
#include <string>
#include <stdio.h>

enum
{
    ERR_KDM_LOAD_XML = 1,
    ERR_KDM_NO_MSG_UUID,
    ERR_KDM_BAD_MSG_UUID,
    ERR_KDM_NO_CPL_UUID,
    ERR_KDM_BAD_CPL_UUID,
    ERR_KDM_NO_CONTENT_TITLE,
    ERR_KDM_ASSIGN_CONTENT_TITLE,
    ERR_KDM_NO_NV_BEFORE,
    ERR_KDM_BAD_NV_BEFORE,
    ERR_KDM_NO_NV_AFTER,
    ERR_KDM_BAD_NV_AFTER,
    ERR_KDM_NO_KEY_ID_LIST,
    ERR_KDM_NO_KEY_UUID,
    ERR_KDM_BAD_KEY_UUID,
    ERR_KDM_NO_ENCRYPTED_KEY,
    ERR_KDM_NO_CIPHER_DATA,
    ERR_KDM_NO_CIPHER_VALUE,
    ERR_KDM_BAD_CIPHER_VALUE,
    ERR_KDM_DECRYPT_KEY,
    ERR_KDM_BAD_UTC_NV_BEFORE,
    ERR_KDM_BAD_UTC_NV_AFTER,
    ERR_KDM_BAD_DECRYPT_KEY,
    ERR_KDM_NOT_OPEN,
    ERR_KDM_OPENED,
    ERR_KDM_NOT_DEC,
    ERR_KDM_NOT_BALANCE,
    ERR_KDM_KEY_UUID_NOT_EQUAL,
    ERR_KDM_CPL_UUID_NOT_EQUAL,
    ERR_KDM_NV_BEFORE_NOT_EQUAL,
    ERR_KDM_NV_AFTER_NOT_EQUAL,
    ERR_KDM_NO_KDM_ELEMENT,

    ERR_KDMUTIL_GLOB,
    ERR_KDMUTIL_OPENDIR,
    ERR_KDMUTIL_STRDUP
};

int ParseUuid(unsigned char uuid[16], const char *uuid_str);

enum
{
    UUID_NO_PREFIX = 1,
    UUID_NO_HYPHEN = 2,
};
int PrintUuid(char uuid_buf[45], const unsigned char msg_uuid[16], int flag = 0);
int ParseUtc( unsigned long long *utc, const char *utc_str);

struct KdmEncKey
{
    unsigned char key_uuid_[16];
    unsigned char enc_key_[256];
};

struct KdmDecKey
{
    unsigned char key_uuid_[16];
    unsigned char cpl_uuid_[16];
    unsigned char aes_key_[16];
    unsigned long long nv_before;
    unsigned long long nv_after;
};

class KDM
{
public:
    KDM();
    unsigned char msg_uuid[16];
    unsigned char cpl_uuid[16];
    std::string content_title;
    unsigned long long nv_before;
    unsigned long long nv_after;
    std::list<KdmEncKey> list_enc_key_;
    std::list<KdmDecKey> list_dec_key_;
public:
    ~KDM(void);
    int Open(const char *kdm_fn,int thestorage);
    int Dec(const char *pri_fn);
    void Close(void);
    void GetCplUuid( char uuid_buf[45] ,int flag = 0)
    {
        PrintUuid( uuid_buf, cpl_uuid, flag);
    }
    void GetMsgUuid( char uuid_buf[45], int flag = 0)
    {
        PrintUuid( uuid_buf, msg_uuid, flag);
    }

    const unsigned char *GetCplUuid(void) const
    {
        return cpl_uuid;
    }
    const unsigned char *GetMsgUuid(void) const
    {
        return msg_uuid;
    }
    const char *GetContentTitle(void) const
    {
        return content_title.c_str();
    }
    const std::list<KdmEncKey> &GetEncKeyList(void) const
    {
        return list_enc_key_;
    }
    const std::list<KdmDecKey> &GetDecKeyList(void) const
    {
        return list_dec_key_;
    }
    unsigned int GetEncKeyNum(void) const
    {
        return (unsigned int)(list_enc_key_.size());
    }
    unsigned int GetDecKeyNum(void) const
    {
        return (unsigned int)(list_dec_key_.size());
    }
    bool IsOpen(void) const
    {
        for(int i=0;i<16;i++) if( msg_uuid[i] != 0) return true;
        return false;
    }
    bool IsDec(void) const
    {
        return IsOpen() && GetEncKeyNum() == GetDecKeyNum();
    }
    unsigned long long GetNotValidBefore(void) const
    {
        return nv_before;
    }
    unsigned long long GetNotValidAfter(void) const
    {
        return nv_after;
    }
};

#endif // KDM_H
