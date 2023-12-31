/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing:
*   The licensor of this EXAMPLE CODE is Silicon Laboratories Inc.
*
*   Silicon Laboratories Inc. grants you a personal, worldwide, royalty-free, fully paid-up license to
*   use, copy, modify and distribute the EXAMPLE CODE software, or portions thereof, in any of your
*   products.
*
*   Your use of this EXAMPLE CODE is at your own risk. This EXAMPLE CODE does not come with any
*   warranties, and the licensor disclaims all implied warranties concerning performance, accuracy,
*   non-infringement, merchantability and fitness for your application.
*
*   The EXAMPLE CODE is provided "AS IS" and does not come with any support.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: https://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                          TFTP EXAMPLE CODE
*
* File : ex_tftp_client.c
*********************************************************************************************************
* Note(s) : (1) For additional details on the features available with Micrium OS TFTP Client, the
*               API, the installation, etc. Refer to the Micrium OS TFTP Client documentation
*               available at https://doc.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos_description.h>

#if (defined(RTOS_MODULE_NET_TFTP_CLIENT_AVAIL) && \
     defined(RTOS_MODULE_FS_STORAGE_RAM_DISK_AVAIL))


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <ex_description.h>

#include  <rtos/common/include/rtos_path.h>
#include  <rtos/common/include/rtos_utils.h>

#include  <rtos/net/include/net_app.h>
#include  <rtos/net/include/tftp_client.h>

#include  <rtos/fs/include/fs_ramdisk.h>
#include  <rtos/fs/include/fs_core_cache.h>
#include  <rtos/fs/include/fs_core_file.h>
#include  <rtos/fs/include/fs_core_vol.h>
#include  <rtos/fs/include/fs_core_partition.h>
#include  <rtos/fs/include/fs_media.h>
#include  <rtos/fs/include/fs_fat.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  EX_TFTP_CLIENT_RAMDISK_SEC_SIZE
#define  EX_TFTP_CLIENT_RAMDISK_SEC_SIZE                    512u
#endif

#ifndef  EX_TFTP_CLIENT_RAMDISK_SEC_NBR
#define  EX_TFTP_CLIENT_RAMDISK_SEC_NBR                     104u
#endif


#ifndef  EX_TFTP_CLIENT_SERVER_HOSTNAME
#define  EX_TFTP_CLIENT_SERVER_HOSTNAME            "192.168.1.10"   /* TODO Modify to specify the server address.       */
#endif

#ifndef  EX_TFTP_CLIENT_FILE_VOL
#define  EX_TFTP_CLIENT_FILE_VOL                   "ram_tftpc"
#endif

#ifndef  EX_TFTP_CLIENT_FILE_LOCAL_PATH                             /* TODO Modify to specify the local file path       */
                                                                    /* (volume name + path + file name) to 'download to'*/ 
                                                                    /* or to 'upload from'.                             */
#define  EX_TFTP_CLIENT_FILE_LOCAL_PATH            EX_TFTP_CLIENT_FILE_VOL "/1KB.zip"
#endif

#ifndef  EX_TFTP_CLIENT_FILE_REMOTE_PATH                            /* TODO Modify to specify the remote path(+filename)*/
                                                                    /* of the file when downloading it with GET.        */
#define  EX_TFTP_CLIENT_FILE_REMOTE_PATH           "1KB.zip"                                                                          
#endif

#ifndef  EX_TFTP_CLIENT_FILE_REMOTE_UPLOAD_PATH                     /* TODO Modify to specify the full path and name    */
                                                                    /* the file will have on the remote when uploading  */
                                                                    /* with PUT.                                        */
#define  EX_TFTP_CLIENT_FILE_REMOTE_UPLOAD_PATH    "1KB_from_target.zip"
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_INT08U   Ex_TFTP_Client_RAMDisk[EX_TFTP_CLIENT_RAMDISK_SEC_SIZE * EX_TFTP_CLIENT_RAMDISK_SEC_NBR];
static  FS_CACHE    *Ex_TFTP_Client_CachePtr;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  Ex_TFTP_Client_FS_MediaPrepare (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         Ex_TFTP_ClientGetAndPut()
*
* Description : Get a file from a TFTP server and send it back afterwards.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  Ex_TFTP_ClientGetAndPut (void)
{
    RTOS_ERR    err;


    Ex_TFTP_Client_FS_MediaPrepare();


    TFTPc_Get(EX_TFTP_CLIENT_SERVER_HOSTNAME,
              DEF_NULL,
              EX_TFTP_CLIENT_FILE_LOCAL_PATH,
              EX_TFTP_CLIENT_FILE_REMOTE_PATH,
              TFTPc_MODE_OCTET,
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);


    TFTPc_Put(EX_TFTP_CLIENT_SERVER_HOSTNAME,
              DEF_NULL,
              EX_TFTP_CLIENT_FILE_LOCAL_PATH,
              EX_TFTP_CLIENT_FILE_REMOTE_UPLOAD_PATH,
              TFTPc_MODE_OCTET,
             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                  Ex_TFTP_Client_FS_MediaPrepare()
*
* Description : Example of File System media preparation to contain web files.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  Ex_TFTP_Client_FS_MediaPrepare (void)
{
    FS_RAM_DISK_CFG    ramdisk_cfg;
    FS_BLK_DEV_HANDLE  dev_handle;
    FS_MEDIA_HANDLE    media_handle;
    FS_CACHE_CFG       cache_cfg;
    RTOS_ERR           err;


    ramdisk_cfg.DiskPtr = Ex_TFTP_Client_RAMDisk;               /* Create a RAMDisk that will contain the files.        */
    ramdisk_cfg.LbCnt   = EX_TFTP_CLIENT_RAMDISK_SEC_NBR;
    ramdisk_cfg.LbSize  = EX_TFTP_CLIENT_RAMDISK_SEC_SIZE;

    FS_RAM_Disk_Add(EX_TFTP_CLIENT_FILE_VOL,
                    &ramdisk_cfg,
                    &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    cache_cfg.Align        = sizeof(CPU_ALIGN);                 /* Create cache for RAMDisk block device.               */
    cache_cfg.BlkMemSegPtr = DEF_NULL;
    cache_cfg.MaxLbSize    = 512u;
    cache_cfg.MinLbSize    = 512u;
    cache_cfg.MinBlkCnt    = 1u;

    Ex_TFTP_Client_CachePtr = FSCache_Create(&cache_cfg,
                                             &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    media_handle = FSMedia_Get(EX_TFTP_CLIENT_FILE_VOL);        /* Open block device.                                   */
    APP_RTOS_ASSERT_CRITICAL(!FS_MEDIA_HANDLE_IS_NULL(media_handle), ;);

    dev_handle = FSBlkDev_Open(media_handle, &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    FSCache_Assign(dev_handle,                                  /* Assign cache to block device.                        */
                   Ex_TFTP_Client_CachePtr,
                  &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    FS_FAT_Fmt(dev_handle,                                      /* Format RAMDisk in FAT format.                        */
               FS_PARTITION_NBR_VOID,
               DEF_NULL,
              &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);

    (void)FSVol_Open(dev_handle,                                /* Open volume and call it "ram_https".                 */
                     1u,
                     EX_TFTP_CLIENT_FILE_VOL,
                     FS_VOL_OPT_DFLT,
                    &err);
    APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif /* RTOS_MODULE_NET_TFTP_CLIENT_AVAIL */
