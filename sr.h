/* SR.H         (c)Copyright Greg Smith, 2004-2009                   */
/*              Suspend/Resume a Hercules session                    */

/*
 * The suspend/resume functions allow a hercules instance to be
 * captured to a file and later resumed.  Note that the suspend
 * function also terminates the hercules instance.
 *
 * In order for an instance to be resumed, hercules must be started
 * with a config file describing the configuration at suspend time.
 * For example, mainsize and xpndsize must match.  Also, all devices
 * present at suspend time must be present at resume time.
 *
 * Disk devices must be at the same state as they were at suspend
 * time.  They can, however, be a different file type.  That is,
 * a disk could be a cckd disk at suspend time.  Then a ckd disk
 * could be created using dasdcopy and hercules resumed using the
 * ckd disk instead.
 *
 * Also, hercules must be configured similarly as at suspend time.
 * For example, if 4 emulated CPUs were active at suspend time
 * then the session can not be resumed on a hercules with a
 * maximum of two CPUs.  Another example, you will not be able
 * to resume a session in z900 architecture mode for a hercules
 * that was built without z900 architecture.
 *
 * Device state
 *
 * Currently, device state is only fully saved for CKD disks.
 * Each device class (eg TAPE, RDR, PUN, CTC) will need code
 * to save and restore their state.  Some states may not be
 * possible to restore (eg active tcp/ip connections at the
 * time of suspend).
 *
 * Further limitations
 *
 * Currently the vector facility state is not saved.
 * Also, the ecpsvm state is not currently saved.
 *
 * File Structure
 *
 * The suspend/resume file (.srf) contains some number of `text
 * units'.  A text unit has an 8 byte header followed by zero or
 * more bytes of data.
 *
 * The file is designed to be hercules release independent and
 * to be host architecture independent.  For example, I should be
 * able to take an srf file created on hercules 3.02 on an intel
 * machine and resume on a Sun machine running hercules 3.04.
 *
 * The header contains a 4 byte key and a 4 byte length.  Both
 * the key and the length are stored in big-endian byte order.
 *
 * There are 3 types of data: STRING, BUF and VALUE.
 *
 * A string is a null terminated sequence of bytes.  The string
 * includes the null terminator byte.  The total length of a
 * string cannot exceed SR_MAX_STRING_LENGTH (4096).  The
 * length is checked by SR_READ_STRING.
 *
 * A buf is a sequence of bytes whose length must be provided.
 * The length should be checked before issuing SR_READ_BUF.
 *
 * A value is an arithmetic number.  It's length can be
 * 0, 1, 2, 4 or 8 bytes.  Values are stored in big-endian
 * byte order.  A zero length indicates the value is 0.
 *
 * Text Units
 *
 * There are 4 categories (so far) of text units:
 *   HDR     ...    fields that describe the file
 *   SYS     ...    fields from SYSBLK
 *   CPU     ...    fields from REGS
 *   DEV     ...    fields from DEVBLK
 *
 * The format of a text unit key value is
 *   ace c t xxx
 *
 *   ace  -- all keys start with 0xace
 *   c    -- category (0 - HDR, 1 - SYS, 2 - CPU, 3 - DEV)
 *   t    -- for DEV keys, identifies the subtype for the
 *           device type (0 - general, 1 - CKD, ...)
 *   xxx  -- field identifier
 *
 * Note that there is no array data type.  When an array
 * needs to be used, the elements should have ascending
 * text unit key values.  For example:
 *
 *   #define SR_CPU_GR               0xace20020
 *   #define SR_CPU_GR_0             0xace20020
 *   #define SR_CPU_GR_1             0xace20021
 *   #define SR_CPU_GR_2             0xace20022
 *   #define SR_CPU_GR_3             0xace20023
 *   .  .  .  .  .  .
 *
 * The array can be written during suspend as follows:
 *
 *   for (i = 0; i < 16; i++)
 *     SR_WRITE_VALUE(fd, SR_CPU_GR+i, regs->gr[i], sizeof(regs->gr[0]));
 *
 * The array can be processed during resume as follows
 *
 *   case SR_CPU_GR_0:
 *   case SR_CPU_GR_1:
 *   case SR_CPU_GR_2:
 *   case SR_CPU_GR_3:
 *   .  .  .  .  .  .
 *     i = key - SR_CPU_GR;
 *     SR_READ_VALUE(fd, len, &regs->gr[i], sizeof(regs->gr[0]));
 *     break;
 *
 * The format of the .srf file is deliberately unstructured to
 * allow for flexibility in future enhancements.  However there
 * are a few restrictions.
 *
 * o Key SR_SYS_ARCHNAME shoud be specified before any
 *   SR_CPU keys.  The corresponding CPU is configured
 *   when the SR_CPU key is read, so sysblk.arch_mode must
 *   already be correctly set.
 * o SR_CPU_ keys must follow the corresponding SR_CPU key.
 * o Likewise SR_DEV_ keys must follow the corresponding SR_DEV key
 *
 * There may be other instances where the processing of one
 * key requires that another key has been previously processed.
 *
 */

