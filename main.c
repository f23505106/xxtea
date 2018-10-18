

#include "md5.h"
#include "base64.h"
#include "xxtea.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <zlib.h>

extern int errno ;

static const char* const KWupPassword = "DFG#$%^#%$RGHR(&*M<><";

static const char* const TMP_NAME = "tmp.gz";
static const char* const OUT_FILE_NAME = "yellow.sdb";


int main(int argc, char **argv) {
    char *fileName;
    size_t fileSize;

    const char * outName = OUT_FILE_NAME;
    if (argc < 2) {
        printf("usage: %s 'input_file_name[ out_put_file_name]'\n", argv[0]);
        return 1;
    }

    if(argc > 2){
        outName = argv[2];
    }

    fileName = argv[1];
    if( access(fileName, R_OK ) == -1 ) {
        printf("file : %s 'can not read'\n", fileName);
        return 2;
    }

    struct stat st;
    stat(fileName, &st);
    fileSize = st.st_size;

    size_t buffSize = fileSize+20;
    char *buff = malloc(buffSize);

    if(NULL == buff){
        printf("can not allocate memory size:%zu'\n", buffSize);
        return 3;
    }

    int fd = open(fileName,O_RDONLY);

    if(-1 == fd){
        perror("open file failed");
        goto buff;
    }

    size_t size = read(fd, buff,fileSize);
    if(size != fileSize){
        perror("read file failed");
        close(fd);
        goto buff;
    }
    close(fd);


    gzFile file0 = gzopen(TMP_NAME, "wb9");
    int ret = gzwrite(file0, buff, fileSize);
    if(ret != fileSize){
        printf("gzwrite error:%d\n",ret);
    }
    gzclose(file0);

    stat(TMP_NAME, &st);
    fileSize = st.st_size;

    fd = open(TMP_NAME,O_RDONLY);

    if(-1 == fd){
        perror("open file failed");
        goto buff;
    }

    size = read(fd, buff,fileSize);
    if(size != fileSize){
        perror("read file failed");
        close(fd);
        goto buff;
    }
    close(fd);


    size_t passLen;
    uint8_t passwordMD5 [MD5_LEN];
    passLen = strlen(KWupPassword);
    md5((uint8_t*)KWupPassword, passLen, passwordMD5);

    size_t encryptLen;
    unsigned char *encryptData = xxtea_encrypt(buff,fileSize, passwordMD5, &encryptLen);
    
    printf("file %s len %zu encrypt len %zu\n",fileName,fileSize,encryptLen);

/*
    unsigned long compressLen = buffSize;
    int compressRet =  compress((unsigned char*)buff, &compressLen,encryptData,encryptLen);

    if(Z_OK != compressRet){
        printf("zlib compress error %d\n",compressRet);
        goto encrypt;
    }

*/

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fd = open(outName,O_WRONLY|O_CREAT|O_TRUNC,mode);

    if(-1 == fd){
        perror("open file failed");
        goto encrypt;
    }

    size = write(fd, encryptData,encryptLen);
    if(size != encryptLen){
        perror("write file failed");
    }
    close(fd);

encrypt:
    free(encryptData);
    
buff:
    free(buff);

    return 0;
}