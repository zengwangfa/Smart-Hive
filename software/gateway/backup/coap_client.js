var coap = require('coap');

module.exports = {
    get: function (hostname, pathname, callback) {
    	var req = coap.request({
    		hostname:hostname,
    		pathname:pathname,
    		method:'GET',
    		options: {
    	        'Accept': 'application/json'
    	    }
    	});
    	
    	// 监听响应
    	req.on('response', function(res){
    		callback(res);
    	});
    	 
    	// 结束请求
    	req.end();
    },
    post: function (hostname, pathname, data, callback) {
    	var req = coap.request({
    		hostname:hostname,
    		pathname:pathname,
    		method:'POST',
    		options: {
    	        'Accept': 'application/json',
    	        'Content-Format': 'application/json'
    	    }
    	});
    	
    	req.write(data);
    	
    	// 监听响应
    	req.on('response', function(res){
    		callback(res);
    	});
    	 
    	// 结束请求
    	req.end();
        
    },
    obs: function (hostname, pathname, callback) {
    	var req = coap.request({
    		hostname:hostname,
    		pathname:pathname,
    		observe: true,
    		method:'GET',
    		options: {
    	        'Accept': 'application/json'
    	    }
    	});
    	
    	// 监听响应
    	req.on('response', function(res){
    		res.on('data',function(data){
    			callback(data);
    	    })
    	});
    	 
    	// 结束请求
    	req.end();
    }
};