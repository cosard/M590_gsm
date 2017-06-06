#include <SoftwareSerial.h>

#define USE_PDU_FMT

#ifdef USE_PDU_FMT
  extern "C" {
    #include "pdu.h"
  }
#endif

SoftwareSerial debugSerial(2, 3); // RX, TX

#define PHONE_NUM   "MY_PHONE_NUMBER"

int set_char_set();
int set_sms_mode();
int send_sms(char *ch, char *from);
int check_status(int status);
int list_messages(int type, int *index_result, char *msg_out, char *from, int pdu_fmt);
int read_messages(int index, char *msg);
int delete_messages(int index);
void send_cmd(int cmd);

enum {
  BATTERY = 0,
  SIGNAL,
  SERVICE,
  SOUNDER,
  UNREAD_SMS,
  CALL,
  ROAM,
  SMSFULL,
  GPRS,
  CALL_SETUP,
  CALL_HELD,
  CALL_SERVICE,
}CIND_CASES;
/*
enum {
  SET_CHAR_SET,
  SET_SMS_MODE,
  SEND_SMS,
  CHECK_STATUS,
  LIST_MESSAGES,
  READ_MESSAGES,
  DELETE_MESSAGES,
  GSM_COMMANDS_SET_MAX,
}GSM_COMMANDS_SET;

int* GMS_COMMANDS_PTR[GSM_COMMANDS_SET_MAX] = {
  set_char_set,    // "CSCS"
  set_sms_mode,    // "CMGF"
  send_sms,        // "CMGS"
  check_status,    // "CIND"
  list_messages,   // "CMGL"
  read_messages,   // "CMGR"
  delete_messages, // "CMGD"
  NULL,
};
*/

enum MESSAGE_TYPE {
  REC_UNREAD,
  REC_READ,
  STO_UNSET,
  STO_SET,
  ALL,
  MESSAGE_TYPE_MAX,
};

const char *message_type[] {
  "REC_UNREAD",
  "REC_READ",
  "STO_UNSET",
  "STO_SET",
  "ALL",
  "NaN"
};


int wait_for_ack() {
  
  unsigned long t = millis();
  //char ch[20];
  int i;
  String s;
  
  //Serial.flush();
  //delay(200);
  
  debugSerial.println("ack");
  
  while(1) {
    if(millis() - t > 3000) {
      debugSerial.println("timeout");
      return -1;
    }
    
    if(Serial.available()) {
      t = millis();
      s = Serial.readStringUntil('\n');
      if(s.length() <= 2)
        continue;
      
      debugSerial.print(" ACK: <");
      debugSerial.print(s);
      debugSerial.println(">");
      
      if(s.indexOf("OK") != -1)
        return 0;
      else
        return -1;
    }
  }
  return -1;
}

int set_char_set() {
  Serial.print("AT+CSCS=\"GSM\"\r");
  return wait_for_ack();
}

int set_sms_mode() {
#ifdef USE_PDU_FMT  
  Serial.print("AT+CMGF=0\r");  // pdu mode
#else
  Serial.print("AT+CMGF=1\r");
#endif  
  return wait_for_ack();
}

int send_sms(char *ch, char *from) {
  unsigned long t = millis();
  String s;
  
  Serial.print("AT+CMGS=\"");
  Serial.print(from);
  Serial.print("\"\r");
  delay(300);
  Serial.print(ch);    //The text of the message to be sent
  Serial.print("\r");
  delay(300);
  Serial.write(0x1A);
  delay(5000);
  Serial.flush();
  return 0;
  while(1) {
    if(millis() - t > 10000 )
      break;

    if(Serial.available()) {
      t = millis();
      
      s = Serial.readStringUntil('\n');
      if(s.length() <= 2)
        continue;
      else if(s.indexOf("CMGS"))
        break;
    }
  }
  return wait_for_ack();
}


