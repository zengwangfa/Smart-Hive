var conf = require('./conf');
var mqtt = require('mqtt');
var callback = {};

module.exports = {
    connect: function (url) {
    	var mqtt_options={
			will:{
				topic:'GW0001/pub/status',
				payload:'{\"status\":\"\offline"}'
			}
		};
    	this.mqtt_client = mqtt.connect(url, mqtt_options);
    	
    	this.mqtt_client.on('connect', function () {
    		if(callback.connect != ''){
				callback.connect();
			}
		})
		
		this.mqtt_client.on('message', function (topic, message) {
			// message is Buffer
			if(callback.message != ''){
				callback.message(topic, message);
			}
			
			//mqtt_client.end();
		})
    	
    },
    set_callback: function (type, cb) {
    	if(type == 'connect'){
    		callback.connect = cb;
    	}
    	else if(type == 'message'){
    		callback.message = cb;
    	}
    },
    subscribe: function (topic) {
    	this.mqtt_client.subscribe(topic);
    },
    publish: function (topic, data) {
    	this.mqtt_client.publish(topic, data);
    }
};