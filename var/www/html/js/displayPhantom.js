function getData(type){
    console.log("++++++++++++removing all child nodes of vedioList++++++++");
    var vedioListNode=document.getElementById('vedioList');
    while (vedioListNode.hasChildNodes()) {
        vedioListNode.removeChild(vedioListNode.lastChild);
    }
    console.log("----------liveTag---------");
    console.log(document.getElementById('liveTag'));
    console.log("----------movieTag---------");
    console.log(document.getElementById('movieTag'));
    if(type==0){
        console.log("-------------获取点播列表-------");
        var url_cgi = 'http://' + location.hostname + '/test.cgi?type=0';
        document.getElementById('movieTag').innerHTML = "<b>点播</b>";
        document.getElementById('liveTag').innerHTML = "直播";
        console.log(url_cgi);
        $.get(url_cgi,
        function(data) {
            console.log("++++++++++++data++++++++");
            console.log(data);
            vedioList(data);
        });
    }else{
        console.log("-------------获取直播列表-------");
        var url_cgi = 'http://' + location.hostname + '/test.cgi?type=1';
        document.getElementById('movieTag').innerHTML = "点播";
        document.getElementById('liveTag').innerHTML = "<b>直播</b>";
        var data=eval('(' +'{"type":1,"movieList":[{"imagePath":"cctv-2.png","name":"","vedioPath":"3"},{"imagePath":"cctv-7.png","name":"","vedioPath":"4"},{"imagePath":"cctv-10.png","name":"","vedioPath":"5"},{"imagePath":"cctv-11.png","name":"","vedioPath":"6"},{"imagePath":"cctv-12.png","name":"","vedioPath":"7"},{"imagePath":"cctv-15.png","name":"","vedioPath":"8"}]}'+ ')');
        console.log("++++++++++++data++++++++");
        console.log(data);
        vedioList(data);
    }
    
    
    
}
function vedioList(data){
    //parentNode.removeChild(child);删除单个
    //document.getElementById('vedioList').remove();//是指将自己本身和子元素均删除
    //这个是要将node完全匹配才能删除？
    //var articleDel = document.createElement("article");
    //console.log(document.getElementById('vedioList'));
    //document.getElementById('vedioList').removeChild(articleDel);
    console.log("++++++++++++dealing data++++++++");
    console.log(data);
    var type=data.type;
    console.log(type);
    $.each(data.movieList, function (index, val) {
        /*影片简介信息*/
        var p = document.createElement("p"); 
        p.innerHTML = "ttttttttttttttttttttt";
        
        var divContent=document.createElement("div");
        divContent.setAttribute("class","content");
        divContent.appendChild(p);
        /*影片名称*/
        var h = document.createElement("h2"); 
        h.innerHTML = val.name;
        
        /*播放链接*/
        var a = document.createElement("a"); 
        a.setAttribute("href","/player1.html?name="+val.vedioPath+"&image="+val.imagePath+"&type="+type);
        a.appendChild(h);
        a.appendChild(divContent);
        
        /*电影海报*/
        var img = document.createElement("img"); 
        img.setAttribute("src","/image/"+val.imagePath);
        
        /*展示电影海报*/
        var span = document.createElement("span"); 
        span.setAttribute("class","image");
        span.appendChild(img);
        
        /*一个电影*/
        var article = document.createElement("article"); 
        article.setAttribute("class","style1");
        article.appendChild(span);
        article.appendChild(a);
        
        console.log("-------------article---------");
        console.log(article);
        
        document.getElementById('vedioList').appendChild(article);
        
        //document.getElementById('liveTag').innerHTML = "<b>直播1111111</b>";
    });
}