// $Log$
// Revision 1.15  2007/06/23 00:04:16  ivan
// Update copyright notices to include current year (2007)
//
// Revision 1.14  2006/12/08 09:43:30  jj
// Add CVS message log
//

#ifndef _HERCULES_SR_H
#define _HERCULES_SR_H

#include "opcode.h"

#define SR_ID "Hercules suspend/resume file"
#define SR_MAX_STRING_LENGTH 4096

#define SR_KEY_ID_MASK          0xfff00000
#define SR_KEY_ID               0xace00000

#define SR_HDR_ID               0xace00000
#define SR_HDR_VERSION          0xace00001
#define SR_HDR_DATE             0xace00002

#define SR_SYS_MASK             0xfffff000
#define SR_SYS_STARTED_MASK     0xace10000
#define SR_SYS_INTS_STATE       0xace10001
#define SR_SYS_ARCH_NAME        0xace10002
#define SR_SYS_MAINSIZE         0xace10007
#define SR_SYS_MAINSTOR         0xace10008
#define SR_SYS_SKEYSIZE         0xace10009
#define SR_SYS_STORKEYS         0xace1000a
#define SR_SYS_XPNDSIZE         0xace1000b
#define SR_SYS_XPNDSTOR         0xace1000c
#define SR_SYS_CPUID            0xace1000d
#define SR_SYS_IPLDEV           0xace1000e
#define SR_SYS_IPLCPU           0xace1000f
#define SR_SYS_MBO              0xace10010
#define SR_SYS_MBK              0xace10011
#define SR_SYS_MBM              0xace10012
#define SR_SYS_MBD              0xace10013
#define SR_SYS_IOINTQ           0xace10020
#define SR_SYS_IOPENDING        0xace10021
#define SR_SYS_PCIPENDING       0xace10022
#define SR_SYS_ATTNPENDING      0xace10023
#define SR_SYS_CHP_RESET        0xace10030
#define SR_SYS_CHP_RESET_0      0xace10030
#define SR_SYS_CHP_RESET_1      0xace10031
#define SR_SYS_CHP_RESET_2      0xace10032
#define SR_SYS_CHP_RESET_3      0xace10033
#define SR_SYS_CHP_RESET_4      0xace10034
#define SR_SYS_CHP_RESET_5      0xace10035
#define SR_SYS_CHP_RESET_6      0xace10036
#define SR_SYS_CHP_RESET_7      0xace10037
#define SR_SYS_SERVPARM         0xace10040
#define SR_SYS_SIGINTREQ        0xace10041
#define SR_SYS_VMACTIVE         0xace10042
#define SR_SYS_MSCHDELAY        0xace10043
#define SR_SYS_LOADPARM         0xace10044
 /*
  * Following 3 tags added for Multiple
  * Logical Channel Subsystem support
  */
#define SR_SYS_IOPENDING_LCSS   0xace10045
#define SR_SYS_PCIPENDING_LCSS  0xace10046
#define SR_SYS_ATTNPENDING_LCSS 0xace10047

#define SR_SYS_SERVC            0xace11000

#define SR_SYS_CLOCK            0xace12000

