var coap_client = require('./coap_client');

var val_temp;
var val_weight;

module.exports = {
    temperature: function (hostname, serialnum, callback) {
		coap_client.obs(hostname, '/sensors/temperature', function(data){
			if(data != ''){
				val_temp = data.toString();
				callback(serialnum, val_temp);
			}
			else{
				console.log("temp err");
			}
			
		});
    },
    get_temp: function () {
    	return val_temp;
    },
    weight: function (hostname, serialnum, callback) {
		coap_client.obs(hostname, '/sensors/weight', function(data){
			if(data != ''){
				val_weight = data.toString();
				callback(serialnum, val_weight);
			}
			else{
				console.log("weight err");
			}
		});
    },
    get_weight: function () {
    	return val_weight;
    },
    feed: function (hostname, serialnum, data, callback) {
		coap_client.post(hostname, '/control/feed', data, function(res){
			callback(serialnum, data, res.payload);
		});
    },
    water: function (hostname, serialnum, data, callback) {
		coap_client.post(hostname, '/control/water', data, function(res){
			callback(serialnum, data, res.payload);
		});
    },
    heat: function (hostname, serialnum, data, callback) {
		coap_client.post(hostname, '/control/heat', data, function(res){
			callback(serialnum, data, res.payload);
		});
    }
};