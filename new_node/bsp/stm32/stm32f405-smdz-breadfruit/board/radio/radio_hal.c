/*
 * This file is part of the ART-6LoWPAN Radio Library.
 *
 * Copyright (c) 2017, Armink, <armink.ztl@gmail.com>
 * Copyright (c) 2017, xidongxu, <xi_dongxu@126.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Radio HAL(Hardware Abstraction Layer) layer driver. It provide radio API to `radio_trans_soft.c` or `radio_driver.c` .
 * Created on: 2017-09-15
 */

#include "radio_cfg.h"
#include <board.h>
#include "drv_spi.h"


//#define DRV_DEBUG
#define LOG_TAG             "radio_hal"
#include <drv_log.h>

#include "radio_hal.h"
#include <stdbool.h>
#include <rtthread.h>
#include <drivers/spi.h>

#define RADIO_SPI_BUS_NAME             "spi1"
#define RADIO_SPI_DEV_NAME             "spi10"

#define RADIO_DEFAULT_SPI_CFG                   \
{                                               \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,         \
    .data_width = 8,                            \
    .max_hz = 1 * 1000 * 1000,                  \
}

#define RSSI_UNKNOWN                   -128
/* 状态切换超时，单位：毫秒 */
#define STATE_SWITCH_TIMEOUT           10

U8 Radio_Configuration_Data_Array[] = RADIO_CONFIGURATION_DATA_ARRAY;
tRadioConfiguration RadioConfiguration = RADIO_CONFIGURATION_DATA;
tRadioConfiguration *pRadioConfiguration = &RadioConfiguration;

/* SPI 设备对象 */
static struct rt_spi_device *radio_spi_device;
/* 命令回复的结果锁 */
static rt_mutex_t cmd_reply_locker = NULL;

static void radio_power_up(void) {
    /* Hardware reset the chip */
    si446x_reset();

    /* Wait until reset timeout or Reset IT signal */
    rt_thread_delay(rt_tick_from_millisecond(5));
}

static bool wait_state_switch(uint8_t state) {
    bool switch_ok = false;
    rt_tick_t last_time = rt_tick_get();
    const rt_tick_t timeout = STATE_SWITCH_TIMEOUT * RT_TICK_PER_SECOND / 1000;
    uint8_t cur_state;

    while (rt_tick_get() - last_time < timeout) {
        radio_cmd_reply_lock();
        si446x_request_device_state();
        cur_state = Si446xCmd.REQUEST_DEVICE_STATE.CURR_STATE;
        radio_cmd_reply_unlock();
        if (cur_state == state) {
            switch_ok = true;
            break;
        }
        rt_thread_delay(rt_tick_from_millisecond(1));
    }

    if (rt_tick_get() - last_time >= timeout) {
        LOG_E("radio main state(%d) switch failed!", state);
    }

    return switch_ok;
}

void radio_start_rx(uint8_t channel, uint8_t packet_lenght) {
    /* Set filed2 to max length */
    si446x_set_property(SI446X_PROP_GRP_ID_PKT, 2, SI446X_PROP_PKT_FIELD_2_LENGTH_FIELD_2_LENGTH_12_8_INDEX, 0x04, 0x00);

    /* Reset the RX & TX FIFO */
    si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT);

    /* Switch to SPI_ACTIVE state */
    si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SPI_ACTIVE);

    /* Wait SPI_ACTIVE state switch OK */
    wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_SPI_ACTIVE);

    /* Switch to RX_TUNE state */
    si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_RX_TUNE);

    /* Wait RX_TUNE state switch OK */
    wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_RX_TUNE);

    /* Start Receiving packet, channel, START immediately, Packet length used or not according to packetLength */
    si446x_start_rx(channel, 0u, 0u, SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_RX,
                    SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_RX,
                    SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX );
}