#define SR_CPU                  0xace20000
#define SR_CPU_ARCHMODE         0xace20001
#define SR_CPU_PX               0xace20002
#define SR_CPU_PSW              0xace20003
#define SR_CPU_GR               0xace20020
#define SR_CPU_GR_0             0xace20020
#define SR_CPU_GR_1             0xace20021
#define SR_CPU_GR_2             0xace20022
#define SR_CPU_GR_3             0xace20023
#define SR_CPU_GR_4             0xace20024
#define SR_CPU_GR_5             0xace20025
#define SR_CPU_GR_6             0xace20026
#define SR_CPU_GR_7             0xace20027
#define SR_CPU_GR_8             0xace20028
#define SR_CPU_GR_9             0xace20029
#define SR_CPU_GR_10            0xace2002a
#define SR_CPU_GR_11            0xace2002b
#define SR_CPU_GR_12            0xace2002c
#define SR_CPU_GR_13            0xace2002d
#define SR_CPU_GR_14            0xace2002e
#define SR_CPU_GR_15            0xace2002f
#define SR_CPU_CR               0xace20040
#define SR_CPU_CR_0             0xace20040
#define SR_CPU_CR_1             0xace20041
#define SR_CPU_CR_2             0xace20042
#define SR_CPU_CR_3             0xace20043
#define SR_CPU_CR_4             0xace20044
#define SR_CPU_CR_5             0xace20045
#define SR_CPU_CR_6             0xace20046
#define SR_CPU_CR_7             0xace20047
#define SR_CPU_CR_8             0xace20048
#define SR_CPU_CR_9             0xace20049
#define SR_CPU_CR_10            0xace2004a
#define SR_CPU_CR_11            0xace2004b
#define SR_CPU_CR_12            0xace2004c
#define SR_CPU_CR_13            0xace2004d
#define SR_CPU_CR_14            0xace2004e
#define SR_CPU_CR_15            0xace2004f
#define SR_CPU_AR               0xace20060
#define SR_CPU_AR_0             0xace20060
#define SR_CPU_AR_1             0xace20061
#define SR_CPU_AR_2             0xace20062
#define SR_CPU_AR_3             0xace20063
#define SR_CPU_AR_4             0xace20064
#define SR_CPU_AR_5             0xace20065
#define SR_CPU_AR_6             0xace20066
#define SR_CPU_AR_7             0xace20067
#define SR_CPU_AR_8             0xace20068
#define SR_CPU_AR_9             0xace20069
#define SR_CPU_AR_10            0xace2006a
#define SR_CPU_AR_11            0xace2006b
#define SR_CPU_AR_12            0xace2006c
#define SR_CPU_AR_13            0xace2006d
#define SR_CPU_AR_14            0xace2006e
#define SR_CPU_AR_15            0xace2006f
#define SR_CPU_FPR              0xace20080
#define SR_CPU_FPR_0            0xace20080
#define SR_CPU_FPR_1            0xace20081
#define SR_CPU_FPR_2            0xace20082
#define SR_CPU_FPR_3            0xace20083
#define SR_CPU_FPR_4            0xace20084
#define SR_CPU_FPR_5            0xace20085
#define SR_CPU_FPR_6            0xace20086
#define SR_CPU_FPR_7            0xace20087
#define SR_CPU_FPR_8            0xace20088
#define SR_CPU_FPR_9            0xace20089
#define SR_CPU_FPR_10           0xace2008a
#define SR_CPU_FPR_11           0xace2008b
#define SR_CPU_FPR_12           0xace2008c
#define SR_CPU_FPR_13           0xace2008d
#define SR_CPU_FPR_14           0xace2008e
#define SR_CPU_FPR_15           0xace2008f
#define SR_CPU_FPR_16           0xace20090
#define SR_CPU_FPR_17           0xace20091
#define SR_CPU_FPR_18           0xace20092
#define SR_CPU_FPR_19           0xace20093
#define SR_CPU_FPR_20           0xace20094
#define SR_CPU_FPR_21           0xace20095
#define SR_CPU_FPR_22           0xace20096
#define SR_CPU_FPR_23           0xace20097
#define SR_CPU_FPR_24           0xace20098
#define SR_CPU_FPR_25           0xace20099
#define SR_CPU_FPR_26           0xace2009a
#define SR_CPU_FPR_27           0xace2009b
#define SR_CPU_FPR_28           0xace2009c
#define SR_CPU_FPR_29           0xace2009d
#define SR_CPU_FPR_30           0xace2009e
#define SR_CPU_FPR_31           0xace2009f
#define SR_CPU_FPC              0xace20100
#define SR_CPU_DXC              0xace20101
#define SR_CPU_MC               0xace20102
#define SR_CPU_EA               0xace20103
#define SR_CPU_PTIMER           0xace20104
#define SR_CPU_CLKC             0xace20105
#define SR_CPU_CHANSET          0xace20106
#define SR_CPU_TODPR            0xace20107
#define SR_CPU_MONCLASS         0xace20108
#define SR_CPU_EXCARID          0xace20109
#define SR_CPU_INTS_STATE       0xace2010a
#define SR_CPU_INTS_MASK        0xace2010b
#define SR_CPU_EXTCCPU          0xace2010c
#define SR_CPU_BEAR             0xace2010d
#define SR_CPU_OPNDRID          0xace20110
#define SR_CPU_CHECKSTOP        0xace20111
#define SR_CPU_HOSTINT          0xace20112
#define SR_CPU_EXECFLAG         0xace20113
#define SR_CPU_INSTVALID        0xace20114
#define SR_CPU_PERMODE          0xace20115
#define SR_CPU_LOADSTATE        0xace20116
#define SR_CPU_INVALIDATE       0xace20117
#define SR_CPU_RESET_OPCTAB     0xace20118
#define SR_CPU_SIGPRESET        0xace20119
#define SR_CPU_SIGPIRESET       0xace2011a
#define SR_CPU_VTIMERINT        0xace2011b
#define SR_CPU_RTIMERINT        0xace2011c
#define SR_CPU_MALFCPU          0xace20120
#define SR_CPU_MALFCPU_0        0xace20120
#define SR_CPU_MALFCPU_1        0xace20121
#define SR_CPU_MALFCPU_2        0xace20122
#define SR_CPU_MALFCPU_3        0xace20123
#define SR_CPU_MALFCPU_4        0xace20124
#define SR_CPU_MALFCPU_5        0xace20125
#define SR_CPU_MALFCPU_6        0xace20126
#define SR_CPU_MALFCPU_7        0xace20127
#define SR_CPU_MALFCPU_8        0xace20128
#define SR_CPU_MALFCPU_9        0xace20129
#define SR_CPU_MALFCPU_10       0xace2012a
#define SR_CPU_MALFCPU_11       0xace2012b
#define SR_CPU_MALFCPU_12       0xace2012c
#define SR_CPU_MALFCPU_13       0xace2012d
#define SR_CPU_MALFCPU_14       0xace2012e
#define SR_CPU_MALFCPU_15       0xace2012f
#define SR_CPU_MALFCPU_16       0xace20130
#define SR_CPU_MALFCPU_17       0xace20131
#define SR_CPU_MALFCPU_18       0xace20132
#define SR_CPU_MALFCPU_19       0xace20133
#define SR_CPU_MALFCPU_20       0xace20134
#define SR_CPU_MALFCPU_21       0xace20135
#define SR_CPU_MALFCPU_22       0xace20136
#define SR_CPU_MALFCPU_23       0xace20137
#define SR_CPU_MALFCPU_24       0xace20138
#define SR_CPU_MALFCPU_25       0xace20139
#define SR_CPU_MALFCPU_26       0xace2013a
#define SR_CPU_MALFCPU_27       0xace2013b
#define SR_CPU_MALFCPU_28       0xace2013c
#define SR_CPU_MALFCPU_29       0xace2013d
#define SR_CPU_MALFCPU_30       0xace2013e
#define SR_CPU_MALFCPU_31       0xace2013f
#define SR_CPU_EMERCPU          0xace20140
#define SR_CPU_EMERCPU_0        0xace20140
#define SR_CPU_EMERCPU_1        0xace20141
#define SR_CPU_EMERCPU_2        0xace20142
#define SR_CPU_EMERCPU_3        0xace20143
#define SR_CPU_EMERCPU_4        0xace20144
#define SR_CPU_EMERCPU_5        0xace20145
#define SR_CPU_EMERCPU_6        0xace20146
#define SR_CPU_EMERCPU_7        0xace20147
#define SR_CPU_EMERCPU_8        0xace20148
#define SR_CPU_EMERCPU_9        0xace20149
#define SR_CPU_EMERCPU_10       0xace2014a
#define SR_CPU_EMERCPU_11       0xace2014b
#define SR_CPU_EMERCPU_12       0xace2014c
#define SR_CPU_EMERCPU_13       0xace2014d
#define SR_CPU_EMERCPU_14       0xace2014e
#define SR_CPU_EMERCPU_15       0xace2014f
#define SR_CPU_EMERCPU_16       0xace20150
#define SR_CPU_EMERCPU_17       0xace20151
#define SR_CPU_EMERCPU_18       0xace20152
#define SR_CPU_EMERCPU_19       0xace20153
#define SR_CPU_EMERCPU_20       0xace20154
#define SR_CPU_EMERCPU_21       0xace20155
#define SR_CPU_EMERCPU_22       0xace20156
#define SR_CPU_EMERCPU_23       0xace20157
#define SR_CPU_EMERCPU_24       0xace20158
#define SR_CPU_EMERCPU_25       0xace20159
#define SR_CPU_EMERCPU_26       0xace2015a
#define SR_CPU_EMERCPU_27       0xace2015b
#define SR_CPU_EMERCPU_28       0xace2015c
#define SR_CPU_EMERCPU_29       0xace2015d
#define SR_CPU_EMERCPU_30       0xace2015e
#define SR_CPU_EMERCPU_31       0xace2015f

