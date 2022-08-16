jQuery(function () {
    var queryTimeOut = 5000,
        tid,
        number = 0,
        plugins = [
            "undefined",
            "PING",
            "TCP_PORT",
            "HTTP",
            "SMTP",
            "FTP",
            "DNS",
            "POP",
            "TELNET",
            "SSH",
            "MODULE_CMS",
            "_", "_", "_",
            "_", "_", "_",
            "TCP_PORT2",
            "IMAP",
            "HTTP_HEAD",
            "NRPE",
            "DBCONNECT",
            "SAFE_BROWSING",
            "WHOIS",
            "HTTP_NAGIOS",
            "DOMAIN",
            "PORT_SCAN"
        ],
        queryHosts = function () {
            urls.forEach(function (item, i, urls) {
                $.ajax({
                    url: item + '/request',
                    method: 'post',
                    dataType: 'json',
                    contentType: 'application/json',
                    data: JSON.stringify({
                        command: 'getStatus',
                    }),
                    success: function (data, textStatus) {
                        fillBox($("#id_" + i), data);
                    },
                    error: function (qXHR, textStatus, errorThrown) {
                    }
                });
            });
        },
        fillBox = function (object, data) {
            object.empty();
            var table = $(("<table></table>")).appendTo(object);
            var tr = $(("<tr></tr>")).appendTo(table);
            $("<th>ID</th>")
                .appendTo(tr)
                .css({ "width": "5%" })
            $("<th>IP</th>")
                .appendTo(tr)
                .css({ "width": "8%", "border-left": "none" })
            $("<th>ALL TASKS</th>")
                .appendTo(tr)
                .css({ "width": "5%", "border-left": "none" })
            $("<th>PORT</th>")
                .appendTo(tr)
                .css({ "width": "5%", "border-left": "none" })
            $("<th>ACTIVE</th>")
                .appendTo(tr)
                .css({ "width": "5%", "border-left": "none" })
            $("<th>SOFTVERSION</th>")
                .appendTo(tr)
                .css({ "width": "10%", "border-left": "none" })
            $("<th>INFO</th>")
                .appendTo(tr)
                .css({ "width": "45%", "border-left": "none" })
            $("<th>CREATED</th>")
                .appendTo(tr)
                .css({ "width": "10%", "border-left": "none" })
            tr = $(("<tr></tr>"))
                .appendTo(table);
            $(("<td>" + data.tester.id + "</td>"))
                .appendTo(tr)
            $(("<td><a id='t" + data.tester.id + "'>" + data.tester.ip + "</a></td>"))
                .appendTo(tr)
                .css({ "border-left": "none" })
            $(("<td>" + data.tester.totaltests + "</td>"))
                .appendTo(tr)
                .css({ "border-left": "none" })
            $(("<td>" + data.tester.port + "</td>"))
                .appendTo(tr)
                .css({ "border-left": "none" })
            $("<td>" + data.tester.active + "</td>")
                .appendTo(tr)
                .css({ "border-left": "none" })
            $("<td>" + data.tester.softversion + "</td>")
                .appendTo(tr)
                .css({ "border-left": "none" })
            $("<td>" + data.tester.comment + "</td>")
                .appendTo(tr)
                .css({ "border-left": "none" })
            $("<td>" + data.tester.created_at + "</td>")
                .appendTo(tr)
                .css({ "border-left": "none" })

            table = $(("<table class='tester_tasks'></table>")).appendTo(object);
            tr = $(("<tr></tr>"))
                .appendTo(table)
                .css({ "width": "100%" });
            data.tester.loads.forEach(function (item, i, urls) {
                var background = "rgba(170, 255, 170, 0.5)";
                var title = "Всего заданий " + plugins[item.type];
                if (item.enable == "false") {
                    background = "rgba(100, 100, 100, 0.2)";
                    title = "Для проверки " + plugins[item.type] + " нет плагина";
                }
                var width = item.number * 100.0 / data.tester.totaltests || 10;
                $("<td><b>" + item.type + "</b> (" + item.number + ")" + "</td>")
                    .appendTo(tr)
                    .css({ "width": width + "%", "background": background })
                    .prop("title", title)
            });

        },
        createControl = function () {
            $("#reload_all").click(function() {
                urls.forEach(function (item, i, urls) {
                    $.ajax({
                        url: item + '/request',
                        method: 'post',
                        dataType: 'json',
                        contentType: 'application/json',
                        data: JSON.stringify({
                            command: 'reload',
                        }),
                        success: function (data, textStatus) {
                        },
                        error: function (qXHR, textStatus, errorThrown) {
                        }
                    });
                });
                
            });

        },
        createPanels = function () {
            urls.forEach(function (item, i, urls) {
                $("#testers").append("<div class='box' id='id_" + i + "'></div>");
            });

        },
        start = function (queryTimeOut) {
            var tid = setTimeout(requestor, queryTimeOut);

        },
        requestor = function () {
            number++;
            queryHosts();
            tid = setTimeout(requestor, queryTimeOut);
        };

    createControl();
    createPanels();
    start();
})

