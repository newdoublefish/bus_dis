
function showPay(){
	window.location = "payvip.html?username="+name;
//	if($('#payDiv').length>0){return;}
//	var div = '<div id="payDiv">开通会员<div class="month-pay"><button id="reduce">-</button><input id="month" type="number" min="1" value="1"/><button id="add">+</button></div><div class="nav-pay"><button id="confirm">确定</button><button id="cancel">取消</button></div></div>';
//	$('body').append(div);
//	$('#reduce').click(function(){
//		var num = $('#month').val();
//		if(num<=1) return;
//		$('#month').val(num-1);
//	});
//	$('#add').click(function(){
//		var num = $('#month').val();
//		$('#month').val(parseInt(num)+1);
//	});
//	
//	$('#confirm').click(function(){
//		getPayparams($('#month').val());
//	});
//	
//	$('#cancel').click(function(){
//		$('#payDiv').remove();
//	});
}



function closePay(){
	$('#payDiv').remove();
}

function getPayparams(num){
	console.log(name);
	$.post(base_url + "aliPayRemoteServer/dealPay/getTradeNo",{
		username:name,
		orderNum:num
	},function(data){
		console.log(data);
		data = JSON.parse(data);
		if(data.code != '1000'){
			alert(data.msg);
			return;
		}
		var a=confirm(data.data.subject);
		if(a == true){
			execPay(data.data);
		}
	});
}

function execPay(data){
	window.location = base_url + 'aliPayRemoteServer/dealPay/doPay?outTradeNo='+data.outTradeNo+'&totalAmount='+data.totalAmount+'&subject='+data.subject;
}

