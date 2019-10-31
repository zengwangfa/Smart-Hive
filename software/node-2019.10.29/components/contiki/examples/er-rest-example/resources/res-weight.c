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
 *      Example of an observable "on-change" temperature resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 * \author
 *      Cristiano De Alti <cristiano_dealti@hotmail.com>
 */

#include "contiki.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "rest-engine.h"
#include "drv_hx711.h"

static void res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_periodic_handler(void);


#define INTERVAL_MIN 1
#define INTERVAL_MAX 60*15
#define MAX_AGE      INTERVAL_MAX*2   //原先为60s
#define CHANGE       0.5   //变化大于0.5kg

static int32_t interval_counter = INTERVAL_MIN;
static float weight_old = 0;
static int weight_random = 0;

PERIODIC_RESOURCE(res_weight,
         "title=\"Weight\";rt=\"Weight\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         CLOCK_SECOND,
         res_periodic_handler);

static void
res_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /*
   * For minimal complexity, request query and options should be ignored for GET on observable resources.
   * Otherwise the requests must be stored with the observer list and passed by REST.notify_subscribers().
   * This would be a TODO in the corresponding files in contiki/apps/erbium/!
   */

  float weight = hx711_weight_get();
  
  //sprintf(str,"res_weight:%f---------------------------",weight);
  //rt_kprintf(str);
  
  unsigned int accept = -1;
  REST.get_header_accept(request, &accept);

  if(accept == REST.type.TEXT_PLAIN) {
    REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%.1f", weight);

    REST.set_response_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == -1 || accept == REST.type.APPLICATION_JSON) {
    REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"weight\":%.1f}", weight);

    REST.set_response_payload(response, buffer, strlen((char *)buffer));
  } else {
    REST.set_response_status(response, REST.status.NOT_ACCEPTABLE);
    const char *msg = "Supporting content-types text/plain and application/json";
    REST.set_response_payload(response, msg, strlen(msg));
  }
  
  REST.set_header_max_age(response, MAX_AGE);

  /* The REST.subscription_handler() will be called for observable resources by the REST framework. */
}

/*
 * Additionally, a handler function named [resource name]_handler must be implemented for each PERIODIC_RESOURCE.
 * It will be called by the REST manager process with the defined period.
 */
static void
res_periodic_handler()
{
  //char str[50] = {0};
  float weight = hx711_weight_get();
  float change = weight>weight_old?(weight-weight_old):(weight_old-weight);
  
  //sprintf(str,"res_weight:%f---------------------------",weight);
  //rt_kprintf(str);
  
  ++interval_counter;

  if((change >= CHANGE && interval_counter >= INTERVAL_MIN) || 
     interval_counter >= (INTERVAL_MAX+weight_random)) {
     interval_counter = 0;
     weight_old = weight;
     weight_random = random_rand()%INTERVAL_MAX;
    /* Notify the registered observers which will trigger the res_get_handler to create the response. */
    REST.notify_subscribers(&res_weight);
  }
}
