#ifndef SMS_PDU_H
#define	SMS_PDU_H

#include <stdint.h>

#define SMS_MESSAGE_SIZE    161
#define SMS_SENDER_SIZE     25

typedef unsigned int uint;
typedef int ssize_t;

typedef struct {
    int id;
    uint8_t message[SMS_MESSAGE_SIZE];
    size_t message_length;
    char telnum[SMS_SENDER_SIZE];
    size_t telnum_length;
    int sender_length;
   /* 
    * Please refer to:
    *   http://www.dreamfabric.com/sms/type_of_address.html
    */    
    uint8_t telnum_type;
} sms_t;

/*
 * @brief	Decode SMS from PDU string received from modem.
 * @param	data	SMS message in PDU format received from modem
 * @param	sz		Length of the data
 * @param	sms		Pointer to output sms_t structure
 * @return	int		Zero on success
 */
extern int sms_decode_pdu(const char *data, size_t sz, sms_t *sms);

/*
 * @brief	Create PDU string from input data.
 * @param	sms		Pointer to output sms_t structure
 * @param	data	Pointer to output buffer
 * @param	sz		Size of hte output buffer
 * @return	int		Nonnegative number of success
 *
 * This function encodes PDU message from input data stored in sms_t structure.
 * Please do not forget to correctly fill all fields in sms_t structure
 * passed to this function. On successful completion function returns length of
 * the encoded PDU string.
 */
extern int sms_encode_pdu(sms_t *sms, char *data, size_t sz);

/*
 * @brief	Write message to the SMS message.
 * @param	mesg	Zero terminated message string
 * @param	sms		Destination SMS
 * @return	int		Nonnegative number of success
 * 
 * Function stores message pointed by mesg pointer into sms message.
 * Useful in case you want to construct reply message to received sms. Just simply call
 * this function and use encoded PDU string.
 */
extern ssize_t sms_write(const char *mesg, sms_t *sms);

#endif	/* SMS_PDU_H */

