void init_i2c();
void i2c_waitidle();
void i2c_start(uint32_t, uint8_t, uint8_t);
void i2c_stop();
int i2c_checknack();
void i2c_clearnack();
int i2c_senddata(uint8_t, const void *, uint8_t);
int i2c_recvdata(uint8_t, void *, uint8_t);

void eeprom_write(uint16_t, const char *, uint8_t);
int eeprom_write_complete();
void eeprom_blocking_wrie(uint8_t, const char*, uint8_t);

void update_users();
void format_users(char []);
void read_users();
void unformat_users(char []);
void reset_users();
void zero_users();

int main_eeprom();

struct user {
    char passcode[20];
};

// struct array []
// array []

struct user user_list[6];
struct user test_list[6];