#define SR_DEV                  0xace30000
#define SR_DEV_DEVTYPE          0xace30001
#define SR_DEV_ARGC             0xace30002
#define SR_DEV_ARGV             0xace30003
#define SR_DEV_TYPNAME          0xace30004
 /*
  * Following tag added for multiple Logical
  * Channel subsystem support
  */
#define SR_DEV_LCSS             0xace30005
#define SR_DEV_ORB              0xace30010
#define SR_DEV_PMCW             0xace30011
#define SR_DEV_SCSW             0xace30012
#define SR_DEV_PCISCSW          0xace30013
#define SR_DEV_ATTNSCSW         0xace30014
#define SR_DEV_CSW              0xace30015
#define SR_DEV_PCICSW           0xace30016
#define SR_DEV_ATTNCSW          0xace30017
#define SR_DEV_ESW              0xace30018
#define SR_DEV_ECW              0xace30019
#define SR_DEV_SENSE            0xace3001a
#define SR_DEV_PGSTAT           0xace3001b
#define SR_DEV_PGID             0xace3001c
 /* By Adrian - SR_DEV_DRVPWD              */
#define SR_DEV_DRVPWD           0xace3001d

#define SR_DEV_BUSY             0xace30020
#define SR_DEV_RESERVED         0xace30021
#define SR_DEV_SUSPENDED        0xace30022
#define SR_DEV_PENDING          0xace30023
#define SR_DEV_PCIPENDING       0xace30024
#define SR_DEV_ATTNPENDING      0xace30025
#define SR_DEV_STARTPENDING     0xace30026
#define SR_DEV_CRWPENDING       0xace30027
#define SR_DEV_CCWADDR          0xace30028
#define SR_DEV_IDAPMASK         0xace30029
#define SR_DEV_IDAWFMT          0xace3002a
#define SR_DEV_CCWFMT           0xace3002b
#define SR_DEV_CCWKEY           0xace3002c

