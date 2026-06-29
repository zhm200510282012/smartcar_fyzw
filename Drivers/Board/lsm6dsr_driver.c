#include "lsm6dsr_driver.h"
#include "../../App/app_config.h"
#include "../../BSP/bsp_timebase.h"

#ifndef HOST_SIL
#include "AI8051U_GPIO.h"
#include "AI8051U_SPI.h"
#include "AI8051U_Switch.h"
#endif

#define LSM6DSR_REG_WHO_AM_I 0x0Fu
#define LSM6DSR_REG_CTRL1_XL 0x10u
#define LSM6DSR_REG_CTRL2_G 0x11u
#define LSM6DSR_REG_CTRL3_C 0x12u
#define LSM6DSR_REG_OUTX_L_G 0x22u
#define LSM6DSR_READ_MASK 0x80u

static lsm6dsr_sample_t g_last_sample;

#ifdef HOST_SIL
static lsm6dsr_sample_t g_host_sample;
#endif

static void clear_sample(lsm6dsr_sample_t *sample)
{
    sample->spi_ok = APP_FALSE;
    sample->id_ok = APP_FALSE;
    sample->sample_valid = APP_FALSE;
    sample->who_am_i = 0u;
    sample->accel_raw[0] = 0;
    sample->accel_raw[1] = 0;
    sample->accel_raw[2] = 0;
    sample->gyro_raw[0] = 0;
    sample->gyro_raw[1] = 0;
    sample->gyro_raw[2] = 0;
    sample->timestamp_ms = 0ul;
}

#ifndef HOST_SIL
static s16 combine_le(u8 low, u8 high)
{
    return (s16)((u16)low | ((u16)high << 8));
}

static void cs_low(void)
{
    gpio_write_pin(P4_0, 0u);
}

static void cs_high(void)
{
    gpio_write_pin(P4_0, 1u);
}

static u8 spi_read_reg(u8 reg)
{
    u8 value;

    cs_low();
    SPI_WriteByte((u8)(reg | LSM6DSR_READ_MASK));
    value = SPI_ReadByte();
    cs_high();
    return value;
}

static void spi_write_reg(u8 reg, u8 value)
{
    cs_low();
    SPI_WriteByte(reg);
    SPI_WriteByte(value);
    cs_high();
}

static void spi_read_bytes(u8 reg, u8 *buffer, u8 count)
{
    u8 index;

    cs_low();
    SPI_WriteByte((u8)(reg | LSM6DSR_READ_MASK));
    for (index = 0u; index < count; index++) {
        buffer[index] = SPI_ReadByte();
    }
    cs_high();
}
#endif

void lsm6dsr_driver_init(void)
{
    clear_sample(&g_last_sample);
#ifdef HOST_SIL
    clear_sample(&g_host_sample);
#else
    {
        SPI_InitTypeDef spi;

        SPI_SW(SPI_P40_P41_P42_P43);
        gpio_init_pin(P4_0, GPIO_Mode_Out_PP);
        gpio_write_pin(P4_0, 1u);

        spi.SPI_Enable = ENABLE;
        spi.SPI_SSIG = ENABLE;
        spi.SPI_FirstBit = SPI_MSB;
        spi.SPI_Mode = SPI_Mode_Master;
        spi.SPI_CPOL = SPI_CPOL_High;
        spi.SPI_CPHA = SPI_CPHA_2Edge;
        spi.SPI_Speed = SPI_Speed_16;
        spi.TimeOutEnable = DISABLE;
        spi.TimeOutINTEnable = DISABLE;
        spi.TimeOutScale = TO_SCALE_1US;
        spi.TimeOutTimer = 0ul;
        SPI_Init(&spi);

        g_last_sample.who_am_i = spi_read_reg(LSM6DSR_REG_WHO_AM_I);
        g_last_sample.spi_ok = APP_TRUE;
        g_last_sample.id_ok = (g_last_sample.who_am_i == LSM6DSR_WHO_AM_I_EXPECTED) ? APP_TRUE : APP_FALSE;
        if (g_last_sample.id_ok != APP_FALSE) {
            spi_write_reg(LSM6DSR_REG_CTRL3_C, 0x44u);
            spi_write_reg(LSM6DSR_REG_CTRL1_XL, 0x40u);
            spi_write_reg(LSM6DSR_REG_CTRL2_G, 0x40u);
        }
    }
#endif
}

lsm6dsr_sample_t lsm6dsr_driver_read(void)
{
#ifdef HOST_SIL
    g_last_sample = g_host_sample;
    g_last_sample.timestamp_ms = bsp_timebase_now_ms();
    return g_last_sample;
#else
    u8 bytes[12];

    clear_sample(&g_last_sample);
    g_last_sample.who_am_i = spi_read_reg(LSM6DSR_REG_WHO_AM_I);
    g_last_sample.spi_ok = APP_TRUE;
    g_last_sample.id_ok = (g_last_sample.who_am_i == LSM6DSR_WHO_AM_I_EXPECTED) ? APP_TRUE : APP_FALSE;
    g_last_sample.timestamp_ms = bsp_timebase_now_ms();
    if (g_last_sample.id_ok == APP_FALSE) {
        return g_last_sample;
    }

    spi_read_bytes(LSM6DSR_REG_OUTX_L_G, bytes, 12u);
    g_last_sample.gyro_raw[0] = combine_le(bytes[0], bytes[1]);
    g_last_sample.gyro_raw[1] = combine_le(bytes[2], bytes[3]);
    g_last_sample.gyro_raw[2] = combine_le(bytes[4], bytes[5]);
    g_last_sample.accel_raw[0] = combine_le(bytes[6], bytes[7]);
    g_last_sample.accel_raw[1] = combine_le(bytes[8], bytes[9]);
    g_last_sample.accel_raw[2] = combine_le(bytes[10], bytes[11]);
    g_last_sample.sample_valid = APP_TRUE;
    return g_last_sample;
#endif
}

lsm6dsr_sample_t lsm6dsr_driver_last_sample(void)
{
    return g_last_sample;
}

#ifdef HOST_SIL
void lsm6dsr_driver_host_set_sample(app_bool_t spi_ok,
                                    u8 who_am_i,
                                    s16 ax,
                                    s16 ay,
                                    s16 az,
                                    s16 gx,
                                    s16 gy,
                                    s16 gz)
{
    g_host_sample.spi_ok = spi_ok;
    g_host_sample.who_am_i = who_am_i;
    g_host_sample.id_ok = (spi_ok != APP_FALSE && who_am_i == LSM6DSR_WHO_AM_I_EXPECTED) ? APP_TRUE : APP_FALSE;
    g_host_sample.sample_valid = (g_host_sample.id_ok != APP_FALSE) ? APP_TRUE : APP_FALSE;
    g_host_sample.accel_raw[0] = ax;
    g_host_sample.accel_raw[1] = ay;
    g_host_sample.accel_raw[2] = az;
    g_host_sample.gyro_raw[0] = gx;
    g_host_sample.gyro_raw[1] = gy;
    g_host_sample.gyro_raw[2] = gz;
    g_host_sample.timestamp_ms = bsp_timebase_now_ms();
}
#endif
