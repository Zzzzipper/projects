jQuery(function () {
    var queryTimeOut = 1000,
        tid,
        number = 0,
        urls = [
            "http://193.161.86.132:8080",
            "http://193.161.86.133:8080",
            "http://193.161.86.135:8080",
            "http://193.161.86.141:8080",
            "http://193.161.86.154:8080"
        ],
        queryHosts = function () {
            console.log(number)
            urls.forEach(function (item, i, urls) {
                console.log(item + '/request');
                var request = new XMLHttpRequest();
                var params = JSON.stringify({
                            command: 'getStatus',
                            message: '{"login":"user", "password":"test"}'
                        });
                request.open('POST', item + '/request', true);
                request.onreadystatechange = function (data) { 
                    console.log(data); 
                };
                request.setRequestHeader("Content-type", "application/json");
                request.setRequestHeader("Content-length", params.length);
                request.setRequestHeader("Connection", "close");
                request.send(params);
                // $.ajax({
                //     url: item + '/request',
                //     method: 'post',
                //     dataType: 'json',
                //     contentType: 'application/json',
                //     data: JSON.stringify({
                //         command: 'getStatus',
                //         message: '{"login":"user", "password":"test"}'
                //     }),
                //     success: function (data, textStatus) {
                //         if (data.redirect) {
                //             windows.location.href = data.redirect;
                //         } else {
                //             console.log(data);
                //         }
                //     },
                //     error: function (qXHR, textStatus, errorThrown) {
                //         console.log(textStatus, " ", errorThrown);
                //     }
                // });
            })
        },
        start = function (queryTimeOut) {
            var tid = setTimeout(requestor, queryTimeOut);

        },
        requestor = function () {
            number++;
            queryHosts();
            tid = setTimeout(requestor, queryTimeOut);
        };

    start();
})