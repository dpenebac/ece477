void init_usart_1(); //Initialize USART 1
void finger_add_user(uint8_t); //Adds user with specified User ID to fingerprint scanner
uint8_t * finger_check_user(uint8_t);
void finger_test();
void init_test();
uint8_t * finger_send_command(uint8_t[8]);
