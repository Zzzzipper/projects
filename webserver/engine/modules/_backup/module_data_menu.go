package modules

import (
	"fmt"
	"net/http"
	"server/engine/gates"
	"server/engine/wrapper"
	"strconv"
	"strings"

	"github.com/golang/gddo/httputil/header"
)

func (this *Modules) RegisterData_Menu() *Data {
	return this.newData(DInfo{
		Mount: "menu",
	},
		func(wrap *wrapper.Wrapper) {

			if wrap.R.Header.Get("Content-Type") != "" {
				value, _ := header.ParseValueAndParams(wrap.R.Header, "Content-Type")
				fmt.Println("RegisterData_Auth Content-type:", value)
				if value != "application/json" {
					msg := "Content-Type header is not application/json"
					wrap.W.Write([]byte("{\"error\": \"" + msg + ", " + strconv.Itoa(http.StatusUnsupportedMediaType) + "\"}"))
					return
				}
			}

			if len(wrap.R.Header["Origin"]) > 0 {
				fmt.Println("Origin for menu: ", wrap.R.Header["Origin"])
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
			conn := wrap.GatePool.Get("scdgate", wrap.S.GetInt("UserId", -1))
			if conn != nil {
				scadaMenuJson, err := conn.(gates.IGate).Menu()
				if err != nil {
					message := strings.Replace(err.Error(), "\"", "'", -1)
					scadaMenuJson = "{\"error\":\"" + message + "\"}"
				} else {
					scadaMenuJson = strings.Replace(scadaMenuJson, "\\", "/", -1)
				}

				// if scadaMenuJson == "" {
				// 	scadaMenuJson = "{}"
				// }

				this.XXXActionHeaders(wrap, http.StatusOK)
				wrap.W.Write([]byte(scadaMenuJson))

			} else {
				wrap.W.Write([]byte("{\"error\":\"not connected\"}"))
			}

		})
}
