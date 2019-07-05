/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */
#define LOG_TAG                        "water"

#include "contiki.h"

#include <string.h>
#include "rest-engine.h"
#include <elog.h>
#include "cJSON.h"
#include "drv_relay.h"

static void res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*A simple actuator example, depending on the color query parameter and post variable mode, corresponding led is activated or deactivated*/
RESOURCE(res_water,
         "title=\"water control\";rt=\"Control\"",
         NULL,
         res_post_put_handler,
         res_post_put_handler,
         NULL);

static void
res_post_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    size_t len = 0;
    uint8_t *payload = 0;
    unsigned int accept = -1;
    unsigned int content_format = -1;
    int ack = -1;
    
    
    REST.get_header_accept(request, &accept);
    REST.get_header_content_type(request, &content_format);

    if(content_format == REST.type.APPLICATION_JSON)
    {
        if(accept == REST.type.APPLICATION_JSON)
        {
            len = REST.get_request_payload(request,&payload);
            if(len)
            {              
                cJSON *root = cJSON_Parse(payload);
                if(root)
                {
                    cJSON *mode = cJSON_GetObjectItem(root, "mode");
                    if(mode)
                    {
                        log_i("mode:%s",mode->valuestring);
                        if(strncmp(mode->valuestring, "on", 2) == 0)
                        {
                            cJSON *delay = cJSON_GetObjectItem(root, "delay");
                            if(delay)
                            {
                                log_i("delay:%d",delay->valueint);
                                relay_control(RELAY_WATER, 1, delay->valueint);
                                ack = 0;
                            }
                        }
                        else if(strncmp(mode->valuestring, "off", 3) == 0)
                        {
                            relay_control(RELAY_WATER, 0, -1);
                            ack = 0;
                        }
                    }
                    cJSON_Delete(root);
                }
            }
        }
    }
    else
    {
        REST.set_response_status(response, REST.status.BAD_REQUEST);
        return;
    }
    
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"ack\":%d}", ack);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
}