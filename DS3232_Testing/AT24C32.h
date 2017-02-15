#ifndef AT24C32_h
#define AT24C32_h
class AT24C32 {
  public:
    AT24C32();

    static uint8_t is_present(void);
    static int read_mem(int addr, uint8_t *buff, uint8_t count);
    static uint8_t write_mem(int addr, uint8_t value);
    
  private:
};

extern AT24C32 ee;
#endif