int check_status(int status) {
  int ret = 0;
  unsigned long t;
  String s;
  int i;
  
  if(status == -1)
    return -1;

  debugSerial.println("chkstat");
  
  Serial.print("AT+XCIND=");
  Serial.print((1 << status));
  Serial.print("\r");
  if(wait_for_ack() == -1)
    return 0;
  
  delay(500);
  Serial.println("AT+CIND?");
  
  t = millis();
  
  while(1) {
    if(millis() - t > 3000) {
      ret = 0;
      debugSerial.println("timeout");
      Serial.flush();
      goto end;
    }
     
    if(Serial.available()) {
      t = millis();
      s = Serial.readStringUntil('\n');
      if(s.length() <= 2)
        continue;
        
      if(s.indexOf("OK") != -1 || s.indexOf("ERROR") != -1)
        continue;
      
      Serial.flush();
      break;
    }
    //delay(50);
  }
  
  debugSerial.println("Stat: ");
  debugSerial.println(s);
  
  if(s.indexOf("1") != -1)
    ret = 1;
  else
    ret = 0;
    
end:

  Serial.print("AT+XCIND=4095\r");
  
  if(wait_for_ack() == -1)
    ret = 0;

  return ret;
}


int list_messages(int type, int *index_result, char *msg_out, char *from, int pdu_fmt) {
  unsigned long t = millis();
  char ch[100];
  int i = 0;
  int j = 0;
  int ret = 0;
  int message_index = 0;
  int message_count = 0;
  int pos = 0;
  char *temp;
  char num[5];
  String s;
  int got_header = 0;
  
  debugSerial.print("list msg. type ");
  debugSerial.println(type);
  
  if(!pdu_fmt) {
    Serial.println("AT+CMGL=" + type );
    wait_for_ack();
  
    while(1) {
      if(millis() - t > 3000) {
        debugSerial.println("timeoutt");
        return -1;
      }
  
      if(Serial.available()) {
        ch[i] = Serial.read();
        if(ch[i] == '\n') {
          if(i > 0 && i < 2) {
            ch[0] = '\0';
            i = 0;
            continue;
          }
        }
        else {
          i++;
          continue;
        }
      }
      else {
        delay(10);
        continue;
      }

      ch[++i] = '\0';
      debugSerial.print("Len: ");
      debugSerial.print(i);
      debugSerial.print("Incoming: <");
      debugSerial.print(ch);
      debugSerial.println(">");

      if(strstr(ch, "ERR") ) {
        ret = -1;
        break;
      }

      if(strstr(ch, "OK") ) {
        ret = 0;
        break;
      }
    
      temp = strstr(ch, "CMGL:");
      if(temp) {
        for(i = 0; i < 5; i++) {
          if(*(temp + 6 + i) == ',')
            break;
        }
        strncpy(num, temp + 6, i);      
        message_index += atoi(num);
        *(index_result + message_count++) = message_index;

        debugSerial.print("msg index: ");
        debugSerial.println(message_index);

        
        temp = strchr(temp, ',');
        if(temp) {
          temp = strchr(temp + 1, ',');
          if(temp)
            strncpy(from, temp + 2, strchrnul(temp + 2, ',') - (temp + 2) - 1);
        }
      
        ch[0] = '\0';
        i = 0;
        continue;
      }
      break;
    }
  }
  else {
    Serial.println("AT+CMGL=" + type );
    wait_for_ack();
    while(1) {
      if(millis() - t > 3000) {
        debugSerial.println("timeout");
        return -1;
      }
  
      if(Serial.available()) {
        t = millis();
        s = Serial.readStringUntil('\n');
        if(s.length() <= 2)
          continue;
    
        if(!got_header) {
          debugSerial.print("header: <");
          debugSerial.print(s);
          debugSerial.println(">");
          got_header = 1;
          
          i = s.indexOf("CMGL");
          if(i != -1) {
            temp = (char *)(s.c_str() + i);
            for(i = 0; i < 5; i++) {
              if(*(temp + 6 + i) == ',')
              break;
            }
            strncpy(num, temp + 6, i);
            message_index += atoi(num);
            *(index_result + message_count++) = message_index;

            debugSerial.print("msg index: ");
            debugSerial.println(message_index);
          }
          continue;
        }
        else {
          if(message_count == 1) {
            strncpy(msg_out, s.c_str(), s.length() - 1);
          }
          debugSerial.print("PDU: <");
          debugSerial.print(s);
          debugSerial.println(">");
          break;
        }       
      }
    }
  }
  debugSerial.print("msg count: ");
  debugSerial.println(message_count);
  return message_count;
}