/*!
 *  Set Radio to TX mode, variable packet length.
 *  Use it when multiple fields are in use (START_TX is called with 0 length).
 *
 *  @param channel Freq. Channel, Packet to be sent length of of the packet sent to TXFIFO
 *
 *  @note
 *
 */
void radio_start_tx_variable_packet_multifield(uint8_t channel, uint8_t *radio_packet, uint8_t length) {
    /* Reset the TX & RX FIFO */
    si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);

    /* Switch to SPI_ACTIVE state */
    si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SPI_ACTIVE);

    /* Wait SPI_ACTIVE state switch OK */
    wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_SPI_ACTIVE);

    /* Fill the TX FIFO with datas */
    si446x_write_tx_fifo(length, radio_packet);

    /* Switch to SPI_ACTIVE state */
    si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_SPI_ACTIVE);

    /* Wait SPI_ACTIVE state switch OK */
    wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_SPI_ACTIVE);

    /* Switch to TX_TUNE state */
    si446x_change_state(SI446X_CMD_CHANGE_STATE_ARG_NEXT_STATE1_NEW_STATE_ENUM_TX_TUNE);

    /* Wait TX_TUNE state switch OK */
    wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_TX_TUNE);

    /* Start sending packet, channel 0, START immediately, stay TX status when send finish */
    si446x_start_tx(channel, 0x00, 0);
}

uint8_t radio_hal_init(void) {
    uint8_t result = 0;
    struct rt_spi_configuration cfg = RADIO_DEFAULT_SPI_CFG;

    rt_hw_spi_device_attach(RADIO_SPI_BUS_NAME, RADIO_SPI_DEV_NAME, GPIOA, GPIO_PIN_4);
    
    radio_spi_device = (struct rt_spi_device *) rt_device_find(RADIO_SPI_DEV_NAME);
    if (radio_spi_device == RT_NULL || radio_spi_device->parent.type != RT_Device_Class_SPIDevice) {
        LOG_E("ERROR: SPI device %s not found!\n", RADIO_SPI_DEV_NAME);
        RT_ASSERT(0);
    }
    rt_spi_configure(radio_spi_device, &cfg);

    if (!cmd_reply_locker) {
        cmd_reply_locker = rt_mutex_create("cmd_reply", RT_IPC_FLAG_FIFO);
        RT_ASSERT(cmd_reply_locker);
    }

    /* Power Up the radio chip */
    radio_power_up();

    si446x_part_info();
    LOG_D("si446x chip revision: %x, part id: %x, rom id: %d", Si446xCmd.PART_INFO.CHIPREV, Si446xCmd.PART_INFO.PART, Si446xCmd.PART_INFO.ROMID);

    if (Si446xCmd.PART_INFO.PART != 0x4463) {
        LOG_E("Radio chip part info(%d) error. This radio driver only support SI4463", Si446xCmd.PART_INFO.PART);
        return SI446X_INIT_ERROR;
    }

#if SI446X_USING_REV_B1
    if (Si446xCmd.PART_INFO.CHIPREV != 0x11) {
        LOG_E("Radio chip revision(%d) don't match. This radio driver only support SI4463 Rev.B1(0x11)", Si446xCmd.PART_INFO.CHIPREV);
        return SI446X_INIT_ERROR;
    }
#else
    if (Si446xCmd.PART_INFO.CHIPREV != 0x22) {
        log_e("Radio chip revision(%d) don't match. This radio driver only support SI4463 Rev.C2(0x22)", Si446xCmd.PART_INFO.CHIPREV);
        return SI446X_INIT_ERROR;
    }
#endif /* SI446X_USING_REV_B1 */

    /* Load radio configuration */
    if (SI446X_SUCCESS != (result = si446x_configuration_init(pRadioConfiguration->Radio_ConfigurationArray))) {
        /* Power Up the radio chip */
        radio_power_up();
        LOG_E("SI4463 radio hal configuration init failed(%d)", result);
        return result;
    } else {
        LOG_I("SI4463 radio hal configuration init success");
    }

    /* 配置 field1 长度 */
    si446x_set_property(SI446X_PROP_GRP_ID_PKT, 2, SI446X_PROP_PKT_FIELD_1_LENGTH_FIELD_1_LENGTH_12_8_INDEX, 0x00, RADIO_PH_FILE1_LEN);
    /* 配置 field2 长度 */
    si446x_set_property(SI446X_PROP_GRP_ID_PKT, 2, SI446X_PROP_PKT_FIELD_2_LENGTH_FIELD_2_LENGTH_12_8_INDEX, 0x04, 0x00);
    // Read ITs, clear pending ones
    si446x_get_int_status(0u, 0u, 0u);
    // Start RX with Packet handler settings
    radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);

    return result;
}

