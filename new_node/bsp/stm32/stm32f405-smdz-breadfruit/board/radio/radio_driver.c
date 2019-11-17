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
 * Function: Contiki radio driver layer port.
 * Created on: 2017-10-7
 */

#include "radio_cfg.h"


//#define DRV_DEBUG
#define LOG_TAG             "radio_drv"
#include <drv_log.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <rtthread.h>
#include "Si446x/si446x_api_lib.h"
#include "radio_hal.h"

#define RADIO_FIRMWARE_VERSION         "0.1.1"

/* The outgoing frame buffer */
#define TX_BUF_PAYLOAD_LEN             180
/* Special value returned by CMD_IEEE_CCA_REQ when an RSSI is not available */
#define RSSI_UNKNOWN                   255
/* 射频接收信号的阈值，低于此信号，则认为当前环境无射频传输信号 */
#define RSSI_THRESHOLD                 -80
/* Used for the return value of channel_clear */
#define RF_CCA_CLEAR                   1
#define RF_CCA_BUSY                    0


static uint8_t tx_buf[TX_BUF_PAYLOAD_LEN];
static bool rf_is_on = false;
static int rssi_threshold = RSSI_THRESHOLD;

static void recved_hook(size_t recv_len) {
    process_poll(&si446x_recv_process);
}

static int init(void) {
    extern void radio_recved_set_hook(void (*hook)(size_t recv_len));
    int result = 1;

    if (radio_hal_init() != SI446X_SUCCESS) {
        result = 0;
        goto __exit;
    }

    radio_software_init();

    process_start(&si446x_recv_process, NULL);

    radio_recved_set_hook(recved_hook);

__exit:
    if (result) {
        LOG_I("Radio V%s init success", RADIO_FIRMWARE_VERSION);
    } else {
        LOG_E("Radio V%s init failed", RADIO_FIRMWARE_VERSION);
    }

    return result;
}

static int prepare(const void *payload, unsigned short payload_len) {
    int len = MIN(payload_len, TX_BUF_PAYLOAD_LEN);

    memcpy(tx_buf, payload, len);

    return 0;
}

static int transmit(unsigned short transmit_len) {
    extern rt_err_t radio_pkt_send_sync(const uint8_t *buf, size_t size);

    if (radio_pkt_send_sync(tx_buf, transmit_len) != RT_EOK) {
        return RADIO_TX_ERR;
    } else {
        return RADIO_TX_OK;
    }
}

static int send(const void *payload, unsigned short payload_len) {
    prepare(payload, payload_len);

    return transmit(payload_len);
}

static int read(void *buf, unsigned short buf_len) {
    extern rt_err_t radio_pkt_get_recved(uint8_t* buf, size_t size, size_t *len, int *rssi);
    int recved_len = 0, rssi;

    radio_pkt_get_recved(buf, buf_len, (size_t *)&recved_len, &rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, 0x7F);

    return recved_len;
}

static int channel_clear(void) {
    extern bool radio_is_sending(void);
    extern bool radio_is_recving(void);

    if (radio_is_sending() || radio_is_recving()) {
        return RF_CCA_BUSY;
    }

    if (radio_get_cur_rssi() >= rssi_threshold) {
        return RF_CCA_BUSY;
    } else {
        return RF_CCA_CLEAR;
    }
}

/**
 * 当前是否处于并正在接收包数据状态
 */
static int receiving_packet(void) {
    if (!rf_is_on) {
        return 0;
    }

    /* 当前通道很干净，一定不处于接收状态 */
    if (channel_clear()) {
        return 0;
    }

    return 1;
}

/**
 * 当前是否已经接收到的包数据
 */
static int pending_packet(void) {
    extern bool radio_has_recved_pkt(void);

    if (radio_has_recved_pkt()) {
        process_poll(&si446x_recv_process);
        return 1;
    } else {
        return 0;
    }
}

static int on(void) {
    radio_hal_DeassertShutdown();
    rf_is_on = true;

    return 1;
}

static int off(void) {
    radio_hal_AssertShutdown();
    rf_is_on = false;

    return 1;
}

static uint8_t get_channel(void) {
    return radio_get_channel();
}

static void set_channel(uint8_t channel) {
    radio_set_channel(channel);
}

static radio_result_t get_value(radio_param_t param, radio_value_t *value) {
    if (!value) {
        return RADIO_RESULT_INVALID_VALUE;
    }

    switch (param) {
    case RADIO_PARAM_POWER_MODE:
        /* On / off */
        *value = rf_is_on ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
        return RADIO_RESULT_OK;
    case RADIO_PARAM_CHANNEL:
      *value = (radio_value_t)get_channel();
//TODO 待实现
        return RADIO_RESULT_OK;
    case RADIO_PARAM_TXPOWER:
//      *value = get_tx_power();
        //TODO 待实现
        return RADIO_RESULT_OK;
    case RADIO_PARAM_CCA_THRESHOLD:
        *value = rssi_threshold;
        return RADIO_RESULT_OK;
    case RADIO_PARAM_RSSI:
        *value = radio_get_cur_rssi();

        if (*value == RSSI_UNKNOWN) {
            return RADIO_RESULT_ERROR;
        } else {
            return RADIO_RESULT_OK;
        }
    case RADIO_CONST_CHANNEL_MIN:
        *value = 0;
        return RADIO_RESULT_OK;
    case RADIO_CONST_CHANNEL_MAX:
//      *value = DOT_15_4G_CHANNEL_MAX;
        //TODO 待实现
        return RADIO_RESULT_OK;
    case RADIO_CONST_TXPOWER_MIN:
//      *value = TX_POWER_DRIVER[get_tx_power_array_last_element()].dbm;
        //TODO 待实现
        return RADIO_RESULT_OK;
    case RADIO_CONST_TXPOWER_MAX:
//      *value = OUTPUT_POWER_MAX;
        //TODO 待实现
        return RADIO_RESULT_OK;
    default:
        return RADIO_RESULT_NOT_SUPPORTED;
    }
}

PROCESS_THREAD(si446x_recv_process, ev, data)
{
    static int len;

    while (1) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
        do {
            packetbuf_clear();
            len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);

            if (len > 0) {
                packetbuf_set_datalen(len);

                NETSTACK_RDC.input();
            }
        } while (len > 0);
    }

}

