package modules

import (
	"encoding/json"
	"fmt"
	"net/http"
	"server/engine/gates"
	"server/engine/wrapper"
	"strconv"

	"github.com/golang/gddo/httputil/header"
)

type Request struct {
	Command string `json:command`
	Message string `json:message`
	Mod_id  int32  `json:mod_id`
}

func (this *Modules) RegisterData_Request() *Data {
	return this.newData(DInfo{
		Mount: "request",
	},
		func(wrap *wrapper.Wrapper) {

			if wrap.R.Header.Get("Content-Type") != "" {
				value, _ := header.ParseValueAndParams(wrap.R.Header, "Content-Type")
				if value != "application/json" {
					msg := "Content-Type header is not application/json"
					wrap.W.Write([]byte("{\"error\": \"" + msg + ", " + strconv.Itoa(http.StatusUnsupportedMediaType) + "\"}"))
					return
				}
			}

			wrap.R.Body = http.MaxBytesReader(wrap.W, wrap.R.Body, 1048576)

			dec := json.NewDecoder(wrap.R.Body)
			dec.DisallowUnknownFields()

			var r Request
			err := dec.Decode(&r)

			if err != nil && wrap.R.Method != "OPTIONS" {
				wrap.W.Write([]byte("{\"error\": " + err.Error() + "}"))
				fmt.Println(err.Error())
				return
			}

			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("- Адрес клиента: ", wrap.R.Header["Origin"])
				wrap.W.Header().Add("Access-Control-Allow-Origin", wrap.R.Header["Origin"][0])
			}

			wrap.W.Header().Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			wrap.W.Header().Add("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
			wrap.W.Header().Add("Access-Control-Allow-Credentials", "true")
			wrap.W.Header().Add("Content-Type", "application/json; charset=utf-8")

			if wrap.R.Method == "OPTIONS" {
				this.XXXActionHeaders(wrap, http.StatusOK)
				return
			}

			//
			// TODO: исправить поиск шлюза в зависимости о запроса
			//
			userid := wrap.S.GetInt("UserId", -1)

			fmt.Println("- Поступил запрос \"", r.Command, "\" в сессию ", userid)

			conn := wrap.GatePool.Get(userid)

			if conn != nil {
				requestData := conn.(gates.IGate).DoRequest(r.Command, r.Message, r.Mod_id)
				if !requestData.HasError() {
					this.XXXActionHeaders(wrap, http.StatusOK)
				}
				wrap.W.Write(requestData.Json())
			} else {
				wrap.W.Write([]byte(gates.NewResult(gates.NotConnected, nil, nil).Json()))
			}
		})
}