void radio_software_init(void) {
    radio_transmit_software_init();
}

extern void si446x_sdn_enabled(bool enabled);

void radio_hal_AssertShutdown(void)
{
    si446x_sdn_enabled(false);
}

void radio_hal_DeassertShutdown(void)
{
    si446x_sdn_enabled(true);
}

void radio_hal_ClearNsel(void)
{
//    RF_NSEL = 0;
}

void radio_hal_SetNsel(void)
{
//    RF_NSEL = 1;
}

bool si446x_irq_pin_state(void);
BIT radio_hal_NirqLevel(void)
{
    return si446x_irq_pin_state();
}

void radio_hal_SpiWriteThenRead(U8 byteSend, U8* pSendData, U8 byteRecv, U8* pRecvData)
{
    rt_spi_send_then_recv(radio_spi_device, pSendData, byteSend, pRecvData, byteRecv);
}

void radio_hal_SpiWriteThenWrite(U8 sendByte1, U8* pSendData1, U8 sendByte2, U8* pSendData2)
{
    rt_spi_send_then_send(radio_spi_device, pSendData1, sendByte1, pSendData2, sendByte2);
}

void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
    rt_spi_send(radio_spi_device, pData, byteCount);
}

void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
    rt_spi_recv(radio_spi_device, pData, byteCount);
}

U8 radio_find_property(U8* pbArray, U8 bGroup, U8 bAddress, U8* pbValue) {
    U16 wInd = 0;
    U8 bPropertyValue = 0x00;
    BIT gPropertyFound = FALSE;

    // Input validation
    if (pbArray == NULL) {
        return FALSE;
    }

    // Search until reaching the terminating 0
    while (pbArray[wInd] != 0) {
        // Looking for SET_PROPERTY = 0x11 command
        if (pbArray[wInd + 1] != 0x11) {
            wInd += pbArray[wInd] + 1;
            continue;
        }

        // It is a SET_PROPERTY command, check if the corresponding row makes sense (i.e. the array is not broken)
        if (pbArray[wInd] < 0x05 || pbArray[wInd] > 0x10
                || pbArray[wInd] != (pbArray[wInd + 3] + 4)) // Command length in line with API length
                        {
            return FALSE;
        }

        // Look for property value
        if (pbArray[wInd + 2] == bGroup) {
            if ((pbArray[wInd + 4] <= bAddress)
                    && (bAddress < pbArray[wInd + 3] + pbArray[wInd + 4])) {
                bPropertyValue = pbArray[wInd + 5 + (bAddress - pbArray[wInd + 4])];
                gPropertyFound = TRUE;
                // Don't break the loop here, check the rest of the array
            }
        }
        wInd += pbArray[wInd] + 1;
    }

    if (gPropertyFound) {
        *pbValue = bPropertyValue;
    }
    return gPropertyFound;
}

/**
 * radio SPI 通信超时处理回调
 *
 * @return 0: 超时异常异常处理成功
 */