#define SR_DEV_MASK             0xfffff000
#define SR_DEV_CKD              0xace31000
#define SR_DEV_FBA              0xace32000
#define SR_DEV_TTY              0xace33000
#define SR_DEV_3270             0xace34000
#define SR_DEV_RDR              0xace35000
#define SR_DEV_PUN              0xace36000
#define SR_DEV_PRT              0xace37000
#define SR_DEV_TAPE             0xace38000
#define SR_DEV_COMM             0xace39000
#define SR_DEV_CTC              0xace3a000
#define SR_DEV_CTCI             0xace3b000
#define SR_DEV_CTCT             0xace3c000
#define SR_DEV_VMNET            0xace3d000
#define SR_DEV_LCS              0xace3e000
#define SR_DEV_CTCE             0xace3f000

#define SR_DELIMITER            0xaceffffe
#define SR_EOF                  0xacefffff

#if defined (_HERCULES_SR_C)
#define SR_WRITE_ERROR       goto sr_write_error
#define SR_READ_ERROR        goto sr_read_error
#define SR_SEEK_ERROR        goto sr_seek_error
#define SR_VALUE_ERROR       goto sr_value_error
#define SR_STRING_ERROR      goto sr_string_error
#else
#define SR_WRITE_ERROR \
 do { \
   logmsg(_("HHCSR010E write error: %s\n"), strerror(errno)); \
   return -1; \
 } while (0)
