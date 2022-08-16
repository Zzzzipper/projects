package modules

import (
	"fmt"
	"net/http"
	"server/engine/gates"
	"server/engine/wrapper"
)

func (this *Modules) RegisterData_Status() *Data {
	return this.newData(DInfo{
		Mount: "status",
	},
		func(wrap *wrapper.Wrapper) {
			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("Origin for status: ", wrap.R.Header["Origin"])
				wrap.W.Header().Add("Access-Control-Allow-Origin", wrap.R.Header["Origin"][0])
			}
			wrap.W.Header().Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			wrap.W.Header().Add("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
			wrap.W.Header().Add("Access-Control-Allow-Credentials", "true")

			userid := wrap.S.GetInt("UserId", -1)

			fmt.Println("- Поступил запрос статуса в сессию ", userid)

			conn := wrap.GatePool.Get(userid)

			if conn != nil {
				_, requestData := conn.(gates.IGate).Status()
				if !requestData.HasError() {
					this.XXXActionHeaders(wrap, http.StatusOK)
				}
				wrap.W.Write(requestData.Json())
			} else {
				wrap.W.Write([]byte(gates.NewResult(gates.NotConnected, nil, nil).Json()))
			}
		})
}