uint8_t radio_comm_err_cb(void) {
#define COMM_ERR_REINIT_MAX_TIMES      50

    static uint8_t reinit_times = 0;

    if (reinit_times < COMM_ERR_REINIT_MAX_TIMES) {
        LOG_E("SI446x radio communicate error. The radio will reinitialize.");
        radio_hal_init();
        reinit_times ++;
    } else if (reinit_times == COMM_ERR_REINIT_MAX_TIMES) {
        reinit_times ++;
        LOG_E("SI446x radio has too many(>=%d) communicate error. Please check!", COMM_ERR_REINIT_MAX_TIMES);
        return 1;
    } else if (reinit_times > COMM_ERR_REINIT_MAX_TIMES) {
        rt_thread_delay(rt_tick_from_millisecond(100));
        return 1;
    }

    return 0;
}

static int calc_rssi_from_modem_status(uint8_t status) {
    return status / 2 - 0x40 - 70;
}

/**
 * 获取当前的信号强度，单位：dBm
 */
int radio_get_cur_rssi(void) {
    extern bool radio_is_sending(void);

    static int last_rssi = RSSI_UNKNOWN;
    uint8_t rssi_status;
    int cur_rssi;

    //TODO 目前是通过 SPI 读取，后期改为通过 GPIO 输出 CCA 状态
    /* Wait RX state switch OK */
    if (!radio_is_sending() && wait_state_switch(SI446X_CMD_REQUEST_DEVICE_STATE_REP_CURR_STATE_MAIN_STATE_ENUM_RX)) {
        radio_cmd_reply_lock();
        si446x_get_modem_status(SI446X_CMD_GET_MODEM_STATUS_ARG_MODEM_CLR_PEND_MASK);
        if ((rssi_status = Si446xCmd.GET_MODEM_STATUS.CURR_RSSI) != 0) {
            cur_rssi = calc_rssi_from_modem_status(rssi_status);
            last_rssi = cur_rssi;
        } else {
            cur_rssi = last_rssi;
        }
        radio_cmd_reply_unlock();
    } else {
        /* 切换失败，则认为当前通道不干净，返回较高的 RSSI 值 */
        cur_rssi = 0;
    }

//    log_d("cur rssi: %d, latch rssi %d, ant1: %d, ant2: %d", Si446xCmd.GET_MODEM_STATUS.CURR_RSSI,
//            Si446xCmd.GET_MODEM_STATUS.LATCH_RSSI, Si446xCmd.GET_MODEM_STATUS.ANT1_RSSI,
//            Si446xCmd.GET_MODEM_STATUS.ANT1_RSSI);

    return cur_rssi;

}

/**
 * 获取上次接收数据时的信号强度，单位：dBm
 */
int radio_get_latch_rssi(void) {
    static int rssi = 0;
    uint8_t rssi_status;

    radio_cmd_reply_lock();
    si446x_get_modem_status(SI446X_CMD_GET_MODEM_STATUS_ARG_MODEM_CLR_PEND_RSSI_PEND_CLR_BIT);
    if ((rssi_status = Si446xCmd.GET_MODEM_STATUS.LATCH_RSSI) != 0) {
        rssi = calc_rssi_from_modem_status(rssi_status);
    }
    radio_cmd_reply_unlock();

    return rssi;
}

/**
 * 获取当前为第几信道
 *
 * @return 信道数
 */
uint8_t radio_get_channel(void) {
    return pRadioConfiguration->Radio_ChannelNumber;
}

/**
 * 设置指定信道
 *
 * @param channel 信道数
 */
void radio_set_channel(uint8_t channel) {
    pRadioConfiguration->Radio_ChannelNumber = channel;
}

/**
 * 加锁命令的回复
 */
void radio_cmd_reply_lock(void) {
    RT_ASSERT(cmd_reply_locker);
    rt_mutex_take(cmd_reply_locker, RT_WAITING_FOREVER);
}

/**
 * 解锁命令的回复
 */
void radio_cmd_reply_unlock(void) {
    RT_ASSERT(cmd_reply_locker);
    rt_mutex_release(cmd_reply_locker);
}