#define SR_READ_ERROR \
 do { \
   logmsg(_("HHCSR011E read error: %s\n"), strerror(errno)); \
   return -1; \
 } while (0)
#define SR_SEEK_ERROR \
 do { \
   logmsg(_("HHCSR012E seek error: %s\n"), strerror(errno)); \
   return -1; \
 } while (0)
#define SR_VALUE_ERROR \
 do { \
   logmsg(_("HHCSR013E value error, incorrect length\n")); \
   return -1; \
 } while (0)
#define SR_STRING_ERROR \
 do { \
   logmsg(_("HHCSR014E string error, incorrect length\n")); \
   return -1; \
 } while (0)
#endif

#ifdef HAVE_LIBZ
#define SR_DEFAULT_FILENAME "hercules.srf.gz"
#define SR_FILE gzFile
#define SR_OPEN(_path, _mode) \
 gzopen((_path), (_mode))
#define SR_READ(_ptr, _size, _nmemb, _stream) \
 gzread((_stream), (_ptr), (unsigned int)((_size) * (_nmemb)))
#define SR_WRITE(_ptr, _size, _nmemb, _stream) \
 gzwrite((_stream), (_ptr), (unsigned int)((_size) * (_nmemb)))
#define SR_SEEK(_stream, _offset, _whence) \
 gzseek((_stream), (_offset), (_whence))
#define SR_CLOSE(_stream) \
 gzclose((_stream))
#else
#define SR_DEFAULT_FILENAME "hercules.srf"
#define SR_FILE FILE*
#define SR_OPEN(_path, _mode) \
 fopen((_path), (_mode))
#define SR_READ(_ptr, _size, _nmemb, _stream) \
 fread((_ptr), (_size), (_nmemb), (_stream))
#define SR_WRITE(_ptr, _size, _nmemb, _stream) \
 fwrite((_ptr), (_size), (_nmemb), (_stream))
#define SR_SEEK(_stream, _offset, _whence) \
 fseek((_stream), (_offset), (_whence))
#define SR_CLOSE(_stream) \
 fclose((_stream))
#endif

#define SR_WRITE_HDR(_file, _key, _len) \
 do { \
   size_t _rc; \
   BYTE _buf[8]; \
   store_fw (_buf, (_key)); \
   store_fw (_buf+4, (_len)); \
   _rc = SR_WRITE(_buf, 1, 8, (_file)); \
   if (_rc != 8) SR_WRITE_ERROR; \
 } while (0)

#define SR_WRITE_STRING(_file, _key, _s) \
do { \
  size_t _rc; \
  if (strlen((_s)) + 1 > SR_MAX_STRING_LENGTH) SR_STRING_ERROR; \
  SR_WRITE_HDR((_file), (_key), strlen((_s)) + 1); \
  _rc = SR_WRITE((_s), 1, strlen((_s)) + 1, (_file)); \
  if (_rc != strlen((_s)) + 1) SR_WRITE_ERROR; \
} while (0);

#define SR_WRITE_BUF(_file, _key, _buf, _len) \
do { \
  size_t _rc; \
  if ((_len)) { \
    SR_WRITE_HDR((_file), (_key), (_len)); \
    _rc = SR_WRITE((_buf), 1, (_len), (_file)); \
    if (_rc != (_len)) SR_WRITE_ERROR; \
  } else \
    SR_WRITE_HDR((_file), (_key), 0); \
} while (0)

