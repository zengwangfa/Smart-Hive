var conf = require('./conf');
var hive = require('./hive');
var mqtt_client = require('./mqtt_client');
var coap = require('coap');
var HashMap = require('hashmap');
var exec = require('child_process').exec;

var coap_server = coap.createServer({ type: 'udp6' });
var client_map = new HashMap();
var cmd = 'route -A inet6 add fd00::/64 gw bbbb::100';

function publish_client_list() {
	var client_list = {'count':0};
	var i = 0;
	
	client_list.count = client_map.size;
	client_list.deviceInfos = [];
	
	client_map.forEach(function(value, key) {
    	var data = {};
		data["serialnum"] = value.serialnum;
		data["online"] = value.online;
		client_list.deviceInfos.push(data);
	});
	console.log(client_list);
	mqtt_client.publish(conf.id+'/pub/dev_list', JSON.stringify(client_list));
}

function mqtt_keepalive_client_list()
{
	publish_client_list();
	setTimeout(function(arg1){
		mqtt_keepalive_client_list();
	}, 300*1000, null);//五分��
}

function client_sensor_obs(serialnum){
	var map_data = client_map.get(serialnum);
	
	hive.temperature(map_data.ip, serialnum, function(num ,data){
		if(data != ''){
			var send_data = JSON.parse(data);
			send_data.serialnum = num;
			console.log(num, 'temp upload!');
			mqtt_client.publish(conf.id+'/pub/temperature', JSON.stringify(send_data));
		}
	});
	
	hive.weight(map_data.ip, serialnum, function(num, data){
		if(data != ''){
			var send_data = JSON.parse(data);
			send_data.serialnum = num;
			console.log(num, 'weight upload!');
			mqtt_client.publish(conf.id+'/pub/weight', JSON.stringify(send_data));
		}
	});
}

coap_server.on('request', function(req, res) {  
    if(req.url == '/online')
	{
    	var client_serial = req.payload.toString();
    	var data = {};

		if(client_map.size == 0)
		{
			exec(cmd, function(error, stdout, stderr) 
			{
				
			});
		}
    	if(client_map.has(client_serial) == false)
		{
			data["serialnum"] = client_serial;
			data["online"] = 1;
			data["ip"] = req.rsinfo.address;
			console.log(client_serial, 'online!');
    		client_map.set(client_serial, data);
			client_sensor_obs(client_serial);
			publish_client_list();
    	}
    	else
		{
    		data = client_map.get(client_serial);
    		if(data.online == 1)
			{
    			//console.log(client_serial, 'clear!');
		        clearTimeout(data.timer);  
				data.timer = null;
		    }
    		else
			{
    			console.log(client_serial, 'online!');
    			data.online = 1;
    			client_map.set(client_serial, data);
    			client_sensor_obs(client_serial);
				publish_client_list();
    		}
    	}
	    
	    data.timer = setTimeout(
		function(arg1)
		{
	        console.log(arg1,"offline!");
	        data = client_map.get(arg1);
	        data.online = 0;
			data.timer = null;
	        client_map.set(arg1, data);
			publish_client_list();
	    },
		 600*1000, client_serial);
		 res.code = '2.05';
	}
	else if(req.url == '/keepalive')
	{
		var client_serial = req.payload.toString();
		var data = {};
		
		if(client_map.has(client_serial) == true)
		{
			data = client_map.get(client_serial);
			if(data.online == 1)
			{
				console.log(client_serial, 'clear!');
		        clearTimeout(data.timer);  
				data.timer = null;
		    }
			else
			{
				console.log(client_serial, 'online!');
				data.online = 1;
				client_map.set(client_serial, data);
				client_sensor_obs(client_serial);
				publish_client_list();
			}
			
			data.timer = setTimeout(
			function(arg1)
			{
			    console.log(arg1,"offline!");
			    data = client_map.get(arg1);
			    data.online = 0;
				data.timer = null;
			    client_map.set(arg1, data);
				publish_client_list();
			},
			 600*1000, client_serial);
		}
		
		 res.code = '2.05';
	}
})

coap_server.listen(function() {
	mqtt_keepalive_client_list();
    console.log('coap server started');
});

mqtt_client.set_callback('connect',function(){
	console.log("connect");
	var data = {status:'online'};
	
	mqtt_client.subscribe(conf.id+'/sub/#');
	mqtt_client.publish(conf.id+'/pub/status', JSON.stringify(data));
});

mqtt_client.set_callback('message',function(topic, message){
	console.log(topic,':',message.toString());
	
	var serialnum;
	
	if(topic == conf.id+'/sub/feed'){
		serialnum = JSON.parse(message.toString()).serialnum;
		if(client_map.has(serialnum) == false) return;
		var map_data = client_map.get(serialnum);
		
		hive.feed(map_data.ip, serialnum, message.toString(), function(num, msg, data){
			if(data != ''){
				var send_data = JSON.parse(data.toString());
				var timestamp = JSON.parse(msg).timestamp;
				send_data.serialnum = num;
				send_data.timestamp = timestamp;
				mqtt_client.publish(conf.id+'/pub/feed', JSON.stringify(send_data));
			}
		});
	}
	else if(topic == conf.id+'/sub/water'){
		serialnum = JSON.parse(message.toString()).serialnum;
		if(client_map.has(serialnum) == false) return;
		var map_data = client_map.get(serialnum);

		hive.water(map_data.ip, serialnum, message.toString(), function(num ,msg, data){
			if(data != ''){
				var send_data = JSON.parse(data.toString());
				var timestamp = JSON.parse(msg).timestamp;
				send_data.serialnum = num;
				send_data.timestamp = timestamp;
				mqtt_client.publish(conf.id+'/pub/water', JSON.stringify(send_data));
			}
		});

	}
	else if(topic == conf.id+'/sub/heat'){
		serialnum = JSON.parse(message.toString()).serialnum;
		if(client_map.has(serialnum) == false) return;
		var map_data = client_map.get(serialnum);
		
		hive.heat(map_data.ip, serialnum, message.toString(), function(num, msg, data){
			if(data != ''){
				var send_data = JSON.parse(data.toString());
				var timestamp = JSON.parse(msg).timestamp;
				send_data.serialnum = num;
				send_data.timestamp = timestamp;
				mqtt_client.publish(conf.id+'/pub/heat', JSON.stringify(send_data));
			}
		});
	}
	else if(topic == conf.id+'/sub/temperature'){
		serialnum = JSON.parse(message.toString()).serialnum;
		if(client_map.has(serialnum) == false) return;
		
		var temp = hive.get_temp();
		if(temp != ''){
			var send_data = JSON.parse(temp);
			send_data.serialnum = serialnum;
			mqtt_client.publish(conf.id+'/pub/temperature', JSON.stringify(send_data));
		}	
	}
	else if(topic == conf.id+'/sub/weight'){
		serialnum = JSON.parse(message.toString()).serialnum;
		if(client_map.has(serialnum) == false) return;
		
		var weight = hive.get_weight();
		if(weight != ''){
			var send_data = JSON.parse(weight);
			send_data.serialnum = serialnum;
			mqtt_client.publish(conf.id+'/pub/temperature', JSON.stringify(send_data));
		}
	}
	else if(topic == conf.id+'/sub/dev_list'){
		publish_client_list();
	}
});

		
mqtt_client.connect('mqtt://aliyun.nblink-tech.com:1883');

process.on('uncaughtException', function (err) {
     //打印出错误
     console.log(err);
     //打印出错误的调用栈方便调试
     console.log(err.stack);
});
