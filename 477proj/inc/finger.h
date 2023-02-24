void init_usart1(); //Initialize USART 1
void finger_add_user(uint8_t); //Adds user with specified User ID to fingerprint scanner
uint8_t * finger_check_user();
void finger_test();
void init_test();
uint8_t * finger_send_command(uint8_t[8]);

#define ACK_SUCCESS 0x00 //Operation successfully
#define ACK_FAIL 0x01 // Operation failed
#define ACK_FULL 0x04 // Fingerprint database is full
#define ACK_NOUSER 0x05 //No such user
#define ACK_USER_EXIST 0x07 // User already exists
#define ACK_TIMEOUT 0x08 // Acquisition timeout