#define SR_WRITE_VALUE(_file, _key, _val, _len) \
do { \
  size_t _rc; \
  BYTE _buf[8]; \
  if ((_len) != 1 && (_len) != 2 && (_len) != 4 && (_len) != 8) \
    SR_VALUE_ERROR; \
  SR_WRITE_HDR((_file), (_key), (_len)); \
  switch ((_len)) { \
    case 1: _buf[0] = (_val); break; \
    case 2: store_hw(_buf, (_val)); break; \
    case 4: store_fw(_buf, (_val)); break; \
    case 8: store_dw(_buf, (_val)); break; \
  } \
  _rc = SR_WRITE(_buf, 1, (_len), (_file)); \
  if (_rc != (_len)) SR_WRITE_ERROR; \
} while (0)

#define SR_READ_HDR(_file, _key, _len) \
do { \
  size_t _rc; \
  BYTE _buf[8]; \
  _rc = SR_READ(_buf, 1, 8, (_file)); \
  if (_rc != 8) SR_READ_ERROR; \
  (_key) = fetch_fw(_buf); \
  (_len) = fetch_fw(_buf+4); \
} while (0)

//FIXME: Workaround for problem involving gzseek
//       and large files.  Just read the data.
#define SR_READ_SKIP(_file, _len) \
do { \
  size_t _rc; \
  size_t _l; \
  BYTE _buf[256]; \
  _l = (_len); \
  while (_l) { \
    _rc = SR_READ(_buf, 1, _l < 256 ? _l : 256, (_file)); \
    if (_rc == (size_t)-1) SR_READ_ERROR; \
    _l -= _l < 256 ? _l : 256; \
  } \
} while (0)

#define SR_READ_STRING(_file, _p, _len) \
do { \
  size_t _rc; \
  if ((_len) > SR_MAX_STRING_LENGTH) SR_STRING_ERROR; \
  _rc = SR_READ((_p), 1, (_len), (_file)); \
  if (_rc != (_len)) SR_READ_ERROR; \
} while (0)

#define SR_READ_BUF(_file, _p, _len) \
do { \
  size_t _rc; \
  _rc = SR_READ((_p), 1, (_len), (_file)); \
  if (_rc != (_len)) SR_READ_ERROR; \
} while (0)

#define SR_READ_VALUE(_file, _len1, _p, _len2) \
do { \
  size_t _rc; \
  BYTE _buf[8]; \
  U64 _value; \
  if ((_len1) != 1 && (_len1) != 2 && (_len1) != 4 && (_len1) != 8) \
    SR_VALUE_ERROR; \
  _rc = SR_READ(_buf, 1, (_len1), (_file)); \
  if (_rc != (_len1)) SR_READ_ERROR; \
  switch ((_len1)) { \
    case 1: _value = _buf[0]; break; \
    case 2: _value = fetch_hw(_buf); break; \
    case 4: _value = fetch_fw(_buf); break; \
    case 8: _value = fetch_dw(_buf); break; \
    default: _value=0; break; /* To ward off gcc -Wall */ \
  } \
  switch ((_len2)) { \
  case 1: \
    { \
      BYTE *_ptr = (void *)(_p); \
      *_ptr = _value & 0xff; \
      break; \
    } \
  case 2: \
    { \
      U16 *_ptr = (void *)(_p); \
      *_ptr = _value & 0xffff; \
      break; \
    } \
  case 4: \
    { \
      U32 *_ptr = (void *)(_p); \
      *_ptr = _value & 0xffffffff; \
      break; \
    } \
  case 8: \
    { \
      U64 *_ptr = (void *)(_p); \
      *_ptr = _value; \
      break; \
    } \
  } \
} while (0)

#define SR_SKIP_NULL_DEV(_dev, _file, _len) \
  if ((_dev) == NULL) { \
    SR_READ_SKIP((_file),(_len)); \
    break; \
  }

#endif /* !defined(_HERCULES_SR_H) */