int read_messages(int index, char *msg) {
  String s;
  unsigned long t;
  int i = 0;
  int ret = 0;
  char ch[128] = {0};
  
  t = millis();
  Serial.print("AT+CMGR=");
  Serial.print(index);
  Serial.print("\r");
  
  while(1) {
    if(millis() - t > 3000) {
      debugSerial.println("timeoutt");
      return -1;
    }
      
    if(Serial.available()) {
      ch[i] = Serial.read();
      if(ch[i] == '\n') {
        if(i > 0 && i < 4) {
          ch[0] = '\0';
          i = 0;
          continue;
        }
      }
      else {
        i++;
        continue;
      }
    }
    else {
      delay(10);
      continue;
    }

    ch[++i] = '\0';
    debugSerial.println(ch);
    
    if(strstr(ch, "ERR") ) {
      ret = -1;
      break;
    }

    if(strstr(ch, "CMGR") ) {
      ch[0] = '\0';
      i = 0;
      continue;
    }
    else {
      wait_for_ack();
      strcpy(msg, ch);
      ret = 0;
      debugSerial.print("i: ");
      debugSerial.println(i);
      debugSerial.print("Msg: ");
      debugSerial.println(ch);
      break;
    }
  }
  debugSerial.print("ret: ");
  debugSerial.println(ret);
  return ret;
}

int delete_messages(int index) {
  Serial.print("AT+CMGD=");
  Serial.print(index, DEC);
  Serial.print("\r");
  return wait_for_ack();
}

  
void send_cmd(int cmd) {
  
}

void setup()
{
  Serial.begin(9600);  //Baud rate of the GSM/GPRS Module 
 
  debugSerial.begin(9600);
  debugSerial.println("\nStart..");
#if 0
  sms_t sms;
  if ( sms_decode_pdu(pdu, strlen(pdu), &sms) == 0 ) {
    /* write response to the received SMS */
    debugSerial.print("This is my response. Hello world!\n");
  }
  else {
    debugSerial.print("Failed to process SMS message\n");
  }

  debugSerial.println((char *)sms.message);

  for(;;);
#endif
  pinMode(13, OUTPUT);  // D13 is output pin
  
  digitalWrite(13, LOW);
  
  delay(5000);
  
  digitalWrite(13, HIGH);
  delay(50);
  digitalWrite(13, LOW);
  delay(50);
  digitalWrite(13, HIGH);
  delay(50);
  digitalWrite(13, LOW);
    
  Serial.print("ATE0\r");   // no echo
  delay(500);
  
  set_char_set();
  delay(500); 
  set_sms_mode();
  delay(500);  
  
  debugSerial.println("Delete Messages");
  Serial.print("AT+CMGD=0,4\r");
  wait_for_ack();
  
  delay(100);
  Serial.flush();
}


void loop()
{
  int msg_count = 0;
  int msg_index[16] = {0};
  char msg[128] = {0};
  char from[25] = {0};

  if(check_status(UNREAD_SMS) != 0) {
    debugSerial.println("yeni mesaj var");
    digitalWrite(13, HIGH);

    debugSerial.println("View Unread Messages");
#ifdef USE_PDU_FMT
    sms_t sms;
    msg_count = list_messages(REC_UNREAD, msg_index, msg, from, 1);

    if(msg_count) {
      if ( sms_decode_pdu(msg, strlen(msg), &sms) == 0 ) {
        debugSerial.print("Incoming msg: ");
        debugSerial.println((char *)sms.message);
        //delay(1000);
        //send_sms(msg);
        //delay(1000);
        //delete_messages(msg_index[0]);
        delay(500);
      }
    }
#else

    msg_count = list_messages(REC_UNREAD, msg_index, msg, from, 0);
    if(msg_count) {
      debugSerial.print("From: ");
      debugSerial.println(from);

      read_messages(msg_index[0], msg);
      debugSerial.print("Msg: ");
      debugSerial.println(msg);
      delay(500);
      send_sms(msg, from);
      delay(500);
      delete_messages(msg_index[0]);
      delay(500);
    }
#endif  
  }
  else
    digitalWrite(13, LOW);
  
  delay(2000);
}

